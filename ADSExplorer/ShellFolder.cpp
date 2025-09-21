/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

#include "StdAfx.h"  // Precompiled header; include first

#include "ADSExplorer_h.h"
#include "ShellFolder.h"

#include <atlstr.h>
#include <sstream>

#include "EnumIDList.h"
#include "ADSXItem.h"
#include "DataObject.h"
#include "ShellView.h"

// Debug log prefix for ADSX::CShellFolder
#define P_RSF L"ADSX::CShellFolder(0x" << std::hex << this << L")::"

namespace ADSX {

/**
 * STRRET maker
 *
 * @pre: *strret is initialized
 */
bool SetReturnString(_In_ PCWSTR pszSource, _Out_ STRRET *strret) {
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


#pragma region ADSX::CShellFolder
CShellFolder::CShellFolder()
	: m_pidlRoot(NULL)
	, m_pidl(NULL) {
	// LOG(P_RSF << L"CONSTRUCTOR");
}


CShellFolder::~CShellFolder() {
	// LOG(P_RSF << L"DESTRUCTOR");
	if (m_pidlRoot != NULL) CoTaskMemFree(m_pidlRoot);
	if (m_pidl != NULL) CoTaskMemFree(m_pidl);
}
#pragma endregion


#pragma region IPersist
STDMETHODIMP CShellFolder::GetClassID(_Out_ CLSID *pclsid) {
	if (pclsid == NULL) return E_POINTER;
	// if (pclsid == NULL) return WrapReturn(E_POINTER);
	*pclsid = CLSID_ADSExplorerShellFolder;
	return S_OK;
	// return WrapReturn(S_OK);
}
#pragma endregion


#pragma region IPersistFolder
/**
 * Initialize() is passed the PIDL of the folder where our extension is.
 * Copies, does not take ownership of, the PIDL.
 * @pre: PIDL is [Desktop\ADS Explorer] or [ADS Explorer].
 *       (I _assume_ those are the only two values Explorer ever passes to us.)
 * @pre: 0 < PIDL length < 3.
 * @post: this ADSX​::CShellFolder instance is ready to be used.
 */
STDMETHODIMP CShellFolder::Initialize(_In_ PCIDLIST_ABSOLUTE pidlRoot) {
	// LOG(P_RSF << L"Initialize(pidl=[" << PidlToString(pidlRoot) << L"])");

	// Don't initialize more than once.
	// This is necessary because for reasons beyond me Windows tries to.
	if (m_pidlRoot != NULL) {
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
		// return WrapReturn(HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED));
	}

	// Validate input PIDL
	if (
		ILIsEmpty(pidlRoot) ||
		(!ILIsEmpty(ILNext(pidlRoot)) && !ILIsEmpty(ILNext(ILNext(pidlRoot))))
	) {
		// PIDL length is 0 or more than 2
		return E_INVALIDARG;
		// return WrapReturn(E_INVALIDARG);
	}

	// Keep this around for use elsewhere
	m_pidlRoot = ILCloneFull(pidlRoot);
	if (m_pidlRoot == NULL) return E_OUTOFMEMORY;
	// if (m_pidlRoot == NULL) return WrapReturn(E_OUTOFMEMORY);

	// Initialize to the root of the namespace, [Desktop]
	HRESULT hr = SHGetDesktopFolder(&m_psf);
	if (FAILED(hr)) return hr;
	// if (FAILED(hr)) return WrapReturn(hr);

	return hr;
	// return WrapReturn(hr);
}


STDMETHODIMP
CShellFolder::GetCurFolder(_Outptr_ PIDLIST_ABSOLUTE *ppidl) {
	// LOG(P_RSF << L"GetCurFolder()");
	if (ppidl == NULL) return E_POINTER;
	// if (ppidl == NULL) return WrapReturn(E_POINTER);
	*ppidl = ILCloneFull(m_pidlRoot);
	return *ppidl != NULL ? S_OK : E_OUTOFMEMORY;
	// return WrapReturn(*ppidl != NULL ? S_OK : E_OUTOFMEMORY);
}

#pragma endregion


#pragma region IShellFolder
HRESULT CShellFolder::BindToObjectInitialize(
	_In_      IShellFolder*      psfParent,
	_In_      PCIDLIST_ABSOLUTE  pidlRoot,
	_In_      PCIDLIST_ABSOLUTE  pidlParent,
	_In_      PCUIDLIST_RELATIVE pidlNext,
	_In_opt_  IBindCtx*          pbc,
	_In_      REFIID             riid
) {
	HRESULT hr;

	// Carry on the legacy
	m_pidlRoot = ILCloneFull(pidlRoot);
	if (m_pidlRoot == NULL) return WrapReturn(E_OUTOFMEMORY);

	// Browse this path internally.
	hr = psfParent->BindToObject(
		pidlNext,
		pbc,
		riid,
		reinterpret_cast<void**>(&m_psf)
	);
	LOG(L" ** Inner BindToObject -> " << HRESULTToString(hr));
	if (hr == E_FAIL) {
		// Tried to browse into a file, which is fine for us.
		// If EnumObjects is called for this instance of the ADSX Shell Folder,
		// we'll be enumerating the file's ADSes.
		m_psf = psfParent;
	} else if (FAILED(hr)) {
		return WrapReturnFailOK(hr);
	}

	// Set the new instance's internal PIDL.
	// These are the two cases I've seen: either a child PIDL directly relative
	// to our current path, or an absolute path/PIDL.
	// Either way, what the next instance's PIDL is supposed to be is
	// straightforward.
	m_pidl = ILIsChild(pidlNext) ?
		ILCombine(pidlParent, pidlNext) :
		ILCloneFull(reinterpret_cast<PCIDLIST_ABSOLUTE>(pidlNext));
	if (m_pidl == NULL) return WrapReturn(E_OUTOFMEMORY);

	LOG(L" ** New instance's PIDL: " << PidlToString(m_pidl));
	return WrapReturn(S_OK);
}


// TODO(garlic-os): Explain this function
STDMETHODIMP CShellFolder::BindToObject(
	_In_         PCUIDLIST_RELATIVE pidl,
	_In_opt_     IBindCtx*          pbc,
	_In_         REFIID             riid,
	_COM_Outptr_ void**             ppShellFolder
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

	// Return a new instance of self as the Shell Folder.
	CComObject<CShellFolder> *pShellFolder;
	hr = CComObject<CShellFolder>::CreateInstance(&pShellFolder);
	if (FAILED(hr)) return WrapReturn(hr);
	pShellFolder->AddRef();
	defer({ pShellFolder->Release(); });
	hr = pShellFolder->BindToObjectInitialize(
		m_psf,
		m_pidlRoot,
		m_pidl,
		pidl,
		pbc,
		riid
	);
	if (FAILED(hr)) return hr;  // Was already Wrap'd
	hr = pShellFolder->QueryInterface(riid, ppShellFolder);
	if (FAILED(hr)) return WrapReturn(hr);
	
	return WrapReturn(S_OK);
}


/**
 * Return the sort order of two PIDLs.
 * lParam can be the 0-based Index of the details column
 * @pre pidl1 and pidl2 hold ADSX::CItems.
 */
STDMETHODIMP CShellFolder::CompareIDs(
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
	if (!ADSX::CItem::IsOwn(pidl1) && !ADSX::CItem::IsOwn(pidl2)) {
		return m_psf->CompareIDs(lParam, pidl1, pidl2);
	}

	// Both PIDLs must either be ours or not ours
	ATLASSERT(ADSX::CItem::IsOwn(pidl1) && ADSX::CItem::IsOwn(pidl2));
	if (!ADSX::CItem::IsOwn(pidl1) || !ADSX::CItem::IsOwn(pidl2)) {
		return WrapReturn(E_INVALIDARG);
	}

	// Only child ADS PIDLs supported
	ATLASSERT(ILIsChild(pidl1) && ILIsChild(pidl2));
	if (!ILIsChild(pidl1) || !ILIsChild(pidl2)) {
		return WrapReturn(E_INVALIDARG);
	}
	auto Item1 = ADSX::CItem::Get(static_cast<PCUITEMID_CHILD>(pidl1));
	auto Item2 = ADSX::CItem::Get(static_cast<PCUITEMID_CHILD>(pidl2));

	USHORT Result = 0;  // see note below (MAKE_HRESULT)

	switch (lParam & SHCIDS_COLUMNMASK) {
		case DetailsColumn::Name:
			Result = wcscmp(Item1->pszName, Item2->pszName);
			break;
		case DetailsColumn::Filesize:
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
STDMETHODIMP CShellFolder::CreateViewObject(
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
		CComObject<CADSXShellView> *pViewObject;
		hr = CComObject<CADSXShellView>::CreateInstance(&pViewObject);
		if (FAILED(hr)) return WrapReturn(hr);
		pViewObject->AddRef();
		defer({ pViewObject->Release(); });

		// Tie the view object's lifetime with the current IShellFolder.
		pViewObject->Init(this->GetUnknown());

		// Create the view
		hr = pViewObject->Create(
			hwndOwner,
			this,
			NULL,
			reinterpret_cast<IShellView **>(ppViewObject)
		);

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
 * @pre: i.e., m_pidl is [Desktop\{FS path}]
 * @post: ppEnumIDList holds a CEnumIDList** on {FS path}
 */
STDMETHODIMP CShellFolder::EnumObjects(
	_In_         HWND        hwndOwner,
	_In_         SHCONTF     dwFlags,
	_COM_Outptr_ IEnumIDList **ppEnumIDList
) {
	LOG(P_RSF
		<< L"EnumObjects(dwFlags=[" << SHCONTFToString(&dwFlags) << L"])"
		<< L", Path=[" << PidlToString(m_pidl) << L"]");
	UNREFERENCED_PARAMETER(hwndOwner);

	if (ppEnumIDList == NULL) return WrapReturn(E_POINTER);
	*ppEnumIDList = NULL;
	
	// Don't try to enumerate if nothing has been browsed yet
	if (m_pidl == NULL) return WrapReturn(S_FALSE);

	switch (dwFlags) {
		case SHCONTF_FOLDERS | SHCONTF_NONFOLDERS |
		     SHCONTF_INCLUDEHIDDEN | SHCONTF_FASTITEMS:
			// Shell passes these flags when it's doing its preliminary
			// enumeration of the contents of the parent of the requested path.
			// In this case the call belongs to our inner ShellFolder.
			return m_psf->EnumObjects(hwndOwner, dwFlags, ppEnumIDList);
		case SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN |
		     SHCONTF_FASTITEMS | SHCONTF_ENABLE_ASYNC:
			// Shell passes these flags when it's really enumerating the path
			// the user requested.
			break;
		default:
			// We don't expect any other set of flags while the folder is
			// holding a non-null PIDL
			LOG(L" ** Unexpected flags [" << SHCONTFToString(&dwFlags) << L"]");
			ATLASSERT(FALSE);
	}

	HRESULT hr;

	// Get the path this folder is bound to in string form.
	PWSTR pszPath = NULL;
	hr = SHGetNameFromIDList(
		m_pidl,
		SIGDN_DESKTOPABSOLUTEPARSING,
		&pszPath
	);
	if (FAILED(hr)) return WrapReturn(hr);
	defer({ CoTaskMemFree(pszPath); });

	// Create an enumerator over this file system object's
	// alternate data streams.
	CComObject<ADSX::CEnumIDList> *pEnum;
	hr = CComObject<ADSX::CEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return WrapReturn(hr);
	pEnum->AddRef();
	defer({ pEnum->Release(); });
	hr = pEnum->Init(this->GetUnknown(), pszPath);
	if (FAILED(hr)) return WrapReturn(hr);

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
STDMETHODIMP CShellFolder::GetAttributesOf(
	_In_    UINT                  cidl,
	_In_    PCUITEMID_CHILD_ARRAY aPidls,
	_Inout_ SFGAOF                *pfAttribs
) {
	LOG(P_RSF << L"GetAttributesOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"pfAttribs=[" << SFGAOFToString(pfAttribs) << L"]"
	L")");

	if (cidl < 1) return WrapReturn(E_INVALIDARG);  // TODO(nate-kean): support more than one item
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
	} else if (!ADSX::CItem::IsOwn(aPidls[0])) {
		// Files and folders
		// FS objects along the way to and including the requested file/folder
		LOG(L" ** FS Object");
		// HRESULT hr = m_psf->GetAttributesOf(cidl, aPidls, pfAttribs);
		// if (FAILED(hr)) return WrapReturn(hr);
		// *pfAttribs &= SFGAO_BROWSABLE;
		*pfAttribs &= SFGAO_HASSUBFOLDER |
		              SFGAO_FOLDER |
		              SFGAO_FILESYSTEM |
		              SFGAO_FILESYSANCESTOR;
	} else {
		// ADSes
		// The ADSX::CItems wrapped in PIDLs that were returned from EnumObjects
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
STDMETHODIMP CShellFolder::GetUIObjectOf(
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

	// If it's not one of the boys then just act natural
	if (!ADSX::CItem::IsOwn(aPidls[0])) {
		LOG(L" ** Proxying to inner ShellFolder");
		return m_psf->GetUIObjectOf(
			hwndOwner, cidl, aPidls, riid, rgfReserved, ppUIObject);
	}

	// Only one item at a time
	// TODO(nate-kean): It was a design decision for Hurni's NSE to support
	// only one item at a time. I should consider supporting multiple.
	if (cidl != 1) return WrapReturn(E_INVALIDARG);

	HRESULT hr;

	if (ppUIObject == NULL) return WrapReturn(E_POINTER);
	*ppUIObject = NULL;

	if (cidl == 0) return WrapReturn(E_INVALIDARG);

	// We must be in the FileDialog; it wants aPidls wrapped in an IDataObject
	// (just to call IDataObject::GetData() and nothing else).
	// https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample#HowItsDone_UseCasesFileDialog_ClickIcon
	if (riid == IID_IDataObject) {

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
		pDataObject->Release();
		return WrapReturn(hr);
	}

	// TODO(nate-kean): implement other interfaces as listed in
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


STDMETHODIMP CShellFolder::BindToStorage(
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
STDMETHODIMP CShellFolder::GetDisplayNameOf(
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

	if (!ADSX::CItem::IsOwn(pidl)) {
		LOG(L" ** FS Object");
		// NOTE(nate-kean): Has returned E_INVALIDARG on [Desktop\C:\] before
		// and I don't know why
		return WrapReturnFailOK(m_psf->GetDisplayNameOf(pidl, uFlags, pName));
	}

	LOG(L" ** ADS");
	auto Item = ADSX::CItem::Get(pidl);
	switch (uFlags) {
		case SHGDN_NORMAL | SHGDN_FORPARSING: {
			// "Desktop\::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}\{fs object's path}:{ADS name}"
			PCIDLIST_ABSOLUTE pidlADSXFSPath = ILCombine(
				m_pidlRoot,
				ILNext(m_pidl)  // remove [Desktop]
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
			return WrapReturn(E_FAIL);  // TODO(nate-kean)
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


STDMETHODIMP CShellFolder::ParseDisplayName(
	_In_        HWND              hwnd,
	_In_opt_    IBindCtx*         pbc,
	_In_        PWSTR             pszDisplayName,
	_Out_opt_   ULONG*            pchEaten,
	_Outptr_    PIDLIST_RELATIVE* ppidl,
	_Inout_opt_ SFGAOF*           pfAttributes
) {
	LOG(P_RSF << L"ParseDisplayName("
		L"name=\"" << pszDisplayName << L"\", "
		L"attributes=[" << SFGAOFToString(pfAttributes) << L"]"
	L")");

	if (pchEaten != NULL) {
		*pchEaten = 0;
	}

	HRESULT hr;
	hr = m_psf->ParseDisplayName(
		hwnd,
		pbc,
		pszDisplayName,
		pchEaten,
		ppidl,
		pfAttributes
	);
	// if (FAILED(hr)) return WrapReturn(hr);
	if (FAILED(hr)) return WrapReturnFailOK(hr);
	LOG(" ** Parsed: [" << PidlToString(*ppidl) << L"]");
	LOG(" ** Attributes: " << SFGAOFToString(pfAttributes));

	return WrapReturn(S_OK);
}


// TODO(nate-kean): should this be implemented?
STDMETHODIMP CShellFolder::SetNameOf(
	_In_     HWND,
	_In_     PCUITEMID_CHILD,
	_In_     PCWSTR,
	_In_     SHGDNF,
	_Outptr_ PITEMID_CHILD *
) {
	LOG(P_RSF << L"SetNameOf()");
	return WrapReturnFailOK(E_NOTIMPL);
}

#pragma endregion


#pragma region IShellDetails

STDMETHODIMP CShellFolder::ColumnClick(_In_ UINT uColumn) {
	LOG(P_RSF << L"ColumnClick(uColumn=" << uColumn << L")");
	// Tell the caller to sort the column itself
	return WrapReturn(S_FALSE);
}


// Called for uColumn = 0, 1, 2, ... until function returns E_FAIL
// (uColumn >= DetailsColumn::MAX)
STDMETHODIMP CShellFolder::GetDetailsOf(
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
		if (uColumn >= DetailsColumn::MAX) return WrapReturnFailOK(E_FAIL);
		const WORD wResourceID = IDS_COLUMN_NAME + uColumn;
		const CStringW ColumnName(MAKEINTRESOURCE(wResourceID));
		pDetails->fmt = LVCFMT_LEFT;
		pDetails->cxChar = 32;
		return WrapReturn(
			SetReturnString(
				static_cast<PCWSTR>(ColumnName),
				&pDetails->str
			) ? S_OK : E_OUTOFMEMORY
		);
	}

	if (!ADSX::CItem::IsOwn(pidl)) {
		// Lazy load this because this doesn't happen for every shellfolder
		// instance (e.g., during browsing's "drill down" phase)
		if (m_psd == NULL) {
			hr = m_psf->BindToObject(pidl, NULL, IID_PPV_ARGS(&m_psd));
			if (FAILED(hr)) return WrapReturnFailOK(hr);
		}
		return WrapReturnFailOK(m_psd->GetDetailsOf(pidl, uColumn, pDetails));
	}

	if (uColumn >= DetailsColumn::MAX) return WrapReturnFailOK(E_FAIL);

	// Okay, this time it's for a real item
	auto Item = ADSX::CItem::Get(pidl);
	switch (uColumn) {
		case DetailsColumn::Name:
			pDetails->fmt = LVCFMT_LEFT;
			ATLASSERT(wcslen(Item->pszName) <= INT_MAX);
			pDetails->cxChar = static_cast<int>(wcslen(Item->pszName));
			return WrapReturn(
				SetReturnString(
					Item->pszName,
					&pDetails->str
				) ? S_OK : E_OUTOFMEMORY
			);

		case DetailsColumn::Filesize:
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

#pragma endregion


#pragma region IShellFolder2

STDMETHODIMP
CShellFolder::EnumSearches(_COM_Outptr_ IEnumExtraSearch **ppEnum) {
	LOG(P_RSF << L"EnumSearches()");
	if (ppEnum != NULL) *ppEnum = NULL;
	return WrapReturnFailOK(E_NOTIMPL);
}


STDMETHODIMP CShellFolder::GetDefaultColumn(
	_In_  DWORD dwReserved,
	_Out_ ULONG *pSort,
	_Out_ ULONG *pDisplay
) {
	LOG(P_RSF << L"GetDefaultColumn()");

	if (pSort == NULL || pDisplay == NULL) return WrapReturn(E_POINTER);

	*pSort = DetailsColumn::Name;
	*pDisplay = DetailsColumn::Name;

	return WrapReturn(S_OK);
}


STDMETHODIMP CShellFolder::GetDefaultColumnState(
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
		case DetailsColumn::Name:
			*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
			break;
		case DetailsColumn::Filesize:
			*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;
			break;
		default:
			return WrapReturn(E_INVALIDARG);
	}

	return WrapReturn(S_OK);
}


STDMETHODIMP CShellFolder::GetDefaultSearchGUID(_Out_ GUID *pguid) {
	LOG(P_RSF << L"GetDefaultSearchGUID()");
	return WrapReturnFailOK(E_NOTIMPL);
}


STDMETHODIMP CShellFolder::GetDetailsEx(
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
STDMETHODIMP CShellFolder::MapColumnToSCID(
	_In_ UINT uColumn,
	_Out_ SHCOLUMNID *pscid
) {
	LOG(P_RSF << L"MapColumnToSCID(uColumn=" << uColumn << L")");

	#if defined(ADSX_PKEYS_SUPPORT)
		// This will map the columns to some built-in properties on Vista.
		// It's needed for the tile subtitles to display properly.
		switch (uColumn) {
			case DetailsColumn::Name:
				*pscid = PKEY_ItemNameDisplay;
				return WrapReturn(S_OK);
			case DetailsColumn::Filesize:
				// TODO(nate-kean): is this right? where are PKEYs'
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

#pragma endregion

} // namespace ADSX
