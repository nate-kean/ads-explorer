/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

#include "StdAfx.h"  // Precompiled header; include first

#include "DataObject.h"

// Debug log prefix for CDataObject
#define P_DO L"CDataObject(0x" << std::hex << this << L")::"

namespace ADSX {

// helper function that creates a CFSTR_SHELLIDLIST format from given pidls.
static HGLOBAL CreateShellIDList(
	_In_ PIDLIST_ABSOLUTE pidlParent,
	_In_ PCUITEMID_CHILD  pidl
) {
	// Get the combined size of the parent folder's PIDL and the other PIDL
	UINT cbpidl = ILGetSize(pidlParent) + ILGetSize(pidl);

	// Find the end of the CIDA structure. This is the size of the
	// CIDA structure itself (which includes one element of aoffset) plus the
	// additional number of elements in aoffset.
	UINT uCurPos = sizeof(CIDA) + sizeof(UINT);

	// Allocate the memory for the CIDA structure and it's variable length
	// members.
	const HGLOBAL hGlobal = GlobalAlloc(
		GPTR | GMEM_SHARE,
		(size_t) (uCurPos +	// size of the CIDA structure and the additional
							// aoffset elements
				 (cbpidl + 1))
	);	// size of the pidls
	if (hGlobal == NULL) return NULL;

	const LPIDA pData = static_cast<LPIDA>(GlobalLock(hGlobal));
	if (pData == NULL) return hGlobal;
	defer({ GlobalUnlock(hGlobal); });

	pData->cidl = 1;
	pData->aoffset[0] = uCurPos;

	// add the PIDL for the parent folder
	cbpidl = ILGetSize(pidlParent);
	CopyMemory(VOID_OFFSET(pData, uCurPos), pidlParent, cbpidl);
	uCurPos += cbpidl;

	// get the size of the PIDL
	cbpidl = ILGetSize(pidl);

	// fill out the members of the CIDA structure.
	pData->aoffset[1] = uCurPos;

	// copy the contents of the PIDL
	CopyMemory(VOID_OFFSET(pData, uCurPos), pidl, cbpidl);

	// set up the position of the next PIDL
	uCurPos += cbpidl;

	return hGlobal;
}


#pragma region CDataObject

CDataObject::~CDataObject() {
	if (m_pidl != NULL) CoTaskMemFree(m_pidl);
	if (m_pidlParent != NULL) CoTaskMemFree(m_pidlParent);
}

void CDataObject::Init(
	IUnknown *pUnkOwner,
	PCIDLIST_ABSOLUTE pidlParent,
	PCUITEMID_CHILD pidl
) {
	m_UnkOwnerPtr = pUnkOwner;
	m_pidlParent = ILCloneFull(pidlParent);
	m_pidl = ILCloneChild(pidl);
	m_cfShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

#pragma endregion

#pragma region IDataObject

STDMETHODIMP CDataObject::GetData(
	_In_  LPFORMATETC pFE,
	_Out_ LPSTGMEDIUM pStgMedium
) {
	LOG(P_DO << L"GetData()");
	if (pFE->cfFormat != m_cfShellIDList) return WrapReturn(E_INVALIDARG);

	pStgMedium->hGlobal = CreateShellIDList(m_pidlParent, m_pidl);
	if (pStgMedium->hGlobal == NULL) return WrapReturn(E_OUTOFMEMORY);

	pStgMedium->tymed = TYMED_HGLOBAL;
	// Even if our tymed is HGLOBAL, WinXP calls ReleaseStgMedium()
	// which tries to call pUnkForRelease->Release() -- BANG!
	pStgMedium->pUnkForRelease = NULL;
	return WrapReturn(S_OK);
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM) {
	LOG(P_DO << L"GetDataHere()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC) {
	LOG(P_DO << L"QueryGetData()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC) {
	LOG(P_DO << L"GetCanonicalFormatEtc()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL) {
	LOG(P_DO << L"SetData()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, IEnumFORMATETC**) {
	LOG(P_DO << L"EnumFormatEtc()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC, DWORD, IAdviseSink*, LPDWORD) {
	LOG(P_DO << L"DAdvise()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::DUnadvise(DWORD) {
	LOG(P_DO << L"DUnadvise()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA**) {
	LOG(P_DO << L"EnumDAdvise()");
	return WrapReturnFailOK(E_NOTIMPL);
}

#pragma endregion

#pragma region IEnumFORMATETC

STDMETHODIMP CDataObject::Next(ULONG, LPFORMATETC, ULONG*) {
	LOG(P_DO << L"Next()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Skip(ULONG) {
	LOG(P_DO << L"Skip()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Reset() {
	LOG(P_DO << L"Reset()");
	return WrapReturnFailOK(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC*) {
	LOG(P_DO << L"Clone()");
	return WrapReturnFailOK(E_NOTIMPL);
}

#pragma endregion

}  // namespace ADSX
