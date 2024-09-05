/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "stdafx.h"  // MUST be included first

#if _MSC_VER > 1200
	#include "ADSExplorer_h.h"
#else
	// the IDL compiler on VC++6 puts it here instead. weird!
	#include "ADSExplorer.h"
#endif

#include <atlstr.h>
#include <string>

#include "ADSXEnumIDList.h"
#include "ADSXItem.h"
#include "DebugPrint.h"
#include "RootShellFolder.h"
#include "RootShellView.h"
#include "DataObject.h"
#include "defer.h"

//==============================================================================
// Helpers

// Debug log prefix for CADSXRootShellFolder
#define P_RSF L"CADSXRootShellFolder(0x" << std::hex << this << L")::"

// STRRET helper functions
bool SetReturnStringA(LPCSTR Source, STRRET &str) {
	SIZE_T StringLen = strlen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = static_cast<LPOLESTR>(CoTaskMemAlloc(StringLen * sizeof(OLECHAR)));
	if (str.pOleStr == NULL) {
		return false;
	}

	mbstowcs_s(NULL, str.pOleStr, StringLen, Source, StringLen);
	return true;
}
bool SetReturnStringW(LPCWSTR Source, STRRET &str) {
	SIZE_T StringLen = wcslen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = static_cast<LPOLESTR>(CoTaskMemAlloc(StringLen * sizeof(OLECHAR)));
	if (str.pOleStr == NULL) {
		return false;
	}

	wcsncpy_s(str.pOleStr, StringLen, Source, StringLen);
	return true;
}


// #define _DEBUG
#ifdef _DEBUG
	#include <sstream>
	#include <string>
	#include "iids.h"

	static std::wstring PidlToString(PCUIDLIST_RELATIVE pidl) {
		if (pidl == NULL) return L"<null>";
		std::wostringstream oss;
		bool first = true;
		for (; !ILIsEmpty(pidl); pidl = ILNext(pidl)) {
			if (!first) {
				oss << L"--";
			}
			PITEMID_CHILD pidlChild = ILCloneFirst(pidl);
			defer({ CoTaskMemFree(pidlChild); });
			if (CADSXItem::IsOwn(pidl)) {
				oss << CADSXItem::Get(pidlChild)->m_Name;
			} else {
				WCHAR tmp[16];
				swprintf_s(tmp, L"<unk-%02d>", pidl->mkid.cb);
				oss << tmp;
			}
			first = false;
		}
		return oss.str();
	}

	static std::wstring PidlArrayToString(UINT cidl, PCUITEMID_CHILD_ARRAY aPidls) {
		std::wostringstream oss;
		oss << L"[";
		defer({ oss << L"]"; });
		for (UINT i = 0; i < cidl; i++) {
			oss << PidlToString(aPidls[i]);
			if (i < cidl - 1) {
				oss << L", ";
			}
		}
		return oss.str();
	}

	static std::wstring IIDToString(const std::wstring &sIID) {
		auto search = iids.find(sIID);
		if (search != iids.end()) {
			return std::wstring(search->second);
		} else {
			return sIID;
		}
	}
	static std::wstring IIDToString(const IID &iid) {
		LPOLESTR pszGUID = NULL;
		HRESULT hr = StringFromCLSID(iid, &pszGUID);
		if (FAILED(hr)) return L"Catastrophe! Failed to convert IID to string";
		defer({ CoTaskMemFree(pszGUID); });
		auto sIID = std::wstring(pszGUID);
		return IIDToString(sIID);
	}

	static std::wstring InitializationPidlToString(PCIDLIST_ABSOLUTE pidl) {
		if (pidl == NULL) return L"<null>";
		PWSTR pszPath = NULL;
		HRESULT hr = SHGetNameFromIDList(
			pidl,
			SIGDN_DESKTOPABSOLUTEPARSING,
			&pszPath
		);
		defer({ CoTaskMemFree(pszPath); });
		if (FAILED(hr)) return L"ERROR";
		std::wstring wstrPath(pszPath);  // name is copied so this is safe
		if (wstrPath == L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}") {
			return L"[Desktop]\\ADS Explorer";
		} else if (wstrPath == L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}") {
			return L"ADS Explorer";
		}
		return wstrPath;
	}

	static std::wstring SFGAOToString(const SFGAOF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wstringstream ss;
		if (*pfAttribs & SFGAO_CANCOPY) ss << L"CANCOPY | ";
		if (*pfAttribs & SFGAO_CANMOVE) ss << L"CANMOVE | ";
		if (*pfAttribs & SFGAO_CANLINK) ss << L"CANLINK | ";
		if (*pfAttribs & SFGAO_STORAGE) ss << L"STORAGE | ";
		if (*pfAttribs & SFGAO_CANRENAME) ss << L"CANRENAME | ";
		if (*pfAttribs & SFGAO_CANDELETE) ss << L"CANDELETE | ";
		if (*pfAttribs & SFGAO_HASPROPSHEET) ss << L"HASPROPSHEET | ";
		if (*pfAttribs & SFGAO_DROPTARGET) ss << L"DROPTARGET | ";
		if (*pfAttribs & SFGAO_CAPABILITYMASK) ss << L"CAPABILITYMASK | ";
		if (*pfAttribs & SFGAO_PLACEHOLDER) ss << L"PLACEHOLDER | ";
		if (*pfAttribs & SFGAO_SYSTEM) ss << L"SYSTEM | ";
		if (*pfAttribs & SFGAO_ENCRYPTED) ss << L"ENCRYPTED | ";
		if (*pfAttribs & SFGAO_ISSLOW) ss << L"ISSLOW | ";
		if (*pfAttribs & SFGAO_GHOSTED) ss << L"GHOSTED | ";
		if (*pfAttribs & SFGAO_LINK) ss << L"LINK | ";
		if (*pfAttribs & SFGAO_SHARE) ss << L"SHARE | ";
		if (*pfAttribs & SFGAO_READONLY) ss << L"READONLY | ";
		if (*pfAttribs & SFGAO_HIDDEN) ss << L"HIDDEN | ";
		if (*pfAttribs & SFGAO_DISPLAYATTRMASK) ss << L"DISPLAYATTRMASK | ";
		if (*pfAttribs & SFGAO_FILESYSANCESTOR) ss << L"FILESYSANCESTOR | ";
		if (*pfAttribs & SFGAO_FOLDER) ss << L"FOLDER | ";
		if (*pfAttribs & SFGAO_FILESYSTEM) ss << L"FILESYSTEM | ";
		if (*pfAttribs & SFGAO_HASSUBFOLDER) ss << L"HASSUBFOLDER | ";
		if (*pfAttribs & SFGAO_CONTENTSMASK) ss << L"CONTENTSMASK | ";
		if (*pfAttribs & SFGAO_VALIDATE) ss << L"VALIDATE | ";
		if (*pfAttribs & SFGAO_REMOVABLE) ss << L"REMOVABLE | ";
		if (*pfAttribs & SFGAO_COMPRESSED) ss << L"COMPRESSED | ";
		if (*pfAttribs & SFGAO_BROWSABLE) ss << L"BROWSABLE | ";
		if (*pfAttribs & SFGAO_NONENUMERATED) ss << L"NONENUMERATED | ";
		if (*pfAttribs & SFGAO_NEWCONTENT) ss << L"NEWCONTENT | ";
		if (*pfAttribs & SFGAO_CANMONIKER) ss << L"CANMONIKER | ";
		if (*pfAttribs & SFGAO_HASSTORAGE) ss << L"HASSTORAGE | ";
		if (*pfAttribs & SFGAO_STREAM) ss << L"STREAM | ";
		if (*pfAttribs & SFGAO_STORAGEANCESTOR) ss << L"STORAGEANCESTOR | ";
		if (*pfAttribs & SFGAO_STORAGECAPMASK) ss << L"STORAGECAPMASK | ";
		if (*pfAttribs & SFGAO_PKEYSFGAOMASK) ss << L"PKEYSFGAOMASK | ";
		return ss.str();
	}

	static std::wstring SHCONTFToString(const SHCONTF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wstringstream ss;
		if (*pfAttribs & SHCONTF_CHECKING_FOR_CHILDREN) ss << L"CHECKING_FOR_CHILDREN | ";
		if (*pfAttribs & SHCONTF_FOLDERS) ss << L"FOLDERS | ";
		if (*pfAttribs & SHCONTF_NONFOLDERS) ss << L"NONFOLDERS | ";
		if (*pfAttribs & SHCONTF_INCLUDEHIDDEN) ss << L"INCLUDEHIDDEN | ";
		if (*pfAttribs & SHCONTF_INIT_ON_FIRST_NEXT) ss << L"INIT_ON_FIRST_NEXT | ";
		if (*pfAttribs & SHCONTF_NETPRINTERSRCH) ss << L"NETPRINTERSRCH | ";
		if (*pfAttribs & SHCONTF_SHAREABLE) ss << L"SHAREABLE | ";
		if (*pfAttribs & SHCONTF_STORAGE) ss << L"STORAGE | ";
		if (*pfAttribs & SHCONTF_NAVIGATION_ENUM) ss << L"NAVIGATION_ENUM | ";
		if (*pfAttribs & SHCONTF_FASTITEMS) ss << L"FASTITEMS | ";
		if (*pfAttribs & SHCONTF_FLATLIST) ss << L"FLATLIST | ";
		if (*pfAttribs & SHCONTF_ENABLE_ASYNC) ss << L"ENABLE_ASYNC | ";
		if (*pfAttribs & SHCONTF_INCLUDESUPERHIDDEN) ss << L"INCLUDESUPERHIDDEN | ";
		return ss.str();
	}

	static std::wstring SHGDNFToString(const SHGDNF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wostringstream ss;
		if (*pfAttribs & SHGDN_NORMAL) ss << L"NORMAL | ";
		if (*pfAttribs & SHGDN_INFOLDER) ss << L"INFOLDER | ";
		if (*pfAttribs & SHGDN_FOREDITING) ss << L"FOREDITING | ";
		if (*pfAttribs & SHGDN_FORADDRESSBAR) ss << L"FORADDRESSBAR | ";
		if (*pfAttribs & SHGDN_FORPARSING) ss << L"FORPARSING | ";
		return ss.str();
	}
#else
	#define PidlToString(...) (void) 0
	#define PidlArrayToString(...) (void) 0
	#define InitializationPidlToString(...) (void) 0
	#define IIDToString(...) (void) 0
	#define SFGAOToString(...) (void) 0
	#define SHCONTFToString(...) (void) 0
	#define SHGDNFToString(...) (void) 0
#endif



//==============================================================================
// CADSXRootShellFolder
CADSXRootShellFolder::CADSXRootShellFolder()
	: m_pidlRoot(NULL)
	, m_pidlPath(NULL) {
	// LOG(P_RSF << L"CONSTRUCTOR");
}

CADSXRootShellFolder::~CADSXRootShellFolder() {
	// LOG(P_RSF << L"DESTRUCTOR");
	if (m_pidlRoot != NULL) CoTaskMemFree(m_pidlRoot);
	if (m_pidlPath != NULL) CoTaskMemFree(m_pidlPath);
}

STDMETHODIMP CADSXRootShellFolder::GetClassID(_Out_ CLSID *pclsid) {
	if (pclsid == NULL) return LogReturn(E_POINTER);
	*pclsid = CLSID_ADSExplorerRootShellFolder;
	return LogReturn(S_OK);
}


// Stolen from dj-tech/reactos
// Licensed under GPLv3
// TO DO: make sure project is GPL compliant
// Definition: https://gitlab.com/dj-tech/reactos/-/blob/master/reactos/dll/win32/shell32/shlfolder.cpp#L151
// Usage: https://gitlab.com/dj-tech/reactos/-/blob/master/reactos/dll/win32/shell32/folders/CAdminToolsFolder.cpp#L171
static HRESULT SHELL32_CoCreateInitSF(
	_In_         PCIDLIST_ABSOLUTE pidlRoot,
	_In_         REFIID            riid,
	_COM_Outptr_ void              **ppvOut
) {
	CComPtr<IShellFolder> psf;

	HRESULT hr = SHCoCreateInstance(
		NULL,
		&CLSID_ShellFSFolder,
		NULL,
		IID_PPV_ARGS(&psf)
	);
	if (FAILED(hr)) return LogReturn(hr);

	CComPtr<IPersistFolder> ppf;
	CComPtr<IPersistFolder3> ppf3;

	if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARGS(&ppf3)))) {
		PERSIST_FOLDER_TARGET_INFO pfti = {0};
		pfti.dwAttributes = -1;
		pfti.csidl = -1;

		ppf3->InitializeEx(NULL, pidlRoot, &pfti);
	}
	else if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARGS(&ppf)))) {
		ppf->Initialize(pidlRoot);
	}

	return psf->QueryInterface(riid, ppvOut);
}

// Initialize() is passed the PIDL of the folder where our extension is.
// Copies and does not take ownership of pidl.
STDMETHODIMP CADSXRootShellFolder::Initialize(_In_ PCIDLIST_ABSOLUTE pidl) {
	// LOG(P_RSF << L"Initialize(pidl=[" << InitializationPidlToString(pidl) << L"])");

	if (m_pidlRoot != NULL) CoTaskMemFree(m_pidlRoot);

	m_pidlRoot = ILCloneFull(pidl);
	if (m_pidlRoot == NULL) return E_OUTOFMEMORY;
	// if (m_pidlRoot == NULL) return LogReturn(E_OUTOFMEMORY);

	HRESULT hr = SHELL32_CoCreateInitSF(m_pidlRoot, IID_PPV_ARGS(&m_psfRoot));
	if (FAILED(hr)) return hr;
	// if (FAILED(hr)) return LogReturn(hr);

	hr = SHGetDesktopFolder(&m_psfDesktop);
	if (FAILED(hr)) return hr;
	// if (FAILED(hr)) return LogReturn(hr);

	return hr;
	// return LogReturn(hr);
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

// Called when an item in an ADSX folder is double-clicked.
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	_In_         PCUIDLIST_RELATIVE pidl,
	_In_opt_     IBindCtx           *pbc,
	_In_         REFIID             riid,
	_COM_Outptr_ void               **ppvOut
) {
	LOG(P_RSF << L"BindToObject("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);

	if (ppvOut == NULL) return LogReturn(E_POINTER);
	*ppvOut = NULL;

	if (riid != IID_IShellFolder && riid != IID_IShellFolder2) {
		return LogReturn(E_NOINTERFACE);
	}

	// If the passed pidl is not ours, fail(?).
	if (!CADSXItem::IsOwn(pidl)) return LogReturn(E_INVALIDARG);

	// All items in an ADS Explorer view are children
	// (until I implement pseudofolders)
	// return LogReturn(E_NOTIMPL);

	HRESULT hr;
	CComObject<CADSXRootShellFolder> *prsfNew;
	hr = CComObject<CADSXRootShellFolder>::CreateInstance(&prsfNew);
	if (FAILED(hr)) return LogReturn(hr);
	prsfNew->AddRef();
	defer({ prsfNew->Release(); });

	PIDLIST_ABSOLUTE pidlAbs = ILCombine(m_pidlRoot, pidl);
	if (pidlAbs == NULL) return LogReturn(E_OUTOFMEMORY);
	defer({ CoTaskMemFree(pidlAbs); });

	hr = prsfNew->Initialize(pidlAbs);
	if (FAILED(hr)) return LogReturn(hr);

	hr = prsfNew->QueryInterface(riid, ppvOut);
	return LogReturn(hr);
}

// Return the sort order of two PIDLs.
// lParam can be the 0-based Index of the details column
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

	// First check if the pidl are ours
	if (!CADSXItem::IsOwn(pidl1) || !CADSXItem::IsOwn(pidl2)) {
		return LogReturn(E_INVALIDARG);
	}

	// Now check if the pidl are one or multi level, in case they are
	// multi-level, return non-equality
	if (!ILIsChild(pidl1) || !ILIsChild(pidl2)) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
	}

	USHORT Result = 0;  // see note below (MAKE_HRESULT)

	auto Item1 = CADSXItem::Get((PCUITEMID_CHILD) pidl1);
	auto Item2 = CADSXItem::Get((PCUITEMID_CHILD) pidl2);

	switch (lParam & SHCIDS_COLUMNMASK) {
		case DETAILS_COLUMN_NAME:
			Result = Item1->m_Name.compare(Item2->m_Name);
			break;
		case DETAILS_COLUMN_FILESIZE:
			Result = static_cast<USHORT>(Item1->m_Filesize - Item2->m_Filesize);
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

// Return a COM object that implements IShellView.
STDMETHODIMP CADSXRootShellFolder::CreateViewObject(
	_In_         HWND   hwndOwner,
	_In_         REFIID riid,
	_COM_Outptr_ void   **ppvOut
) {
	// Not logging all interface requests because there are too many
	// LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");

	if (ppvOut == NULL) return E_POINTER;
	// if (ppvOut == NULL) return LogReturn(E_POINTER);
	*ppvOut = NULL;

	HRESULT hr;

	// We handle only the IShellView
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
			reinterpret_cast<IShellView **>(ppvOut),
			hwndOwner,
			static_cast<IShellFolder *>(this)
		);

		// We are finished with our own use of the view object (AddRef()'d
		// above by us, AddRef()'ed by Create)
		pViewObject->Release();

		return LogReturn(hr);
	}

	// We do not handle other objects
	return E_NOINTERFACE;
	// return LogReturn(E_NOINTERFACE);
}

// Return a COM object that implements IEnumIDList and enumerates the ADSes in
// the current folder.
STDMETHODIMP CADSXRootShellFolder::EnumObjects(
	_In_         HWND        hwndOwner,
	_In_         SHCONTF     dwFlags,
	_COM_Outptr_ IEnumIDList **ppEnumIDList
) {
	LOG(P_RSF << L"EnumObjects(dwFlags=[" << SHCONTFToString(&dwFlags) << L"])");
	UNREFERENCED_PARAMETER(hwndOwner);

	if (ppEnumIDList == NULL) return LogReturn(E_POINTER);
	*ppEnumIDList = NULL;

	// A path hasn't been browsed yet in this instance's lifetime, so there are
	// no ADSes to enumerate.
	// if (m_pidlPath == NULL)e return LogReturn(S_FALSE);

	// Create an enumerator over this file system object's alternate data streams.
	CComObject<CADSXEnumIDList> *pEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return LogReturn(hr);
	pEnum->AddRef();
	defer({ pEnum->Release(); });

	wchar_t pszPath[MAX_PATH];
	SHGetPathFromIDListW(m_pidlRoot, pszPath);
	std::wstring wstrPath(pszPath);
	// std::wstring wstrPath =
	// 	L"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
	// 	L"ADS Explorer\\Test\\Files\\3streams.txt";
	// std::wstring wstrPath =
	// 	L"C:\\Users\\sisdf\\Documents\\GitHub\\ads-explorer"
	// 	L"\\Test\\Files\\3streams.txt";
	LOG(L" ** EnumObjects: Path=" << wstrPath);
	pEnum->Init(this->GetUnknown(), wstrPath);

	// Return an IEnumIDList interface to the caller.
	hr = pEnum->QueryInterface(IID_PPV_ARGS(ppEnumIDList));
	return LogReturn(hr);
}

// Return if the items represented by the given PIDLs have the attributes
// requested.
// For each bit flag:
//   1 if the flag is set on input and all the given items have that attribute,
//   0 if the flag is not set on input or if any of the given items do not have
//   that attribute.
STDMETHODIMP CADSXRootShellFolder::GetAttributesOf(
	_In_    UINT                  cidl,
	_In_    PCUITEMID_CHILD_ARRAY aPidls,
	_Inout_ SFGAOF                *pfAttribs
) {
	LOG(P_RSF << L"GetAttributesOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"pfAttribs=[" << SFGAOToString(pfAttribs) << L"]"
	L")");

	if (cidl == 0 || aPidls[0]->mkid.cb == 0) {
		// Root folder: [Desktop]\ADS Explorer
		// Represents the ADS Explorer view of [Desktop]
		// Not a real filesystem object -> not accessible from ADS Explorer
		*pfAttribs &= SFGAO_HASSUBFOLDER |
		              SFGAO_FOLDER |
		              SFGAO_FILESYSTEM |
		              SFGAO_FILESYSANCESTOR |
		            //   SFGAO_BROWSABLE |
		              SFGAO_NONENUMERATED;
	} else {
		// Child files/folders: [Desktop]\ADS Explorer\{fs object's path}
		// Represents the ADS Explorer view of the file/folder at {fs object's path}
		// Real filesystem object -> accessible from ADS Explorer
		*pfAttribs &= SFGAO_FILESYSTEM |
		              SFGAO_CANCOPY |
		              SFGAO_CANMOVE |
		              SFGAO_CANRENAME |
		              SFGAO_CANDELETE;
	}

	return LogReturn(S_OK);
}

// GetUIObjectOf() is called to get several sub-objects like IExtractIcon and
// IDataObject
STDMETHODIMP CADSXRootShellFolder::GetUIObjectOf(
	_In_         HWND                  hwndOwner,
	_In_         UINT                  cidl,
	_In_         PCUITEMID_CHILD_ARRAY aPidls,
	_In_         REFIID                riid,
	_Inout_      UINT                  *rgfReserved,
	_COM_Outptr_ void                  **ppvOut
) {
	LOG(P_RSF << L"GetUIObjectOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);
	UNREFERENCED_PARAMETER(hwndOwner);

	HRESULT hr;

	if (ppvOut == NULL) return LogReturn(E_POINTER);
	*ppvOut = NULL;

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
		hr = pDataObject->QueryInterface(riid, ppvOut);
		// We do no more need our ref (note that the object will not die because
		// the QueryInterface above, AddRef'd it)
		pDataObject->Release();
		return LogReturn(hr);
	}

	// TODO(garlic-os): implement other interfaces as listed in
	// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getuiobjectof#remarks.
	// OpenWindows had the luxury of their objects being real/normal filesystem
	// objects (i.e., the folders other Explorer windows were open to), so it
	// could just proxy these requests on to those objects' parent folders.
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
	_COM_Outptr_ void **ppvOut
) {
	LOG(P_RSF << L"BindToStorage()");
	if (ppvOut != NULL) *ppvOut = NULL;
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
				LOG(L" ** GetDisplayNameOf: Root folder");
				return LogReturn(
					SetReturnStringW(
						L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}",
						*pName
					) ? S_OK : E_FAIL
				);
			default:
				// We don't handle other combinations of flags for the root pidl
				// return LogReturn(E_FAIL);
				LOG(L" ** GetDisplayNameOf: Root folder");
				return LogReturn(
					SetReturnStringW(
						L"GetDisplayNameOf test",
						*pName
					) ? S_OK : E_FAIL
				);
		}
	}
	LOG(L" ** GetDisplayNameOf: Child file/folder");

	// At this stage, the pidl should be one of ours
	if (!CADSXItem::IsOwn(pidl)) return LogReturn(E_INVALIDARG);

	auto Item = CADSXItem::Get(pidl);
	switch (uFlags) {
		case SHGDN_NORMAL | SHGDN_FORPARSING:
			// TODO(garlic-os): this should return:
			// "::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}\{fs object's path}:{Item->m_Name}"
			return LogReturn(
				SetReturnStringW(Item->m_Name.c_str(), *pName) ? S_OK : E_FAIL
			);

		case SHGDN_NORMAL | SHGDN_FOREDITING:
		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return LogReturn(E_FAIL);  // TODO(garlic-os)
			return E_FAIL;  // TODO(garlic-os)

		case SHGDN_INFOLDER:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
		default:
			return LogReturn(
				SetReturnStringW(Item->m_Name.c_str(), *pName) ? S_OK : E_FAIL
			);
			return SetReturnStringW(Item->m_Name.c_str(), *pName)
				? S_OK : E_FAIL;
	}
}

// static bool StartsWith(LPCWSTR pszText, LPCWSTR pszComparand) {
// 	return wcsncmp(pszText, pszComparand, sizeof(pszComparand) - 1) == 0;
// }

// Clip My Computer off the PIDL if it's there
// TODO(garlic-os): Can SHGetSpecialFolderLocation be used instead of
// SHGetIDListFromObject and psfDesktop?
// TODO(garlic-os): Can m_psf be used instead of psfDesktop?
// HRESULT ClipDesktop(PIDLIST_RELATIVE *ppidl) {
// 	HRESULT hr;

// 	PITEMID_CHILD pidlFirst = ILCloneFirst(*ppidl);
// 	if (pidlFirst == NULL) return LogReturn(E_OUTOFMEMORY);
// 	defer({ CoTaskMemFree(pidlFirst); });

// 	PIDLIST_ABSOLUTE m_psfDesktop;
// 	hr = SHGetIDListFromObject(psfDesktop, &m_psfDesktop);
// 	if (FAILED(hr)) return LogReturn(hr);
// 	defer({ CoTaskMemFree(m_psfDesktop); });

// 	if (m_psfDesktop->CompareIDs(0, pidlFirst, m_psfDesktop) == 0) {
// 		*ppidl = ILNext(*ppidl);
// 		return LogReturn(S_OK);
// 	}
// 	return LogReturn(S_FALSE);
// }

// HRESULT CADSXRootShellFolder::ClipADSX(_Inout_ PIDLIST_RELATIVE *ppidl) {
// 	HRESULT hr;

// 	PITEMID_CHILD pidlFirst = ILCloneFirst(*ppidl);
// 	if (pidlFirst == NULL) return LogReturn(E_OUTOFMEMORY);
// 	defer({ CoTaskMemFree(pidlFirst); });

// 	PIDLIST_RELATIVE pidlRootTemp = ILCloneFull(m_pidlRoot);
// 	if (pidlRootTemp == NULL) return LogReturn(E_OUTOFMEMORY);
// 	defer({ CoTaskMemFree(pidlRootTemp); });

// 	hr = ClipDesktop(&pidlRootTemp);
// 	if (FAILED(hr)) return LogReturn(hr);

// 	if (this->CompareIDs(0, pidlFirst, pidlRootTemp) == 0) {
// 		*ppidl = ILNext(*ppidl);
// 		return LogReturn(S_OK);
// 	}
// 	return LogReturn(S_FALSE);
// }

STDMETHODIMP CADSXRootShellFolder::ParseDisplayName(
	_In_        HWND             hwnd,
	_In_opt_    IBindCtx         *pbc,
	_In_        LPWSTR           pszDisplayName,
	_Out_opt_   ULONG            *pchEaten,
	_Outptr_    PIDLIST_RELATIVE *ppidl,
	_Inout_opt_ ULONG            *pfAttributes
) {
	LOG(P_RSF << L"ParseDisplayName("
		L"name=\"" << pszDisplayName << L"\", "
		L"attributes=[" << SFGAOToString(pfAttributes) << L"]"
	L")");

	if (pchEaten != NULL) {
		*pchEaten = 0;
	}

	// WCHAR pszNameOverride[] =
	// 	L"G:\\Garlic\\Documents\\Code\\Visual Studio\\"
	// 	L"ADS Explorer Saga\\ADS Explorer\\Test\\Files\\3streams.txt";
	// LOG(L" ** ParseDisplayName override: " << pszNameOverride);
	// pszDisplayName = pszNameOverride;

	HRESULT hr;

	hr = m_psfDesktop->ParseDisplayName(
		hwnd,
		pbc,
		pszDisplayName,
		pchEaten,
		ppidl,
		pfAttributes
	);
	if (FAILED(hr)) {
		LOG(L" ** m_psfDesktop->ParseDisplayName failed: " << hr);
		return LogReturn(hr);
	}

	LOG(" ** Parsed: [" << PidlToString(*ppidl) << L"]");

	
	// m_pidlPath = ILClone(*ppidl);
	// if (m_pidlPath == NULL) return LogReturn(E_OUTOFMEMORY);

	// hr = ClipDesktop(&m_pidlPath);
	// if (FAILED(hr)) return LogReturn(hr);
	// hr = ClipADSX(&m_pidlPath);
	// if (FAILED(hr)) return LogReturn(hr);


	return LogReturn(hr);
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
				SetReturnStringW(Item->m_Name.c_str(), pDetails->str) ? S_OK : E_OUTOFMEMORY
			);

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			constexpr UINT8 uLongLongStrLenMax =
				_countof("-9,223,372,036,854,775,808");
			WCHAR pszSize[uLongLongStrLenMax] = {0};
			StrFormatByteSizeW(Item->m_Filesize, pszSize, uLongLongStrLenMax);
			pDetails->cxChar = static_cast<UINT8>(wcslen(pszSize));
			return LogReturn(
				SetReturnStringW(pszSize, pDetails->str) ? S_OK : E_OUTOFMEMORY
			);
	}

	return LogReturn(E_INVALIDARG);
}

//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP CADSXRootShellFolder::EnumSearches(
	_COM_Outptr_ IEnumExtraSearch **ppEnum
) {
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
