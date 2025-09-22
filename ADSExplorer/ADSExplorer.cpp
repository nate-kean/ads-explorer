/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 */

// dllmain.cpp : Defines the entry point for the DLL application.
#include "StdAfx.h"  // Precompiled header; include first

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
	_In_ HINSTANCE hInstance,
	_In_ DWORD     ul_reason_for_call,
	_In_ LPVOID    lpReserved
) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap, hInstance, &LIBID_ADSEXPLORERLib);
		DisableThreadLibraryCalls(hInstance);
	} else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		_Module.Term();
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
		_Module.UnregisterServer(TRUE);
		return hr;
	}
	return hr;
}

/**
 * Remove entries from the system registry.
 */
STDAPI DllUnregisterServer() {
	HRESULT hr = _Module.UnregisterServer(TRUE);
	// Even if unregistration fails partially, we still return the result.
	// The caller can check the return value to determine if cleanup was
	// successful.
	return hr;
}
