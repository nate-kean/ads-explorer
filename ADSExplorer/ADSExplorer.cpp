/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"  // MUST be included first

#if _MSC_VER > 1200
#include "ADSExplorer_h.h"
#else
// the IDL compiler on VC++6 puts it here instead. weird!
#include "ADSExplorer.h"
#endif
#include "ADSExplorer_i.c"
#include "RootShellFolder.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ADSExplorerRootShellFolder, CADSXRootShellFolder)
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
	_Outptr_ LPVOID   *ppv
) {
	return _Module.GetClassObject(rclsid, riid, ppv);
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
