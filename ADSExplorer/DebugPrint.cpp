#include "stdafx.h"

#include "DebugPrint.h"
#include <winnt.h>
// #include <fstream>


CDebugStream::CDebugStream() : std::wostream(this) {}
CDebugStream::~CDebugStream() {}


std::streamsize
CDebugStream::xsputn(const CDebugStream::Base::char_type* s, std::streamsize n) {
	// #define _DEBUG
	#ifdef _DEBUG
		OutputDebugStringW(s);
	#endif
	return n;
};

CDebugStream::Base::int_type
CDebugStream::overflow(CDebugStream::Base::int_type c) {
	// #define _DEBUG
	#ifdef _DEBUG
		WCHAR w = c;
		WCHAR s[2] = {w, '\0'};
		OutputDebugStringW(s);
	#endif
	return 1;
}

// std::wofstream g_DebugStream(
// 	L"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
// 	L"ADS Explorer\\adsx.log",
// 	std::ios::out | std::ios::trunc
// );
