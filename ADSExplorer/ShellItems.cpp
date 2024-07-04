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

#include "ShellItems.h"
#include <combaseapi.h>

//==============================================================================
// Helper for STRRET

bool SetReturnStringA(LPCSTR Source, STRRET &str) {
	SIZE_T StringLen = strlen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
	if (!str.pOleStr) {
		return false;
	}

	mbstowcs_s(NULL, str.pOleStr, StringLen, Source, StringLen);
	return true;
}

bool SetReturnStringW(LPCWSTR Source, STRRET &str) {
	SIZE_T StringLen = wcslen(Source) + 1;
	str.uType = STRRET_WSTR;
	str.pOleStr = (LPOLESTR) CoTaskMemAlloc(StringLen * sizeof(OLECHAR));
	if (!str.pOleStr) {
		return false;
	}

	wcsncpy_s(str.pOleStr, StringLen, Source, StringLen);
	return true;
}

ULONG COWItem::GetSize() {
	return sizeof(COWItem);
}

// @pre: pTarget is a buffer of at least GetSize() bytes
void COWItem::CopyTo(void *pTarget) {
	*((DWORD *) pTarget) = MAGIC;
	COWItem *pNewItem = (COWItem *) pTarget;
	new (&pNewItem->m_Path) _bstr_t();
	new (&pNewItem->m_Name) _bstr_t();
	pNewItem->m_Path = m_Path;
	pNewItem->m_Name = m_Name;
}

//-------------------------------------------------------------------------------

void COWItem::SetPath(LPCWSTR Path) {
	m_Path = Path;
}

void COWItem::SetName(LPCWSTR Name) {
	m_Name = Name;
}

//-------------------------------------------------------------------------------

bool COWItem::IsOwn(LPCITEMIDLIST pidl) {
	return
		pidl != NULL &&
		pidl->mkid.cb == sizeof(COWItem) + sizeof(ITEMIDLIST) &&
		// pidl->mkid.cb >= sizeof(COWItem) &&
		*((DWORD *) pidl->mkid.abID) == MAGIC;
}

LPOLESTR COWItem::GetPath(LPCITEMIDLIST pidl) {
	auto that = (COWItem *) &pidl->mkid.abID;
	return that->m_Path.GetBSTR();
}

LPOLESTR COWItem::GetName(LPCITEMIDLIST pidl) {
	auto that = (COWItem *) &pidl->mkid.abID;
	return that->m_Name.GetBSTR();
}


//========================================================================================
// CDataObject

// helper function that creates a CFSTR_SHELLIDLIST format from given pidls.
HGLOBAL CreateShellIDList(
	LPCITEMIDLIST pidlParent,
	LPCITEMIDLIST *aPidls,
	UINT uItemCount
) {
	HGLOBAL hGlobal = NULL;
	LPIDA pData;
	UINT iCurPos;
	UINT cbPidl;
	UINT i;

	// get the size of the parent folder's PIDL
	cbPidl = PidlMgr::GetSize(pidlParent);

	// get the total size of all of the PIDLs
	for (i = 0; i < uItemCount; i++) {
		cbPidl += PidlMgr::GetSize(aPidls[i]);
	}

	// Find the end of the CIDA structure. This is the size of the
	// CIDA structure itself (which includes one element of aoffset) plus the
	// additional number of elements in aoffset.
	iCurPos = sizeof(CIDA) + (uItemCount * sizeof(UINT));

	// Allocate the memory for the CIDA structure and it's variable length
	// members.
	hGlobal = GlobalAlloc(
		GPTR | GMEM_SHARE,
		(DWORD) (iCurPos +	// size of the CIDA structure and the additional
							// aoffset elements
				 (cbPidl + 1))
	);	// size of the pidls
	if (!hGlobal) {
		return NULL;
	}

	pData = (LPIDA) GlobalLock(hGlobal);
	if (pData != NULL) {
		pData->cidl = uItemCount;
		pData->aoffset[0] = iCurPos;

		// add the PIDL for the parent folder
		cbPidl = PidlMgr::GetSize(pidlParent);
		CopyMemory((LPBYTE) (pData) + iCurPos, (LPBYTE) pidlParent, cbPidl);
		iCurPos += cbPidl;

		for (i = 0; i < uItemCount; i++) {
			// get the size of the PIDL
			cbPidl = PidlMgr::GetSize(aPidls[i]);

			// fill out the members of the CIDA structure.
			pData->aoffset[i + 1] = iCurPos;

			// copy the contents of the PIDL
			CopyMemory((LPBYTE) (pData) + iCurPos, (LPBYTE) aPidls[i], cbPidl);

			// set up the position of the next PIDL
			iCurPos += cbPidl;
		}

		GlobalUnlock(hGlobal);
	}

	return hGlobal;
}

CDataObject::CDataObject() : m_pidl(NULL), m_pidlParent(NULL) {
	m_cfShellIDList = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
}

CDataObject::~CDataObject() {
	PidlMgr::Delete(m_pidl);
	PidlMgr::Delete(m_pidlParent);
}

void CDataObject::Init(IUnknown *pUnkOwner) { m_UnkOwnerPtr = pUnkOwner; }

void CDataObject::SetPidl(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl) {
	m_pidlParent = PidlMgr::Copy(pidlParent);
	m_pidl = PidlMgr::Copy(pidl);
}

//-------------------------------------------------------------------------------

STDMETHODIMP CDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium) {
	AtlTrace("CDataObject::GetData()\n");

	if (pFE->cfFormat == m_cfShellIDList) {
		pStgMedium->hGlobal =
			CreateShellIDList(m_pidlParent, (LPCITEMIDLIST *) &m_pidl, 1);

		if (pStgMedium->hGlobal) {
			pStgMedium->tymed = TYMED_HGLOBAL;
			pStgMedium->pUnkForRelease =
				NULL;  // Even if our tymed is HGLOBAL, WinXP calls
					   // ReleaseStgMedium() which tries to call
					   // pUnkForRelease->Release() : BANG!
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM) {
	AtlTrace("CDataObject::GetDataHere()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC) {
	AtlTrace("CDataObject::QueryGetData()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC) {
	AtlTrace("CDataObject::GetCanonicalFormatEtc()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::SetData(LPFORMATETC, LPSTGMEDIUM, BOOL) {
	AtlTrace("CDataObject::SetData()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, IEnumFORMATETC **) {
	AtlTrace("CDataObject::EnumFormatEtc()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DAdvise(LPFORMATETC, DWORD, IAdviseSink *, LPDWORD) {
	AtlTrace("CDataObject::DAdvise()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection) {
	AtlTrace("CDataObject::DUnadvise()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise) {
	AtlTrace("CDataObject::EnumDAdvise()\n");
	return E_NOTIMPL;
}

//-------------------------------------------------------------------------------

STDMETHODIMP CDataObject::Next(ULONG, LPFORMATETC, ULONG *) {
	AtlTrace("CDataObject::Next()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Skip(ULONG) {
	AtlTrace("CDataObject::Skip()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Reset() {
	AtlTrace("CDataObject::Reset()\n");
	return E_NOTIMPL;
}

STDMETHODIMP CDataObject::Clone(LPENUMFORMATETC *) {
	AtlTrace("CDataObject::Clone()\n");
	return E_NOTIMPL;
}
