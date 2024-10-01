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

// STRRET helper function
bool SetReturnString(LPCWSTR pszSource, STRRET &strret) {
	LOG(L" ** " << pszSource);
	SIZE_T cwStringLen = wcslen(pszSource) + 1;
	strret.uType = STRRET_WSTR;
	strret.pOleStr = static_cast<LPOLESTR>(
		CoTaskMemAlloc(cwStringLen * sizeof(OLECHAR))
	);
	if (strret.pOleStr == NULL) return false;

	wcsncpy_s(strret.pOleStr, cwStringLen, pszSource, cwStringLen);
	return true;
}


//==============================================================================
// CADSXRootShellFolder
CADSXRootShellFolder::CADSXRootShellFolder()
	: m_pidlRoot(NULL)
	, m_pidlFSPath(NULL) {
	// LOG(P_RSF << L"CONSTRUCTOR");
}


CADSXRootShellFolder::~CADSXRootShellFolder() {
	// LOG(P_RSF << L"DESTRUCTOR");
	if (m_pidlRoot != NULL) CoTaskMemFree(m_pidlRoot);
	if (m_pidlFSPath != NULL) CoTaskMemFree(m_pidlFSPath);
}


STDMETHODIMP CADSXRootShellFolder::GetClassID(_Out_ CLSID *pclsid) {
	if (pclsid == NULL) return LogReturn(E_POINTER);
	*pclsid = CLSID_ADSExplorerRootShellFolder;
	return LogReturn(S_OK);
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

	// Enforce that this function is only called once
	ATLASSERT(m_pidlRoot != NULL);
	// if (m_pidlRoot != NULL) {
	// 	return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	// }

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
	// if (ppidl == NULL) return LogReturn(E_POINTER);
	*ppidl = ILCloneFull(m_pidlRoot);
	return *ppidl != NULL ? S_OK : E_OUTOFMEMORY;
	// return LogReturn(*ppidl != NULL ? S_OK : E_OUTOFMEMORY);
}


//-------------------------------------------------------------------------------
// IShellFolder

/// Called when an item in an ADSX folder is double-clicked.
/// TODO(garlic-os): Explain this function better
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	_In_         PCUIDLIST_RELATIVE pidl,
	_In_opt_     IBindCtx           *pbc,
	_In_         REFIID             riid,
	_COM_Outptr_ void               **ppv
) {
	LOG(P_RSF << L"BindToObject("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);

	if (ppv == NULL) return LogReturn(E_POINTER);
	*ppv = NULL;

	if (riid != IID_IShellFolder && riid != IID_IShellFolder2) {
		return LogReturn(E_NOINTERFACE);
	}

	HRESULT hr;

	// TODO(garlic-os): Better way to check when we're at the end of the path?
	// Assumption here is BindToObject receives a PIDL of length 1 iff we are at
	// the end of the path.
	if (ILIsChild(pidl)) {
		// We've made it to the target FS object's direct parent.
		// Our normal ShellFolder companion cannot browse any farther. It is now
		// right where we need it to be to get the target object's path for
		// EnumObjects. From here, we browse one level deeper without it.
		// m_psfFSPath and m_pidlFSPath had always stayed in sync, but now
		// m_psfFSPath is ..\{fs object path}, and upon returning from this
		// function m_pidlFSPath will be {fs object path}.
		LOG(L" ** End of path reached");
		m_pidlFSPath = static_cast<PIDLIST_ABSOLUTE>(
			ILAppendID(m_pidlFSPath, &pidl->mkid, TRUE)
		);
		if (m_pidlFSPath == NULL) return LogReturn(E_OUTOFMEMORY);
	} else {
		// Browse this path internally.
		hr = m_psfFSPath->BindToObject(pidl, pbc, riid, ppv);
		if (FAILED(hr)) return LogReturn(hr);
		m_psfFSPath = static_cast<IShellFolder*>(*ppv);

		if (m_pidlFSPath != NULL) CoTaskMemFree(m_pidlFSPath);
		m_pidlFSPath = static_cast<PIDLIST_ABSOLUTE>(ILClone(pidl));
		if (m_pidlFSPath == NULL) return LogReturn(E_OUTOFMEMORY);
	}

	LOG(L" ** New m_pidlFSPath: " << PidlToString(m_pidlFSPath));

	// Return self as the ShellFolder.
	// TODO(garlic-os): Windows may not like receiving the same object back and
	// may expect a copy
	this->AddRef();
	*ppv = this;
	return LogReturn(S_OK);
}


/// Return the sort order of two PIDLs.
/// lParam can be the 0-based Index of the details column
/// @pre pidl1 and pidl2 hold CADSXItems.
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

	// Check if the PIDLs are ours
	ATLASSERT(CADSXItem::IsOwn(pidl1) && CADSXItem::IsOwn(pidl2));
	if (!CADSXItem::IsOwn(pidl1) || !CADSXItem::IsOwn(pidl2)) {
		return LogReturn(E_INVALIDARG);
	}

	// Only child PIDLs supported
	ATLASSERT(ILIsChild(pidl1) && ILIsChild(pidl2));
	if (!ILIsChild(pidl1) || !ILIsChild(pidl2)) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
	}

	USHORT Result = 0;  // see note below (MAKE_HRESULT)

	auto Item1 = CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl1));
	auto Item2 = CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl2));

	switch (lParam & SHCIDS_COLUMNMASK) {
		case DETAILS_COLUMN_NAME:
			Result = Item1->m_Name.compare(Item2->m_Name);
			break;
		case DETAILS_COLUMN_FILESIZE:
			Result = static_cast<USHORT>(Item1->llFilesize - Item2->llFilesize);
			if (Result < 0) Result = -1;
			else if (Result > 0) Result = 1;
			break;
		default:
			return LogReturn(E_INVALIDARG);
	}

	// Warning: the last param MUST be unsigned, if not (ie: short) a negative
	// value will trash the high order word of the HRESULT!
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, /*-1,0,1*/ Result);
}


/// Return a COM object that implements IShellView.
STDMETHODIMP CADSXRootShellFolder::CreateViewObject(
	_In_         HWND   hwndOwner,
	_In_         REFIID riid,
	_COM_Outptr_ void   **ppv
) {
	// Not logging all interface requests because there are too many
	// LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");

	if (ppv == NULL) return E_POINTER;
	// if (ppv == NULL) return LogReturn(E_POINTER);
	*ppv = NULL;

	HRESULT hr;

	// We only handle IShellView
	if (riid == IID_IShellView) {
		LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");
		// Create a view object
		CComObject<CADSXRootShellView> *pViewObject;
		hr = CComObject<CADSXRootShellView>::CreateInstance(&pViewObject);
		if (FAILED(hr)) return LogReturn(hr);

		// AddRef the object while we are using it
		pViewObject->AddRef();

		// Tie the view object lifetime with the current IShellFolder.
		pViewObject->Init(this->GetUnknown());

		// Create the view
		hr = pViewObject->Create(
			reinterpret_cast<IShellView **>(ppv),
			hwndOwner,
			static_cast<IShellFolder *>(this)
		);

		// We are finished with our own use of the view object (AddRef()'d
		// above by us, AddRef()'ed by Create)
		pViewObject->Release();

		return LogReturn(hr);
	}

	return E_NOINTERFACE;
	// return LogReturn(E_NOINTERFACE);
}


/// Return a COM object that implements IEnumIDList and enumerates the ADSes in
/// the current folder.
/// @pre: Windows has browsed to a path of the format
///       [Desktop\ADS Explorer\{FS path}]
/// @pre: i.e., m_pidlFSPath is [Desktop\{FS path}]
/// @post: ppEnumIDList holds a CADSXEnumIDList** on {FS path}
STDMETHODIMP CADSXRootShellFolder::EnumObjects(
	_In_         HWND        hwndOwner,
	_In_         SHCONTF     dwFlags,
	_COM_Outptr_ IEnumIDList **ppEnumIDList
) {
	LOG(P_RSF << L"EnumObjects(dwFlags=[" << SHCONTFToString(&dwFlags) << L"])");
	UNREFERENCED_PARAMETER(hwndOwner);

	if (ppEnumIDList == NULL) return LogReturn(E_POINTER);
	*ppEnumIDList = NULL;

	// Don't try to enumerate if nothing has been browsed yet
	if (m_pidlFSPath == NULL) return LogReturn(S_FALSE);

	// Create an enumerator over this file system object's
	// alternate data streams.
	CComObject<CADSXEnumIDList> *pEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return LogReturn(hr);
	pEnum->AddRef();
	defer({ pEnum->Release(); });

	// Get the path this instance of ADSX is bound to in string form.
	STRRET pName;
	LPWSTR pszName;
	PUITEMID_CHILD pidlFSPathLast = ILFindLastID(m_pidlFSPath);
	hr = m_psfFSPath->GetDisplayNameOf(pidlFSPathLast, SHGDN_FORPARSING, &pName);
	if (FAILED(hr)) return LogReturn(hr);
	hr = StrRetToStrW(&pName, pidlFSPathLast, &pszName);
	if (FAILED(hr)) return LogReturn(hr);

	LOG(L" ** EnumObjects: Path=" << pszName);
	pEnum->Init(this->GetUnknown(), pszName);

	// Return an IEnumIDList interface to the caller.
	hr = pEnum->QueryInterface(IID_PPV_ARGS(ppEnumIDList));
	return LogReturn(hr);
}


/// Return if the items represented by the given PIDLs have the attributes
/// requested.
/// For each bit flag:
///   1 if the flag is set on input and all the given items have that attribute,
///   0 if the flag is not set on input or if any of the given items do not have
///   that attribute.
STDMETHODIMP CADSXRootShellFolder::GetAttributesOf(
	_In_    UINT                  cidl,
	_In_    PCUITEMID_CHILD_ARRAY aPidls,
	_Inout_ SFGAOF                *pfAttribs
) {
	LOG(P_RSF << L"GetAttributesOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"pfAttribs=[" << SFGAOFToString(pfAttribs) << L"]"
	L")");

	if (cidl < 1) return LogReturn(E_INVALIDARG);  // TODO(garlic-os): support more than one item
	if (aPidls == NULL) return LogReturn(E_POINTER);

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
		// Parent folders
		// Folders along the way to and including the requested file/folder
		LOG(L" ** FS Object");
		// HRESULT hr = m_psfFSPath->GetAttributesOf(cidl, aPidls, pfAttribs);
		// if (FAILED(hr)) return LogReturn(hr);
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
	return LogReturn(S_OK);
}


/// GetUIObjectOf() is called to get several sub-objects like IExtractIcon and
/// IDataObject
STDMETHODIMP CADSXRootShellFolder::GetUIObjectOf(
	_In_         HWND                  hwndOwner,
	_In_         UINT                  cidl,
	_In_         PCUITEMID_CHILD_ARRAY aPidls,
	_In_         REFIID                riid,
	_Inout_      UINT                  *rgfReserved,
	_COM_Outptr_ void                  **ppv
) {
	LOG(P_RSF << L"GetUIObjectOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);
	UNREFERENCED_PARAMETER(hwndOwner);

	HRESULT hr;

	if (ppv == NULL) return LogReturn(E_POINTER);
	*ppv = NULL;

	if (cidl == 0) return LogReturn(E_INVALIDARG);

	// We must be in the FileDialog; it wants aPidls wrapped in an IDataObject
	// (just to call IDataObject::GetData() and nothing else).
	// https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample#HowItsDone_UseCasesFileDialog_ClickIcon
	// TODO(garlic-os): It was a design descision for Hurni's NSE to support
	// only one item at a time. I should consider supporting multiple.
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) return LogReturn(E_INVALIDARG);

		// Is this really one of our item?
		if (!CADSXItem::IsOwn(aPidls[0])) return LogReturn(E_INVALIDARG);

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) return LogReturn(hr);

		// AddRef it while we are working with it to keep it from an early
		// destruction.
		pDataObject->AddRef();
		// Tie its lifetime with this object (the IShellFolder object)
		// and embed the PIDL in the data
		pDataObject->Init(this->GetUnknown(), m_pidlRoot, aPidls[0]);
		// Return the requested interface to the caller
		hr = pDataObject->QueryInterface(riid, ppv);
		// We do no more need our ref (note that the object will not die because
		// the QueryInterface above, AddRef'd it)
		pDataObject->Release();
		return LogReturn(hr);
	}

	// TODO(garlic-os): implement other interfaces as listed in
	// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getuiobjectof#remarks.
	// OpenWindows had the luxury of their objects being real/normal filesystem
	// objects (i.e., the folders other Explorer windows were open to), so it
	// could just proxy these requests on to those objects ('s parent folders).
	// Our objects are not real/normal filesystem objects, so we have to
	// implement these interfaces ourselves.
	else if (riid == IID_IContextMenu) {
		return LogReturn(E_NOINTERFACE);
	}

	else if (riid == IID_IContextMenu2) {
		return LogReturn(E_NOINTERFACE);
	}
	
	else if (riid == IID_IDropTarget) {
		return LogReturn(E_NOINTERFACE);
	}

	else if (riid == IID_IExtractIcon) {
		return LogReturn(E_NOINTERFACE);
	}

	else if (riid == IID_IQueryInfo) {
		return LogReturn(E_NOINTERFACE);
	}

	return LogReturn(E_NOINTERFACE);
}


STDMETHODIMP CADSXRootShellFolder::BindToStorage(
	_In_         PCUIDLIST_RELATIVE,
	_In_         IBindCtx *,
	_In_         REFIID,
	_COM_Outptr_ void **ppv
) {
	LOG(P_RSF << L"BindToStorage()");
	if (ppv != NULL) *ppv = NULL;
	return LogReturn(E_NOTIMPL);
}


STDMETHODIMP CADSXRootShellFolder::GetDisplayNameOf(
	_In_  PCUITEMID_CHILD pidl,
	_In_  SHGDNF          uFlags,
	_Out_ STRRET          *pName
) {
	LOG(P_RSF << L"GetDisplayNameOf("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"uFlags=[" << SHGDNFToString(&uFlags) << L"]"
	L")");

	if (pidl == NULL || pName == NULL) return LogReturn(E_POINTER);

	// Return name of Root
	if (pidl->mkid.cb == 0) {
		switch (uFlags) {
			// If wantsFORPARSING is present in the registry.
			// As stated in the SDK, we should return here our virtual junction
			// point which is in the form "::{GUID}" So we should return
			// "::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}".
			case SHGDN_NORMAL | SHGDN_FORPARSING:
				LOG(L" ** Root folder");
				return LogReturn(
					SetReturnString(
						L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}",
						*pName
					) ? S_OK : E_FAIL
				);
			default:
				// We don't handle other combinations of flags for the root pidl
				return LogReturn(E_FAIL);
				// LOG(L" ** Root folder");
				// return LogReturn(
				// 	SetReturnString(
				// 		L"GetDisplayNameOf test",
				// 		*pName
				// 	) ? S_OK : E_FAIL
				// );
		}
	}

	if (!CADSXItem::IsOwn(pidl)) {
		LOG(L" ** FS Object");
		return LogReturn(m_psfFSPath->GetDisplayNameOf(pidl, uFlags, pName));
	}

	LOG(L" ** ADS");
	auto Item = CADSXItem::Get(pidl);
	switch (uFlags) {
		case SHGDN_NORMAL | SHGDN_FORPARSING: {
			// "Desktop\::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}\{fs object's path}:{ADS's m_Name}"
			PIDLIST_ABSOLUTE pidlADSXFSPath = ILCombine(
				m_pidlRoot,
				ILNext(m_pidlFSPath)  // remove [Desktop]
			);
			PWSTR pszPath = NULL;
			HRESULT hr = SHGetNameFromIDList(
				pidlADSXFSPath,
				SIGDN_DESKTOPABSOLUTEPARSING,
				&pszPath
			);
			if (FAILED(hr)) return LogReturn(hr);
			defer({ CoTaskMemFree(pszPath); });
			std::wostringstream ossPath;
			ossPath << pszPath << L":" << Item->m_Name;
			return LogReturn(
				SetReturnString(ossPath.str().c_str(), *pName) ? S_OK : E_FAIL
			);
		}

		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return LogReturn(E_FAIL);  // TODO(garlic-os)
			// return E_FAIL;  // TODO(garlic-os)

		case SHGDN_INFOLDER:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
		default:
			return LogReturn(
				SetReturnString(Item->m_Name.c_str(), *pName) ? S_OK : E_FAIL
			);
			// return SetReturnString(Item->m_Name.c_str(), *pName)
			// 	? S_OK : E_FAIL;
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
	if (FAILED(hr)) return LogReturn(hr);
	LOG(" ** Parsed: [" << PidlToString(*ppidl) << L"]");
	LOG(" ** Attributes: " << SFGAOFToString(pfAttributes));

	return LogReturn(S_OK);
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
	return LogReturn(E_NOTIMPL);
}


//-------------------------------------------------------------------------------
// IShellDetails

STDMETHODIMP CADSXRootShellFolder::ColumnClick(_In_ UINT uColumn) {
	LOG(P_RSF << L"ColumnClick(uColumn=" << uColumn << L")");
	// Tell the caller to sort the column itself
	return LogReturn(S_FALSE);
}


STDMETHODIMP CADSXRootShellFolder::GetDetailsOf(
	_In_opt_ PCUITEMID_CHILD pidl,
	_In_     UINT uColumn,
	_Out_    SHELLDETAILS *pDetails
) {
	LOG(P_RSF << L"GetDetailsOf("
		L"uColumn=" << uColumn << L", "
		L"pidl=[" << PidlToString(pidl) << L"])"
	);

	if (uColumn >= DETAILS_COLUMN_MAX) return LogReturn(E_FAIL);

	// Shell asks for the column headers
	if (pidl == NULL) {
		// Load the uColumn based string from the resource
		// TODO(garlic-os): do we haaave to use CString here?
		// this entire kind of string is not used anywhere else in the program
		WORD wResourceID = IDS_COLUMN_NAME + uColumn;
		CStringW ColumnName(MAKEINTRESOURCE(wResourceID));
		pDetails->fmt = LVCFMT_LEFT;
		pDetails->cxChar = 32;
		return LogReturn(
			SetReturnString(ColumnName, pDetails->str) ? S_OK : E_OUTOFMEMORY
		);
	}

	// Okay, this time it's for a real item
	auto Item = CADSXItem::Get(pidl);
	switch (uColumn) {
		case DETAILS_COLUMN_NAME:
			pDetails->fmt = LVCFMT_LEFT;
			ATLASSERT(Item->m_Name.length() <= INT_MAX);
			pDetails->cxChar = static_cast<int>(Item->m_Name.length());
			return LogReturn(
				SetReturnString(Item->m_Name.c_str(), pDetails->str) ? S_OK : E_OUTOFMEMORY
			);

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			constexpr UINT8 uLongLongStrLenMax =
				_countof("-9,223,372,036,854,775,808");
			WCHAR pszSize[uLongLongStrLenMax] = {0};
			StrFormatByteSizeW(Item->llFilesize, pszSize, uLongLongStrLenMax);
			pDetails->cxChar = static_cast<UINT8>(wcslen(pszSize));
			return LogReturn(
				SetReturnString(pszSize, pDetails->str) ? S_OK : E_OUTOFMEMORY
			);
	}

	return LogReturn(E_INVALIDARG);
}


//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP
CADSXRootShellFolder::EnumSearches(_COM_Outptr_ IEnumExtraSearch **ppEnum) {
	LOG(P_RSF << L"EnumSearches()");
	if (ppEnum != NULL) *ppEnum = NULL;
	return LogReturn(E_NOTIMPL);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultColumn(
	_In_  DWORD dwReserved,
	_Out_ ULONG *pSort,
	_Out_ ULONG *pDisplay
) {
	LOG(P_RSF << L"GetDefaultColumn()");

	if (pSort == NULL || pDisplay == NULL) return LogReturn(E_POINTER);

	*pSort = DETAILS_COLUMN_NAME;
	*pDisplay = DETAILS_COLUMN_NAME;

	return LogReturn(S_OK);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultColumnState(
	_In_  UINT uColumn,
	_Out_ SHCOLSTATEF *pcsFlags
) {
	LOG(P_RSF << L"GetDefaultColumnState(uColumn=" << uColumn << L")");

	if (pcsFlags == NULL) return LogReturn(E_POINTER);

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
			return LogReturn(E_INVALIDARG);
	}

	return LogReturn(S_OK);
}


STDMETHODIMP CADSXRootShellFolder::GetDefaultSearchGUID(_Out_ GUID *pguid) {
	LOG(P_RSF << L"GetDefaultSearchGUID()");
	return LogReturn(E_NOTIMPL);
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
			return LogReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo");
			return LogReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_PreviewDetails");
			return LogReturn(InitVariantFromString(L"prop:System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_FullDetails");
			return LogReturn(InitVariantFromString(L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv));
		} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
			LOG(L" ** GetDetailsEx: PKEY_ItemType");
			return LogReturn(InitVariantFromString(L"Directory", pv));
		}
	#endif

	return LogReturn(E_NOTIMPL);
}


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
				return LogReturn(S_OK);
			case DETAILS_COLUMN_FILESIZE:
				// TODO(garlic-os): is this right? where are PKEYs' documentation?
				*pscid = PKEY_TotalFileSize;
				// *pscid = PKEY_Size;
				// *pscid = PKEY_FileAllocationSize;
				return LogReturn(S_OK);
		}
		return LogReturn(E_FAIL);
	#endif
	return LogReturn(E_NOTIMPL);
}
