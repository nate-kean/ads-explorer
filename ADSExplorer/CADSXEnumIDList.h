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
	
	// Ensure the owner object is not freed before this one
	void Init(IUnknown *pUnkOwner, const BSTR pszPath);

	// IEnumIDList
	STDMETHOD(Next)(ULONG, PITEMID_CHILD *, ULONG *);
	STDMETHOD(Skip)(ULONG);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumIDList **);

  private:
  	using FnConsume = std::function<bool(PITEMID_CHILD *, ULONG *)>;
	HRESULT NextInternal(FnConsume fnConsume, ULONG celt, PITEMID_CHILD *rgelt, ULONG *pceltFetched);
	bool PushPidl(PITEMID_CHILD * pelt, ULONG *nActual);

  protected:
	CComPtr<IUnknown> m_pUnkOwner;

	BSTR m_pszPath;
	HANDLE m_hFinder;
	WIN32_FIND_STREAM_DATA m_fsd;
	ULONG m_nTotalFetched;
};
