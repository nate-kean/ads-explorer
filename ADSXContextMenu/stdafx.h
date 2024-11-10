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


#include "targetver.h"

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <Windows.h>

#include <atlbase.h>  // Defines CComModule and required by atlwin.h
#include <atlwin.h>  // CMessageMap, ProcessWindowMessage, MESSAGE_HANDLER

#include <string>  // TODO: Remove C++ STL

#include "debug.h"
#include "../Common/defer.h"

extern CComModule _Module;
