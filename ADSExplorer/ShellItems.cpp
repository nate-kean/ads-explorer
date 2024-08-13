/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
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

// ShellItems.cpp's security blanket. vscode says it doesn't need it but it
// fails to compile without it
#include "stdafx.h"

#include <combaseapi.h>

#include "defer.h"
#include "ShellItems.h"
#include "DebugPrint.h"

// Debug log prefix for CDataObject
#define P_DO L"CDataObject(0x" << std::hex << this << L")::"

//==============================================================================
// Helper for STRRET

bool SetReturnStringA(LPCSTR Source, STRRET &str) {
	SIZE_T StringLen = strlen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
	if (str.pOleStr == NULL) return false;

	mbstowcs_s(NULL, str.pOleStr, StringLen, Source, StringLen);
	return true;
}

bool SetReturnStringW(LPCWSTR Source, STRRET &str) {
	SIZE_T StringLen = wcslen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
	if (str.pOleStr == NULL) return false;

	wcsncpy_s(str.pOleStr, StringLen, Source, StringLen);
	return true;
}


//========================================================================================
// CDataObject

// helper function that creates a CFSTR_SHELLIDLIST format from given pidls.
HGLOBAL CreateShellIDList(
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

	LPIDA pData = (LPIDA) GlobalLock(hGlobal);
	if (pData == NULL) return hGlobal;
	defer({ GlobalUnlock(hGlobal); });

	pData->cidl = 1;
	pData->aoffset[0] = iCurPos;

	// add the PIDL for the parent folder
	cbPidl = ILGetSize(pidlParent);
	CopyMemory(LPBYTE(pData) + iCurPos, (LPBYTE) pidlParent, cbPidl);
	iCurPos += cbPidl;

	// get the size of the PIDL
	cbPidl = ILGetSize(pidl);

	// fill out the members of the CIDA structure.
	pData->aoffset[1] = iCurPos;

	// copy the contents of the PIDL
	CopyMemory((LPBYTE) (pData) + iCurPos, (LPBYTE) pidl, cbPidl);

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
	if (pFE->cfFormat != m_cfShellIDList) return E_INVALIDARG;

	pStgMedium->hGlobal = CreateShellIDList(m_pidlParent, m_pidl);
	if (pStgMedium->hGlobal == NULL) return E_OUTOFMEMORY;

	pStgMedium->tymed = TYMED_HGLOBAL;
	// Even if our tymed is HGLOBAL, WinXP calls ReleaseStgMedium()
	// which tries to call pUnkForRelease->Release() -- BANG!
	pStgMedium->pUnkForRelease = NULL;
	return S_OK;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM) {
	LOG(P_DO << L"GetDataHere()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC) {
	LOG(P_DO << L"QueryGetData()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC) {
	LOG(P_DO << L"GetCanonicalFormatEtc()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL) {
	LOG(P_DO << L"SetData()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, IEnumFORMATETC **) {
	LOG(P_DO << L"EnumFormatEtc()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC, DWORD, IAdviseSink *, LPDWORD) {
	LOG(P_DO << L"DAdvise()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection) {
	LOG(P_DO << L"DUnadvise()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise) {
	LOG(P_DO << L"EnumDAdvise()");
	return E_NOTIMPL;
}

//-------------------------------------------------------------------------------

STDMETHODIMP CDataObject::Next(ULONG, LPFORMATETC, ULONG *) {
	LOG(P_DO << L"Next()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Skip(ULONG) {
	LOG(P_DO << L"Skip()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Reset() {
	LOG(P_DO << L"Reset()");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC *) {
	LOG(P_DO << L"Clone()");
	return E_NOTIMPL;
}
