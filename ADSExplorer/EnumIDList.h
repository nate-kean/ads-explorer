/**
 * 2024 Nate Kean
 *
 * A Windows Enumerator object that produces LPITEMIDLISTs that contain
 * ADSX::CItem structures.
 */

#pragma once

#include "StdAfx.h"

#include <functional>

namespace ADSX {


class ATL_NO_VTABLE CEnumIDList
	: public IEnumIDList,
	  public CComObjectRootEx<CComSingleThreadModel> {

  public:
	BEGIN_COM_MAP(CEnumIDList)
		COM_INTERFACE_ENTRY(IEnumIDList)
	END_COM_MAP()

	CEnumIDList();
	virtual ~CEnumIDList();
	
	/**
	 * Language-agnostic constructor apart from C++'s CEnumIDList() to play nice
	 * with COM because it's language-agnostic.
	 * @post: pszPath is copied and ownership remains with caller.
	 */
	HRESULT Init(_In_ IUnknown *pUnkOwner, _In_ PCWSTR pszPath);

	// -------------------------------------------------------------------------
	// IEnumIDList
	STDMETHOD(Next)(
		_In_     ULONG,
		_Outptr_ PITEMID_CHILD*,
		_Out_    ULONG*
	);
	STDMETHOD(Skip)(
		_In_ ULONG
	);
	STDMETHOD(Reset)(
		void
	);
	STDMETHOD(Clone)(
		_COM_Outptr_ IEnumIDList**
	);

  protected:
	using FnConsume = std::function<
		bool (
			_In_     const WIN32_FIND_STREAM_DATA &fsd,
			_Outptr_ PITEMID_CHILD                **ppelt,
			_Out_    ULONG                        *nActual
		)
	>;
	HRESULT NextInternal(
		_In_     FnConsume     fnConsume,
		_In_     ULONG         celt,
		_Outptr_ PITEMID_CHILD *rgelt,
		_Out_    ULONG         *pceltFetched
	);

	// A sentinel COM object to represent the lifetime of the owner object.
	// This exists to prevent the owner object from being freed before this one.
	CComPtr<IUnknown> m_punkOwner;

	PWSTR m_pszPath;  // path on which to find streams
	HANDLE m_hFinder;
	ULONG m_nTotalFetched;  // just to bring a clone up to speed
};

}  // namespace ADSX
