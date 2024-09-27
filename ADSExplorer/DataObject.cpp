/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

// vscode says it doesn't need this but it fails to compile without it
#include "stdafx.h"

#include "DataObject.h"

#include <combaseapi.h>

#include "defer.h"
#include "debug.h"

// Debug log prefix for CDataObject
#define P_DO L"CDataObject(0x" << std::hex << this << L")::"


//========================================================================================
// CDataObject

// helper function that creates a CFSTR_SHELLIDLIST format from given pidls.
static HGLOBAL
CreateShellIDList(
	PIDLIST_ABSOLUTE pidlParent,
	PCUITEMID_CHILD pidl
) {
	// Get the combined size of the parent folder's PIDL and the other PIDL
	UINT cbPidl = ILGetSize(pidlParent) + ILGetSize(pidl);

	// Find the end of the CIDA structure. This is the size of the
	// CIDA structure itself (which includes one element of aoffset) plus the
	// additional number of elements in aoffset.
	UINT iCurPos = sizeof(CIDA) + (sizeof(UINT));

	// Allocate the memory for the CIDA structure and it's variable length
	// members.
	HGLOBAL hGlobal = GlobalAlloc(
		GPTR | GMEM_SHARE,
		(size_t) (iCurPos +	// size of the CIDA structure and the additional
							// aoffset elements
				 (cbPidl + 1))
	);	// size of the pidls
	if (hGlobal == NULL) return NULL;

	LPIDA pData = static_cast<LPIDA>(GlobalLock(hGlobal));
	if (pData == NULL) return hGlobal;
	defer({ GlobalUnlock(hGlobal); });

	pData->cidl = 1;
	pData->aoffset[0] = iCurPos;

	// add the PIDL for the parent folder
	cbPidl = ILGetSize(pidlParent);
	CopyMemory(VOID_OFFSET(pData, iCurPos), pidlParent, cbPidl);
	iCurPos += cbPidl;

	// get the size of the PIDL
	cbPidl = ILGetSize(pidl);

	// fill out the members of the CIDA structure.
	pData->aoffset[1] = iCurPos;

	// copy the contents of the PIDL
	CopyMemory(VOID_OFFSET(pData, iCurPos), pidl, cbPidl);

	// set up the position of the next PIDL
	iCurPos += cbPidl;

	return hGlobal;
}

CDataObject::~CDataObject() {
	if (m_pidl != NULL) CoTaskMemFree(m_pidl);
	if (m_pidlParent != NULL) CoTaskMemFree(m_pidlParent);
}

void CDataObject::Init(IUnknown *pUnkOwner, PCIDLIST_ABSOLUTE pidlParent, PCUITEMID_CHILD pidl) {
	m_UnkOwnerPtr = pUnkOwner;
	m_pidlParent = ILCloneFull(pidlParent);
	m_pidl = ILCloneChild(pidl);
	m_cfShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

//-------------------------------------------------------------------------------

STDMETHODIMP CDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium) {
	LOG(P_DO << L"GetData()");
	if (pFE->cfFormat != m_cfShellIDList) return LogReturn(E_INVALIDARG);

	pStgMedium->hGlobal = CreateShellIDList(m_pidlParent, m_pidl);
	if (pStgMedium->hGlobal == NULL) return LogReturn(E_OUTOFMEMORY);

	pStgMedium->tymed = TYMED_HGLOBAL;
	// Even if our tymed is HGLOBAL, WinXP calls ReleaseStgMedium()
	// which tries to call pUnkForRelease->Release() -- BANG!
	pStgMedium->pUnkForRelease = NULL;
	return LogReturn(S_OK);
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM) {
	LOG(P_DO << L"GetDataHere()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC) {
	LOG(P_DO << L"QueryGetData()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC) {
	LOG(P_DO << L"GetCanonicalFormatEtc()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL) {
	LOG(P_DO << L"SetData()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, IEnumFORMATETC **) {
	LOG(P_DO << L"EnumFormatEtc()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC, DWORD, IAdviseSink *, LPDWORD) {
	LOG(P_DO << L"DAdvise()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection) {
	LOG(P_DO << L"DUnadvise()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise) {
	LOG(P_DO << L"EnumDAdvise()");
	return LogReturn(E_NOTIMPL);
}

//-------------------------------------------------------------------------------

STDMETHODIMP CDataObject::Next(ULONG, LPFORMATETC, ULONG *) {
	LOG(P_DO << L"Next()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Skip(ULONG) {
	LOG(P_DO << L"Skip()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Reset() {
	LOG(P_DO << L"Reset()");
	return LogReturn(E_NOTIMPL);
}

STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC *) {
	LOG(P_DO << L"Clone()");
	return LogReturn(E_NOTIMPL);
}
