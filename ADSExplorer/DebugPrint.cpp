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

#ifdef _DEBUG
	#include <iomanip>
	#include <sstream>
	#include <string>

	static std::wstring HRESULTToString(HRESULT hr) {
		switch (hr) {
			case S_OK: return L"S_OK";
			case S_FALSE: return L"S_FALSE";
			case E_NOTIMPL: return L"E_NOTIMPL";
			case E_NOINTERFACE: return L"E_NOINTERFACE";
			case E_POINTER: return L"E_POINTER";
			case E_OUTOFMEMORY: return L"E_OUTOFMEMORY";
			case E_INVALIDARG: return L"E_INVALIDARG";
			case E_FAIL: return L"E_FAIL";
			default:
				std::wstringstream ss;
				ss << std::hex << std::setw(8) << std::setfill(L'0') << hr;
				return ss.str();
		}
	}

	HRESULT LogReturn(HRESULT hr) {
		LOG(L" -> " << HRESULTToString(hr));
		return hr;
	}
#endif
