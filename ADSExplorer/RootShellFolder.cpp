/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
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

#include "stdafx.h"
#if _MSC_VER > 1200
#include "ADSExplorer_h.h"
#else
// the IDL compiler on VC++6 puts it here instead. weird!
#include "ADSExplorer.h"
#endif
#include "RootShellFolder.h"
#include "RootShellView.h"
#include "shtypes.h"

//==============================================================================
// Helpers

#ifdef _DEBUG

static LPTSTR PidlToString(LPCITEMIDLIST pidl) {
	static int which = 0;
	static TCHAR str1[200];
	static TCHAR str2[200];
	TCHAR *str;
	which ^= 1;
	str = which ? str1 : str2;

	str[0] = '\0';

	if (pidl == NULL) {
		_tcscpy(str, _T("<null>"));
		return str;
	}

	while (pidl->mkid.cb != 0) {
		if (*((DWORD *) (pidl->mkid.abID)) == COWItem::MAGIC) {
#ifndef _UNICODE
			char tmp[128];
			mbstowcs((wchar_t *) (pidl->mkid.abID) + 4, tmp, 128);
			_tcscat(str, tmp);
#else
			_tcscat(str, (wchar_t *) (pidl->mkid.abID) + 4);
#endif
			_tcscat(str, _T("::"));
		} else {
			TCHAR tmp[16];
			_stprintf(tmp, _T("<unk-%02d>::"), pidl->mkid.cb);
			_tcscat(str, tmp);
		}

		pidl = LPITEMIDLIST(LPBYTE(pidl) + pidl->mkid.cb);
	}
	return str;
}

#define DUMPIID(iid) AtlDumpIID(iid, NULL, S_OK)

#else
#define PidlToString
#define DUMPIID
#endif

//==============================================================================
// Copy policy for COWComEnumOnCArrays
class CCopyItemPidl {
   public:
	static void init(LPITEMIDLIST *p) {}

	static HRESULT copy(LPITEMIDLIST *pTo, COWItem *pFrom) {
		*pTo = s_PidlMgr.Create(*pFrom);
		return (*pTo != NULL) ? S_OK : E_OUTOFMEMORY;
	}

	static void destroy(LPITEMIDLIST *p) { s_PidlMgr.Delete(*p); }

   protected:
	static CPidlMgr s_PidlMgr;
};

// TODO(garlic-os): what does this line do
CPidlMgr CCopyItemPidl::s_PidlMgr;

// This class implements the IEnumIDList for our OpenWindow items.
typedef CComEnumOnCArray<
	IEnumIDList,
	&IID_IEnumIDList,
	LPITEMIDLIST,
	CCopyItemPidl,
	COWItemList
> COWEnumItemsIDList;

//==============================================================================
// COWRootShellFolder
COWRootShellFolder::COWRootShellFolder() : m_pidlRoot(NULL) {}

STDMETHODIMP COWRootShellFolder::GetClassID(CLSID *pClassID) {
	if (pClassID == NULL) {
		return E_POINTER;
	}

	// Return our GUID to the shell.
	*pClassID = CLSID_ADSExplorerRootShellFolder;

	return S_OK;
}

// Initialize() is passed the PIDL of the folder where our extension is.
STDMETHODIMP COWRootShellFolder::Initialize(PCUIDLIST_ABSOLUTE pidl) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::Initialize() pidl=[%s]\n"),
		this,
		PidlToString(pidl)
	);

	m_pidlRoot = (PIDLIST_ABSOLUTE) m_PidlMgr.Copy(pidl);

	return S_OK;
}

STDMETHODIMP COWRootShellFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl) {
	ATLTRACE(_T("COWRootShellFolder(0x%08x)::GetCurFolder()\n"), this);

	if (ppidl == NULL) {
		return E_POINTER;
	}

	*ppidl = (PIDLIST_ABSOLUTE) m_PidlMgr.Copy(m_pidlRoot);

	return S_OK;
}

//------------------------------------------------------------------------------
// IShellFolder

// BindToObject() is called when a folder in our part of the namespace is being
// browsed.
STDMETHODIMP COWRootShellFolder::BindToObject(
	PCUIDLIST_RELATIVE pidl,
	IBindCtx *pbcReserved,
	REFIID riid,
	void **ppvOut
) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::BindToObject() pidl=[%s]\n"),
		this,
		PidlToString(pidl)
	);

	// If the passed pidl is not ours, fail.
	if (!COWItem::IsOwn(pidl)) {
		return E_INVALIDARG;
	}

	HRESULT hr;
	CComPtr<IShellFolder> psfDesktop;

	hr = SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr)) {
		return hr;
	}

	PIDLIST_ABSOLUTE pidlAbsolute;
	hr = psfDesktop->ParseDisplayName(
		NULL,
		pbcReserved,
		COWItem::GetPath(pidl),
		NULL,
		(PIDLIST_RELATIVE *) &pidlAbsolute,
		NULL
	);
	if (FAILED(hr)) {
		return hr;
	}

	// We only support single-level pidl (coz subitems are, in reality, plain
	// path to other locations) BUT, the modified Office FileDialog still uses
	// our root IShellFolder to request bindings for sub-sub-items, so we do it!
	// Note that we also use multi-level pidl if "EnableFavoritesSubfolders" is
	// enabled.
	if (!m_PidlMgr.IsSingle(pidl)) {
		// Bind to the root folder of the favorite folder
		CComPtr<IShellFolder> psfRootFolder;
		hr = psfDesktop->BindToObject(
			pidlAbsolute, NULL, IID_IShellFolder, (void **) &psfRootFolder
		);
		ILFree(pidlAbsolute);
		if (FAILED(hr)) {
			return hr;
		}

		// And now bind to the sub-item of it
		return psfRootFolder->BindToObject(
			(PCUIDLIST_RELATIVE) m_PidlMgr.GetNextItem(pidl),
			pbcReserved,
			riid,
			ppvOut
		);
	}

	// Okay, browsing into a favorite item will redirect to its real path.
	hr = psfDesktop->BindToObject(pidlAbsolute, pbcReserved, riid, ppvOut);
	ILFree(pidlAbsolute);

	return hr;
	// TODO(Pascal Hurni): Could also use ILCreateFromPath?
}

// CompareIDs() is responsible for returning the sort order of two PIDLs.
// lParam can be the 0-based Index of the details column
STDMETHODIMP COWRootShellFolder::CompareIDs(
	LPARAM lParam,
	PCUIDLIST_RELATIVE pidl1,
	PCUIDLIST_RELATIVE pidl2
) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::CompareIDs(lParam=%d) pidl1=[%s], ")
		_T("pidl2=[%s]\n"),
		this,
		lParam,
		PidlToString(pidl1),
		PidlToString(pidl2)
	);

	// First check if the pidl are ours
	if (!COWItem::IsOwn(pidl1) || !COWItem::IsOwn(pidl2)) {
		return E_INVALIDARG;
	}

	// Now check if the pidl are one or multi level, in case they are
	// multi-level, return non-equality
	if (!m_PidlMgr.IsSingle(pidl1) || !m_PidlMgr.IsSingle(pidl2)) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
	}

	USHORT Result = 0;	// see note below (MAKE_HRESULT)

	switch (lParam & SHCIDS_COLUMNMASK) {
		case DETAILS_COLUMN_NAME:
			Result = wcscmp(COWItem::GetName(pidl1), COWItem::GetName(pidl2));
			break;
		case DETAILS_COLUMN_PATH:
			Result = wcscmp(COWItem::GetPath(pidl1), COWItem::GetPath(pidl2));
			break;
		case DETAILS_COLUMN_RANK:
			Result = COWItem::GetRank(pidl1) - COWItem::GetRank(pidl2);
			break;
		default:
			return E_INVALIDARG;
	}

	// Warning: the last param MUST be unsigned, if not (ie: short) a negative
	// value will trash the high order word of the HRESULT!
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, /*-1,0,1*/ Result);
}

// CreateViewObject() creates a new COM object that implements IShellView.
STDMETHODIMP COWRootShellFolder::CreateViewObject(
	HWND hwndOwner,
	REFIID riid,
	void **ppvOut
) {
	ATLTRACE(_T("COWRootShellFolder(0x%08x)::CreateViewObject()"), this);
	// DUMPIID(riid);

	HRESULT hr;

	if (ppvOut == NULL) {
		return E_POINTER;
	}

	*ppvOut = NULL;

	// We handle only the IShellView
	if (riid == IID_IShellView) {
		ATLTRACE(_T(" ** CreateViewObject for IShellView"));

		// Create a view object
		CComObject<COWRootShellView> *pViewObject;
		hr = CComObject<COWRootShellView>::CreateInstance(&pViewObject);
		if (FAILED(hr)) {
			return hr;
		}

		// AddRef the object while we are using it
		pViewObject->AddRef();

		// Tight the view object lifetime with the current IShellFolder.
		pViewObject->Init(GetUnknown());

		// Create the view
		hr = pViewObject->Create(
			(IShellView **) ppvOut, hwndOwner, (IShellFolder *) this
		);

		// We are finished with our own use of the view object (AddRef()'ed
		// above by us, AddRef()'ed by Create)
		pViewObject->Release();

		return hr;
	}

#if _DEBUG
	// vista sludge that no one knows what it actually is
	static const GUID
		unknownVistaGuid =	// {93F81976-6A0D-42C3-94DD-AA258A155470}
		{0x93F81976,
		 0x6A0D,
		 0x42C3,
		 {0x94, 0xDD, 0xAA, 0x25, 0x8A, 0x15, 0x54, 0x70}};

	if (riid != unknownVistaGuid) {
		LPOLESTR unkIid;
		hr = StringFromIID(riid, &unkIid);
		if (FAILED(hr)) {
			return hr;
		}
		ATLTRACE(_T(" ** CreateViewObject is unknown: %s"), unkIid);
		CoTaskMemFree(unkIid);
	}
#endif

	// We do not handle other objects
	return E_NOINTERFACE;
}

// EnumObjects() creates a COM object that implements IEnumIDList.
STDMETHODIMP COWRootShellFolder::EnumObjects(
	HWND hwndOwner,
	SHCONTF grfFlags,
	IEnumIDList **ppEnumIDList
) {
	ATLTRACE(
		"COWRootShellFolder(0x%08x)::EnumObjects(grfFlags=0x%04x)\n",
		this,
		grfFlags
	);

	HRESULT hr;

	if (ppEnumIDList == NULL) {
		return E_POINTER;
	}

	*ppEnumIDList = NULL;

	// Enumerate DOpus Favorites and put them in an array
	static COWItemList OpenWindows;
	OpenWindows.RemoveAll();

	EnumerateExplorerWindows(&OpenWindows, hwndOwner);

	ATLTRACE(
		_T(" ** EnumObjects: Now have %d items"), OpenWindows.GetSize()
	);

	// Create an enumerator with COWComEnumOnCArray<> and our copy policy class.
	CComObject<COWEnumItemsIDList> *pEnum;
	hr = CComObject<COWEnumItemsIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) {
		return hr;
	}

	// AddRef() the object while we're using it.
	pEnum->AddRef();

	// Init the enumerator. Init() will AddRef() our IUnknown (obtained with
	// GetUnknown()) so this object will stay alive as long as the enumerator
	// needs access to the collection m_Favorites.
	hr = pEnum->Init(GetUnknown(), OpenWindows);

	// Return an IEnumIDList interface to the caller.
	if (SUCCEEDED(hr)) {
		hr = pEnum->QueryInterface(IID_IEnumIDList, (void **) ppEnumIDList);
	}

	pEnum->Release();

	return hr;
}

// GetAttributesOf() returns the attributes for the items whose PIDLs are passed
// in.
STDMETHODIMP COWRootShellFolder::GetAttributesOf(
	UINT cidl,
	PCUITEMID_CHILD_ARRAY apidl,
	SFGAOF *rgfInOut
) {
#ifdef _DEBUG
	if (cidl >= 1) {
		ATLTRACE(
			_T("COWRootShellFolder(0x%08x)::GetAttributesOf(cidl=%d) ")
			_T("pidl=[%s]\n"),
			this,
			cidl,
			PidlToString(apidl[0])
		);
	} else {
		ATLTRACE(
			"COWRootShellFolder(0x%08x)::GetAttributesOf(cidl=%d)\n", this, cidl
		);
	}
#endif

	// We limit the tree, by indicating that the favorites folder does not
	// contain sub-folders

	if ((cidl == 0) || (apidl[0]->mkid.cb == 0)) {
		*rgfInOut &= SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_FILESYSTEM |
					 SFGAO_FILESYSANCESTOR | SFGAO_BROWSABLE;
	} else {
		*rgfInOut &= SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR |
					 SFGAO_BROWSABLE | SFGAO_LINK;
	}

	return S_OK;
}

// GetUIObjectOf() is called to get several sub-objects like IExtractIcon and
// IDataObject
STDMETHODIMP COWRootShellFolder::GetUIObjectOf(
	HWND hwndOwner,
	UINT cidl,
	PCUITEMID_CHILD_ARRAY apidl,
	REFIID riid,
	UINT *rgfReserved,
	void **ppvOut
) {
#ifdef _DEBUG
	if (cidl >= 1) {
		ATLTRACE(
			_T("COWRootShellFolder(0x%08x)::GetUIObjectOf(uCount=%d) ")
			_T("pidl=[%s]"),
			this,
			cidl,
			PidlToString(*apidl)
		);
	} else {
		ATLTRACE(
			_T("COWRootShellFolder(0x%08x)::GetUIObjectOf(uCount=%d)"),
			this,
			cidl
		);
	}
	// DUMPIID(riid);
#endif

	HRESULT hr;

	if (ppvOut == NULL) {
		return E_POINTER;
	}

	*ppvOut = NULL;

	if (cidl == 0) {
		return E_INVALIDARG;
	}

	// Special case: DataObject -- when the FileDialog needs to embed some data
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) {
			return E_INVALIDARG;
		}

		// Is this really one of our item?
		if (!COWItem::IsOwn(*apidl)) {
			return E_INVALIDARG;
		}

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) {
			return hr;
		}

		// Increment the object's refcount while we are working with it to keep
		// it from being destroyed early
		pDataObject->AddRef();

		// Tie its lifetime to this object (the IShellFolder object)
		pDataObject->Init(GetUnknown());

		// Now embed the pidl into the data object
		pDataObject->SetPidl(m_pidlRoot, *apidl);

		// Return the requested interface to the caller
		hr = pDataObject->QueryInterface(riid, ppvOut);

		// Decrement the object's refcount (note: it's still not destroyed at
		// this point because the QueryInterface above AddRef'd it)
		pDataObject->Release();
		return hr;
	}

	// All other requests are delegated to the target path's IShellFolder

	// Pascal Hurni: Because multiple items can point to different storages
	// [sic], we choose not to handle groups of items.
	if (cidl > 1) {
		return E_NOINTERFACE;
	}

	CComPtr<IShellFolder> psfParent;
	CComPtr<IShellFolder> psfDesktop;

	hr = SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr)) {
		return hr;
	}

	PIDLIST_ABSOLUTE pidlAbsolute;
	hr = psfDesktop->ParseDisplayName(
		NULL,
		NULL,
		COWItem::GetPath(*apidl),
		NULL,
		(PIDLIST_RELATIVE *) &pidlAbsolute,
		NULL
	);
	if (FAILED(hr)) {
		return hr;
	}

	PCUITEMID_CHILD pidlParent;

	hr = SHBindToParent(
		pidlAbsolute, IID_IShellFolder, (void **) &psfParent, &pidlParent
	);
	ILFree(pidlAbsolute);
	if (FAILED(hr)) {
		// ILFree(pidlParent);
		return hr;
	}
	//------------------------------

	hr = psfParent->GetUIObjectOf(
		hwndOwner, 1, &pidlParent, riid, rgfReserved, ppvOut
	);

	// SHBindToParent does not allocate a new PIDL, it just derives a new
	// pointer from pidlAbsolute, so pidlParent does not need to be freed.
	// ILFree(pidlParent);

	return hr;
}

STDMETHODIMP
COWRootShellFolder::BindToStorage(
	PCUIDLIST_RELATIVE pidl,
	IBindCtx *pbc,
	REFIID riid,
	void **ppvOut
) {
	ATLTRACE("COWRootShellFolder(0x%08x)::BindToStorage()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP COWRootShellFolder::GetDisplayNameOf(
	PCUITEMID_CHILD pidl,
	SHGDNF uFlags,
	STRRET *pName
) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::GetDisplayNameOf(uFlags=0x%04x) ")
		_T("pidl=[%s]\n"),
		this,
		uFlags,
		PidlToString(pidl)
	);

	if ((pidl == NULL) || (pName == NULL)) {
		return E_POINTER;
	}

	// Return name of Root
	if (pidl->mkid.cb == 0) {
		switch (uFlags) {
			case SHGDN_NORMAL | SHGDN_FORPARSING:  // <- if wantsFORPARSING is
												   // present in the regitry
				// As stated in the SDK, we should return here our virtual
				// junction point which is in the form "::{GUID}" So we should
				// return "::{F74755E7-E951-4B20-B2F4-80B1B0340FA5}". This works
				// (well I guess it's never used) great except for the modified
				// FileDialog of Office (97, 2000). Thanx to MS, they always
				// have to tweak their own app. The drawback is that Office WILL
				// check for real filesystem existance of the returned string.
				// As you know, "::{GUID}" is not a real filesystem path. This
				// stops Office FileDialog from browsing our namespace
				// extension! To workaround it, instead returning "::{GUID}", we
				// return a real filesystem path, which will never be used by us
				// nor by the FileDialog. I choosed to return the system
				// temporary directory, which ought te be valid on every system.
				TCHAR TempPath[MAX_PATH];
				if (GetTempPath(MAX_PATH, TempPath) == 0) {
					return E_FAIL;
				}

				return SetReturnString(TempPath, *pName) ? S_OK : E_FAIL;

				// See note above
				// return
				// SetReturnString(_T("::{F74755E7-E951-4B20-B2F4-80B1B0340FA5}"),
				// *lpName) ? S_OK : E_FAIL;
		}
		// We dont' handle other combinations of flags
		return E_FAIL;
	}

	// At this stage, the pidl should be one of ours
	if (!COWItem::IsOwn(pidl)) {
		return E_INVALIDARG;
	}

	switch (uFlags) {
		case SHGDN_NORMAL | SHGDN_FORPARSING:
		case SHGDN_INFOLDER | SHGDN_FORPARSING:
			return SetReturnStringW(COWItem::GetPath(pidl), *pName) ? S_OK
																	: E_FAIL;

		case SHGDN_NORMAL | SHGDN_FOREDITING:
		case SHGDN_INFOLDER | SHGDN_FOREDITING:
			return E_FAIL;	// Can't rename!
	}

	// Any other combination results in returning the name.
	return SetReturnStringW(COWItem::GetName(pidl), *pName) ? S_OK : E_FAIL;
}

STDMETHODIMP COWRootShellFolder::ParseDisplayName(
	HWND hwnd,
	IBindCtx *pbc,
	LPWSTR pszDisplayName,
	ULONG *pchEaten,
	PIDLIST_RELATIVE *ppidl,
	ULONG *pdwAttributes
) {
	ATLTRACE("COWRootShellFolder(0x%08x)::ParseDisplayName()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP COWRootShellFolder::SetNameOf(
	HWND hwnd,
	PCUITEMID_CHILD pidl,
	LPCWSTR pszName,
	SHGDNF uFlags,
	PITEMID_CHILD *ppidlOut
) {
	ATLTRACE("COWRootShellFolder(0x%08x)::SetNameOf()\n", this);
	return E_NOTIMPL;
}

//------------------------------------------------------------------------------
// IShellDetails
STDMETHODIMP COWRootShellFolder::ColumnClick(UINT iColumn) {
	ATLTRACE(
		"COWRootShellFolder(0x%08x)::ColumnClick(iColumn=%d)\n", this, iColumn
	);

	// The caller must sort the column itself
	return S_FALSE;
}

STDMETHODIMP COWRootShellFolder::GetDetailsOf(
	PCUITEMID_CHILD pidl,
	UINT iColumn,
	LPSHELLDETAILS pDetails
) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::GetDetailsOf(iColumn=%d) pidl=[%s]\n"),
		this,
		iColumn,
		PidlToString(pidl)
	);

	if (iColumn >= DETAILS_COLUMN_MAX) {
		return E_FAIL;
	}

	// Shell asks for the column headers
	if (pidl == NULL) {
		// Load the iColumn based string from the resource
		CString ColumnName(MAKEINTRESOURCE(IDS_COLUMN_NAME + iColumn));

		if (iColumn == DETAILS_COLUMN_RANK) {
			pDetails->fmt = LVCFMT_RIGHT;
			pDetails->cxChar = 6;
		} else {
			pDetails->fmt = LVCFMT_LEFT;
			pDetails->cxChar = 32;
		}
		return SetReturnString(ColumnName, pDetails->str) ? S_OK
														  : E_OUTOFMEMORY;
	}

	// Okay, this time it's for a real item
	TCHAR ReturnString[16];
	switch (iColumn) {
		case DETAILS_COLUMN_NAME:
			pDetails->fmt = LVCFMT_LEFT;
			pDetails->cxChar = (int) wcslen(COWItem::GetName(pidl));
			return SetReturnStringW(COWItem::GetName(pidl), pDetails->str)
					   ? S_OK
					   : E_OUTOFMEMORY;

		case DETAILS_COLUMN_PATH:
			pDetails->fmt = LVCFMT_LEFT;
			pDetails->cxChar = (int) wcslen(COWItem::GetName(pidl));
			return SetReturnStringW(COWItem::GetPath(pidl), pDetails->str)
					   ? S_OK
					   : E_OUTOFMEMORY;

		case DETAILS_COLUMN_RANK:
			pDetails->fmt = LVCFMT_RIGHT;
			pDetails->cxChar = 6;
			wsprintf(ReturnString, _T("%d"), COWItem::GetRank(pidl));
			return SetReturnString(ReturnString, pDetails->str) ? S_OK
																: E_OUTOFMEMORY;
	}

	return E_INVALIDARG;
}

//------------------------------------------------------------------------------
// IShellFolder2
STDMETHODIMP COWRootShellFolder::EnumSearches(IEnumExtraSearch **ppEnum) {
	ATLTRACE("COWRootShellFolder(0x%08x)::EnumSearches()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP COWRootShellFolder::GetDefaultColumn(
	DWORD dwReserved,
	ULONG *pSort,
	ULONG *pDisplay
) {
	ATLTRACE("COWRootShellFolder(0x%08x)::GetDefaultColumn()\n", this);

	if (!pSort || !pDisplay) {
		return E_POINTER;
	}

	*pSort = DETAILS_COLUMN_RANK;
	*pDisplay = DETAILS_COLUMN_NAME;

	return S_OK;
}

STDMETHODIMP
COWRootShellFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags) {
	ATLTRACE(
		"COWRootShellFolder(0x%08x)::GetDefaultColumnState(iColumn=%d)\n",
		this,
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
		case DETAILS_COLUMN_PATH:
			*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
			break;
		case DETAILS_COLUMN_RANK:
			*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;
			break;
		default:
			return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP COWRootShellFolder::GetDefaultSearchGUID(GUID *pguid) {
	ATLTRACE("COWRootShellFolder(0x%08x)::GetDefaultSearchGUID()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP COWRootShellFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv
) {
	ATLTRACE(
		_T("COWRootShellFolder(0x%08x)::GetDetailsEx(pscid->pid=%d) ")
		_T("pidl=[%s]\n"),
		this,
		pscid->pid,
		PidlToString(pidl)
	);

#if defined(OW_PKEYS_SUPPORT)
	/*
	 * Vista required. It appears ItemNameDisplay and ItemPathDisplay come
	 * from their real FS representation. The API is also wide-only and is
	 * only available on XP SP2+ on, so it won't harm 9x.
	 */
	if (IsEqualPropertyKey(*pscid, PKEY_PropList_TileInfo)) {
		ATLTRACE(_T(" ** GetDetailsEx: PKEY_PropList_TileInfo"));
		return SUCCEEDED(
			InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
		);
	} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
		ATLTRACE(_T(" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo"));
		return SUCCEEDED(
			InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
		);
	} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
		ATLTRACE(_T(" ** GetDetailsEx: PKEY_PropList_PreviewDetails"));
		return SUCCEEDED(
			InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
		);
	} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
		ATLTRACE(_T(" ** GetDetailsEx: PKEY_PropList_FullDetails"));
		return SUCCEEDED(InitVariantFromString(
			L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv
		));
	} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
		ATLTRACE(_T(" ** GetDetailsEx: PKEY_ItemType"));
		return SUCCEEDED(InitVariantFromString(L"Directory", pv));
	}
#endif

	ATLTRACE(_T(" ** GetDetailsEx: Not implemented"));
	return E_NOTIMPL;
}

STDMETHODIMP
COWRootShellFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid) {
	ATLTRACE(
		"COWRootShellFolder(0x%08x)::MapColumnToSCID(iColumn=%d)\n",
		this,
		iColumn
	);
#if defined(OW_PKEYS_SUPPORT)
	// This will map the columns to some built-in properties on Vista.
	// It's needed for the tile subtitles to display properly.
	switch (iColumn) {
		case DETAILS_COLUMN_NAME:
			*pscid = PKEY_ItemNameDisplay;
			return S_OK;
		case DETAILS_COLUMN_PATH:
			*pscid = PKEY_ItemPathDisplay;
			return S_OK;
			// We can seemingly skip rank and let it fall through to the legacy
			// impl
	}
	return E_FAIL;
#endif
	return E_NOTIMPL;
}
