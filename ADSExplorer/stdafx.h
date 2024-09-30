/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2020 Calvin Buckley
 * Copyright (c) 2024 Nate Kean
 */

// StdAfx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

// Strict PIDL typing
#define STRICT_TYPED_ITEMIDS

// Force Unicode
// If you want ANSI use UNICOWS
#ifndef UNICODE
	#define UNICODE
#endif
#ifndef _UNICODE
	#define _UNICODE
#endif

// More strict CString constructors
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

// #define _ATL_DEBUG_QI

// Property keys on Windows Vista, used for tile view subtitles.
// On older platforms, this can usually be still defined, but polyfilled.
// TODO(NattyNarwhal): Perhaps it could disable all of IShellFolder2 for really
// old SDKs?
#define ADSX_PKEYS_SUPPORT

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <Windows.h>

#include <ShlObj.h>  // Shell objects like IShellFolder
#include <atlbase.h>  // Defines CComModule and required by atlwin.h
#include <atlwin.h>  // CMessageMap, ProcessWindowMessage, MESSAGE_HANDLER

#include <string>  // TODO: Remove C++ STL

#include "debug.h"
#include "defer.h"

extern CComModule _Module;


#if defined(ADSX_PKEYS_SUPPORT) && VER_PRODUCTBUILD >= 6000
	// These are probably supportable on 2600 in some obscure WDS SDK
	#include <propkey.h>
	#include <propvarutil.h>
	#elif defined(ADSX_PKEYS_SUPPORT)
	// Create polyfills for these (which don't link to anything, are just header
	// magic) definitions. The DEFINE one is typedef' PROPERTYKEY in the real SDK,
	// but we'll just save a line and define it to SHCOLUMNID for those old SDKs.
	// These are taken from the Windows 7 Platform SDK.
	#define DEFINE_PROPERTYKEY(                                \
		name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid   \
	)                                                          \
		EXTERN_C const SHCOLUMNID DECLSPEC_SELECTANY name = {  \
			{l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}, pid \
		}

	DEFINE_PROPERTYKEY(
		PKEY_PropList_TileInfo,
		0xC9944A21, 0xA406, 0x48FE, 0x82, 0x25, 0xAE, 0xC7, 0xE2, 0x4C, 0x21, 0x1B,
		3
	);
	DEFINE_PROPERTYKEY(
		PKEY_PropList_ExtendedTileInfo,
		0xC9944A21, 0xA406, 0x48FE, 0x82, 0x25,	0xAE, 0xC7, 0xE2, 0x4C, 0x21, 0x1B,
		9
	);
	DEFINE_PROPERTYKEY(
		PKEY_PropList_PreviewDetails,
		0xC9944A21, 0xA406, 0x48FE, 0x82, 0x25, 0xAE, 0xC7, 0xE2, 0x4C, 0x21, 0x1B,
		8
	);
	DEFINE_PROPERTYKEY(
		PKEY_PropList_FullDetails,
		0xC9944A21, 0xA406, 0x48FE, 0x82, 0x25, 0xAE, 0xC7, 0xE2, 0x4C, 0x21, 0x1B,
		2
	);
	DEFINE_PROPERTYKEY(
		PKEY_ItemType,
		0x28636AA6, 0x953D, 0x11D2, 0xB5, 0xD6, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xD0,
		11
	);
	DEFINE_PROPERTYKEY(
		PKEY_ItemNameDisplay,
		0xB725F130, 0x47EF, 0x101A, 0xA5, 0xF1, 0x02, 0x60,	0x8C, 0x9E, 0xEB, 0xAC,
		10
	);
	DEFINE_PROPERTYKEY(
		PKEY_ItemPathDisplay,
		0xE3E0584C, 0xB788, 0x4A5A, 0xBB, 0x20, 0x7F, 0x5A, 0x44, 0xC9, 0xAC, 0xDD,
		7
	);

	// From propkeydef.h
	#define IsEqualPropertyKey(a, b) \
		(((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid))

	// no in/out for VC++6 compat
	inline HRESULT InitVariantFromString(PCWSTR psz, VARIANT *pvar) {
		pvar->vt = VT_BSTR;
		pvar->bstrVal = SysAllocString(psz);
		HRESULT hr = pvar->bstrVal ? S_OK : (psz ? E_OUTOFMEMORY : E_INVALIDARG);
		if (FAILED(hr)) {
			VariantInit(pvar);
		}
		return hr;
	}
#endif
