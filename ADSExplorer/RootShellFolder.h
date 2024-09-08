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
	virtual ~CADSXRootShellFolder();

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
	STDMETHOD(GetClassID)(_Out_ CLSID*);

	//--------------------------------------------------------------------------
	// IPersistFolder(2)
	STDMETHOD(Initialize)(_In_ PCIDLIST_ABSOLUTE);
	STDMETHOD(GetCurFolder)(_Outptr_ PIDLIST_ABSOLUTE*);

	//--------------------------------------------------------------------------
	// IShellFolder
	STDMETHOD(BindToObject)(_In_ PCUIDLIST_RELATIVE, _In_opt_ IBindCtx*, _In_ REFIID, _COM_Outptr_ void**);
	STDMETHOD(CompareIDs)(_In_ LPARAM, _In_ PCUIDLIST_RELATIVE, _In_ PCUIDLIST_RELATIVE);
	STDMETHOD(CreateViewObject)(_In_ HWND, _In_ REFIID, _COM_Outptr_ void**);
	STDMETHOD(EnumObjects)(_In_ HWND, _In_ SHCONTF, _COM_Outptr_ IEnumIDList**);
	STDMETHOD(GetAttributesOf)(_In_ UINT, _In_ PCUITEMID_CHILD_ARRAY, _Inout_ SFGAOF*);
	STDMETHOD(GetUIObjectOf)(_In_ HWND, _In_ UINT, _In_ PCUITEMID_CHILD_ARRAY, _In_ REFIID, _Inout_ UINT*, _COM_Outptr_ void**);
	STDMETHOD(BindToStorage)(_In_ PCUIDLIST_RELATIVE, _In_ IBindCtx*, _In_ REFIID, _COM_Outptr_ void**);
	STDMETHOD(GetDisplayNameOf)(_In_ PCUITEMID_CHILD, _In_ SHGDNF, _Out_ STRRET*);
	STDMETHOD(ParseDisplayName)(_In_ HWND, _In_opt_ IBindCtx*, _In_ LPWSTR, _Out_opt_ ULONG*, _Outptr_ PIDLIST_RELATIVE*, _Inout_opt_ ULONG*);
	STDMETHOD(SetNameOf)(_In_ HWND, _In_ PCUITEMID_CHILD, _In_ LPCWSTR, _In_ SHGDNF, _Outptr_ PITEMID_CHILD*);

	//--------------------------------------------------------------------------
	// IShellDetails
	STDMETHOD(ColumnClick)(_In_ UINT);
	STDMETHOD(GetDetailsOf)(_In_opt_ PCUITEMID_CHILD, _In_ UINT, _Out_ SHELLDETAILS*);

	//--------------------------------------------------------------------------
	// IShellFolder2
	STDMETHOD(EnumSearches)(_COM_Outptr_ IEnumExtraSearch**);
	STDMETHOD(GetDefaultColumn)(_In_ DWORD, _Out_ ULONG*, _Out_ ULONG*);
	STDMETHOD(GetDefaultColumnState)(_In_ UINT, _Out_ SHCOLSTATEF*);
	STDMETHOD(GetDefaultSearchGUID)(_Out_ GUID*);
	STDMETHOD(GetDetailsEx)(_In_ PCUITEMID_CHILD, _In_ const SHCOLUMNID*, _Out_ VARIANT*);
	// Already in IShellDetails:
	// STDMETHOD(GetDetailsOf) (PCUITEMID_CHILD, UINT, SHELLDETAILS*);
	STDMETHOD(MapColumnToSCID)(_In_ UINT, _Out_ SHCOLUMNID*);

	//--------------------------------------------------------------------------
	// Helper member functions
	HRESULT ClipDesktop(PUIDLIST_RELATIVE *ppidl);
	HRESULT ClipADSX(PUIDLIST_RELATIVE *ppidl);

	//--------------------------------------------------------------------------

   protected:
	PIDLIST_ABSOLUTE m_pidlRoot;  // [Desktop]\ADS Explorer
	PIDLIST_ABSOLUTE m_pidlPath;  // The path to the filesystem object to view ADSes of, relative to [Desktop]
	CComPtr<IShellFolder2> m_psfObjPath;
	CComPtr<IShellFolder> m_psfDesktop;
};
