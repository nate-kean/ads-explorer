/**
 * Copyright (c) 2024 Nate Kean
 */

#pragma once

#include "stdafx.h"

#include <ostream>

#ifndef _DEBUG
	// Require a semicolon after LOG() to avoid "expected ';' after expression"
	// error while still compiling down to nothing in release builds.
	#define DBG_NOTHING do { } while (false)
#endif

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

extern std::wostream g_DebugStream;
// extern std::wofstream g_DebugStream;

// https://stackoverflow.com/a/3371577
// Usage: LOG(L"Hello" << ' ' << L"World!" << 1);
// #define _DEBUG
#ifdef _DEBUG
	#define LOG(psz) do { \
		CDebugStream::get_instance() << psz << std::endl; \
		/* g_DebugStream << psz << std::endl; */ \
	} while(false)

	HRESULT LogReturn(HRESULT hr);
#else
	#define LOG(_) DBG_NOTHING
	#define LogReturn(hr) (hr)
#endif


#ifdef _DEBUG
	std::wstring PidlToString(PCUIDLIST_RELATIVE pidl);
	std::wstring PidlToString(PCIDLIST_ABSOLUTE pidl);
	std::wstring PidlArrayToString(UINT cidl, PCUITEMID_CHILD_ARRAY aPidls);
	std::wstring IIDToString(const std::wstring &sIID);
	std::wstring IIDToString(const IID &iid);
	std::wstring SFGAOFToString(const SFGAOF *pfAttribs);
	std::wstring SHCONTFToString(const SHCONTF *pfAttribs);
	std::wstring SHGDNFToString(const SHGDNF *pfAttribs);
	std::wstring HRESULTToString(HRESULT hr);
#else
	#define PidlToString(_) DBG_NOTHING
	#define PidlArrayToString(_) DBG_NOTHING
	#define IIDToString(_) DBG_NOTHING
	#define SFGAOFToString(_) DBG_NOTHING
	#define SHCONTFToString(_) DBG_NOTHING
	#define SHGDNFToString(_) DBG_NOTHING
	#define HRESULTToString(_) DBG_NOTHING
#endif
