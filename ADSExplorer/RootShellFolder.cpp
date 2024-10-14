/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

#include "StdAfx.h"  // Precompiled header; include first

#if _MSC_VER > 1200
	#include "ADSExplorer_h.h"
#else
	// the IDL compiler on VC++6 puts it here instead. weird!
	#include "ADSExplorer.h"
#endif

#include <atlstr.h>
#include <sstream>

#include "ADSXEnumIDList.h"
#include "ADSXItem.h"
#include "DataObject.h"
#include "RootShellFolder.h"
#include "RootShellView.h"

//==============================================================================
// Helpers

// Debug log prefix for CADSXRootShellFolder
#define P_RSF L"CADSXRootShellFolder(0x" << std::hex << this << L")::"

/**
 * STRRET maker
 *
 * @pre: *strret is initialized
 */
bool SetReturnString(_In_ LPCWSTR pszSource, _Out_ STRRET *strret) {
	LOG(L" ** " << pszSource);
	const SIZE_T cwStringLen = wcslen(pszSource) + 1;
	strret->uType = STRRET_WSTR;
	strret->pOleStr = static_cast<LPOLESTR>(
		CoTaskMemAlloc(cwStringLen * sizeof(OLECHAR))
	);
	if (strret->pOleStr == NULL) return false;
	wcsncpy_s(strret->pOleStr, cwStringLen, pszSource, cwStringLen);
	return true;
}


//==============================================================================
// CADSXRootShellFolder
CADSXRootShellFolder::CADSXRootShellFolder()
	: m_pidlRoot(NULL)
	, m_pidlFSPath(NULL)
	, m_psdFSPath(NULL)
	, m_bEndOfPath(false) 
	, m_bPathIsFile(false) {
	// LOG(P_RSF << L"CONSTRUCTOR");
}


CADSXRootShellFolder::~CADSXRootShellFolder() {
	// LOG(P_RSF << L"DESTRUCTOR");
	if (m_pidlRoot != NULL) CoTaskMemFree(m_pidlRoot);
	if (m_pidlFSPath != NULL) CoTaskMemFree(m_pidlFSPath);
	if (m_psdFSPath != NULL) m_psdFSPath->Release();
}


STDMETHODIMP CADSXRootShellFolder::GetClassID(_Out_ CLSID *pclsid) {
	if (pclsid == NULL) return WrapReturn(E_POINTER);
	*pclsid = CLSID_ADSExplorerRootShellFolder;
	return WrapReturn(S_OK);
}


/**
 * Initialize() is passed the PIDL of the folder where our extension is.
 * Copies, does not take ownership of, the PIDL.
 * @pre: PIDL is [Desktop\ADS Explorer] or [ADS Explorer].
 *       (I _assume_ those are the only two values Explorer ever passes to us.)
 * @pre: 0 < PIDL length < 3.
 * @post: this CADSXRootShellFolder instance is ready to be used.
 */
STDMETHODIMP CADSXRootShellFolder::Initialize(_In_ PCIDLIST_ABSOLUTE pidlRoot) {
	// LOG(P_RSF << L"Initialize(pidl=[" << PidlToString(pidlRoot) << L"])");

	// Don't initialize more than once.
	// This is necessary because for reasons beyond me Windows tries to.
	if (m_pidlRoot != NULL) {
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	}

	// Validate input PIDL
	if (
		ILIsEmpty(pidlRoot) ||
		(!ILIsEmpty(ILNext(pidlRoot)) && !ILIsEmpty(ILNext(ILNext(pidlRoot))))
	) {
		// PIDL length is 0 or more than 2
		return E_INVALIDARG;
	}

	// Keep this around for use elsewhere
	m_pidlRoot = ILCloneFull(pidlRoot);
	if (m_pidlRoot == NULL) return E_OUTOFMEMORY;

	// Initialize to the root of the namespace, [Desktop]
	HRESULT hr = SHGetDesktopFolder(&m_psfFSPath);
	if (FAILED(hr)) return hr;

	return hr;
}


STDMETHODIMP
CADSXRootShellFolder::GetCurFolder(_Outptr_ PIDLIST_ABSOLUTE *ppidl) {
	// LOG(P_RSF << L"GetCurFolder()");
	if (ppidl == NULL) return E_POINTER;
	// if (ppidl == NULL) return WrapReturn(E_POINTER);
	*ppidl = ILCloneFull(m_pidlRoot);
	return *ppidl != NULL ? S_OK : E_OUTOFMEMORY;
	// return WrapReturn(*ppidl != NULL ? S_OK : E_OUTOFMEMORY);
}


//-------------------------------------------------------------------------------
// IShellFolder

// TODO(garlic-os): Explain this function
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	_In_         PCUIDLIST_RELATIVE pidl,
	_In_opt_     IBindCtx           *pbc,
	_In_         REFIID             riid,
	_COM_Outptr_ void               **ppShellFolder
) {
	if (ppShellFolder == NULL) return E_POINTER;
	*ppShellFolder = NULL;

	// Don't log unsupported interfaces
	if (riid != IID_IShellFolder && riid != IID_IShellFolder2) {
		return E_NOINTERFACE;
	}
	
	LOG(P_RSF << L"BindToObject("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);

	HRESULT hr;

	m_bEndOfPath = ILIsChild(pidl);

	// Browse this path internally.
	hr = m_psfFSPath->BindToObject(pidl, pbc, riid, ppShellFolder);
	LOG(L" ** Inner BindToObject -> " << HRESULTToString(hr));
	if (SUCCEEDED(hr)) {
		// Browsed into a folder
		m_psfFSPath = static_cast<IShellFolder*>(*ppShellFolder);
	} else if (hr == E_FAIL) {
		// Tried to browse into a file, which is fine for us.
		// When EnumObjects is called, we'll be enumerating the file's ADSes.
		m_bPathIsFile = true;
	} else {
		return WrapReturnFailOK(hr);
	}

	// Update our internal PIDL.
	// These are the two cases I've seen: either a child directly relative to
	// our path, or an absolute path.
	if (ILIsChild(pidl)) {
		m_pidlFSPath = static_cast<PIDLIST_ABSOLUTE>(
			ILAppendID(m_pidlFSPath, &pidl->mkid, TRUE)
		);
	} else {
		if (m_pidlFSPath != NULL) CoTaskMemFree(m_pidlFSPath);
		m_pidlFSPath = static_cast<PIDLIST_ABSOLUTE>(ILClone(pidl));
	}
	if (m_pidlFSPath == NULL) return WrapReturn(E_OUTOFMEMORY);

	LOG(L" ** New m_pidlFSPath: " << PidlToString(m_pidlFSPath));

	// Return self as the ShellFolder.
	// TODO(garlic-os): Windows may not like receiving the same object back and
	// may expect a copy
	this->AddRef();
	*ppShellFolder = this;
	return WrapReturn(S_OK);
}


/**
 * Return the sort order of two PIDLs.
 * lParam can be the 0-based Index of the details column
 * @pre pidl1 and pidl2 hold CADSXItems.
 */
STDMETHODIMP CADSXRootShellFolder::CompareIDs(
	_In_ LPARAM             lParam,
	_In_ PCUIDLIST_RELATIVE pidl1,
	_In_ PCUIDLIST_RELATIVE pidl2
) {
	LOG(P_RSF << L"CompareIDs("
		L"lParam=" << std::hex << static_cast<long>(lParam) << L"), "
		L"pidl1=[" << PidlToString(pidl1) << L"], "
		L"pidl2=[" << PidlToString(pidl2) << L"])"
	);

	// Delegate PIDLs that aren't ours to the real ShellFolder
	if (!CADSXItem::IsOwn(pidl1) && !CADSXItem::IsOwn(pidl2)) {
		return m_psfFSPath->CompareIDs(lParam, pidl1, pidl2);
	}

	// Both PIDLs must either be ours or not ours
	ATLASSERT(CADSXItem::IsOwn(pidl1) && CADSXItem::IsOwn(pidl2));
	if (!CADSXItem::IsOwn(pidl1) || !CADSXItem::IsOwn(pidl2)) {
		return WrapReturn(E_INVALIDARG);
	}

	// Only child ADS PIDLs supported
	ATLASSERT(ILIsChild(pidl1) && ILIsChild(pidl2));
	if (!ILIsChild(pidl1) || !ILIsChild(pidl2)) {
		return WrapReturn(E_INVALIDARG);
	}
	auto Item1 = CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl1));
	auto Item2 = CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl2));

	USHORT Result = 0;  // see note below (MAKE_HRESULT)

	switch (lParam & SHCIDS_COLUMNMASK) {
		case DETAILS_COLUMN_NAME:
			Result = wcscmp(Item1->pszName, Item2->pszName);
			break;
		case DETAILS_COLUMN_FILESIZE:
			Result = static_cast<USHORT>(Item1->llFilesize - Item2->llFilesize);
			if (Result < 0) Result = -1;
			else if (Result > 0) Result = 1;
			break;
		default:
			return WrapReturn(E_INVALIDARG);
	}

	// Warning: the last param MUST be unsigned, if not (ie: short) a negative
	// value will trash the high order word of the HRESULT!
	return WrapReturn(MAKE_HRESULT(SEVERITY_SUCCESS, 0, /*-1,0,1*/ Result));
}


/**
 * Return a COM object that implements IShellView.
 */
STDMETHODIMP CADSXRootShellFolder::CreateViewObject(
	_In_         HWND   hwndOwner,
	_In_         REFIID riid,
	_COM_Outptr_ void   **ppViewObject
) {
	// Not logging all interface requests because there are too many
	// LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");

	if (ppViewObject == NULL) return E_POINTER;
	// if (ppvViewObject == NULL) return WrapReturn(E_POINTER);
	*ppViewObject = NULL;

	HRESULT hr;

	// We only handle IShellView
	if (riid == IID_IShellView) {
		LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");
		// Create a view object
		CComObject<CADSXRootShellView> *pViewObject;
		hr = CComObject<CADSXRootShellView>::CreateInstance(&pViewObject);
		if (FAILED(hr)) return WrapReturn(hr);

		// AddRef the object while we are using it
		pViewObject->AddRef();

		// Tie the view object lifetime with the current IShellFolder.
		pViewObject->Init(this->GetUnknown());

		// Create the view
		hr = pViewObject->Create(
			reinterpret_cast<IShellView **>(ppViewObject),
			hwndOwner,
			static_cast<IShellFolder *>(this)
		);

		// We are finished with our own use of the view object (AddRef()'d
		// above by us, AddRef()'ed by Create)
		pViewObject->Release();

		return WrapReturn(hr);
	}

	return E_NOINTERFACE;
	// return WrapReturnFailOK(E_NOINTERFACE);
}


/**
 * Return a COM object that implements IEnumIDList and enumerates the ADSes in
 * the current folder.
 * @pre: Windows has browsed to a path of the format
 *       [Desktop\ADS Explorer\{FS path}]
 * @pre: i.e., m_pidlFSPath is [Desktop\{FS path}]
 * @post: ppEnumIDList holds a CADSXEnumIDList** on {FS path}
 */
STDMETHODIMP CADSXRootShellFolder::EnumObjects(
	_In_         HWND        hwndOwner,
	_In_         SHCONTF     dwFlags,
	_COM_Outptr_ IEnumIDList **ppEnumIDList
) {
	LOG(P_RSF
		<< L"EnumObjects(dwFlags=[" << SHCONTFToString(&dwFlags) << L"])"
		<< L", Path=[" << PidlToString(m_pidlFSPath) << L"]");
	UNREFERENCED_PARAMETER(hwndOwner);

	if (ppEnumIDList == NULL) return WrapReturn(E_POINTER);
	*ppEnumIDList = NULL;

	// Don't try to enumerate if nothing has been browsed yet
	if (m_pidlFSPath == NULL) return WrapReturn(S_FALSE);

	// Create an enumerator over this file system object's
	// alternate data streams.
	CComObject<CADSXEnumIDList> *pEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return WrapReturn(hr);
	pEnum->AddRef();
	defer({ pEnum->Release(); });

	// Get the path this instance of ADSX is bound to in string form.
	STRRET pName;
	LPWSTR pszName;
	CComPtr<IShellFolder> psf;
	PCUITEMID_CHILD pidlFSPathLast;
	if (m_bPathIsFile) {
		psf = m_psfFSPath;
		pidlFSPathLast = ILFindLastID(m_pidlFSPath);
	} else {
		hr = SHBindToParent(m_pidlFSPath, IID_PPV_ARGS(&psf), &pidlFSPathLast);
		if (FAILED(hr)) return WrapReturn(hr);
	}
	hr = psf->GetDisplayNameOf(pidlFSPathLast, SHGDN_FORPARSING, &pName);
	if (FAILED(hr)) return WrapReturn(hr);
	hr = StrRetToStrW(&pName, pidlFSPathLast, &pszName);
	if (FAILED(hr)) return WrapReturn(hr);
	defer({ CoTaskMemFree(pszName); });

	pEnum->Init(this->GetUnknown(), pszName);

	// Return an IEnumIDList interface to the caller.
	hr = pEnum->QueryInterface(IID_PPV_ARGS(ppEnumIDList));
	return WrapReturn(hr);
}


/**
 * Return if the items represented by the given PIDLs have the attributes
 * requested.
 * For each bit flag:
 *   1 if the flag is set on input and all the given items have that attribute,
 *   0 if the flag is not set on input or if any of the given items do not have
 *   that attribute.
 */
STDMETHODIMP CADSXRootShellFolder::GetAttributesOf(
	_In_    UINT                  cidl,
	_In_    PCUITEMID_CHILD_ARRAY aPidls,
	_Inout_ SFGAOF                *pfAttribs
) {
	LOG(P_RSF << L"GetAttributesOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"pfAttribs=[" << SFGAOFToString(pfAttribs) << L"]"
	L")");

	if (cidl < 1) return WrapReturn(E_INVALIDARG);  // TODO(garlic-os): support more than one item
	if (aPidls == NULL) return WrapReturn(E_POINTER);

	if (cidl == 0 || aPidls[0]->mkid.cb == 0) {
		// Root folder: [Desktop\ADS Explorer] or [ADS Explorer]
		// Not a real filesystem object -> not accessible from ADS Explorer
		LOG(L" ** Root folder");
		*pfAttribs &= SFGAO_HASSUBFOLDER |
		              SFGAO_FOLDER |
		              SFGAO_FILESYSTEM |
		              SFGAO_FILESYSANCESTOR |
		              SFGAO_NONENUMERATED;
	} else if (!CADSXItem::IsOwn(aPidls[0])) {
		// Files and folders
		// FS objects along the way to and including the requested file/folder
		LOG(L" ** FS Object");
		// HRESULT hr = m_psfFSPath->GetAttributesOf(cidl, aPidls, pfAttribs);
		// if (FAILED(hr)) return WrapReturn(hr);
		// *pfAttribs &= SFGAO_BROWSABLE;
		*pfAttribs &= SFGAO_HASSUBFOLDER |
		              SFGAO_FOLDER |
		              SFGAO_FILESYSTEM |
		              SFGAO_FILESYSANCESTOR;
	} else {
		// ADSes
		// The CADSXItems wrapped in PIDLs that were returned from EnumObjects
		// for this file/folder.
		LOG(L" ** ADS");
		*pfAttribs &= SFGAO_FILESYSTEM |
		              SFGAO_CANCOPY |
		              SFGAO_CANMOVE |
		              SFGAO_CANRENAME |
		              SFGAO_CANDELETE;
	}

	LOG(L" ** Result: " << SFGAOFToString(pfAttribs));
	return WrapReturn(S_OK);
}


// Provide any of several sub-objects like IExtractIcon and IDataObject.
STDMETHODIMP CADSXRootShellFolder::GetUIObjectOf(
	_In_         HWND                  hwndOwner,
	_In_         UINT                  cidl,
	_In_         PCUITEMID_CHILD_ARRAY aPidls,
	_In_         REFIID                riid,
	_Inout_      UINT                  *rgfReserved,
	_COM_Outptr_ void                  **ppUIObject
) {
	LOG(P_RSF << L"GetUIObjectOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);
	UNREFERENCED_PARAMETER(hwndOwner);

	HRESULT hr;

	if (ppUIObject == NULL) return WrapReturn(E_POINTER);
	*ppUIObject = NULL;

	if (cidl == 0) return WrapReturn(E_INVALIDARG);

	// We must be in the FileDialog; it wants aPidls wrapped in an IDataObject
	// (just to call IDataObject::GetData() and nothing else).
	// https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample#HowItsDone_UseCasesFileDialog_ClickIcon
	// TODO(garlic-os): It was a design descision for Hurni's NSE to support
	// only one item at a time. I should consider supporting multiple.
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) return WrapReturn(E_INVALIDARG);

		// Is this really one of our item?
		if (!CADSXItem::IsOwn(aPidls[0])) return WrapReturn(E_INVALIDARG);

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) return WrapReturn(hr);

		// AddRef it while we are working with it to keep it from an early
		// destruction.
		pDataObject->AddRef();
		// Tie its lifetime with this object (the IShellFolder object)
		// and embed the PIDL in the data
		pDataObject->Init(this->GetUnknown(), m_pidlRoot, aPidls[0]);
		// Return the requested interface to the caller
		hr = pDataObject->QueryInterface(riid, ppUIObject);
		// We do no more need our ref (note that the object will not die because
		// the QueryInterface above, AddRef'd it)
		pDataObject->Release();
		return WrapReturn(hr);
	}

	// TODO(garlic-os): implement other interfaces as listed in
	// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getuiobjectof#remarks.
	// OpenWindows had the luxury of their objects being real/normal filesystem
	// objects (i.e., the folders other Explorer windows were open to), so it
	// could just proxy these requests on to those objects ('s parent folders).
	// Our objects are not real/normal filesystem objects, so we have to
	// implement these interfaces ourselves.
	else if (riid == IID_IContextMenu) {
		return WrapReturnFailOK(E_NOINTERFACE);
	}

	else if (riid == IID_IContextMenu2) {
		return WrapReturnFailOK(E_NOINTERFACE);
	}
	
	else if (riid == IID_IDropTarget) {
		return WrapReturnFailOK(E_NOINTERFACE);
	}

	else if (riid == IID_IExtractIcon) {
		return WrapReturnFailOK(E_NOINTERFACE);
	}

	else if (riid == IID_IQueryInfo) {
		return WrapReturnFailOK(E_NOINTERFACE);
	}

	return WrapReturnFailOK(E_NOINTERFACE);
}


STDMETHODIMP CADSXRootShellFolder::BindToStorage(
	_In_         PCUIDLIST_RELATIVE,
	_In_         IBindCtx *,
	_In_         REFIID,
	_COM_Outptr_ void **ppStorage
) {
	LOG(P_RSF << L"BindToStorage()");
	if (ppStorage != NULL) *ppStorage = NULL;
	return WrapReturnFailOK(E_NOTIMPL);
}


/**
 * Return a string form of the path this object represents.
 *
 * @pre: *pName struct is initialized
 */
STDMETHODIMP CADSXRootShellFolder::GetDisplayNameOf(
	_In_  PCUITEMID_CHILD pidl,
	_In_  SHGDNF          uFlags,
	_Out_ STRRET          *pName
) {
	LOG(P_RSF << L"GetDisplayNameOf("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"uFlags=[" << SHGDNFToString(&uFlags) << L"]"
	L")");

	if (pidl == NULL || pName == NULL) return WrapReturn(E_POINTER);

	// Return name of Root
	if (pidl->mkid.cb == 0) {
		switch (uFlags) {
			// If wantsFORPARSING is present in the registry.
			// As stated in the SDK, we should return here our virtual junction
			// point which is in the form "::{GUID}" So we should return
			// "::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}".
			case SHGDN_NORMAL | SHGDN_FORPARSING:
				LOG(L" ** Root folder");
				return WrapReturn(
					SetReturnString(
						L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}",
						pName
					) ? S_OK : E_FAIL
				);
			default:
				// We don't handle other combinations of flags for the root pidl
				return WrapReturn(E_FAIL);
				// LOG(L" ** Root folder");
				// return WrapReturn(
				// 	SetReturnString(
				// 		L"GetDisplayNameOf test",
				// 		pName
				// 	) ? S_OK : E_FAIL
				// );
		}
	}

	if (!CADSXItem::IsOwn(pidl)) {
		LOG(L" ** FS Object");
		// NOTE(garlic-os): Has returned E_INVALIDARG on [Desktop\C:\] before
		// and I don't know why
		return WrapReturnFailOK(m_psfFSPath->GetDisplayNameOf(pidl, uFlags, pName));
	}

	LOG(L" ** ADS");
	auto Item = CADSXItem::Get(pidl);
	switch (uFlags) {
		case SHGDN_NORMAL | SHGDN_FORPARSING: {
			// "Desktop\::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}\{fs object's path}:{ADS name}"
			PCIDLIST_ABSOLUTE pidlADSXFSPath = ILCombine(
				m_pidlRoot,
				ILNext(m_pidlFSPath)  // remove [Desktop]
			);
			PWSTR pszPath = NULL;
			HRESULT hr = SHGetNameFromIDList(
				pidlADSXFSPath,
				SIGDN_DESKTOPABSOLUTEPARSING,
				&pszPath
			);
			if (FAILED(hr)) return WrapReturn(hr);
			defer({ CoTaskMemFree(pszPath); });
			std::wostringstream ossPath;
			ossPath << pszPath << L":" << Item->pszName;
			return WrapReturn(
				SetReturnString(ossPath.str().c_str(), pName) ? S_OK : E_FAIL
			);
		}

		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return WrapReturn(E_FAIL);  // TODO(garlic-os)
			// return E_FAIL;

		case SHGDN_INFOLDER:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
		default:
			return WrapReturn(
				SetReturnString(Item->pszName, pName) ? S_OK : E_FAIL
			);
			// return SetReturnString(Item->pszName, *pName) ? S_OK : E_FAIL;
	}
}


STDMETHODIMP CADSXRootShellFolder::ParseDisplayName(
	_In_        HWND             hwnd,
	_In_opt_    IBindCtx         *pbc,
	_In_        LPWSTR           pszDisplayName,
	_Out_opt_   ULONG            *pchEaten,
	_Outptr_    PIDLIST_RELATIVE *ppidl,
	_Inout_opt_ SFGAOF           *pfAttributes
) {
	LOG(P_RSF << L"ParseDisplayName("
		L"name=\"" << pszDisplayName << L"\", "
		L"attributes=[" << SFGAOFToString(pfAttributes) << L"]"
	L")");

	if (pchEaten != NULL) {
		*pchEaten = 0;
	}

	HRESULT hr;
	hr = m_psfFSPath->ParseDisplayName(
		hwnd,
		pbc,
		pszDisplayName,
		pchEaten,
		ppidl,
		pfAttributes
	);
	if (FAILED(hr)) return WrapReturn(hr);
	LOG(" ** Parsed: [" << PidlToString(*ppidl) << L"]");
	LOG(" ** Attributes: " << SFGAOFToString(pfAttributes));

	return WrapReturn(S_OK);
}


// TODO(garlic-os): should this be implemented?
STDMETHODIMP CADSXRootShellFolder::SetNameOf(
	_In_     HWND,
	_In_     PCUITEMID_CHILD,
	_In_     LPCWSTR,
	_In_     SHGDNF,
	_Outptr_ PITEMID_CHILD *
) {
	LOG(P_RSF << L"SetNameOf()");
	return WrapReturnFailOK(E_NOTIMPL);
}


//-------------------------------------------------------------------------------
// IShellDetails

STDMETHODIMP CADSXRootShellFolder::ColumnClick(_In_ UINT uColumn) {
	LOG(P_RSF << L"ColumnClick(uColumn=" << uColumn << L")");
	// Tell the caller to sort the column itself
	return WrapReturn(S_FALSE);
}


// Called for uColumn = 0, 1, 2, ... until function returns E_FAIL
// (uColumn >= DETAILS_COLUMN_MAX)
STDMETHODIMP CADSXRootShellFolder::GetDetailsOf(
	_In_opt_ PCUITEMID_CHILD pidl,
	_In_     UINT uColumn,
	_Out_    SHELLDETAILS *pDetails
) {
	LOG(P_RSF << L"GetDetailsOf("
		L"uColumn=" << uColumn << L", "
		L"pidl=[" << PidlToString(pidl) << L"])"
	);

	HRESULT hr;

	// Shell is asking for the column headers
	if (pidl == NULL) {
		// Load the uColumn based string from the resource
		// TODO(garlic-os): do we haaave to use CString here?
		// this entire kind of string is not used anywhere else in the program
		if (uColumn >= DETAILS_COLUMN_MAX) return WrapReturnFailOK(E_FAIL);
		const WORD wResourceID = IDS_COLUMN_NAME + uColumn;
		const CStringW ColumnName(MAKEINTRESOURCE(wResourceID));
		pDetails->fmt = LVCFMT_LEFT;
		pDetails->cxChar = 32;
		return WrapReturn(
			SetReturnString(
				static_cast<LPCWSTR>(ColumnName),
				&pDetails->str
			) ? S_OK : E_OUTOFMEMORY
		);
	}

	if (!CADSXItem::IsOwn(pidl)) {
		// Lazy load this because this doesn't happen for every shellfolder
		// instance (e.g., during browsing's "drill down" phase)
		if (m_psdFSPath == NULL) {
			hr = m_psfFSPath->BindToObject(pidl, NULL, IID_PPV_ARGS(&m_psdFSPath));
			if (FAILED(hr)) return WrapReturnFailOK(hr);
		}
		return WrapReturnFailOK(
			m_psdFSPath->GetDetailsOf(pidl, uColumn, pDetails)
		);
	}

	if (uColumn >= DETAILS_COLUMN_MAX) return WrapReturnFailOK(E_FAIL);

	// Okay, this time it's for a real item
	auto Item = CADSXItem::Get(pidl);
	switch (uColumn) {
		case DETAILS_COLUMN_NAME:
			pDetails->fmt = LVCFMT_LEFT;
			ATLASSERT(wcslen(Item->pszName) <= INT_MAX);
			pDetails->cxChar = static_cast<int>(wcslen(Item->pszName));
			return WrapReturn(
				SetReturnString(
					Item->pszName,
					&pDetails->str
				) ? S_OK : E_OUTOFMEMORY
			);

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			constexpr UINT8 uLongLongStrLenMax =
				_countof("-9,223,372,036,854,775,808");
			WCHAR pszSize[uLongLongStrLenMax] = {0};
			StrFormatByteSizeW(Item->llFilesize, pszSize, uLongLongStrLenMax);
			pDetails->cxChar = static_cast<UINT8>(wcslen(pszSize));
			return WrapReturn(
				SetReturnString(pszSize, &pDetails->str) ? S_OK : E_OUTOFMEMORY
			);
	}

	return WrapReturn(E_INVALIDARG);
}


//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP
CADSXRootShellFolder::EnumSearches(_COM_Outptr_ IEnumExtraSearch **ppEnum) {
	LOG(P_RSF << L"EnumSearches()");
	if (ppEnum != NULL) *ppEnum = NULL;
	return WrapReturnFailOK(E_NOTIMPL);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultColumn(
	_In_  DWORD dwReserved,
	_Out_ ULONG *pSort,
	_Out_ ULONG *pDisplay
) {
	LOG(P_RSF << L"GetDefaultColumn()");

	if (pSort == NULL || pDisplay == NULL) return WrapReturn(E_POINTER);

	*pSort = DETAILS_COLUMN_NAME;
	*pDisplay = DETAILS_COLUMN_NAME;

	return WrapReturn(S_OK);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultColumnState(
	_In_  UINT uColumn,
	_Out_ SHCOLSTATEF *pcsFlags
) {
	LOG(P_RSF << L"GetDefaultColumnState(uColumn=" << uColumn << L")");

	if (pcsFlags == NULL) return WrapReturn(E_POINTER);

	// Seems that SHCOLSTATE_PREFER_VARCMP doesn't have any noticeable effect
	// (if supplied or not) for Win2K, but don't set it for WinXP, since it will
	// not sort the column. (not setting it means that our CompareIDs() will be
	// called)
	switch (uColumn) {
		case DETAILS_COLUMN_NAME:
			*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
			break;
		case DETAILS_COLUMN_FILESIZE:
			*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;
			break;
		default:
			return WrapReturn(E_INVALIDARG);
	}

	return WrapReturn(S_OK);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultSearchGUID(_Out_ GUID *pguid) {
	LOG(P_RSF << L"GetDefaultSearchGUID()");
	return WrapReturnFailOK(E_NOTIMPL);
}


STDMETHODIMP CADSXRootShellFolder::GetDetailsEx(
	_In_  PCUITEMID_CHILD pidl,
	_In_  const SHCOLUMNID *pscid,
	_Out_ VARIANT *pv
) {
	LOG(P_RSF << L"GetDetailsEx("
		L"pscid->pid=" << pscid->pid << L", "
		L"pidl=[" << PidlToString(pidl) << L"])"
	);

	#ifdef ADSX_PKEYS_SUPPORT
		// Vista required. It appears ItemNameDisplay and ItemPathDisplay come
		// from their real FS representation. The API is also wide-only and is
		// only available on XP SP2+ on, so it won't harm 9x.
		if (IsEqualPropertyKey(*pscid, PKEY_PropList_TileInfo)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_TileInfo");
			return WrapReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo");
			return WrapReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_PreviewDetails");
			return WrapReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_FullDetails");
			return WrapReturn(InitVariantFromString(L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
			LOG(L" ** GetDetailsEx: PKEY_ItemType");
			return WrapReturn(InitVariantFromString(L"Directory", pv));
		}
	#endif

	return WrapReturnFailOK(E_NOTIMPL);
}


// Called for uColumn = 0, 1, 2, ... until function returns E_FAIL
STDMETHODIMP CADSXRootShellFolder::MapColumnToSCID(
	_In_ UINT uColumn,
	_Out_ SHCOLUMNID *pscid
) {
	LOG(P_RSF << L"MapColumnToSCID(uColumn=" << uColumn << L")");

	#if defined(ADSX_PKEYS_SUPPORT)
		// This will map the columns to some built-in properties on Vista.
		// It's needed for the tile subtitles to display properly.
		switch (uColumn) {
			case DETAILS_COLUMN_NAME:
				*pscid = PKEY_ItemNameDisplay;
				return WrapReturn(S_OK);
			case DETAILS_COLUMN_FILESIZE:
				// TODO(garlic-os): is this right? where are PKEYs'
				// documentation?
				*pscid = PKEY_TotalFileSize;
				// *pscid = PKEY_Size;
				// *pscid = PKEY_FileAllocationSize;
				return WrapReturn(S_OK);
			default:
				return WrapReturnFailOK(E_FAIL);
		}
	#endif
	return WrapReturnFailOK(E_NOTIMPL);
}
