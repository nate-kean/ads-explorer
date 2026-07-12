/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2026 Nate Kean
 */

#pragma once

#include "pch.h"  // Precompiled header; include first

#include "ContextMenuEntry.h"

#include "debug.h"
#include "resource.h"  // Resource IDs from the RC file

// Debug log prefix for ADSX::CContextMenuEntry
#define P_CME L"ADSX::CContextMenuEntry(0x" << std::hex << this << L")::"

namespace ADSX {

#pragma region ADSX::CContextMenuEntry

CContextMenuEntry::CContextMenuEntry() : m_pszADSPath(NULL) {
	LOG(P_CME << L"CONSTRUCTOR");
}

CContextMenuEntry::~CContextMenuEntry() {
	LOG(P_CME << L"DESTRUCTOR");
	if (m_pszADSPath != NULL) CoTaskMemFree(m_pszADSPath);
}

#pragma endregion

#pragma region IShellExtInit

WCHAR ADSX::CContextMenuEntry::s_szMessage[
	_countof(ADSX::CContextMenuEntry::s_szMessage)
] = {};

IFACEMETHODIMP CContextMenuEntry::Initialize(
	_In_opt_ PCIDLIST_ABSOLUTE pidlaFolder,
	_In_     IDataObject*      pdo,
	_In_     HKEY              hkeyProgID
) {
	UNREFERENCED_PARAMETER(pidlaFolder);
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
	auto hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
	if (hDrop == NULL) return WrapReturn(E_INVALIDARG);
	defer({ GlobalUnlock(stg.hGlobal); });

	// Make sure there is exactly one FS object selected.
	// We can't show the ADS view of two things at once (or of nothing).
	// (0xFFFFFFFF = get file count)
	UINT uNumFiles = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
	if (uNumFiles != 1) return WrapReturn(E_INVALIDARG);
	
	// Get the required buffer size for the first path and allocate it.
	// (Returned size does not include null terminator)
	UINT cchPath = DragQueryFileW(hDrop, 0, NULL, 0);
	PWSTR pszPath = new WCHAR[cchPath + 1];
	defer({ delete[] pszPath; });

	// Load the path into the buffer.
	UINT cchPathCopied = DragQueryFileW(hDrop, 0, pszPath, cchPath + 1);
	if (cchPathCopied == 0) return WrapReturn(E_INVALIDARG);
	LOG(L" ** Received file path: " << pszPath);

	// ADS path = prefix + path
	size_t cchADSPath = _countof(s_szPrefix) + cchPath + 1;
	size_t cbADSPath = cchADSPath * sizeof(WCHAR);
	m_pszADSPath = static_cast<PWSTR>(CoTaskMemAlloc(cbADSPath));
	wcsncpy_s(m_pszADSPath, cbADSPath, s_szPrefix, _countof(s_szPrefix));
	wcsncat_s(m_pszADSPath, cbADSPath, pszPath, cchPath);
	LOG(L" ** Computed destination: " << m_pszADSPath);

	static bool s_bMessageLoaded = false;
	if (!s_bMessageLoaded) {
		int cchMessageCopied = LoadStringW(
			NULL,
			IDS_MSG_BROWSE,
			s_szMessage,
			_countof(s_szMessage)
		);
		if (cchMessageCopied == 0) {
			return WrapReturn(AtlHresultFromLastError());
		}
		s_bMessageLoaded = true;
	}

	return WrapReturn(S_OK);
}

#pragma endregion


#pragma region IContextMenu

IFACEMETHODIMP CContextMenuEntry::GetCommandString(
	_In_                 UINT_PTR idCmd,
	_In_                 UINT     uFlags,
	_In_                 UINT*    puReserved,
	_Out_writes_(cchMax) LPSTR    pszName,
	_In_                 UINT     cchMax
) {
	LOG(P_CME << L"GetCommandString(idCmd=" << idCmd << L")");

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
				// L"Our time on Earth is borrowed",
				// L"Stop what you're doing and go to hehe cat folder",
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

	// Go to the ADS Explorer version of this path in Explorer.
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
	LOG(P_CME << L"QueryContextMenu(i=" << i << L", uidCmdFirst=" << uidCmdFirst
			  << L", uidCmdLast=" << uidCmdLast << L", uFlags="
			  << CMFToString(uFlags) << L")");

	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if (uFlags & CMF_DEFAULTONLY) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	// Called with this flag in a bogus duplicate Context Menu Entry instance
	// with an incorrect parent dir of %USERPROFILE%\Desktop. I don't get it,
	// result is not used, documentation makes it seem optional, and....
	// following through causes heap corruption!!!!! ?????? So we skip it.
	// TODO: Find out what the second instance with the weird file path is for
	// TODO: Surely there is something *I* am doing wrong to cause the heap
	// corruption
	if (uFlags == CMF_OPTIMIZEFORINVOKE) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	// TODO: Add this back once base functionality is working
	UINT nItems = 0;
	// InsertMenuW(hmenu, i + nItems++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

	MENUITEMINFOW mii = {
		.cbSize = sizeof(MENUITEMINFOW),
		.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE,
		.fType = MFT_STRING,
		.fState = MFS_ENABLED | MFS_UNCHECKED | MFS_UNHILITE,
		.wID = uidCmdFirst,
		.hSubMenu = hmenu,
		.hbmpChecked = NULL,
		.hbmpUnchecked = NULL,
		.dwItemData = 0,
		// .dwTypeData = GetCMEMessage()->GetBuffer(),
		// .cch = static_cast<UINT>(GetCMEMessage()->GetLength())
		.dwTypeData = s_szMessage,
		.cch = _countof(s_szMessage)
	};
	#if (WINVER >= 0x0500)
		mii.hbmpItem = NULL;
	#endif /* WINVER >= 0x0500 */
	InsertMenuItemW(hmenu, i + nItems++, TRUE, &mii);

	return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, nItems);
}

#pragma endregion

}  // namespace ADSX
