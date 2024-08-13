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

#include "stdafx.h"

#include <combaseapi.h>
#include <comutil.h>

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
// Light implementation of IDataObject.
//
// This object is used when you double-click on an item in the FileDialog.
// Its purpose is simply to encapsulate the complete pidl for the item
// (remember it's a Favorite item) into the IDataObject, so that the FileDialog
// can pass it further to our IShellFolder::BindToObject(). Because I'm only
// interested in the FileDialog behaviour, every methods returns E_NOTIMPL
// except GetData().

class ATL_NO_VTABLE CDataObject
	: public CComObjectRootEx<CComSingleThreadModel>,
	  public IDataObject,
	  public IEnumFORMATETC {
   public:
	BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY_IID(IID_IDataObject, IDataObject)
	COM_INTERFACE_ENTRY_IID(IID_IEnumFORMATETC, IEnumFORMATETC)
	END_COM_MAP()

	//--------------------------------------------------------------------------

	~CDataObject();

	// Ensure the owner object is not freed before this one and 
	// populate the object with the Favorite Item pidl.
	// This member must be called before any IDataObject member.
	void Init(IUnknown *pUnkOwner, PCIDLIST_ABSOLUTE pidlParent, PCUITEMID_CHILD pidl);

	//--------------------------------------------------------------------------
	// IDataObject methods
	STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM);
	STDMETHOD(QueryGetData)(LPFORMATETC);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC);
	STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL);
	STDMETHOD(EnumFormatEtc)(DWORD, IEnumFORMATETC**);
	STDMETHOD(DAdvise)(LPFORMATETC, DWORD, IAdviseSink*, LPDWORD);
	STDMETHOD(DUnadvise)(DWORD);
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA**);

	//--------------------------------------------------------------------------
	// IEnumFORMATETC members
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
