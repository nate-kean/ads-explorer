/*
 * Copyright (c) 2004 Pascal Hurni
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

#ifndef __ROOTSHELLFOLDER_H_
#define __ROOTSHELLFOLDER_H_

#include "MPidlMgr.h"
#include "resource.h"  // main symbols
using namespace Mortimer;

#include "CComEnumOnCArray.h"
#include "ShellItems.h"

//========================================================================================

enum {
	DETAILS_COLUMN_NAME,
	DETAILS_COLUMN_PATH,
	DETAILS_COLUMN_RANK,

	DETAILS_COLUMN_MAX
};

//========================================================================================
// CADSXRootShellFolder
class ATL_NO_VTABLE CADSXRootShellFolder
	: public CComObjectRootEx<CComSingleThreadModel>,
	  public CComCoClass<CADSXRootShellFolder, &CLSID_ADSExplorerRootShellFolder>,
	  public IShellFolder2,
	  public IPersistFolder2,
	  public IShellDetails {
   public:
	CADSXRootShellFolder();

	DECLARE_REGISTRY_RESOURCEID(IDR_ROOTSHELLFOLDER)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CADSXRootShellFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY(IShellFolder2)
		COM_INTERFACE_ENTRY(IPersistFolder)
		COM_INTERFACE_ENTRY(IPersistFolder2)
		COM_INTERFACE_ENTRY(IPersist)
		COM_INTERFACE_ENTRY_IID(IID_IShellDetails, IShellDetails)
	END_COM_MAP()

   public:
	//-------------------------------------------------------------------------------
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID);

	//-------------------------------------------------------------------------------
	// IPersistFolder(2)
	STDMETHOD(Initialize)(PCUIDLIST_ABSOLUTE pidl);
	STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE *ppidl);

	//-------------------------------------------------------------------------------
	// IShellFolder
	STDMETHOD(BindToObject) (
		PCUIDLIST_RELATIVE pidl,
		IBindCtx *pbcReserved,
		REFIID riid,
		void **ppvOut
	);
	STDMETHOD(CompareIDs) (
		LPARAM lParam,
		PCUIDLIST_RELATIVE pidl1,
		PCUIDLIST_RELATIVE pidl2
	);
	STDMETHOD(CreateViewObject) (
		HWND hwndOwner,
		REFIID riid,
		void **ppvOut
	);
	STDMETHOD(EnumObjects) (
		HWND hwndOwner,
		SHCONTF grfFlags,
		IEnumIDList **ppEnumIDList
	);
	STDMETHOD(GetAttributesOf) (
		UINT cidl,
		PCUITEMID_CHILD_ARRAY apidl,
		SFGAOF *rgfInOut
	);
	STDMETHOD(GetUIObjectOf) (
		HWND hwndOwner,
		UINT cidl,
		PCUITEMID_CHILD_ARRAY apidl,
		REFIID riid,
		UINT *rgfReserved,
		void **ppvOut
	);
	STDMETHOD(BindToStorage) (
		PCUIDLIST_RELATIVE pidl,
		IBindCtx *pbc,
		REFIID riid,
		void **ppvOut
	);
	STDMETHOD(GetDisplayNameOf)	(
		PCUITEMID_CHILD pidl,
		SHGDNF uFlags,
		STRRET *pName
	);
	STDMETHOD(ParseDisplayName) (
		HWND hwnd,
		IBindCtx *pbc,
		LPWSTR pszDisplayName,
		ULONG *pchEaten,
		PIDLIST_RELATIVE *ppidl,
		ULONG *pdwAttributes
	);
	STDMETHOD(SetNameOf) (
		HWND hwnd,
		PCUITEMID_CHILD pidl,
		LPCWSTR pszName,
		SHGDNF uFlags,
		PITEMID_CHILD *ppidlOut
	);

	//-------------------------------------------------------------------------------
	// IShellDetails
	STDMETHOD(ColumnClick) (
		UINT iColumn
	);
	STDMETHOD(GetDetailsOf) (
		PCUITEMID_CHILD pidl,
		UINT iColumn,
		LPSHELLDETAILS pDetails
	);

	//-------------------------------------------------------------------------------
	// IShellFolder2
	STDMETHOD(EnumSearches) (
		IEnumExtraSearch **ppEnum
	);
	STDMETHOD(GetDefaultColumn) (
		DWORD dwReserved,
		ULONG *pSort,
		ULONG *pDisplay
	);
	STDMETHOD(GetDefaultColumnState) (
		UINT iColumn,
		SHCOLSTATEF *pcsFlags
	);
	STDMETHOD(GetDefaultSearchGUID) (
		GUID *pguid
	);
	STDMETHOD(GetDetailsEx) (
		PCUITEMID_CHILD pidl,
		const SHCOLUMNID *pscid,
		VARIANT *pv
	);
	// already in IShellDetails: STDMETHOD(GetDetailsOf) (LPCITEMIDLIST pidl,
	// UINT iColumn, SHELLDETAILS *psd);
	STDMETHOD(MapColumnToSCID) (
		UINT iColumn,
		SHCOLUMNID *pscid
	);

	//-------------------------------------------------------------------------------

   protected:
	CPidlMgr m_PidlMgr;
	PIDLIST_ABSOLUTE m_pidlRoot;
};

#endif	//__ROOTSHELLFOLDER_H_
