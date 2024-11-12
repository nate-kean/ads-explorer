/*
 * Copyright (c) 2004 Pascal Hurni
 */

#pragma once


#include "StdAfx.h"  // Precompiled header; include first

#include "ADSXContextMenu_h.h"

#include "resource.h"  // main symbols


//==============================================================================
// CADSXContextMenu

class ATL_NO_VTABLE CADSXContextMenu
	: public CComObjectRootEx<CComSingleThreadModel>,
	  public CComCoClass<CADSXContextMenu, &CLSID_ADSXContextMenuContextMenu>,
	  public IExplorerCommand,
	  public IObjectWithSite {
   public:
	// CADSXContextMenu(void);
	// virtual ~CADSXContextMenu(void);

	DECLARE_REGISTRY_RESOURCEID(IDR_CONTEXTMENU)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CADSXContextMenu)
		COM_INTERFACE_ENTRY(IExplorerCommand)
		COM_INTERFACE_ENTRY(IObjectWithSite)
	END_COM_MAP()

	//--------------------------------------------------------------------------
	// IExplorerCommand
	IFACEMETHOD(GetTitle)(_In_opt_ IShellItemArray* psiaSelection, _Outptr_result_nullonfailure_ PWSTR* ppszName);
	IFACEMETHOD(GetIcon)(_In_opt_ IShellItemArray* psiaSelection, _Outptr_result_nullonfailure_ PWSTR* ppszIcon);
	IFACEMETHOD(GetToolTip)(_In_opt_ IShellItemArray* psiaSelection, _Outptr_result_nullonfailure_ PWSTR* ppszInfotip);
	IFACEMETHOD(GetCanonicalName)(_Out_ GUID* pguidCommandName);
	IFACEMETHOD(GetState)(_In_opt_ IShellItemArray* psiaSelection, _In_ BOOL bOkToBeSlow, _Out_ EXPCMDSTATE* pfCmdState);
	IFACEMETHOD(Invoke)(_In_opt_ IShellItemArray* psiaSelection, _In_opt_ IBindCtx* pbc) noexcept;
	IFACEMETHOD(GetFlags)(_Out_ EXPCMDFLAGS* pFlags);
	IFACEMETHOD(EnumSubCommands)(_COM_Outptr_ IEnumExplorerCommand** ppEnum);

	//--------------------------------------------------------------------------
	// IObjectWithSite
	IFACEMETHOD(SetSite)(_In_opt_ IUnknown* punkSite);
	_Check_return_ IFACEMETHOD(GetSite)(
		_In_ REFIID riid,
		_COM_Outptr_result_maybenull_ void** ppunkSite
	) noexcept;

	//--------------------------------------------------------------------------

   protected:
	CComPtr<IUnknown> m_punkSite;
};
