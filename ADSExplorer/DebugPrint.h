#pragma once

#include "stdafx.h"

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

  private:
	// --- Singleton ---
	CDebugStream();
	virtual ~CDebugStream();
	// --- /Singleton ---
};


// https://stackoverflow.com/a/3371577
// Usage: LOG(L"Hello" << ' ' << L"World!" << 1);
// #define _DEBUG
#ifdef _DEBUG
	#define LOG(psz) do { \
		CDebugStream::get_instance() << psz << std::endl; \
	} while(false)
#else
	#define LOG(...) do { } while (false)
#endif
