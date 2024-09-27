/*
 * Copyright (c) 2004 Pascal Hurni
 */

//==============================================================================
// Implements a IShellView like requested by IShellFolder::CreateViewObject().
// Derive from this class and add an ATL message map to handle the SFVM_*
// messages. When request to IShellView is made, construct your derived object
// and call Create() with the first param to the storage receiving the new
// IShellView.
//
// Example:
//	CRootShellView is a CShellFolderViewImpl subclass
/*
	STDMETHODIMP CRootShellFolder::CreateViewObject(HWND hwndOwner, REFIID riid,
   void** ppv)
	{
		HRESULT hr;

		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		if (riid == IID_IShellView)
		{
			// Create a view object
			CComObject<CRootShellView>* pViewObject;
			hr = CComObject<CRootShellView>::CreateInstance(&pViewObject);
			if (FAILED(hr))
				return hr;

			// AddRef the object while we are using it
			pViewObject->AddRef();

			// Create the view
			hr = pViewObject->Create((IShellView**)ppv, hwndOwner,
   (IShellFolder*)this);

			// We are finished with our own use of the view object (AddRef()'ed
   above by us, AddRef()'ed by Create) pViewObject->Release();

			return hr;
		}

		return E_NOINTERFACE;
	}
*/
// ATL message maps:
// If you did not handled the message set bHandled to FALSE. (Defaults to TRUE
// when your message handler is called) This will return E_NOTIMPL to the caller
// which means that the message was not handled. When you have handled the
// message, return (as LRESULT) S_OK (which is 0) or any other valid value
// described in the SDK for your message.
//
// In any of your message handler, you can use m_pISF which is a pointer to the
// related IShellFolder.
//
//==============================================================================

#pragma once

#include "stdafx.h"  // MUST be included first

class CShellFolderViewImpl : public CMessageMap,
							 public CComObjectRoot,
							 public IShellFolderViewCB {
   public:
	BEGIN_COM_MAP(CShellFolderViewImpl)
		COM_INTERFACE_ENTRY_IID(IID_IShellFolderViewCB, IShellFolderViewCB)
	END_COM_MAP()
	// ppISV - will receive the interface pointer to the view (that one should be
	// returned from IShellFolder::CreateViewObject)
	// hwndOwner - The window handle of the parent to the new shell view
	// pISF - The IShellFolder related to the view
	HRESULT Create(
		IShellView **ppISV,
		HWND hwndOwner,
		IShellFolder *pISF,
		IShellView *psvOuter = NULL
	) {
		m_hwndOwner = hwndOwner;
		m_psf = pISF;

		SFV_CREATE sfv;
		sfv.cbSize = sizeof(SFV_CREATE);
		sfv.pshf = m_psf;
		sfv.psvOuter = psvOuter;
		sfv.psfvcb = static_cast<IShellFolderViewCB *>(this);

		return SHCreateShellFolderView(&sfv, ppISV);
	}

	// Used to send messages back to the shell view
	LRESULT SendFolderViewMessage(UINT uMsg, LPARAM lParam) {
		return SHShellFolderView_Message(m_hwndOwner, uMsg, lParam);
	}

	STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam) {
		LRESULT lResult = NULL;
		BOOL bResult = ProcessWindowMessage(
			NULL,
			uMsg,
			wParam,
			lParam,
			lResult,
			0
		);
		return bResult ? static_cast<HRESULT>(lResult) : E_NOTIMPL;
	}

   protected:
	HWND m_hwndOwner;
	// This one is not ref-counted. This object should be guaranteed to live
	// until the view is destroyed. So the lifetime is handled by
	// SHCreateShellFolderView().
	IShellFolder *m_psf;
};

//==============================================================================
