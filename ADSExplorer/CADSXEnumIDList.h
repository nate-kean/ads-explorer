#pragma once

#include "stdafx.h"

#include <functional>


class ATL_NO_VTABLE CADSXEnumIDList
	: public IEnumIDList,
	  public CComObjectRootEx<CComObjectThreadModel> {
	BEGIN_COM_MAP(CADSXEnumIDList)
		COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
	END_COM_MAP()
  public:
	CADSXEnumIDList();
	virtual ~CADSXEnumIDList();
	
	// Initialization logic in a method separate because COM object constructors
	// are called in a weird way that makes it so that, AFAIK, they can't have
	// parameters.
	void Init(IUnknown *pUnkOwner, const BSTR pszPath);

	// IEnumIDList
	STDMETHOD(Next)(ULONG, PITEMID_CHILD *, ULONG *);
	STDMETHOD(Skip)(ULONG);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumIDList **);

  protected:
	using FnConsume = std::function<bool(PITEMID_CHILD **, ULONG *)>;
	HRESULT NextInternal(
		FnConsume fnConsume,
		ULONG celt,
		PITEMID_CHILD *rgelt,
		ULONG *pceltFetched
	);
	bool PushPidl(PITEMID_CHILD **ppelt, ULONG *nActual) const;

	// A sentinel COM object to represent the lifetime of the owner object.
	// This exists to prevent the owner object from being freed before this one.
	CComPtr<IUnknown> m_pUnkOwner;

	BSTR m_pszPath;  // path on which to find streams
	HANDLE m_hFinder;
	WIN32_FIND_STREAM_DATA m_fsd;
	ULONG m_nTotalFetched;  // just to bring a clone up to speed
};
