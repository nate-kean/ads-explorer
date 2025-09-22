/*
 * Copyright (c) 2004 Pascal Hurni
 */

#pragma once

#include "StdAfx.h"  // Precompiled header; include first
#include "debug.h"

#include <iomanip>

// Debug log prefix for CADSXShellView
#define P_RSV L"CADSXShellView(0x" << std::hex << this << L")::"


/**
 * Implements a IShellView like requested by IShellFolder::CreateViewObject().
 * Derive from this class and add an ATL message map to handle the SFVM_*
 * messages. When request to IShellView is made, construct your derived object
 * and call Create() with the first param to the storage receiving the new
 * IShellView.
 *
 * ATL message maps:
 * If you did not handled the message set bHandled to FALSE. (Defaults to TRUE
 * when your message handler is called) This will return E_NOTIMPL to the caller
 * which means that the message was not handled. When you have handled the
 * message, return (as LRESULT) S_OK (which is 0) or any other valid value
 * described in the SDK for your message.
 *
 * In any of your message handler, you can use m_pISF which is a pointer to the
 * related IShellFolder.
 */
class CShellFolderViewImpl : public CMessageMap,
							 public CComObjectRoot,
							 public IShellFolderViewCB {
   public:
	BEGIN_COM_MAP(CShellFolderViewImpl)
		COM_INTERFACE_ENTRY(IShellFolderViewCB)
	END_COM_MAP()
	// ppISV - will receive the interface pointer to the view (that one should be
	// returned from IShellFolder::CreateViewObject)
	// hwndOwner - The window handle of the parent to the new shell view
	// pISF - The IShellFolder related to the view
	HRESULT Create(
		_In_ HWND hwndOwner,
		_In_ IShellFolder *pShellFolder,
		_In_opt_ IShellView *psvOuter,
		_Outptr_ IShellView **ppShellView
	) {
		// LOG(P_RSV << L"Create()");
		m_hwndOwner = hwndOwner;
		m_psf = pShellFolder;

		SFV_CREATE sfv;
		sfv.cbSize = sizeof(SFV_CREATE);
		sfv.pshf = m_psf;
		sfv.psvOuter = psvOuter;
		sfv.psfvcb = static_cast<IShellFolderViewCB *>(this);

		return SHCreateShellFolderView(&sfv, ppShellView);
		// return WrapReturn(SHCreateShellFolderView(&sfv, ppShellView));
	}

	// Used to send messages back to the shell view
	LRESULT SendFolderViewMessage(UINT uMsg, LPARAM lParam) {
		// LOG(P_RSV << L"SendFolderViewMessage(uMsg=" << uMsg << L")");
		return SHShellFolderView_Message(m_hwndOwner, uMsg, lParam);
	}

	STDMETHODIMP MessageSFVCB(
		_In_ UINT   uMsg,
		     WPARAM wParam,
		     LPARAM lParam
	) noexcept {
		// LOG(P_RSV << L"MessageSFVCB(uMsg=" << uMsg << L")");
		LRESULT lResult = NULL;
		BOOL bResult = this->ProcessWindowMessage(
			NULL,
			uMsg,
			wParam,
			lParam,
			lResult,
			0
		);
		return bResult ? static_cast<HRESULT>(lResult) : E_NOTIMPL;
		// return WrapReturnFailOK(bResult ? static_cast<HRESULT>(lResult) : E_NOTIMPL);
	}

   protected:
	HWND m_hwndOwner;
	// This one is not ref-counted. This object should be guaranteed to live
	// until the view is destroyed. So the lifetime is handled by
	// SHCreateShellFolderView().
	IShellFolder *m_psf;
};


/**
 * This class does very little but trace the messages.
 * You can add message handlers like the SFVM_COLUMNCLICK.
 */
class CADSXShellView : public CShellFolderViewImpl {
   public:
	CADSXShellView() {
		// LOG(P_RSV << L"CADSXShellView()");
	}

	~CADSXShellView() {
		// LOG(P_RSV << L"~CADSXShellView()");
	}

	// If called, the passed object will be held (AddRef()'d) until the View
	// gets deleted.
	void Init(IUnknown *pUnkOwner = NULL) { m_UnkOwnerPtr = pUnkOwner; }

	// The message map
	BEGIN_MSG_MAP(CADSXShellView)
		MESSAGE_HANDLER(SFVM_COLUMNCLICK, OnColumnClick)
		MESSAGE_HANDLER(SFVM_GETDETAILSOF, OnGetDetailsOf)
		MESSAGE_HANDLER(SFVM_DEFVIEWMODE, OnDefViewMode)
	END_MSG_MAP()

	// Offer to set the default view mode
	LRESULT OnDefViewMode(
		UINT   uMsg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL   &bHandled
	) {
		LOG(P_RSV << L"OnDefViewMode()");
		#ifdef FVM_CONTENT
			/* Requires Windows 7+, by Gravis' request */
			DWORD ver, maj, min;
			ver = GetVersion();
			maj = (DWORD) (LOBYTE(LOWORD(dwVersion)));
			min = (DWORD) (HIBYTE(LOWORD(dwVersion)));
			if (maj > 6 || (maj == 6 && min >= 1)) {
				*(FOLDERVIEWMODE *) lParam = FVM_CONTENT;
			}
		#endif
		// return S_OK;
		return WrapReturn(S_OK);
	}

	// When a user clicks on a column header in details mode
	LRESULT OnColumnClick(
		UINT   uMsg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL   &bHandled
	) {
		// LOG(P_RSV << L"OnColumnClick(iColumn=" << static_cast<int>(wParam) << L")");

		// Shell version 4.7x doesn't understand S_FALSE as described in the
		// SDK.
		SendFolderViewMessage(SFVM_REARRANGE, wParam);

		return S_OK;
		// return WrapReturn(S_OK);
	}

	// This message is used with shell version 4.7x, shell 5 and above prefer to
	// use IShellFolder2::GetDetailsOf()
	LRESULT OnGetDetailsOf(
		UINT   uMsg,
		WPARAM wParam,
		LPARAM lParam,
		BOOL   &bHandled
	) {
		int iColumn = static_cast<int>(wParam);
		auto pDetailsInfo = reinterpret_cast<DETAILSINFO *>(lParam);

		// LOG(P_RSV << L"OnGetDetailsOf(iColumn=" << iColumn << L")");

		if (pDetailsInfo == NULL) return E_POINTER;

		HRESULT hr;
		SHELLDETAILS ShellDetails;

		IShellDetails *pShellDetails;
		hr = m_psf->QueryInterface(IID_PPV_ARGS(&pShellDetails));
		// if (FAILED(hr)) return hr;
		if (FAILED(hr)) return WrapReturn(hr);

		hr = pShellDetails->GetDetailsOf(pDetailsInfo->pidl, iColumn, &ShellDetails);
		pShellDetails->Release();
		// if (FAILED(hr)) return hr;
		if (FAILED(hr)) return WrapReturn(hr);

		pDetailsInfo->cxChar = ShellDetails.cxChar;
		pDetailsInfo->fmt = ShellDetails.fmt;
		pDetailsInfo->str = ShellDetails.str;
		pDetailsInfo->iImage = 0;

		// return S_OK;
		return WrapReturn(S_OK);
	}

   protected:
	CComPtr<IUnknown> m_UnkOwnerPtr;
};
