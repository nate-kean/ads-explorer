/**
 * 2024 Nate Kean
 *
 * A Windows Enumerator object that produces LPITEMIDLISTs that contain
 * CADSXItem structures.
 */

#pragma once

#include "StdAfx.h"

#include <functional>


class ATL_NO_VTABLE CADSXEnumIDList
	: public IEnumIDList,
	//   public CComObjectRootEx<CComObjectThreadModel> {
	  public CComObjectRootEx<CComSingleThreadModel> {

  public:
	BEGIN_COM_MAP(CADSXEnumIDList)
		COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
	END_COM_MAP()

	CADSXEnumIDList();
	virtual ~CADSXEnumIDList();
	
	/**
	 * Initialization logic in a separate method because COM object constructors
	 * are called in a weird way that makes it so that AFAIK they can't have
	 * parameters
	 * @post: pszPath is cloned and ownership remains with caller
	 */
	HRESULT Init(_In_ IUnknown *pUnkOwner, _In_ LPCWSTR pszPath);

	// IEnumIDList
	STDMETHOD(Next)(_In_ ULONG, _Outptr_ PITEMID_CHILD*, _Out_ ULONG*);
	STDMETHOD(Skip)(_In_ ULONG);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(_COM_Outptr_ IEnumIDList**);

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

	LPWSTR m_pszPath;  // path on which to find streams
	HANDLE m_hFinder;
	ULONG m_nTotalFetched;  // just to bring a clone up to speed
};
