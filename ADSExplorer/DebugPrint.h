#pragma once

#include "stdafx.h"


// #define _DEBUG
#ifdef _DEBUG
	void DebugPrint(LPCTSTR fmt, ...);
#else
	#define DebugPrint(...) (void) 0
#endif
