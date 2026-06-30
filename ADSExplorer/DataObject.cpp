/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

#include "pch.h"  // Precompiled header; include first

#include "DataObject.h"

// Debug log prefix for CDataObject
#define P_DO L"CDataObject(0x" << std::hex << this << L")::"

namespace ADSX {

// Helper function that creates a CFSTR_SHELLIDLIST format from given PIDLs.
static HGLOBAL CreateShellIDList(
	_In_ PIDLIST_ABSOLUTE pidlaParent,
	_In_ PCUITEMID_CHILD  pidlc
) {
	// Get the combined size of the parent folder's PIDL and the other PIDL.
	UINT cbpidl = ILGetSize(pidlaParent) + ILGetSize(pidlc);

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
	);	// Size of the PIDLs
	if (hGlobal == NULL) return NULL;

	const LPIDA pData = static_cast<LPIDA>(GlobalLock(hGlobal));
	if (pData == NULL) return hGlobal;
	defer({ GlobalUnlock(hGlobal); });

	pData->cidl = 1;
	pData->aoffset[0] = uCurPos;

	// Add the PIDL for the parent folder.
	cbpidl = ILGetSize(pidlaParent);
	CopyMemory(VOID_OFFSET(pData, uCurPos), pidlaParent, cbpidl);
	uCurPos += cbpidl;

	// Get the size of the PIDL.
	cbpidl = ILGetSize(pidlc);

	// Fill out the members of the CIDA structure.
	pData->aoffset[1] = uCurPos;

	// Copy the contents of the PIDL.
	CopyMemory(VOID_OFFSET(pData, uCurPos), pidlc, cbpidl);

	// Set up the position of the next PIDL.
	uCurPos += cbpidl;

	return hGlobal;
}


#pragma region CDataObject

CDataObject::~CDataObject() {
	if (m_pidlc != NULL) CoTaskMemFree(m_pidlc);
	if (m_pidlaParent != NULL) CoTaskMemFree(m_pidlaParent);
}

void CDataObject::Init(
	IUnknown *pUnkOwner,
	PCIDLIST_ABSOLUTE pidlaParent,
	PCUITEMID_CHILD pidlc
) {
	m_UnkOwnerPtr = pUnkOwner;
	m_pidlaParent = ILCloneFull(pidlaParent);
	m_pidlc = ILCloneChild(pidlc);
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

	pStgMedium->hGlobal = CreateShellIDList(m_pidlaParent, m_pidlc);
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
