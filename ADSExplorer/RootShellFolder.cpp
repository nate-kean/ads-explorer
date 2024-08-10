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
#include <comutil.h>

#include "CADSXEnumIDList.h"
#include "CADSXItem.h"
#include "DebugPrint.h"
#include "RootShellFolder.h"
#include "RootShellView.h"
#include "ShellItems.h"
#include "defer.h"

//==============================================================================
// Helpers
// #define _DEBUG

#ifdef _DEBUG
	#include "iids.h"
	LPCWSTR CADSXRootShellFolder::PidlToString(PCUIDLIST_RELATIVE pidl) const {
		static _bstr_t str;
		str = "";
		if (pidl == NULL) {
			return str + "<null>";
		}
		bool first = true;
		for (; !ILIsEmpty(pidl); pidl = ILNext(pidl)) {
			if (!first) {
				str += "--";
			}
			if (CADSXItem::IsOwn(pidl)) {
				_bstr_t sName = CADSXItem::Get((PCUITEMID_CHILD) pidl)->m_Name;
				str += sName;
			} else if (pidl == m_pidlRoot) {
				WCHAR tmp[MAX_PATH];
				SHGetPathFromIDListW((PIDLIST_ABSOLUTE) pidl, tmp);
				str += tmp;
			} else {
				WCHAR tmp[16];
				swprintf_s(tmp, L"<unk-%02d>", pidl->mkid.cb);
				str += tmp;
			}
			first = false;
		}
		return str.GetBSTR();
	}

	LPCWSTR CADSXRootShellFolder::PidlArrayToString(UINT cidl, PCUITEMID_CHILD_ARRAY aPidls) const {
		static _bstr_t str;
		str = "[";
		defer({ str += "]"; });
		for (UINT i = 0; i < cidl; i++) {
			str += PidlToString(aPidls[i]);
			if (i < cidl - 1) {
				str += ", ";
			}
		}
		return str.GetBSTR();
	}

	static LPCWSTR IIDToString(const IID &iid) {
		static LPOLESTR pszGUID = NULL;
		HRESULT hr = StringFromCLSID(iid, &pszGUID);
		if (FAILED(hr)) return L"Catastrophe! Failed to convert IID to string";
		defer({ CoTaskMemFree(pszGUID); });
		// Search as an interface
		auto search = iids.find(pszGUID);
		if (search != iids.end()) {
			return search->second;
		}
		return pszGUID;
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
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::Initialize() pidl=[%s]\n",
		(size_t) this,
		PidlToString(pidl)
	);
	m_pidlRoot = ILCloneFull(pidl);
	return S_OK;
}

STDMETHODIMP CADSXRootShellFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl) {
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::GetCurFolder()\n", (size_t) this);
	if (ppidl == NULL) return E_POINTER;
	*ppidl = ILCloneFull(m_pidlRoot);
	return S_OK;
}

//-------------------------------------------------------------------------------
// IShellFolder

// Called when an item in an ADSX folder is double-clicked.
// TODO(garlic-os): is this also called for an ADSX folder itself?
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	/* [in]  */ PCUIDLIST_RELATIVE pidl,
	/* [in]  */ IBindCtx *pbc,
	/* [in]  */ REFIID riid,
	/* [out] */ void **ppvOut
) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::BindToObject(pidl=[%s], riid=[%s])\n",
		(size_t) this,
		PidlToString(pidl),
		IIDToString(riid)
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
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::CompareIDs(lParam=%d) pidl1=[%s], "
		"pidl2=[%s]\n",
		(size_t) this,
		(int) lParam,
		PidlToString(pidl1),
		PidlToString(pidl2)
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
			Result = wcscmp(Item1->m_Name, Item2->m_Name);
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
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::CreateViewObject(riid=[%s])\n",
		(size_t) this,
		IIDToString(riid)
	);

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
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::EnumObjects(dwFlags=0x%04x)\n",
		(size_t) this,
		dwFlags
	);

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
	_bstr_t bstrPath =
		"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
		"ADS Explorer\\Test\\Files\\3streams.txt";
	DebugPrint(L" ** EnumObjects: Path=%s\n", bstrPath.GetBSTR());
	pEnum->Init(this->GetUnknown(), bstrPath.Detach());

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
	#ifdef _DEBUG
		DebugPrint(
			L"CADSXRootShellFolder(0x%08zu)::GetAttributesOf(pidls=[%s])\n",
			(size_t) this,
			PidlArrayToString(cidl, aPidls)
		);
	#endif

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
	#ifdef _DEBUG
		DebugPrint(
			L"CADSXRootShellFolder(0x%08zu)::GetUIObjectOf(pidls=[%s], riid=[%s])"
			"\n",
			(size_t) this,
			PidlArrayToString(cidl, aPidls),
			IIDToString(riid)
		);
	#endif

	HRESULT hr;

	if (ppvOut == NULL) return E_POINTER;
	*ppvOut = NULL;

	if (cidl == 0) return E_INVALIDARG;

	// Does the FileDialog need to embed some data?
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) return E_INVALIDARG;

		// Is this really one of our item?
		if (!CADSXItem::IsOwn(aPidls[0])) return E_INVALIDARG;

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) return hr;

		// AddRef it while we are working with it, this prevent from an early
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
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::BindToStorage()\n", (size_t) this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDisplayNameOf(
	PCUITEMID_CHILD pidl,
	SHGDNF uFlags,
	STRRET *pName
) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::GetDisplayNameOf(uFlags=0x%04x) "
		"pidl=[%s]\n",
		(size_t) this,
		uFlags,
		PidlToString(pidl)
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
		return E_FAIL;
	}

	// At this stage, the pidl should be one of ours
	if (!CADSXItem::IsOwn(pidl)) return E_INVALIDARG;

	auto Item = CADSXItem::Get(pidl);
	switch (uFlags) {
		// TODO(garlic-os)
		case SHGDN_NORMAL | SHGDN_FORPARSING:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
			return SetReturnStringW(L"Placeholder", *pName) ? S_OK
			                                               : E_FAIL;

		case SHGDN_NORMAL | SHGDN_FOREDITING:
		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return E_FAIL;  // Can't rename!
	}

	// Any other combination results in returning the name.
	return SetReturnStringW(Item->m_Name, *pName) ? S_OK : E_FAIL;
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
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::ParseDisplayName()\n", (size_t) this);
	return E_NOTIMPL;
}

// TODO(garlic-os): should this be implemented?
STDMETHODIMP CADSXRootShellFolder::SetNameOf(HWND, PCUITEMID_CHILD, LPCWSTR, SHGDNF, PITEMID_CHILD *) {
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::SetNameOf()\n", (size_t) this);
	return E_NOTIMPL;
}

//-------------------------------------------------------------------------------
// IShellDetails

STDMETHODIMP CADSXRootShellFolder::ColumnClick(UINT iColumn) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::ColumnClick(iColumn=%d)\n", (size_t) this, iColumn
	);

	// The caller must sort the column itself
	return S_FALSE;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsOf(
	/* [in, optional] */ PCUITEMID_CHILD pidl,
	/* [in]           */ UINT uColumn,
	/* [out]          */ SHELLDETAILS *pDetails
) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::GetDetailsOf(uColumn=%d) pidl=[%s]\n",
		(size_t) this,
		uColumn,
		PidlToString(pidl)
	);

	if (uColumn >= DETAILS_COLUMN_MAX) {
		return E_FAIL;
	}

	// Shell asks for the column headers
	// TODO(garlic-os): should filesize be here too?
	if (pidl == NULL) {
		// Load the iColumn based string from the resource
		// TODO(garlic-os): can CString be changed out for something else?
		CStringW ColumnName(MAKEINTRESOURCE(IDS_COLUMN_NAME + uColumn));
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
			pDetails->cxChar = (int) wcslen(Item->m_Name);
			return SetReturnString(Item->m_Name, pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			BSTR pszSize[16] = {0};
			StrFormatByteSizeW(Item->m_Filesize, (BSTR) pszSize, 16);
			pDetails->cxChar = (int) wcslen((BSTR) pszSize);
			return SetReturnString((BSTR) pszSize, pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;
	}

	return E_INVALIDARG;
}

//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP CADSXRootShellFolder::EnumSearches(IEnumExtraSearch **ppEnum) {
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::EnumSearches()\n", (size_t) this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDefaultColumn(
	DWORD dwReserved,
	ULONG *pSort,
	ULONG *pDisplay
) {
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::GetDefaultColumn()\n", (size_t) this);

	if (!pSort || !pDisplay) {
		return E_POINTER;
	}

	*pSort = DETAILS_COLUMN_NAME;
	*pDisplay = DETAILS_COLUMN_NAME;

	return S_OK;
}

STDMETHODIMP
CADSXRootShellFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::GetDefaultColumnState(iColumn=%d)\n",
		(size_t) this,
		iColumn
	);

	if (!pcsFlags) {
		return E_POINTER;
	}

	// Seems that SHCOLSTATE_PREFER_VARCMP doesn't have any noticeable effect
	// (if supplied or not) for Win2K, but don't set it for WinXP, since it will
	// not sort the column. (not setting it means that our CompareIDs() will be
	// called)
	switch (iColumn) {
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
	DebugPrint(L"CADSXRootShellFolder(0x%08zu)::GetDefaultSearchGUID()\n", (size_t) this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv
) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::GetDetailsEx(pscid->pid=%d) "
		"pidl=[%s]\n",
		(size_t) this,
		pscid->pid,
		PidlToString(pidl)
	);

	#if defined(ADSX_PKEYS_SUPPORT)
		/*
		* Vista required. It appears ItemNameDisplay and ItemPathDisplay come
		* from their real FS representation. The API is also wide-only and is
		* only available on XP SP2+ on, so it won't harm 9x.
		*/
		if (IsEqualPropertyKey(*pscid, PKEY_PropList_TileInfo)) {
			DebugPrint(L" ** GetDetailsEx: PKEY_PropList_TileInfo\n");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
			DebugPrint(L" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo\n");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
			DebugPrint(L" ** GetDetailsEx: PKEY_PropList_PreviewDetails\n");
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
			DebugPrint(L" ** GetDetailsEx: PKEY_PropList_FullDetails\n");
			return SUCCEEDED(InitVariantFromString(
				L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv
			));
		} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
			DebugPrint(L" ** GetDetailsEx: PKEY_ItemType\n");
			return SUCCEEDED(InitVariantFromString(L"Directory", pv));
		}
	#endif

	DebugPrint(L" ** GetDetailsEx: Not implemented\n");
	return E_NOTIMPL;
}

STDMETHODIMP
CADSXRootShellFolder::MapColumnToSCID(UINT uColumn, SHCOLUMNID *pscid) {
	DebugPrint(
		L"CADSXRootShellFolder(0x%08zu)::MapColumnToSCID(iColumn=%d)\n",
		(size_t) this,
		uColumn
	);
	#if defined(ADSX_PKEYS_SUPPORT)
		// This will map the columns to some built-in properties on Vista.
		// It's needed for the tile subtitles to display properly.
		switch (uColumn) {
			case DETAILS_COLUMN_NAME:
				*pscid = PKEY_ItemNameDisplay;
				return S_OK;
			case DETAILS_COLUMN_FILESIZE:
				// TODO(garlic-os): what do I put here?
				*pscid = PKEY_ItemNameDisplay;
				return S_OK;
		}
		return E_FAIL;
	#endif
	return E_NOTIMPL;
}
