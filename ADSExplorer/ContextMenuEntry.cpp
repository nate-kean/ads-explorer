/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2024 Nate Kean
 */

#pragma once

#include "StdAfx.h"  // Precompiled header; include first
#include "resource.h"  // Resource IDs from the RC file

#include "ContextMenuEntry.h"

#include "debug.h"

// Debug log prefix for ADSX::CContextMenuEntry
#define P_CME L"ADSX::CContextMenuEntry(0x" << std::hex << this << L")::"

namespace ADSX {


//==========================================================================
// ADSX::CContextMenuEntry
CContextMenuEntry::CContextMenuEntry() : m_pszADSPath((NULL)) {
	LOG(P_CME << L"CONSTRUCTOR");
}

CContextMenuEntry::~CContextMenuEntry() {
	LOG(P_CME << L"DESTRUCTOR");
	if (m_pszADSPath != NULL) CoTaskMemFree(m_pszADSPath);
}


//==========================================================================
// IShellExtInit
IFACEMETHODIMP CContextMenuEntry::Initialize(
	_In_opt_ PCIDLIST_ABSOLUTE pidlFolder,
	_In_     IDataObject*      pdo,
	_In_     HKEY              hkeyProgID
) {
	UNREFERENCED_PARAMETER(pidlFolder);
	UNREFERENCED_PARAMETER(hkeyProgID);
	LOG(P_CME << L"Initialize()");

	HRESULT hr;
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg = { TYMED_HGLOBAL };

	// Look for CF_HDROP data in the data object. If there
	// is no such data, return an error back to Explorer.
	hr = pdo->GetData(&fmt, &stg);
	if (FAILED(hr)) return WrapReturn(E_INVALIDARG);
	defer({ ReleaseStgMedium(&stg); });
	
	// Get a pointer to the actual data.
	HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
	if (hDrop == NULL) return WrapReturn(E_INVALIDARG);
	defer({ GlobalUnlock(stg.hGlobal); });

	// Sanity check – make sure there is at least one filename.
	// (0xFFFFFFFF = get file count)
	UINT uNumFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
	if (uNumFiles == 0) return WrapReturn(E_INVALIDARG);
	
	// Get the required size of the first file's name buffer.
	// (Returned size does not include null terminator)
	UINT cchPath = DragQueryFileW(hDrop, 0, NULL, 0);
	PWSTR pszFilePath = new WCHAR[cchPath + 1];
	defer({ delete[] pszFilePath; });

	// Get the first file's name.
	UINT uResult = DragQueryFileW(hDrop, 0, pszFilePath, cchPath + 1);
	if (uResult == 0) return WrapReturn(E_INVALIDARG);
	LOG(L" ** " << pszFilePath);

	// ADS path = prefix + path
	size_t cchADSPath = _countof(szPrefix) + cchPath + 1;
	size_t cbADSPath = cchADSPath * sizeof(WCHAR);
	m_pszADSPath = static_cast<PWSTR>(CoTaskMemAlloc(cbADSPath));
	wcsncpy_s(m_pszADSPath, cbADSPath, szPrefix, _countof(szPrefix));
	wcsncat_s(m_pszADSPath, cbADSPath, pszFilePath, cchPath);
	
	return WrapReturn(S_OK);
}


//==========================================================================
// IContextMenu
IFACEMETHODIMP CContextMenuEntry::GetCommandString(
	_In_                 UINT_PTR idCmd,
	_In_                 UINT     uFlags,
	_In_                 UINT*    puReserved,
	_Out_writes_(cchMax) LPSTR    pszName,
	_In_                 UINT     cchMax
) {
	LOG(P_CME << L"GetCommandString()");

	if (uFlags & GCS_VERBW) {
		LOG(L" ** GCS_VERBW");
		// If Explorer is asking for a verb string, and idCmd is the
		// command associated with our context menu, copy our verb
		// string into the pszName buffer.
		if (idCmd == 0) {
			lstrcpynW(
				reinterpret_cast<PWSTR>(pszName),
				L"BrowseADSes",
				cchMax
			);
			return WrapReturn(S_OK);
		}
	} else if (uFlags & GCS_HELPTEXT) {
		LOG(L" ** GCS_HELPTEXT");
		// If Explorer is asking for a help text string, and idCmd is
		// the command associated with our context menu, copy our
		// help text string into the pszName buffer.
		if (idCmd == 0) {
			lstrcpynW(
				reinterpret_cast<PWSTR>(pszName),
				L"Browse alternate data streams",
				cchMax
			);
			return WrapReturn(S_OK);
		}
	}
	return WrapReturn(E_INVALIDARG);
}


IFACEMETHODIMP CContextMenuEntry::InvokeCommand(
	_In_ CMINVOKECOMMANDINFO* pcmici
) {
	LOG(P_CME << L"InvokeCommand()");

	// If lpVerb really points to a string, ignore this function call.
	// if (HIWORD(pici->lpVerb) != 0) return WrapReturn(E_INVALIDARG);
	if (!IS_INTRESOURCE((pcmici->lpVerb))) return WrapReturnFailOK(E_INVALIDARG);

	// Get the command index from the low word of lpcmi->lpVerb.
	UINT uCmd = LOWORD(pcmici->lpVerb);

	// If the command index is 0, then it's the command associated with our
	// context menu.
	if (uCmd != 0) return WrapReturnFailOK(E_INVALIDARG);

	ShellExecuteW(
		pcmici->hwnd,
		L"explore",
		m_pszADSPath,
		NULL,
		NULL,
		SW_SHOWNORMAL
	);

	return WrapReturn(S_OK);
}


IFACEMETHODIMP CContextMenuEntry::QueryContextMenu(
	_In_ HMENU hmenu,
	_In_ UINT  i,
	_In_ UINT  uidCmdFirst,
	_In_ UINT  uidCmdLast,
	_In_ UINT  uFlags
) {
	LOG(P_CME << L"QueryContextMenu()");

	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if (uFlags & CMF_DEFAULTONLY) {
		return WrapReturn(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0));
	}

	// Use the helper function InsertMenu to add a new menu item.
	// We specify CMF_SEPARATOR to add a separator.
	// InsertMenuW(hmenu, i, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	InsertMenuW(
		hmenu,
		i,
		MF_STRING | MF_BYPOSITION,
		uidCmdFirst,
		// TODO(garlic-os): put this in a resource string
		L"Browse alternate data streams"
	);
	return WrapReturn(MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1));
}

}  // namespace ADSX
