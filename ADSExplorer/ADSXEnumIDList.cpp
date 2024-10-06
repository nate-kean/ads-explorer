/**
 * 2024 Nate Kean
 */

#include "StdAfx.h"  // Precompiled header; include first

#include "ADSXEnumIDList.h"

#include "ADSXItem.h"

// Debug log prefix for CADSXEnumIDList
#define P_EIDL L"CADSXEnumIDList(0x" << std::hex << this << L")::"


/**
 * Convert a WIN32_FIND_STREAM_DATA to a PIDL and add it to the output array.
 * pushin p
 * @post: ppelt array cursor is advanced by one element
 * @post: nActual is incremented
 */
static bool PushPidl(
	_In_    const WIN32_FIND_STREAM_DATA &fsd,
	// POINTER! to the destination array cursor because we're going to
	// modify it (advance it).
	// Fun Fact: This is a pointer to an array of pointers to ITEMID_CHILDren.
	// A real triple pointer. How awful is that? :)
	_Inout_ PITEMID_CHILD                **ppelt,
	_Inout_ ULONG                        *nActual
) {
	// Reusable item
	static CADSXItem Item = { .pszName = NULL };

	std::wstring sName = std::wstring(fsd.cStreamName);
	// All ADSes follow this name pattern AFAIK, but if they don't,
	// 1: we shouldn't modify its name
	// 2: I want to know about it
	ATLASSERT(sName.starts_with(L":") && sName.ends_with(L":$DATA"));
	if (sName.starts_with(L":") && sName.ends_with(L":$DATA")) {
		sName = sName.substr(
			_countof(L":") - 1,
			sName.length() - _countof(L":$DATA")
		);
	}

 	// Skip the main stream. We're too hipster
	if (sName.empty()) return true;

	LOG(
		L" ** Stream: " << sName <<
		L" (" << fsd.StreamSize.QuadPart << L" bytes)"
	);

	// Fill in the item
	Item.llFilesize = fsd.StreamSize.QuadPart;
	if (Item.pszName != NULL) CoTaskMemFree(Item.pszName);
	Item.pszName = static_cast<LPWSTR>(
		CoTaskMemAlloc(sName.length() + sizeof(WCHAR))
	);
	sName.copy(Item.pszName, sName.length());

	// Copy this item into a PIDL
	PITEMID_CHILD pidl = Item.ToPidl();
	if (pidl == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return false;
	}

	// Put that PIDL into the output array
	**ppelt = pidl;

	// Advance the enumerator
	*ppelt += sizeof(PITEMID_CHILD);
	++*nActual;
	return true;
}

/**
 * Find one or more items with NextInternal and discard them.
 */
static bool NoOp(const WIN32_FIND_STREAM_DATA &, PITEMID_CHILD **, ULONG *) {
	return true;
}


CADSXEnumIDList::CADSXEnumIDList()
	: m_hFinder(NULL)
	, m_nTotalFetched(0)
	, m_bPathBeingShared(false) {
	LOG(P_EIDL << L"CADSXEnumIDList()");
}

CADSXEnumIDList::~CADSXEnumIDList() {
	LOG(P_EIDL << L"~CADSXEnumIDList()");
	if (m_hFinder != NULL) {
		FindClose(m_hFinder);
	}
	if (!m_bPathBeingShared) {
		CoTaskMemFree(m_pszPath);
	}
}

void CADSXEnumIDList::Init(_In_ IUnknown *pUnkOwner, _In_ LPWSTR pszPath) {
	LOG(P_EIDL << L"Init()");
	m_pUnkOwner = pUnkOwner;
	m_pszPath = pszPath;
}


/**
 * Find one or more items with NextInternal and push them to the
 * output array rgelt with PushPidl.
 */
HRESULT CADSXEnumIDList::Next(
	_In_ ULONG celt,
	_Outptr_ PITEMID_CHILD *rgelt,
	_Out_ ULONG *pceltFetched
) {
	LOG(P_EIDL << L"Next(celt=" << celt << L")");
	return NextInternal(&PushPidl, celt, rgelt, pceltFetched);
}


HRESULT CADSXEnumIDList::NextInternal(
	_In_     FnConsume     fnConsume,     // callback on item found
	_In_     ULONG         celt,          // number of pidls requested
	_Outptr_ PITEMID_CHILD *rgelt,        // array of pidls
	_Out_    ULONG         *pceltFetched  // actual number of pidls fetched
) {
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL)) {
		LOG(L" ** Bad argument(s)");
		return WrapReturn(E_POINTER);
	}
	if (celt == 0) {
		LOG(L" ** 0 requested :/ vacuous success");
		*pceltFetched = 0;
		return WrapReturn(S_OK);
	}

	static WIN32_FIND_STREAM_DATA fsd;
	ULONG nActual = 0;
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
						L" ** FindFirstStreamW returned INVALID_HANDLE_VALUE "
						L"but GetLastError() == ERROR_SUCCESS"
					);
					return WrapReturn(E_FAIL);
				case ERROR_HANDLE_EOF:
					LOG(L" ** No streams found");
					*pceltFetched = 0;
					return WrapReturn(S_FALSE);
				default:
					LOG(L" ** Error: " << GetLastError());
					return HRESULT_FROM_WIN32(GetLastError());
			}
		}
		if (GetLastError() == ERROR_HANDLE_EOF) {
			LOG(L" ** No streams found");
			*pceltFetched = 0;
			return WrapReturn(S_FALSE);
		}
		bPushPidlSuccess = fnConsume(fsd, &rgelt, &nActual);
		if (!bPushPidlSuccess) {
			LOG(L" ** Error: " << GetLastError());
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
				LOG(L" ** Error: " << GetLastError());
				return HRESULT_FROM_WIN32(GetLastError());
			}
		} else {
			// Consume stream
			bPushPidlSuccess = fnConsume(fsd, &rgelt, &nActual);
			if (!bPushPidlSuccess) {
				LOG(L" ** Error: " << GetLastError());
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	if (pceltFetched != NULL) {  // Bookkeeping
		*pceltFetched = nActual;
	}
	m_nTotalFetched += nActual;
	if (nActual < celt) {
		LOG(L" ** Ran out");
		return WrapReturn(S_FALSE);
	}
	return WrapReturn(S_OK);
}


HRESULT CADSXEnumIDList::Reset() {
	LOG(P_EIDL << L"Reset()");
	if (m_hFinder != NULL) {
		BOOL success = FindClose(m_hFinder);
		m_hFinder = NULL;
		m_nTotalFetched = 0;
		if (!success) return HRESULT_FROM_WIN32(GetLastError());
	}
	return WrapReturn(S_OK);
}


HRESULT CADSXEnumIDList::Skip(_In_ ULONG celt) {
	LOG(P_EIDL << L"Skip(celt=" << celt << L")");
	ULONG pceltFetchedFake = 0;
	PITEMID_CHILD *rgeltFake = NULL;
	return NextInternal(&NoOp, celt, rgeltFake, &pceltFetchedFake);
}


HRESULT CADSXEnumIDList::Clone(_COM_Outptr_ IEnumIDList **ppEnum) {
	LOG(P_EIDL << L"Clone()");
	if (ppEnum == NULL) return WrapReturn(E_POINTER);
	*ppEnum = NULL;

	CComObject<CADSXEnumIDList> *pEnumNew;
	HRESULT hr = CComObject<CADSXEnumIDList>::CreateInstance(&pEnumNew);
	if (FAILED(hr)) return hr;
	pEnumNew->Init(m_pUnkOwner, m_pszPath);
	m_bPathBeingShared = true;

	// Unfortunately I don't see any more an efficient way to do this with
	// the Find Stream API :(
	pEnumNew->Skip(m_nTotalFetched);

	hr = pEnumNew->QueryInterface(IID_PPV_ARGS(ppEnum));
	if (FAILED(hr)) return hr;
	return WrapReturn(S_OK);
}
