/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

#include "StdAfx.h"  // Precompiled header; include first

#if _MSC_VER > 1200
	#include "ADSXContextMenu_h.h"
#else
	// the IDL compiler on VC++6 puts it here instead. weird!
	#include "ADSXContextMenu.h"
#endif

#include <atlstr.h>

#include "ContextMenu.h"

// Debug log prefix for CADSXContextMenu
#define P_XCM L"CADSXContextMenu(0x" << std::hex << this << L")::"


//==============================================================================
// CADSXContextMenu
// CADSXContextMenu::CADSXContextMenu(void) {
// 	// LOG(P_XCM << L"CONSTRUCTOR");
// }


// CADSXContextMenu::~CADSXContextMenu(void) {
// 	// LOG(P_XCM << L"DESTRUCTOR");
// }



#pragma region IExplorerCommand
IFACEMETHODIMP CADSXContextMenu::GetTitle(
	_In_opt_                      IShellItemArray* psiaSelection,
	_Outptr_result_nullonfailure_ PWSTR*           ppszName
) {
	UNREFERENCED_PARAMETER(psiaSelection);
	LOG(P_XCM << L"GetTitle()");
	if (ppszName == NULL) return E_POINTER;

	static constexpr PCWSTR pszName = L"Browse alternate data streams";
	return SHStrDup(pszName, ppszName);
}

IFACEMETHODIMP CADSXContextMenu::GetIcon(
	_In_opt_                      IShellItemArray* psiaSelection,
	_Outptr_result_nullonfailure_ PWSTR*           ppszIcon
) {
	UNREFERENCED_PARAMETER(psiaSelection);
	LOG(P_XCM << L"GetIcon()");
	if (ppszIcon == NULL) return E_POINTER;

	// std::wstring iconResourcePath = get_module_folderpath(g_hInst);
	// iconResourcePath += L"\\Assets\\FileLocksmith\\";
	// iconResourcePath += L"FileLocksmith.ico";
	// return SHStrDup(iconResourcePath.c_str(), ppszIcon);
	*ppszIcon = NULL;
	return E_NOTIMPL;
}

IFACEMETHODIMP CADSXContextMenu::GetToolTip(
	_In_opt_                      IShellItemArray* psiaSelection,
	_Outptr_result_nullonfailure_ PWSTR*           ppszInfotip
) {
	UNREFERENCED_PARAMETER(psiaSelection);
	LOG(P_XCM << L"GetToolTip()");
	if (ppszInfotip == NULL) return E_POINTER;
	*ppszInfotip = NULL;
	return E_NOTIMPL;
}

IFACEMETHODIMP
CADSXContextMenu::GetCanonicalName(_Out_ GUID* pguidCommandName) {
	LOG(P_XCM << L"GetCanonicalName()");
	if (pguidCommandName == NULL) return E_POINTER;
	*pguidCommandName = CLSID_ADSXContextMenu;
	return S_OK;
}

IFACEMETHODIMP CADSXContextMenu::GetState(
	_In_opt_ IShellItemArray* psiaSelection,
	_In_     BOOL             bOkToBeSlow,
	_Out_    EXPCMDSTATE*     pfCmdState
) {
	UNREFERENCED_PARAMETER(psiaSelection);
	UNREFERENCED_PARAMETER(bOkToBeSlow);
	if (pfCmdState == NULL) return E_POINTER;
	*pfCmdState = ECS_ENABLED;

	HRESULT hr;
	DWORD cpidl;
	hr = psiaSelection->GetCount(&cpidl);
	if (FAILED(hr)) return WrapReturn(hr);

	// Only show context menu for one item.
	if (cpidl != 1) {
		*pfCmdState = ECS_HIDDEN;
	}

	return WrapReturn(S_OK);
}

IFACEMETHODIMP CADSXContextMenu::Invoke(
	_In_opt_ IShellItemArray *psiaSelection,
	_In_opt_ IBindCtx        *pbc
) noexcept {
	UNREFERENCED_PARAMETER(pbc);
	LOG(P_XCM << L"Invoke()");

	if (psiaSelection == NULL) return WrapReturn(E_POINTER);

	HRESULT hr;
	DWORD cpidl;
	hr = psiaSelection->GetCount(&cpidl);
	if (FAILED(hr)) return WrapReturn(hr);
	if (cpidl == 0) return WrapReturn(E_INVALIDARG);

	// Only show context menu for one item.
	// TODO(garlic-os): should this check be here too?
	if (cpidl > 1) return WrapReturn(E_INVALIDARG);

	IShellItem *psi;
	hr = psiaSelection->GetItemAt(0, &psi);  // AddRef'd here
	if (FAILED(hr)) return WrapReturn(hr);
	defer({ psi->Release(); });

	PWSTR pszFilePath;
	hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
	if (FAILED(hr)) return WrapReturn(hr);
	defer({ CoTaskMemFree(pszFilePath); });

	// std::wostringstream ossNewPath;
	// ossNewPath << g_pszPrefix << pszFilePath;
	// TODO(garlic-os): browse to new path here
	// ossNewPath.str();

	return WrapReturn(S_OK);
}

IFACEMETHODIMP CADSXContextMenu::GetFlags(_Out_ EXPCMDFLAGS *pFlags) {
	if (pFlags == NULL) return E_POINTER;
	*pFlags = ECF_DEFAULT;
	return S_OK;
}
IFACEMETHODIMP CADSXContextMenu::EnumSubCommands(
	_COM_Outptr_ IEnumExplorerCommand **ppEnumCommands
) {
	if (ppEnumCommands == NULL) return E_POINTER;
	*ppEnumCommands = NULL;
	return E_NOTIMPL;
}

#pragma endregion


//-------------------------------------------------------------------------------
#pragma region IObjectWithSite
IFACEMETHODIMP CADSXContextMenu::SetSite(_In_opt_ IUnknown *punkSite) noexcept {
	m_punkSite = punkSite;
	return S_OK;
}
_Check_return_  IFACEMETHODIMP CADSXContextMenu::GetSite(
	_In_         REFIID riid,
	_COM_Outptr_result_maybenull_ void** ppunkSite
) noexcept {
	if (ppunkSite == NULL) return E_POINTER;
	*ppunkSite = NULL;
	return m_punkSite.CopyTo(reinterpret_cast<IUnknown**>(ppunkSite));
	// IObjectWithSite *pObjectWithSite;
	// HRESULT hr = m_punkSite.QueryInterface(&pObjectWithSite);
	// if (FAILED(hr)) return hr;

}
#pragma endregion
