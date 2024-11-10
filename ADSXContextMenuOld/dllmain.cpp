// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "dllmain_h.h"
#include "dllmain_i.c"

#include <sstream>

#include <atlbase.h>
#include <atlhost.h>
#include <atlstr.h>
#include <shobjidl_core.h>


#define P_CMC L"ADSXContextMenuEntry(0x" << std::hex << this << L")::"

static constexpr WCHAR g_pszPrefix[] =
	L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}\\";
static const CStringW g_strContextMenuCaption(
	MAKEINTRESOURCE(IDS_CONTEXT_MENU_ENTRY)
);

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ADSXContextMenuEntry, ADSXContextMenuEntry)
END_OBJECT_MAP()


BOOL APIENTRY DllMain(
    _In_ HMODULE hModule,
    _In_ DWORD   ul_reason_for_call,
    _In_ PVOID   lpReserved
) {
	UNREFERENCED_PARAMETER(lpReserved);
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap, hModule, &LIBID_ADSXContextMenuLib);
		DisableThreadLibraryCalls(hModule);
	} else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		_Module.Term();
	}
	return TRUE;
}

// class __declspec(uuid("fb902ce2-f0ab-443e-b32a-99007ae9dcc2"))
class ATL_NO_VTABLE ADSXContextMenuEntry final
	: public CComObjectRootEx<CComSingleThreadModel>,
	  public CComCoClass<ADSXContextMenuEntry, &CLSID_ADSXContextMenuEntry>,
	  public IExplorerCommand,
	  public IObjectWithSite {
   public:
	// virtual PCWSTR Title() { return L"ADS Explorer"; }
	// virtual const EXPCMDFLAGS Flags() { return ECF_DEFAULT; }
	// virtual const EXPCMDSTATE State(_In_opt_ IShellItemArray *papidlSelection) {
	// 	UNREFERENCED_PARAMETER(papidlSelection);
	// 	return ECS_ENABLED;
	// }
	DECLARE_REGISTRY_RESOURCEID(IDR_CONTEXTMENUENTRY)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(ADSXContextMenuEntry)
		COM_INTERFACE_ENTRY(IExplorerCommand)
		COM_INTERFACE_ENTRY(IObjectWithSite)
	END_COM_MAP()

	#pragma region IExplorerCmd
	IFACEMETHODIMP GetTitle(
		_In_opt_                      IShellItemArray *paItems,
		_Outptr_result_nullonfailure_ PWSTR           *ppszName
	) {
		UNREFERENCED_PARAMETER(paItems);
		if (ppszName == NULL) return E_POINTER;
		return SHStrDup(g_strContextMenuCaption, ppszName);
	}

	IFACEMETHODIMP GetIcon(
		_In_opt_                      IShellItemArray *paItems,
		_Outptr_result_nullonfailure_ PWSTR           *ppszIcon
	) {
		UNREFERENCED_PARAMETER(paItems);
		// std::wstring iconResourcePath = get_module_folderpath(g_hInst);
		// iconResourcePath += L"\\Assets\\FileLocksmith\\";
		// iconResourcePath += L"FileLocksmith.ico";
		// return SHStrDup(iconResourcePath.c_str(), ppszIcon);
		if (ppszIcon == NULL) return E_POINTER;
		*ppszIcon = NULL;
		return E_NOTIMPL;
	}

	IFACEMETHODIMP GetToolTip(
		_In_opt_                      IShellItemArray *paItems,
		_Outptr_result_nullonfailure_ PWSTR           *ppszInfoTip
	) {
		UNREFERENCED_PARAMETER(paItems);
		if (ppszInfoTip == NULL) return E_POINTER;
		*ppszInfoTip = NULL;
		return E_NOTIMPL;
	}

	IFACEMETHODIMP GetCanonicalName(_Out_ GUID *guidCommandName) {
		if (guidCommandName == NULL) return E_POINTER;
		*guidCommandName = __uuidof(this);
		return S_OK;
	}

	IFACEMETHODIMP GetState(
		_In_opt_ IShellItemArray *papidlSelection,
		_In_     BOOL            bOkToBeSlow,
		_Out_    EXPCMDSTATE     *pfCmdState
	) {
		UNREFERENCED_PARAMETER(papidlSelection);
		UNREFERENCED_PARAMETER(bOkToBeSlow);
		if (pfCmdState == NULL) return E_POINTER;
		*pfCmdState = ECS_ENABLED;

		// if (!FileLocksmithSettingsInstance().GetEnabled()) {
		// 	*cmdState = ECS_HIDDEN;
		// }

		// if (FileLocksmithSettingsInstance().GetShowInExtendedContextMenu()) {
		// 	*cmdState = ECS_HIDDEN;
		// }

		return S_OK;
	}

	IFACEMETHODIMP Invoke(
		_In_opt_ IShellItemArray *papidlSelection,
		_In_opt_ IBindCtx        *pbc
	) noexcept {
		UNREFERENCED_PARAMETER(pbc);
		LOG(P_CMC << L"Invoke()");

		if (papidlSelection == NULL) return WrapReturn(E_POINTER);

		HRESULT hr;
		DWORD cpidl;
		hr = papidlSelection->GetCount(&cpidl);
		if (FAILED(hr)) return WrapReturn(hr);
		if (cpidl == 0) return WrapReturn(E_INVALIDARG);

		// Only show context menu for one item.
		// TODO(garlic-os): is this the correct way to do this?
		if (cpidl > 1) return WrapReturn(E_INVALIDARG);

		IShellItem *psi;
		hr = papidlSelection->GetItemAt(0, &psi);  // AddRef'd here
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

	IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS *pFlags) {
		if (pFlags == NULL) return E_POINTER;
		*pFlags = ECF_DEFAULT;
		return S_OK;
	}
	IFACEMETHODIMP EnumSubCommands(
		_COM_Outptr_ IEnumExplorerCommand **ppEnumCommands
	) {
		if (ppEnumCommands == NULL) return E_POINTER;
		*ppEnumCommands = NULL;
		return E_NOTIMPL;
	}
	#pragma endregion

	#pragma region IObjectWithSite
	IFACEMETHODIMP SetSite(_In_ IUnknown *punkSite) noexcept {
		m_punkSite = punkSite;
		return S_OK;
	}
	IFACEMETHODIMP
	GetSite(_In_ REFIID riid, _COM_Outptr_ void **ppunkSite) noexcept {
		if (ppunkSite == NULL) return E_POINTER;
		return m_punkSite.CopyTo(riid, ppunkSite);
	}
	#pragma endregion

   protected:
	CComPtr<IUnknown> m_punkSite;

   private:
	std::wstring context_menu_caption = GET_RESOURCE_STRING_FALLBACK(
		IDS_CONTEXT_MENU_ENTRY,
		L"Browse alternate data streams"
	);
};


STDAPI DllGetActivationFactory(
	_In_         HSTRING            hsActivatableClassId,
	_COM_Outptr_ IActivationFactory **ppFactory
) {
	if (ppFactory == NULL) return E_POINTER;
	return WRL::Module<WRL::ModuleType::InProc>::GetModule().GetActivationFactory(
		hsActivatableClassId, ppFactory
	);
}

STDAPI DllCanUnloadNow() {
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(
	_In_          REFCLSID rclsid,
	_In_          REFIID   riid,
	_COM_Outptr_  void     **ppInstance
) {
	return _Module.GetClassObject(rclsid, riid, ppInstance);
}
