/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 */

// dllmain.cpp : Defines the entry point for the DLL application.
#include "StdAfx.h"  // Precompiled header; include first

#if _MSC_VER > 1200
#include "ADSXContextMenu_h.h"
#else
// the IDL compiler on VC++6 puts it here instead. weird!
#include "ADSXContextMenu.h"
#endif
#include "ADSXContextMenu_i.c"
#include "RootShellFolder.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ADSXContextMenuRootShellFolder, CADSXRootShellFolder)
END_OBJECT_MAP()

BOOL APIENTRY DllMain(
	_In_ HINSTANCE hInstance,
	_In_ DWORD     ul_reason_for_call,
	_In_ LPVOID    lpReserved
) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		_Module.Init(ObjectMap, hInstance, &LIBID_ADSXContextMenuLib);
		DisableThreadLibraryCalls(hInstance);
	} else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		_Module.Term();
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void) {
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
_Check_return_
STDAPI DllGetClassObject(
	_In_     REFCLSID rclsid,
	_In_     REFIID   riid,
	_Outptr_ LPVOID   *ppObject
) {
	return _Module.GetClassObject(rclsid, riid, ppObject);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer() {
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer() {
	return _Module.UnregisterServer(TRUE);
}
