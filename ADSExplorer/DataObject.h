/*
 * Copyright (c) 2004 Pascal Hurni
 */

#pragma once

#include "StdAfx.h"  // Precompiled header; include first

namespace ADSX {

//==============================================================================
// Light implementation of IDataObject.
//
// This object is used when you double-click an item in the FileDialog.
// Its purpose is simply to wrap the PIDL of an Item of ours into the
// IDataObject, so that the FileDialog can pass it further to our
// IShellFolder::BindToObject(). Because I'm only interested in FileDialog's
// behaviour, every method returns E_NOTIMPL except GetData() (Pascal Hurni).
// https://www.codeproject.com/Articles/7973/An-almost-complete-Namespace-Extension-Sample#HowItsDone_UseCasesFileDialog_ClickIcon

class ATL_NO_VTABLE CDataObject
	: public CComObjectRootEx<CComSingleThreadModel>,
	  public IDataObject,
	  public IEnumFORMATETC {
   public:
	BEGIN_COM_MAP(CDataObject)
		COM_INTERFACE_ENTRY(IDataObject)
		COM_INTERFACE_ENTRY(IEnumFORMATETC)
	END_COM_MAP()

	//--------------------------------------------------------------------------

	~CDataObject();

	// Ensure the owner object is not freed before this one and populate the
	// object with the Favorite Item pidl.
	// This member must be called before any IDataObject member (Pascal Hurni).
	void Init(
		IUnknown*         pUnkOwner,
		PCIDLIST_ABSOLUTE pidlParent,
		PCUITEMID_CHILD   pidl
	);

	//--------------------------------------------------------------------------
	// IDataObject
	STDMETHOD(GetData)(
		_In_  LPFORMATETC,
		_Out_ LPSTGMEDIUM
	);
	STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(QueryGetData)(LPFORMATETC);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
	STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
	STDMETHOD(EnumFormatEtc)(DWORD, IEnumFORMATETC**);
	STDMETHOD(DAdvise)(LPFORMATETC, DWORD, IAdviseSink*, LPDWORD);
	STDMETHOD(DUnadvise)(DWORD);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA**);

	//--------------------------------------------------------------------------
	// IEnumFORMATETC
	STDMETHOD(Next)(ULONG, LPFORMATETC, ULONG*);
	STDMETHOD(Skip)(ULONG);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(LPENUMFORMATETC*);

   protected:
	CComPtr<IUnknown> m_UnkOwnerPtr;

	UINT m_cfShellIDList;

	PUITEMID_CHILD m_pidl;
	PIDLIST_ABSOLUTE m_pidlParent;
};

}  // namespace ADSX
