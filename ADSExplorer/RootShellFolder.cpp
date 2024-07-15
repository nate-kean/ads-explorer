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

#include <Shlwapi.h>
#include <objidl.h>
#include <shobjidl_core.h>
#include <shtypes.h>
#include "stdafx.h"  // MUST be included first

#if _MSC_VER > 1200
	#include "ADSExplorer_h.h"
#else
	// the IDL compiler on VC++6 puts it here instead. weird!
	#include "ADSExplorer.h"
#endif

#include <atlcore.h>
#include <winnt.h>

#include "CADSXEnumIDList.h"
#include "CADSXItem.h"
#include "PidlMgr.h"
#include "RootShellFolder.h"
#include "RootShellView.h"
#include "defer.h"

//==============================================================================
// Helpers
// #define _DEBUG

#ifdef _DEBUG
	LPWSTR PidlToString(LPCITEMIDLIST pidl) {
		_bstr_t str;

		if (pidl == NULL) {
			return str + "<null>";
		}

		while (pidl->mkid.cb != 0) {
			if (CADSXItem::IsOwn(pidl)) {
				_bstr_t sName = CADSXItem::Get(pidl)->m_Name;
				str += sName + "--";
			} else {
				bool success = false;
				// success = SHGetPathFromIDListW(pidl, str);
				if (!success) {
					WCHAR tmp[16];
					swprintf_s(tmp, L"<unk-%02d>--", pidl->mkid.cb);
					str += tmp;
				}
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
// CADSXRootShellFolder
CADSXRootShellFolder::CADSXRootShellFolder() : m_pidlRoot(NULL) {}

STDMETHODIMP CADSXRootShellFolder::GetClassID(CLSID *pClsid) {
	if (pClsid == NULL) return E_POINTER;
	*pClsid = CLSID_ADSExplorerRootShellFolder;
	return S_OK;
}

// Initialize() is passed the PIDL of the folder where our extension is.
STDMETHODIMP CADSXRootShellFolder::Initialize(PCIDLIST_ABSOLUTE pidl) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::Initialize() pidl=[%s]\n"),
		this,
		PidlToString(pidl)
	);
	m_pidlRoot = PidlMgr::Copy(pidl);
	return S_OK;
}

STDMETHODIMP CADSXRootShellFolder::GetCurFolder(PIDLIST_ABSOLUTE *ppidl) {
	AtlTrace(_T("CADSXRootShellFolder(0x%08x)::GetCurFolder()\n"), this);
	if (ppidl == NULL) return E_POINTER;
	*ppidl = PidlMgr::Copy(m_pidlRoot);
	return S_OK;
}

//-------------------------------------------------------------------------------
// IShellFolder

// BindToObject() is called when a folder in our part of the namespace is being
// browsed.
STDMETHODIMP CADSXRootShellFolder::BindToObject(
	/* [in]  */ PCUIDLIST_RELATIVE pidl,
	/* [in]  */ IBindCtx *pbc,
	/* [in]  */ REFIID riid,
	/* [out] */ void **ppvOut
) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::BindToObject() pidl=[%s]\n"),
		this,
		PidlToString(pidl)
	);

	// If the passed pidl is not ours, fail.
	if (!CADSXItem::IsOwn(pidl)) return E_INVALIDARG;

	// I'll support multi-level PIDLs when I implement pseudofolders
	if (!PidlMgr::IsChild(pidl)) return E_NOTIMPL;

	HRESULT hr;
	CComPtr<IShellFolder> psfDesktop;

	hr = SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr)) return hr;

	PIDLIST_ABSOLUTE pidlAbsolute = NULL;
	auto Item = CADSXItem::Get(pidl);
	hr = psfDesktop->ParseDisplayName(
		NULL, pbc, Item->m_Path, NULL, &pidlAbsolute, NULL
	);
	if (FAILED(hr)) return hr;
	defer({ ILFree(pidlAbsolute); });

	// Okay, browsing into a favorite item will redirect to its real path.
	hr = psfDesktop->BindToObject(pidlAbsolute, pbc, riid, ppvOut);
	return hr;

	// could also use this one? ILCreateFromPathW
}

// CompareIDs() is responsible for returning the sort order of two PIDLs.
// lParam can be the 0-based Index of the details column
STDMETHODIMP CADSXRootShellFolder::CompareIDs(
	/* [in] */ LPARAM lParam,
	/* [in] */ PCUIDLIST_RELATIVE pidl1,
	/* [in] */ PCUIDLIST_RELATIVE pidl2
) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::CompareIDs(lParam=%d) pidl1=[%s], ")
		_T("pidl2=[%s]\n"),
		this,
		lParam,
		PidlToString(pidl1),
		PidlToString(pidl2)
	);

	// First check if the pidl are ours
	if (!CADSXItem::IsOwn(pidl1) || !CADSXItem::IsOwn(pidl2)) {
		return E_INVALIDARG;
	}

	// Now check if the pidl are one or multi level, in case they are
	// multi-level, return non-equality
	if (!PidlMgr::IsChild(pidl1) || !PidlMgr::IsChild(pidl2)) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
	}

	USHORT Result = 0;  // see note below (MAKE_HRESULT)

	auto Item1 = CADSXItem::Get(pidl1);
	auto Item2 = CADSXItem::Get(pidl2);

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

// CreateViewObject() creates a new COM object that implements IShellView.
STDMETHODIMP CADSXRootShellFolder::CreateViewObject(
	/* [in]  */ HWND hwndOwner,
	/* [in]  */ REFIID riid,
	/* [out] */ void **ppvOut
) {
	AtlTrace(_T("CADSXRootShellFolder(0x%08x)::CreateViewObject()\n"), this);
	// DUMPIID(riid);

	HRESULT hr;

	if (ppvOut == NULL) {
		return E_POINTER;
	}

	*ppvOut = NULL;

	// We handle only the IShellView
	if (riid == IID_IShellView) {
		AtlTrace(_T(" ** CreateViewObject for IShellView\n"));

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
		StringFromIID(riid, &unkIid);
		defer({ CoTaskMemFree(unkIid); });
		AtlTrace(_T(" ** CreateViewObject is unknown: %s\n"), unkIid);
	}
#endif

	// We do not handle other objects
	return E_NOINTERFACE;
}

// EnumObjects() creates a COM object that implements IEnumIDList.
STDMETHODIMP CADSXRootShellFolder::EnumObjects(
	/* [in]  */ HWND hwndOwner,
	/* [in]  */ SHCONTF dwFlags,
	/* [out] */ IEnumIDList **ppEnumIDList
) {
	AtlTrace(
		"CADSXRootShellFolder(0x%08x)::EnumObjects(dwFlags=0x%04x)\n",
		this,
		dwFlags
	);

	HRESULT hr;

	if (ppEnumIDList == NULL) return E_POINTER;
	*ppEnumIDList = NULL;

	// Create an enumerator over this file system object's alternate data streams.
	CComObject<CADSXEnumIDList> *pEnum;
	hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnum);
	if (FAILED(hr)) return hr;
	pEnum->AddRef();
	defer({ pEnum->Release(); });

	wchar_t pszPath[MAX_PATH];
	SHGetPathFromIDListW(m_pidlRoot, pszPath);
	_bstr_t bstrPath(pszPath);

	AtlTrace(_T(" ** EnumObjects: Path=%s\n"), bstrPath.GetBSTR());
	pEnum->Init(GetUnknown(), bstrPath.Detach());

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
		if (uCount >= 1) {
			AtlTrace(
				_T("CADSXRootShellFolder(0x%08x)::GetAttributesOf(uCount=%d) ")
				_T("pidl=[%s]\n"),
				this,
				uCount,
				PidlToString(aPidls[0])
			);
		} else {
			AtlTrace(
				"CADSXRootShellFolder(0x%08x)::GetAttributesOf(uCount=%d)\n",
				this,
				uCount
			);
		}
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
	void **ppvReturn
) {
	#ifdef _DEBUG
		if (uCount >= 1) {
			AtlTrace(
				_T("CADSXRootShellFolder(0x%08x)::GetUIObjectOf(uCount=%d) ")
				_T("pidl=[%s]\n"),
				this,
				uCount,
				PidlToString(*pPidl)
			);
		} else {
			AtlTrace(
				_T("CADSXRootShellFolder(0x%08x)::GetUIObjectOf(uCount=%d)\n"),
				this,
				uCount
			);
		}
		// DUMPIID(riid);
	#endif

	HRESULT hr;

	if (ppvReturn == NULL) {
		return E_POINTER;
	}

	*ppvReturn = NULL;

	if (cidl == 0) {
		return E_INVALIDARG;
	}

	// Does the FileDialog need to embed some data?
	if (riid == IID_IDataObject) {
		// Only one item at a time
		if (cidl != 1) return E_INVALIDARG;

		// Is this really one of our item?
		if (!CADSXItem::IsOwn(*aPidls)) return E_INVALIDARG;

		// Create a COM object that exposes IDataObject
		CComObject<CDataObject> *pDataObject;
		hr = CComObject<CDataObject>::CreateInstance(&pDataObject);
		if (FAILED(hr)) return hr;

		// AddRef it while we are working with it, this prevent from an early
		// destruction.
		pDataObject->AddRef();

		// Tight its lifetime with this object (the IShellFolder object)
		pDataObject->Init(GetUnknown());

		// Okay, embed the pidl in the data
		pDataObject->SetPidl(m_pidlRoot, *aPidls);

		// Return the requested interface to the caller
		hr = pDataObject->QueryInterface(riid, ppvReturn);

		// We do no more need our ref (note that the object will not die because
		// the QueryInterface above, AddRef'd it)
		pDataObject->Release();
		return hr;
	}  // All other requests are delegated to the target path's IShellFolder

	// Bbecause multiple items can point to different storages, we can't (easily)
	// handle groups of items.
	// TODO(garlic-os): Still current?
	if (cidl > 1) return E_NOINTERFACE;

	if (!CADSXItem::IsOwn(*aPidls)) return E_NOINTERFACE;

	CComPtr<IShellFolder> psfTargetParent;
	CComPtr<IShellFolder> psfDesktop;

	hr = SHGetDesktopFolder(&psfDesktop);
	if (FAILED(hr)) return hr;

	PIDLIST_ABSOLUTE pidlAbsolute = NULL;
	auto Item = CADSXItem::Get(*aPidls);
	hr = psfDesktop->ParseDisplayName(
		NULL, NULL, Item->m_Path, NULL, &pidlAbsolute, NULL
	);
	if (FAILED(hr)) return hr;
	defer({ ILFree(pidlAbsolute); });

	//------------------------------
	// polyfill for the following line
	// (not available to shell version 4.7x):
	//   hr = SHBindToParent(
	//   	pidlAbsolute,
	//   	IID_IShellFolder,
	//   	(void**) &pTargetParentShellFolder,
	//   	&pidlChild
	//   );
	PUITEMID_CHILD pidlChild = ILClone(ILFindLastID(pidlAbsolute));
	defer({ ILFree(pidlChild); });
	ILRemoveLastID(pidlAbsolute);
	hr = psfDesktop->BindToObject(
		pidlAbsolute, NULL, IID_IShellFolder, (void **) &psfTargetParent
	);
	if (FAILED(hr)) return hr;
	//------------------------------

	hr = psfTargetParent->GetUIObjectOf(
		hwndOwner,
		1,
		(PCUITEMID_CHILD_ARRAY) &pidlChild,
		riid,
		rgfReserved,
		ppvReturn
	);

	return hr;
}

STDMETHODIMP
CADSXRootShellFolder::BindToStorage(PCUIDLIST_RELATIVE, IBindCtx *, REFIID, void **) {
	AtlTrace("CADSXRootShellFolder(0x%08x)::BindToStorage()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDisplayNameOf(
	PCUITEMID_CHILD pidl,
	SHGDNF uFlags,
	STRRET *pName
) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::GetDisplayNameOf(uFlags=0x%04x) ")
		_T("pidl=[%s]\n"),
		this,
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
	AtlTrace("CADSXRootShellFolder(0x%08x)::ParseDisplayName()\n", this);
	return E_NOTIMPL;
}

// TODO(garlic-os): should this be implemented?
STDMETHODIMP CADSXRootShellFolder::SetNameOf(HWND, PCUITEMID_CHILD, LPCWSTR, SHGDNF, PITEMID_CHILD *) {
	AtlTrace("CADSXRootShellFolder(0x%08x)::SetNameOf()\n", this);
	return E_NOTIMPL;
}

//-------------------------------------------------------------------------------
// IShellDetails

STDMETHODIMP CADSXRootShellFolder::ColumnClick(UINT iColumn) {
	AtlTrace(
		"CADSXRootShellFolder(0x%08x)::ColumnClick(iColumn=%d)\n", this, iColumn
	);

	// The caller must sort the column itself
	return S_FALSE;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsOf(
	/* [in, optional] */ PCUITEMID_CHILD pidl,
	/* [in]           */ UINT iColumn,
	/* [out]          */ SHELLDETAILS *pDetails
) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::GetDetailsOf(iColumn=%d) pidl=[%s]\n"),
		this,
		iColumn,
		PidlToString(pidl)
	);

	if (iColumn >= DETAILS_COLUMN_MAX) {
		return E_FAIL;
	}

	// Shell asks for the column headers
	// TODO(garlic-os): should filesize be here too?
	if (pidl == NULL) {
		// Load the iColumn based string from the resource
		// TODO(garlic-os): can CString be changed out for something else?
		CString ColumnName(MAKEINTRESOURCE(IDS_COLUMN_NAME + iColumn));
		pDetails->fmt = LVCFMT_LEFT;
		pDetails->cxChar = 32;
		return SetReturnString(ColumnName, pDetails->str) ? S_OK
														  : E_OUTOFMEMORY;
	}

	// Okay, this time it's for a real item
	auto Item = CADSXItem::Get(pidl);
	switch (iColumn) {
		case DETAILS_COLUMN_NAME:
			pDetails->fmt = LVCFMT_LEFT;
			pDetails->cxChar = (int) wcslen(Item->m_Name);
			return SetReturnStringW(Item->m_Name, pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;

		case DETAILS_COLUMN_FILESIZE:
			pDetails->fmt = LVCFMT_RIGHT;
			BSTR pszSize[16] = {0};
			StrFormatByteSizeW(Item->m_Filesize, (BSTR) pszSize, 16);
			pDetails->cxChar = (int) wcslen((BSTR) pszSize);
			return SetReturnStringW((BSTR) pszSize, pDetails->str)
				? S_OK
				: E_OUTOFMEMORY;
	}

	return E_INVALIDARG;
}

//------------------------------------------------------------------------------
// IShellFolder2

STDMETHODIMP CADSXRootShellFolder::EnumSearches(IEnumExtraSearch **ppEnum) {
	AtlTrace("CADSXRootShellFolder(0x%08x)::EnumSearches()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDefaultColumn(
	DWORD dwReserved,
	ULONG *pSort,
	ULONG *pDisplay
) {
	AtlTrace("CADSXRootShellFolder(0x%08x)::GetDefaultColumn()\n", this);

	if (!pSort || !pDisplay) {
		return E_POINTER;
	}

	*pSort = DETAILS_COLUMN_NAME;
	*pDisplay = DETAILS_COLUMN_NAME;

	return S_OK;
}

STDMETHODIMP
CADSXRootShellFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags) {
	AtlTrace(
		"CADSXRootShellFolder(0x%08x)::GetDefaultColumnState(iColumn=%d)\n",
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
		case DETAILS_COLUMN_FILESIZE:
			*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT;
			break;
		default:
			return E_INVALIDARG;
	}

	return S_OK;
}

STDMETHODIMP CADSXRootShellFolder::GetDefaultSearchGUID(GUID *pguid) {
	AtlTrace("CADSXRootShellFolder(0x%08x)::GetDefaultSearchGUID()\n", this);
	return E_NOTIMPL;
}

STDMETHODIMP CADSXRootShellFolder::GetDetailsEx(
	LPCITEMIDLIST pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv
) {
	AtlTrace(
		_T("CADSXRootShellFolder(0x%08x)::GetDetailsEx(pscid->pid=%d) ")
		_T("pidl=[%s]\n"),
		this,
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
			AtlTrace(_T(" ** GetDetailsEx: PKEY_PropList_TileInfo\n"));
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_ExtendedTileInfo)) {
			AtlTrace(_T(" ** GetDetailsEx: PKEY_PropList_ExtendedTileInfo\n"));
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_PreviewDetails)) {
			AtlTrace(_T(" ** GetDetailsEx: PKEY_PropList_PreviewDetails\n"));
			return SUCCEEDED(
				InitVariantFromString(L"prop:System.ItemPathDisplay", pv)
			);
		} else if (IsEqualPropertyKey(*pscid, PKEY_PropList_FullDetails)) {
			AtlTrace(_T(" ** GetDetailsEx: PKEY_PropList_FullDetails\n"));
			return SUCCEEDED(InitVariantFromString(
				L"prop:System.ItemNameDisplay;System.ItemPathDisplay", pv
			));
		} else if (IsEqualPropertyKey(*pscid, PKEY_ItemType)) {
			AtlTrace(_T(" ** GetDetailsEx: PKEY_ItemType\n"));
			return SUCCEEDED(InitVariantFromString(L"Directory", pv));
		}
	#endif

	AtlTrace(_T(" ** GetDetailsEx: Not implemented\n"));
	return E_NOTIMPL;
}

STDMETHODIMP
CADSXRootShellFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid) {
	AtlTrace(
		"CADSXRootShellFolder(0x%08x)::MapColumnToSCID(iColumn=%d)\n",
		this,
		iColumn
	);
	#if defined(ADSX_PKEYS_SUPPORT)
		// This will map the columns to some built-in properties on Vista.
		// It's needed for the tile subtitles to display properly.
		switch (iColumn) {
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
