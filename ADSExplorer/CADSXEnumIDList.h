#pragma once

#include "stdafx.h"

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
	
	// Initialization logic in a separate method because COM object constructors
	// are called in a weird way that makes it so that AFAIK they can't have
	// parameters
	// @post: this takes ownership of pszPath
	void Init(IUnknown *pUnkOwner, const BSTR pszPath);

	// IEnumIDList
	STDMETHOD(Next)(ULONG, PITEMID_CHILD*, ULONG*);
	STDMETHOD(Skip)(ULONG);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumIDList**);

  protected:
	using FnConsume = std::function<
		bool (WIN32_FIND_STREAM_DATA*, PITEMID_CHILD**, ULONG*)
	>;
	HRESULT NextInternal(
		FnConsume fnConsume,
		ULONG celt,
		PITEMID_CHILD *rgelt,
		ULONG *pceltFetched
	);

	// A sentinel COM object to represent the lifetime of the owner object.
	// This exists to prevent the owner object from being freed before this one.
	CComPtr<IUnknown> m_pUnkOwner;

	BSTR m_pszPath;  // path on which to find streams
	HANDLE m_hFinder;
	ULONG m_nTotalFetched;  // just to bring a clone up to speed
};
