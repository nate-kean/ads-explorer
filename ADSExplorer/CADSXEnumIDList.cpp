#include "stdafx.h"	 // MUST be included first

#include "CADSXEnumIDList.h"

#include <functional>
#include <handleapi.h>
#include <winnt.h>

#include "CADSXItem.h"
#include "PidlMgr.h"


CADSXEnumIDList::CADSXEnumIDList()
	: m_hFinder(NULL),
	  m_nTotalFetched(0) {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::CADSXEnumIDList()\n"), this);
}

CADSXEnumIDList::~CADSXEnumIDList() {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::~CADSXEnumIDList()\n"), this);
	if (m_hFinder != NULL) {
		FindClose(m_hFinder);
	}
}

void CADSXEnumIDList::Init(IUnknown *pUnkOwner, const BSTR pszPath) {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::Init()\n"), this);
	m_pUnkOwner = pUnkOwner;  // the macguffin
	m_pszPath = SysAllocString(pszPath);
}


// Find one or more items with NextInternal and push them to the
// output array rgelt with PushPidl.
HRESULT CADSXEnumIDList::Next(
	/* [in]  */ ULONG celt,
	/* [out] */ PITEMID_CHILD *rgelt,
	/* [out] */ ULONG *pceltFetched
) {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::Next(celt=%lu)\n"), this, celt);
	CADSXEnumIDList::FnConsume fnCbPushPidl = std::bind(
		&CADSXEnumIDList::PushPidl,
		this,
		std::placeholders::_1, std::placeholders::_2
	);
	return NextInternal(fnCbPushPidl, celt, rgelt, pceltFetched);
}


HRESULT CADSXEnumIDList::NextInternal(
	/* [in]  */ FnConsume fnConsume,   // callback on item found
	/* [in]  */ ULONG celt,            // number of pidls requested
	/* [out] */ PITEMID_CHILD *rgelt,  // array of pidls
	/* [out] */ ULONG *pceltFetched    // actual number of pidls fetched
) {
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL)) return E_POINTER;

	ULONG nActual = 0;
	PITEMID_CHILD *pelt = rgelt;
	bool bPushPidlSuccess;

	// Initialize the finder if it hasn't been already with a call to
	// FindFirstStream instead of FindNextStream.
	// Call the callback on this first item.
	if (m_hFinder == NULL) {
		m_hFinder = FindFirstStreamW(m_pszPath, FindStreamInfoStandard, &m_fsd, 0);
		SysFreeString(m_pszPath);  // it has served its purpose
		if (m_hFinder == INVALID_HANDLE_VALUE) {
			m_hFinder = NULL;
			if (GetLastError() == ERROR_SUCCESS) {
				AtlTrace(_T("FindFirstStreamW returned INVALID_HANDLE_VALUE but GetLastError() == ERROR_SUCCESS\n"));
				return E_FAIL;
			}
			return HRESULT_FROM_WIN32(GetLastError());
		}
		if (GetLastError() == ERROR_HANDLE_EOF) return S_FALSE;
		bPushPidlSuccess = fnConsume(pelt, &nActual);
		if (!bPushPidlSuccess) return HRESULT_FROM_WIN32(GetLastError());
	}

	// The main loop body that the rest of the calls to Next will skip to.
	// Each loop calls the callback on another stream.
	bool bFindStreamDone = false;
	while (!bFindStreamDone && nActual < celt) {
		bFindStreamDone = !FindNextStreamW(m_hFinder, &m_fsd);
		if (bFindStreamDone && GetLastError() != ERROR_HANDLE_EOF) {
			return HRESULT_FROM_WIN32(GetLastError());
		}
		bPushPidlSuccess = fnConsume(pelt, &nActual);
		if (!bPushPidlSuccess) return HRESULT_FROM_WIN32(GetLastError());
	}
	if (pceltFetched != NULL) {  // Bookkeeping
		*pceltFetched = nActual;
	}
	m_nTotalFetched += nActual;
	return nActual == celt ? S_OK : S_FALSE;
}


HRESULT CADSXEnumIDList::Reset() {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::Reset()\n"), this);
	if (m_hFinder != NULL) {
		BOOL success = FindClose(m_hFinder);
		m_hFinder = NULL;
		m_nTotalFetched = 0;
		if (!success) return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

// Find one or more items with NextInternal and discard them.
bool NoOp(PITEMID_CHILD *pelt, ULONG *nActual) {
	UNREFERENCED_PARAMETER(pelt);
	UNREFERENCED_PARAMETER(nActual);
	return true;
}
HRESULT CADSXEnumIDList::Skip(/* [in] */ ULONG celt) {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::Skip(celt=%lu)\n"), this, celt);
	ULONG pceltFetchedFake = 0;
	PITEMID_CHILD rgeltFake[1];

	CADSXEnumIDList::FnConsume fnCbNoOp = std::bind(
		&NoOp,
		std::placeholders::_1, std::placeholders::_2
	);
	return NextInternal(fnCbNoOp, celt, rgeltFake, &pceltFetchedFake);
}


HRESULT CADSXEnumIDList::Clone(/* [out] */ IEnumIDList **ppEnum) {
	AtlTrace(_T("CADSXEnumIDList(0x%08x)::Clone()\n"), this);
	if (ppEnum == NULL) return E_POINTER;

	CComObject<CADSXEnumIDList> *pNewEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pNewEnum);
	if (FAILED(hr)) return hr;

	pNewEnum->Init(m_pUnkOwner, m_pszPath);
	
	// Unforuntately I don't see any more an efficient way to do this with
	// the Find Stream API :(
	pNewEnum->Skip(m_nTotalFetched);

	hr = pNewEnum->QueryInterface(IID_IEnumIDList, (void **) ppEnum);
	if (FAILED(hr)) return hr;
	return S_OK;
}


// Convert a WIN32_FIND_STREAM_DATA to a PIDL and add it to the output array
bool CADSXEnumIDList::PushPidl(
	/* [in/out] */ PITEMID_CHILD *pelt,  // destination array cursor
	/* [out]    */ ULONG *nActual
) {
	// Reusable item
	static CADSXItem Item;

	// Fill in the item
	Item.m_Filesize = m_fsd.StreamSize.QuadPart;
	Item.m_Name = m_fsd.cStreamName;

	// Copy this item into a PIDL and put that into the output array
	PITEMID_CHILD pidl = PidlMgr::Create(Item);
	if (pidl == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return false;
	}
	*pelt = pidl;

	// Advance the enumerator
	++*pelt;
	++*nActual;
	return true;
}
