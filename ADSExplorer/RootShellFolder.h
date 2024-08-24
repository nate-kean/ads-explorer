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

#pragma once


#include "stdafx.h"  // MUST be included first

#include "ADSExplorer_h.h"

#include "resource.h"  // main symbols


//==============================================================================

// Set the return string 'Source' in the STRRET struct.
// Note that it always allocate a UNICODE copy of the string.
// Returns false if memory allocation fails.
bool SetReturnStringA(LPCSTR Source, STRRET &str);
bool SetReturnStringW(LPCWSTR Source, STRRET &str);

#ifdef _UNICODE
#define SetReturnString SetReturnStringW
#else
#define SetReturnString SetReturnStringA
#endif


//==============================================================================

enum {
	DETAILS_COLUMN_NAME,
	DETAILS_COLUMN_FILESIZE,

	DETAILS_COLUMN_MAX
};

//==============================================================================
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

	//--------------------------------------------------------------------------
	// IPersist
	STDMETHOD(GetClassID)(CLSID*);

	//--------------------------------------------------------------------------
	// IPersistFolder(2)
	STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE);
	STDMETHOD(GetCurFolder)(PIDLIST_ABSOLUTE*);

	//--------------------------------------------------------------------------
	// IShellFolder
	STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE, IBindCtx*, REFIID, void**);
	STDMETHOD(CompareIDs)(LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE);
	STDMETHOD(CreateViewObject)(HWND, REFIID, void**);
	STDMETHOD(EnumObjects)(HWND, SHCONTF, IEnumIDList**);
	STDMETHOD(GetAttributesOf)(UINT, PCUITEMID_CHILD_ARRAY, SFGAOF*);
	STDMETHOD(GetUIObjectOf)(HWND, UINT, PCUITEMID_CHILD_ARRAY, REFIID, UINT*, void**);
	STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE, IBindCtx*, REFIID, void**);
	STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD, SHGDNF, STRRET*);
	STDMETHOD(ParseDisplayName)(HWND, IBindCtx*, LPWSTR, ULONG*, PIDLIST_RELATIVE*, ULONG*);
	STDMETHOD(SetNameOf)(HWND, PCUITEMID_CHILD, LPCWSTR, SHGDNF, PITEMID_CHILD*);

	//--------------------------------------------------------------------------
	// IShellDetails
	STDMETHOD(ColumnClick)(UINT);
	STDMETHOD(GetDetailsOf)	(PCUITEMID_CHILD, UINT, SHELLDETAILS*);

	//--------------------------------------------------------------------------
	// IShellFolder2
	STDMETHOD(EnumSearches)(IEnumExtraSearch**);
	STDMETHOD(GetDefaultColumn)(DWORD, ULONG*, ULONG*);
	STDMETHOD(GetDefaultColumnState)(UINT, SHCOLSTATEF*);
	STDMETHOD(GetDefaultSearchGUID)(GUID*);
	STDMETHOD(GetDetailsEx)(PCUITEMID_CHILD, const SHCOLUMNID*, VARIANT*);
	// Already in IShellDetails:
	// STDMETHOD(GetDetailsOf) (PCUITEMID_CHILD, UINT, SHELLDETAILS*);
	STDMETHOD(MapColumnToSCID)(UINT, SHCOLUMNID*);

	//--------------------------------------------------------------------------

   protected:
	PIDLIST_ABSOLUTE m_pidlRoot;
};
