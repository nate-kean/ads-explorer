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

#include <ios>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <shtypes.h>
#include <shlobj_core.h>
#include <atlbase.h>  // Defines CComModule and required by atlwin.h

#include "debug.h"
#include "../Common/defer.h"
