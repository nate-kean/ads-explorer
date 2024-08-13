#include "stdafx.h"  // MUST be included first

#include "CADSXEnumIDList.h"

#include <handleapi.h>
#include <winnt.h>

#include "CADSXItem.h"
#include "DebugPrint.h"

// Debug log prefix for CADSXEnumIDList
#define P_EIDL L"CADSXEnumIDList(0x" << std::hex << this << L")::"


// Convert a WIN32_FIND_STREAM_DATA to a PIDL and add it to the output array
// pushin p
static bool PushPidl(
	/* [in]     */ WIN32_FIND_STREAM_DATA *fsd,
	// POINTER! to the destination array cursor because we're going to
	// modify it (advance it).
	// Fun Fact: This is a pointer to an array of pointers to ITEMID_CHILDren.
	// A real triple pointer. How awful is that? :)
	/* [in/out] */ PITEMID_CHILD **ppelt,
	/* [out]    */ ULONG *nActual
) {
	// Reusable item
	static CADSXItem Item;

	// Fill in the item
	Item.m_Filesize = fsd->StreamSize.QuadPart;
	Item.m_Name = fsd->cStreamName;

	// Copy this item into a PIDL
	PITEMID_CHILD pidl = Item.ToPidl();
	if (pidl == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return false;
	}
	**ppelt = pidl;

	// Advance the enumerator
	*ppelt += sizeof(PITEMID_CHILD);
	++*nActual;
	return true;
}

// Find one or more items with NextInternal and discard them.
static bool NoOp(WIN32_FIND_STREAM_DATA *fsd, PITEMID_CHILD **ppelt, ULONG *nActual) {
	UNREFERENCED_PARAMETER(fsd);
	UNREFERENCED_PARAMETER(ppelt);
	UNREFERENCED_PARAMETER(nActual);
	return true;
}


CADSXEnumIDList::CADSXEnumIDList()
	: m_hFinder(NULL),
	  m_nTotalFetched(0) {
	LOG(P_EIDL << L"CADSXEnumIDList()");
}

CADSXEnumIDList::~CADSXEnumIDList() {
	LOG(P_EIDL << L"~CADSXEnumIDList()");
	if (m_hFinder != NULL) {
		FindClose(m_hFinder);
	}
	SysFreeString(m_pszPath);
}

void CADSXEnumIDList::Init(IUnknown *pUnkOwner, const BSTR pszPath) {
	LOG(P_EIDL << L"Init()");
	m_pUnkOwner = pUnkOwner;
	m_pszPath = pszPath;
}


// Find one or more items with NextInternal and push them to the
// output array rgelt with PushPidl.
HRESULT CADSXEnumIDList::Next(
	/* [in]  */ ULONG celt,
	/* [out] */ PITEMID_CHILD *rgelt,
	/* [out] */ ULONG *pceltFetched
) {
	LOG(P_EIDL << L"Next(celt=" << celt << L")");
	return NextInternal(&PushPidl, celt, rgelt, pceltFetched);
}


HRESULT CADSXEnumIDList::NextInternal(
	/* [in]  */ FnConsume fnConsume,   // callback on item found
	/* [in]  */ ULONG celt,            // number of pidls requested
	/* [out] */ PITEMID_CHILD *rgelt,  // array of pidls
	/* [out] */ ULONG *pceltFetched    // actual number of pidls fetched
) {
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL)) {
		LOG(L"** Bad argument(s)");
		return E_POINTER;
	}
	if (celt == 0) {
		LOG(L"** 0 requested :/ vacuous success");
		return S_OK;
	}

	static WIN32_FIND_STREAM_DATA fsd;
	ULONG nActual = 0;
	PITEMID_CHILD *pelt = rgelt;
	bool bPushPidlSuccess;

	// Initialize the finder if it hasn't been already with a call to
	// FindFirstStream instead of FindNextStream.
	// Call the callback on this first item.
	// Hopes and Streams
	if (m_hFinder == NULL) {
		m_hFinder = FindFirstStreamW(m_pszPath, FindStreamInfoStandard, &fsd, 0);
		if (m_hFinder == INVALID_HANDLE_VALUE) {
			m_hFinder = NULL;
			switch (GetLastError()) {
				case ERROR_SUCCESS:
					LOG(
						L"** FindFirstStreamW returned INVALID_HANDLE_VALUE "
						L"but GetLastError() == ERROR_SUCCESS"
					);
					return E_FAIL;
				case ERROR_HANDLE_EOF:
					LOG(L"** No streams found");
					return S_FALSE;
				default:
					LOG(L"** latery Eror! Er Erry Err! later");
					return HRESULT_FROM_WIN32(GetLastError());
			}
		}
		if (GetLastError() == ERROR_HANDLE_EOF) {
			LOG(L"** No streams found");
			return S_FALSE;
		}
		bPushPidlSuccess = fnConsume(&fsd, &pelt, &nActual);
		if (!bPushPidlSuccess) {
			LOG(L"** latery Eror! Er Erry Err! later");
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	// The main loop body that the rest of the calls to Next will skip to.
	// Each loop calls the callback on another stream.
	bool bFindStreamStopped = false;
	while (!bFindStreamStopped && nActual < celt) {
		bFindStreamStopped = !FindNextStreamW(m_hFinder, &fsd);
		if (bFindStreamStopped) {
			if (GetLastError() == ERROR_HANDLE_EOF) {
				// Do nothing and let the loop end
			} else {
				// Stream has stopped unexpectedly
				LOG(L"** latery Eror! Er Erry Err! later");
				return HRESULT_FROM_WIN32(GetLastError());
			}
		} else {
			// Consume stream
			bPushPidlSuccess = fnConsume(&fsd, &pelt, &nActual);
			if (!bPushPidlSuccess) {
				LOG(L"** latery Eror! Er Erry Err! later");
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	if (pceltFetched != NULL) {  // Bookkeeping
		*pceltFetched = nActual;
	}
	m_nTotalFetched += nActual;
	if (nActual < celt) {
		LOG(L"** Ran out");
		return S_FALSE;
	}
	return S_OK;
}


HRESULT CADSXEnumIDList::Reset() {
	LOG(P_EIDL << L"Reset()");
	if (m_hFinder != NULL) {
		BOOL success = FindClose(m_hFinder);
		m_hFinder = NULL;
		m_nTotalFetched = 0;
		if (!success) return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}


HRESULT CADSXEnumIDList::Skip(/* [in] */ ULONG celt) {
	LOG(P_EIDL << L"Skip(celt=" << celt << L")");
	ULONG pceltFetchedFake = 0;
	PITEMID_CHILD *rgeltFake = NULL;
	return NextInternal(&NoOp, celt, rgeltFake, &pceltFetchedFake);
}


HRESULT CADSXEnumIDList::Clone(/* [out] */ IEnumIDList **ppEnum) {
	LOG(P_EIDL << L"Clone()");
	if (ppEnum == NULL) return E_POINTER;

	CComObject<CADSXEnumIDList> *pNewEnum;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pNewEnum);
	if (FAILED(hr)) return hr;
	pNewEnum->Init(m_pUnkOwner, m_pszPath);
	
	// Unfortunately I don't see any more an efficient way to do this with
	// the Find Stream API :(
	pNewEnum->Skip(m_nTotalFetched);

	hr = pNewEnum->QueryInterface(IID_IEnumIDList, (void **) ppEnum);
	if (FAILED(hr)) return hr;
	return S_OK;
}
