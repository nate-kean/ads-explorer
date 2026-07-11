/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 */

// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"  // Precompiled header; include first

#include "ADSExplorer_h.h"
#include "ADSExplorer_i.c"
#include "ShellFolder.h"
#include "ContextMenuEntry.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ADSExplorerShellFolder, ADSX::CShellFolder)
	OBJECT_ENTRY(CLSID_ADSXContextMenuEntry, ADSX::CContextMenuEntry)
END_OBJECT_MAP()

BOOL APIENTRY DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
) {
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH: {
			HRESULT hr = _Module.Init(ObjectMap, hinstDLL, &LIBID_ADSEXPLORERLib);
			if (FAILED(hr)) return FALSE;
			BOOL result = DisableThreadLibraryCalls(hinstDLL);
			if (result == 0) return FALSE;
			break;
		}
		case DLL_PROCESS_DETACH: {
			if (lpvReserved != nullptr) {
				// learn.microsoft.com says not to clean up if there is
				// something in lpvReserved
				break;
			}
			_Module.Term();
			break;
		}
	}
	return TRUE;
}

/**
 * Used to determine whether the DLL can be unloaded by OLE.
 */
__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void) {
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/**
 * Return a class factory to create an object of the requested type.
 */
_Check_return_
STDAPI DllGetClassObject(
	_In_     REFCLSID rclsid,
	_In_     REFIID   riid,
	_Outptr_ LPVOID   *ppObject
) {
	return _Module.GetClassObject(rclsid, riid, ppObject);
}

/**
 * Add entries to the system registry.
 * Registers object, typelib and all interfaces in typelib
 */
STDAPI DllRegisterServer() {
	HRESULT hr = _Module.RegisterServer(TRUE);
	if (FAILED(hr)) {
		// If registration failed, attempt to unregister to clean up any partial
		// registration.
		HRESULT hr2 = _Module.UnregisterServer(TRUE);
		if (FAILED(hr2)) {
			// Megafailure
			return hr2;
		}
	}
	return hr;
}

/**
 * Remove entries from the system registry.
 */
STDAPI DllUnregisterServer() {
	return _Module.UnregisterServer(TRUE);
}
