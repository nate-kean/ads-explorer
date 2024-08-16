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

#include "CADSXEnumIDList.h"
#include "CADSXItem.h"
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
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
	if (str.pOleStr == NULL) {
		return false;
	}

	mbstowcs_s(NULL, str.pOleStr, StringLen, Source, StringLen);
	return true;
}
bool SetReturnStringW(LPCWSTR Source, STRRET &str) {
	SIZE_T StringLen = wcslen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
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
	std::wstring CADSXRootShellFolder::PidlToString(PCUIDLIST_RELATIVE pidl) const {
		if (pidl == NULL) return L"<null>";
		std::wostringstream oss;
		bool first = true;
		for (; !ILIsEmpty(pidl); pidl = ILNext(pidl)) {
			if (!first) {
				oss << L"--";
			}
			if (CADSXItem::IsOwn(pidl)) {
				oss << CADSXItem::Get((PCUITEMID_CHILD) pidl)->m_Name;
			} else if (pidl == m_pidlRoot) {
				WCHAR tmp[MAX_PATH];
				SHGetPathFromIDListW((PIDLIST_ABSOLUTE) pidl, tmp);
				oss << tmp;
			} else {
				WCHAR tmp[16];
				swprintf_s(tmp, L"<unk-%02d>", pidl->mkid.cb);
				oss << tmp;
			}
			first = false;
		}
		return oss.str();
	}

	std::wstring CADSXRootShellFolder::PidlArrayToString(UINT cidl, PCUITEMID_CHILD_ARRAY aPidls) const {
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

	static std::wstring IIDToString(const IID &iid) {
		LPOLESTR pszGUID = NULL;
		HRESULT hr = StringFromCLSID(iid, &pszGUID);
		if (FAILED(hr)) return L"Catastrophe! Failed to convert IID to string";
		defer({ CoTaskMemFree(pszGUID); });
		// Search as an interface
		auto wstrGUID = std::wstring(pszGUID);
		auto search = iids.find(wstrGUID);
		if (search != iids.end()) {
			return std::wstring(search->second);
		} else {
			return wstrGUID;
		}
	}
#else
	#define PidlToString(...) (void) 0
	#define PidlArrayToString(...) (void) 0
	#define IIDToString(...) (void) 0
#endif


//==============================================================================
// CADSXRootShellFolder
CADSXRootShellFolder::CADSXRootShellFolder() : m_pidlRoot(NULL) {}

STDMETHODIMP CADSXRootShellFolder::GetClassID(CLSID *pclsid) {
	if (pclsid == NULL) return E_POINTER;
	*pclsid = CLSID_ADSExplorerRootShellFolder;
	return S_OK;
}

// Initialize() is passed the PIDL of the folder where our extension is.
STDMETHODIMP CADSXRootShellFolder::Initialize(PCIDLIST_ABSOLUTE pidl) {
	LOG(P_RSF << L"Initialize(pidl=[" << PidlToString(pidl) << L"])");
	m_pidlRoot = ILCloneFull(pidl);
	return S_OK;
}

STDMETHODIMP CADSXRootShellFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl) {
	LOG(P_RSF << L"GetCurFolder()");
	if (ppidl == NULL) return E_POINTER;
	*ppidl = ILCloneFull(m_pidlRoot);
	return S_OK;
}

//-------------------------------------------------------------------------------
// IShellFolder

// Called when an item in an ADSX folder is double-clicked.
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	/* [in]  */ PCUIDLIST_RELATIVE pidl,
	/* [in]  */ IBindCtx *pbc,
	/* [in]  */ REFIID riid,
	/* [out] */ void **ppvOut
) {
	LOG(P_RSF << L"BindToObject("
		L"pidl=[" << PidlToString(pidl) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);

	// If the passed pidl is not ours, fail.
	if (!CADSXItem::IsOwn(pidl)) return E_INVALIDARG;

	// I'll support multi-level PIDLs when I implement pseudofolders
	return E_NOTIMPL;
}

// CompareIDs() is responsible for returning the sort order of two PIDLs.
// lParam can be the 0-based Index of the details column
STDMETHODIMP CADSXRootShellFolder::CompareIDs(
	/* [in] */ LPARAM lParam,
	/* [in] */ PCUIDLIST_RELATIVE pidl1,
	/* [in] */ PCUIDLIST_RELATIVE pidl2
) {
	LOG(P_RSF << L"CompareIDs("
		L"lParam=" << (int) lParam << L"), "
		L"pidl1=[" << PidlToString(pidl1) << L"], "
		L"pidl2=[" << PidlToString(pidl2) << L"])"
	);

	// First check if the pidl are ours
	if (!CADSXItem::IsOwn(pidl1) || !CADSXItem::IsOwn(pidl2)) {
		return E_INVALIDARG;
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
			Result = (USHORT) (Item1->m_Filesize - Item2->m_Filesize);
			if (Result < 0) Result = -1;
			else if (Result > 0) Result = 1;
			break;
		default:
			return E_INVALIDARG;
	}

	// Warning: the last param MUST be unsigned, if not (ie: short) a negative
	// value will trash the high order word of the HRESULT!
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, /*-1,0,1*/ Result);
}

// Create a new COM object that implements IShellView.
STDMETHODIMP CADSXRootShellFolder::CreateViewObject(
	/* [in]  */ HWND hwndOwner,
	/* [in]  */ REFIID riid,
	/* [out] */ void **ppvOut
) {
	LOG(P_RSF << L"CreateViewObject(riid=[" << IIDToString(riid) << L"])");

	HRESULT hr;

	if (ppvOut == NULL) return E_POINTER;
	*ppvOut = NULL;

	// We handle only the IShellView
	if (riid == IID_IShellView) {
		// Create a view object
		CComObject<CADSXRootShellView> *pViewObject;
		hr = CComObject<CADSXRootShellView>::CreateInstance(&pViewObject);
		if (FAILED(hr)) return hr;

		// AddRef the object while we are using it
		pViewObject->AddRef();

		// Tie the view object lifetime with the current IShellFolder.
		pViewObject->Init(this->GetUnknown());

		// Create the view
		hr = pViewObject->Create(
			(IShellView **) ppvOut, hwndOwner, (IShellFolder *) this
		);

		// We are finished with our own use of the view object (AddRef()'d
		// above by us, AddRef()'ed by Create)
		pViewObject->Release();

		return hr;
	}

	// We do not handle other objects
	return E_NOINTERFACE;
}

// EnumObjects() creates a COM object that implements IEnumIDList.
STDMETHODIMP CADSXRootShellFolder::EnumObjects(
	/* [in]  */ HWND hwndOwner,
	/* [in]  */ SHCONTF dwFlags,
	/* [out] */ IEnumIDList **ppEnumIDList
) {
	LOG(P_RSF << L"EnumObjects(dwFlags=0x" << std::hex << dwFlags << L")");

	if (ppEnumIDList == NULL) return E_POINTER;
	*ppEnumIDList = NULL;

	// Create an enumerator over this file system object's alternate data streams.
	CComObject<CADSXEnumIDList> *pEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return hr;
	pEnum->AddRef();
	defer({ pEnum->Release(); });

	// wchar_t pszPath[MAX_PATH];
	// SHGetPathFromIDListW(ILNext(m_pidlRoot), pszPath);
	// _bstr_t bstrPath(pszPath);
	std::wstring wstrPath =
		L"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
		L"ADS Explorer\\Test\\Files\\3streams.txt";
	LOG(L" ** EnumObjects: Path=" << wstrPath);
	pEnum->Init(this->GetUnknown(), wstrPath);

	// Return an IEnumIDList interface to the caller.
	hr = pEnum->QueryInterface(IID_IEnumIDList, (void **) ppEnumIDList);
	return hr;
}

// GetAttributesOf() returns the attributes for the items whose PIDLs are passed
// in.
STDMETHODIMP CADSXRootShellFolder::GetAttributesOf(
	/* [in]      */ UINT cidl,
	/* [in]      */ PCUITEMID_CHILD_ARRAY aPidls,
	/* [in, out] */ SFGAOF *pfAttribs
) {
	LOG(P_RSF << L"GetAttributesOf(pidls=[" << PidlArrayToString(cidl, aPidls) << L"])");

	// We limit the tree by indicating that the favorites folder does not
	// contain sub-folders
	if (cidl == 0 || aPidls[0]->mkid.cb == 0) {
		// Root folder attributes
	    *pfAttribs &= SFGAO_HASSUBFOLDER |
					  SFGAO_FOLDER |
					  SFGAO_FILESYSTEM |
					  SFGAO_FILESYSANCESTOR |
					  SFGAO_BROWSABLE;
		// *pdwAttribs &= SFGAO_HASSUBFOLDER |
		//                SFGAO_FOLDER |
		//                SFGAO_FILESYSTEM |
		//                SFGAO_FILESYSANCESTOR |
		//                SFGAO_BROWSABLE |
		//                SFGAO_NONENUMERATED;
	} else {
		// Child folder attributes
	    *pfAttribs &= SFGAO_FOLDER |
					  SFGAO_FILESYSTEM |
					  SFGAO_FILESYSANCESTOR |
					  SFGAO_BROWSABLE |
					  SFGAO_LINK;
		// *pdwAttribs &= SFGAO_FILESYSTEM |
		//             //    SFGAO_FOLDER |
		//             //    SFGAO_BROWSABLE |
		//                SFGAO_STREAM |
		//                SFGAO_CANCOPY |
		//                SFGAO_CANMOVE |
		//                SFGAO_CANRENAME |
		//                SFGAO_CANDELETE;
	}

	return S_OK;
}

// GetUIObjectOf() is called to get several sub-objects like IExtractIcon and
// IDataObject
STDMETHODIMP CADSXRootShellFolder::GetUIObjectOf(
	HWND hwndOwner,
	UINT cidl,
	PCUITEMID_CHILD_ARRAY aPidls,
	REFIID riid,
	UINT *rgfReserved,
	void **ppvOut
) {
	LOG(P_RSF << L"GetUIObjectOf("
		L"pidls=[" << PidlArrayToString(cidl, aPidls) << L"], "
		L"riid=[" << IIDToString(riid) << L"])"
	);

	HRESULT hr;

	if (ppvOut == NULL) return E_POINTER;
	*ppvOut = NULL;

	if (cidl == 0) return E_INVALIDARG;

	// We must be in the FileDialog; it wants aPidls wrapped in an IDataObject
	// (just to call IDataObject::GetData() and nothing else).
	// https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample#HowItsDone_UseCasesFileDialog_ClickIcon
	// TODO(garlic-os): It was a design descision for Hurni's NSE to support
	// only one item at a time. I should consider supporting multiple items.
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) return E_INVALIDARG;

		// Is this really one of our item?
		if (!CADSXItem::IsOwn(aPidls[0])) return E_INVALIDARG;

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) return hr;

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
		return hr;
	}

	// TODO(garlic-os): implement other interfaces as listed in
	// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getuiobjectof#remarks.
	// OpenWindows had the luxury of their objects being real/normal filesystem
	// objects (i.e., the folders other Explorer windows were open to), so it
	// could just proxy these requests on to those objects' parent folders.
	// Our objects are not real/normal filesystem objects, so we have to
	// implement these interfaces ourselves.
	else if (riid == IID_IContextMenu) {
		return E_NOINTERFACE;
	}

	else if (riid == IID_IContextMenu2) {
		return E_NOINTERFACE;
	}
	
	else if (riid == IID_IDropTarget) {
		return E_NOINTERFACE;
	}

	else if (riid == IID_IExtractIcon) {
		return E_NOINTERFACE;
	}

	else if (riid == IID_IQueryInfo) {
		return E_NOINTERFACE;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP
CADSXRootShellFolder::BindToStorage(PCUIDLIST_RELATIVE, IBindCtx *, REFIID, void **) {
	LOG(P_RSF << L"BindToStorage()");
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDisplayNameOf(
	PCUITEMID_CHILD pidl,
	SHGDNF uFlags,
	STRRET *pName
) {
	LOG(P_RSF << L"GetDisplayNameOf("
		L"uFlags=0x" << std::hex << uFlags << L", "
		L"pidl=[" << PidlToString(pidl) << L"])"
	);

	if (pidl == NULL || pName == NULL) return E_POINTER;

	// Return name of Root
	if (pidl->mkid.cb == 0) {
		switch (uFlags) {
			// If wantsFORPARSING is present in the registry.
			// As stated in the SDK, we should return here our virtual junction
			// point which is in the form "::{GUID}" So we should return
			// "::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}".
			case SHGDN_NORMAL | SHGDN_FORPARSING:
				return SetReturnStringW(
					L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}",
					*pName
				) ? S_OK : E_FAIL;
		}
		// We don't handle other combinations of flags for the root pidl
		// return E_FAIL;
		return SetReturnStringW(L"GetDisplayNameOf test", *pName) ? S_OK : E_FAIL;
	}

	// At this stage, the pidl should be one of ours
	if (!CADSXItem::IsOwn(pidl)) return E_INVALIDARG;

	auto Item = CADSXItem::Get(pidl);
	switch (uFlags) {
		// TODO(garlic-os)
		case SHGDN_NORMAL | SHGDN_FORPARSING:
			// TODO(garlic-os): "ADS Explorer\{fs object's path}:{Item->m_Name}"
			return SetReturnStringW(Item->m_Name.c_str(), *pName) ? S_OK
			                                               : E_FAIL;

		case SHGDN_NORMAL | SHGDN_FOREDITING:
		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return E_FAIL;  // TODO(garlic-os)

		case SHGDN_INFOLDER:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
		default:
			return SetReturnStringW(Item->m_Name.c_str(), *pName) ? S_OK
			                                               : E_FAIL;
	}
}

// TODO(garlic-os): root pidl plus pidlized file object's path
STDMETHODIMP CADSXRootShellFolder::ParseDisplayName(
	/* [in]      */ HWND hwnd,
	/* [in]      */ IBindCtx *pbc,
	/* [in]      */ LPWSTR pszDisplayName,
	/* [out]     */ ULONG *pchEaten,
	/* [out]     */ PIDLIST_RELATIVE *ppidl,
	/* [in, out] */ ULONG *pfAttributes
) {
	LOG(P_RSF << L"ParseDisplayName()");
	return E_NOTIMPL;
}

// TODO(garlic-os): should this be implemented?
STDMETHODIMP CADSXRootShellFolder::SetNameOf(HWND, PCUITEMID_CHILD, LPCWSTR, SHGDNF, PITEMID_CHILD *) {
	LOG(P_RSF << L"SetNameOf()");
	return E_NOTIMPL;
}

//-------------------------------------------------------------------------------
// IShellDetails

STDMETHODIMP CADSXRootShellFolder::ColumnClick(UINT uColumn) {
	LOG(P_RSF << L"ColumnClick(uColumn=" << uColumn << L")");
	// Tell the caller to sort the column itself
	return S_FALSE;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsOf(
	/* [in, optional] */ PCUITEMID_CHILD pidl,
	/* [in]           */ UINT uColumn,
	/* [out]          */ SHELLDETAILS *pDetails
) {
	LOG(P_RSF << L"GetDetailsOf("
		L"uColumn=" << uColumn << L", "
		L"pidl=[" << PidlToString(pidl) << L"])"
	);

	if (uColumn >= DETAILS_COLUMN_MAX) return E_FAIL;

	// Shell asks for the column headers
	if (pidl == NULL) {
		// Load the uColumn based string from the resource
		// TODO(garlic-os): do we haaave to use CString here?
		// this entire kind of string is not used anywhere else in the program
		WORD wResourceID = IDS_COLUMN_NAME + uColumn;
		CStringW ColumnName(MAKEINTRESOURCE(wResourceID));
		pDetails->fmt = LVCFMT_LEFT;
		pDetails->cxChar = 32;
		return SetReturnString(ColumnName, pDetails->str) ? S_OK
														  : E_OUTOFMEMORY;
	}

	// Okay, this time it's for a real item
	auto Item = CADSXItem::Get(pidl);
	switch (uColumn) {
		case DETAILS_COLUMN_NAME:
			pDetails->fmt = LVCFMT_LEFT;
			ATLASSERT(Item->m_Name.length() <= INT_MAX);
			pDetails->cxChar = (int) Item->m_Name.length();
			return SetReturnStringW(Item->m_Name.c_str(), pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			constexpr UINT8 uLongLongStrLenMax = _countof("-9,223,372,036,854,775,808");
			WCHAR pszSize[uLongLongStrLenMax] = {0};
			StrFormatByteSizeW(Item->m_Filesize, pszSize, uLongLongStrLenMax);
			pDetails->cxChar = (int) wcslen(pszSize);
			return SetReturnStringW(pszSize, pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;
	}

	return E_INVALIDARG;
}

//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP CADSXRootShellFolder::EnumSearches(IEnumExtraSearch **ppEnum) {
	LOG(P_RSF << L"EnumSearches()");
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDefaultColumn(
	DWORD dwReserved,
	ULONG *pSort,
	ULONG *pDisplay
) {
	LOG(P_RSF << L"GetDefaultColumn()");

	if (!pSort || !pDisplay) {
		return E_POINTER;
	}

	*pSort = DETAILS_COLUMN_NAME;
	*pDisplay = DETAILS_COLUMN_NAME;

	return S_OK;
}

STDMETHODIMP
CADSXRootShellFolder::GetDefaultColumnState(UINT uColumn, SHCOLSTATEF *pcsFlags) {
	LOG(P_RSF << L"GetDefaultColumnState(uColumn=" << uColumn << L")");

	if (pcsFlags == NULL) return E_POINTER;

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
			return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP CADSXRootShellFolder::GetDefaultSearchGUID(GUID *pguid) {
	LOG(P_RSF << L"GetDefaultSearchGUID()");
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv
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
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_PreviewDetails");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
			LOG(L" ** GetDetailsEx: PKEY_PropList_FullDetails");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
			LOG(L" ** GetDetailsEx: PKEY_ItemType");
			return SUCCEEDED(InitVariantFromString(L"Directory", pv));
		}
	#endif

	LOG(L" ** GetDetailsEx: Not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP
CADSXRootShellFolder::MapColumnToSCID(UINT uColumn, SHCOLUMNID *pscid) {
	LOG(P_RSF << L"MapColumnToSCID(uColumn=" << uColumn << L")");

	#if defined(ADSX_PKEYS_SUPPORT)
		// This will map the columns to some built-in properties on Vista.
		// It's needed for the tile subtitles to display properly.
		switch (uColumn) {
			case DETAILS_COLUMN_NAME:
				*pscid = PKEY_ItemNameDisplay;
				return S_OK;
			case DETAILS_COLUMN_FILESIZE:
				// TODO(garlic-os): is this right? where are PKEYs' documentation?
				*pscid = PKEY_TotalFileSize;
				// *pscid = PKEY_Size;
				// *pscid = PKEY_FileAllocationSize;
				return S_OK;
		}
		return E_FAIL;
	#endif
	return E_NOTIMPL;
}
