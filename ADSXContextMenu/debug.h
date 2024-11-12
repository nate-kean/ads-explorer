/**
 * Copyright (c) 2024 Nate Kean
 */

#pragma once

#include "StdAfx.h"

#include <ostream>

// Implements wstreambuf, but only for itself.
// Then implements wostream publicly, so this class can be used as an ostream
// that acts on its internal wstreambuf.
class CDebugStream
	: private std::wstreambuf
	, public std::wostream {
  public:
	// --- Singleton ---
	// https://stackoverflow.com/a/1008289/16247437
	static CDebugStream& get_instance() {
		static CDebugStream instance;
		return instance;
	}
	CDebugStream(CDebugStream const&) = delete;
	void operator=(CDebugStream const&) = delete;
	// --- /Singleton ---

  protected:
	using Base = std::wstreambuf;
	// https://medium.com/@pauljlucas/custom-c-stream-manipulators-ee1854d8b852
	std::streamsize
	xsputn(const CDebugStream::Base::char_type* s, std::streamsize n) override;
	Base::int_type
	overflow(Base::int_type c) override;
	std::ios_base::fmtflags m_initial_flags;

  private:
	// --- Singleton ---
	CDebugStream();
	virtual ~CDebugStream();
	// --- /Singleton ---
};

// #define DBG_LOG_TO_FILE
#ifdef DBG_LOG_TO_FILE
	extern std::wofstream g_DebugStream;
	#define DEBUG_STREAM g_DebugStream
#else
	#define DEBUG_STREAM CDebugStream::get_instance()
#endif

// #define _DEBUG
#ifdef _DEBUG
	// https://stackoverflow.com/a/3371577
	// Usage: LOG(L"Hello" << ' ' << L"World!" << 1);
	#define LOG(ostream_expression) do { \
		DEBUG_STREAM << ostream_expression << std::endl; \
	} while(false)

	_Post_equal_to_(hr) HRESULT WrapReturn(_In_ HRESULT hr);
	_Post_equal_to_(hr) HRESULT WrapReturnFailOK(_In_ HRESULT hr);
	std::wstring SFGAOFToString(const SFGAOF *pfAttribs);
	std::wstring SHCONTFToString(const SHCONTF *pfAttribs);
	std::wstring SHGDNFToString(const SHGDNF *pfAttribs);
	std::wstring HRESULTToString(HRESULT hr);
#else
	// Require a semicolon after LOG() to avoid "expected ';' after expression"
	// error while still compiling down to nothing in release builds.
	#define DBG_NOTHING do { } while (false)

	#define LOG(_) DBG_NOTHING
	#define WrapReturn(hr) (hr)
	#define WrapReturnFailOK(hr) (hr)
	#define PidlToString(_) DBG_NOTHING
	#define PidlArrayToString(_) DBG_NOTHING
	#define IIDToString(_) DBG_NOTHING
	#define SFGAOFToString(_) DBG_NOTHING
	#define SHCONTFToString(_) DBG_NOTHING
	#define SHGDNFToString(_) DBG_NOTHING
	#define HRESULTToString(_) DBG_NOTHING
#endif
