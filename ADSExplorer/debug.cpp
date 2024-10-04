/**
 * 2024 Nate Kean
 */

#include "StdAfx.h"

#include "debug.h"

// #define _DEBUG
#ifdef _DEBUG
	#include "ADSXItem.h"
	#include "defer.h"
	#include "iids.h"
	#include <iomanip>
	#include <sstream>
#endif


CDebugStream::CDebugStream()
	: std::wostream(this)
	, m_initial_flags(this->flags()) {}
CDebugStream::~CDebugStream() {}


std::streamsize
CDebugStream::xsputn(const CDebugStream::Base::char_type* s, std::streamsize n) {
	// #define _DEBUG
	#ifdef _DEBUG
		OutputDebugStringW(s);
		this->flags(m_initial_flags);
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
		this->flags(m_initial_flags);
	#endif
	return 1;
}

#ifdef DBG_LOG_TO_FILE
	#include <fstream>
	std::wofstream g_DebugStream(
		L"G:\\Garlic\\Documents\\Code\\Visual Studio\\ADS Explorer Saga\\"
		L"ADS Explorer\\adsx.log",
		std::ios::out | std::ios::trunc
	);
#endif

#ifdef _DEBUG
	_Post_equal_to_(hr) HRESULT LogReturn(_In_ HRESULT hr) {
		LOG(L" -> " << HRESULTToString(hr));
		return hr;
	}

	std::wstring PidlToString(PCUIDLIST_RELATIVE pidl) {
		if (pidl == NULL) return L"<null>";
		std::wostringstream oss;
		bool first = true;
		for (; !ILIsEmpty(pidl); pidl = ILNext(pidl)) {
			if (!first) {
				oss << L"--";
			}
			if (CADSXItem::IsOwn(pidl)) {
				oss <<
					CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl))->pszName;
			} else {
				WCHAR tmp[16];
				swprintf_s(tmp, L"<unk-%02d>", pidl->mkid.cb);
				oss << tmp;
			}
			first = false;
		}
		return oss.str();
	}

	std::wstring PidlToString(PCIDLIST_ABSOLUTE pidl) {
		PWSTR pszPath = NULL;
		HRESULT hr = SHGetNameFromIDList(
			pidl,
			SIGDN_DESKTOPABSOLUTEPARSING,
			&pszPath
		);
		if (FAILED(hr)) return L"ERROR";
		defer({ CoTaskMemFree(pszPath); });
		std::wstring wstrPath(pszPath);
		if (wstrPath == L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}") {
			return L"[Desktop]";
		} else if (wstrPath == L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}") {
			return L"[Desktop\\ADS Explorer]";
		} else if (wstrPath == L"::{ED383D11-6797-4103-85EF-CBDB8DEB50E2}") {
			return L"[ADS Explorer]";
		}
		return wstrPath;
	}

	std::wstring PidlArrayToString(UINT cidl, PCUITEMID_CHILD_ARRAY aPidls) {
		std::wostringstream oss;
		oss << L"[";
		defer({ oss << L"]"; });
		for (UINT i = 0; i < cidl; i++) {
			oss << PidlToString(aPidls[i]);
			if (i < cidl - 1) {
				oss << L", ";
			}
		}
		return oss.str();
	}

	std::wstring IIDToString(const std::wstring &sIID) {
		auto search = iids.find(sIID);
		if (search != iids.end()) {
			return std::wstring(search->second);
		} else {
			return sIID;
		}
	}
	std::wstring IIDToString(const IID &iid) {
		LPOLESTR pszGUID = NULL;
		HRESULT hr = StringFromCLSID(iid, &pszGUID);
		if (FAILED(hr)) return L"Catastrophe! Failed to convert IID to string";
		defer({ CoTaskMemFree(pszGUID); });
		auto sIID = std::wstring(pszGUID);
		return IIDToString(sIID);
	}

	std::wstring SFGAOFToString(const SFGAOF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wostringstream oss;
		if (*pfAttribs & SFGAO_CANCOPY) oss << L"CANCOPY | ";
		if (*pfAttribs & SFGAO_CANMOVE) oss << L"CANMOVE | ";
		if (*pfAttribs & SFGAO_CANLINK) oss << L"CANLINK | ";
		if (*pfAttribs & SFGAO_STORAGE) oss << L"STORAGE | ";
		if (*pfAttribs & SFGAO_CANRENAME) oss << L"CANRENAME | ";
		if (*pfAttribs & SFGAO_CANDELETE) oss << L"CANDELETE | ";
		if (*pfAttribs & SFGAO_HASPROPSHEET) oss << L"HASPROPSHEET | ";
		if (*pfAttribs & SFGAO_DROPTARGET) oss << L"DROPTARGET | ";
		if (*pfAttribs & SFGAO_CAPABILITYMASK) oss << L"CAPABILITYMASK | ";
		if (*pfAttribs & SFGAO_PLACEHOLDER) oss << L"PLACEHOLDER | ";
		if (*pfAttribs & SFGAO_SYSTEM) oss << L"SYSTEM | ";
		if (*pfAttribs & SFGAO_ENCRYPTED) oss << L"ENCRYPTED | ";
		if (*pfAttribs & SFGAO_ISSLOW) oss << L"ISSLOW | ";
		if (*pfAttribs & SFGAO_GHOSTED) oss << L"GHOSTED | ";
		if (*pfAttribs & SFGAO_LINK) oss << L"LINK | ";
		if (*pfAttribs & SFGAO_SHARE) oss << L"SHARE | ";
		if (*pfAttribs & SFGAO_READONLY) oss << L"READONLY | ";
		if (*pfAttribs & SFGAO_HIDDEN) oss << L"HIDDEN | ";
		if (*pfAttribs & SFGAO_DISPLAYATTRMASK) oss << L"DISPLAYATTRMASK | ";
		if (*pfAttribs & SFGAO_FILESYSANCESTOR) oss << L"FILESYSANCESTOR | ";
		if (*pfAttribs & SFGAO_FOLDER) oss << L"FOLDER | ";
		if (*pfAttribs & SFGAO_FILESYSTEM) oss << L"FILESYSTEM | ";
		if (*pfAttribs & SFGAO_HASSUBFOLDER) oss << L"HASSUBFOLDER | ";
		if (*pfAttribs & SFGAO_CONTENTSMASK) oss << L"CONTENTSMASK | ";
		if (*pfAttribs & SFGAO_VALIDATE) oss << L"VALIDATE | ";
		if (*pfAttribs & SFGAO_REMOVABLE) oss << L"REMOVABLE | ";
		if (*pfAttribs & SFGAO_COMPRESSED) oss << L"COMPRESSED | ";
		if (*pfAttribs & SFGAO_BROWSABLE) oss << L"BROWSABLE | ";
		if (*pfAttribs & SFGAO_NONENUMERATED) oss << L"NONENUMERATED | ";
		if (*pfAttribs & SFGAO_NEWCONTENT) oss << L"NEWCONTENT | ";
		if (*pfAttribs & SFGAO_CANMONIKER) oss << L"CANMONIKER | ";
		if (*pfAttribs & SFGAO_HASSTORAGE) oss << L"HASSTORAGE | ";
		if (*pfAttribs & SFGAO_STREAM) oss << L"STREAM | ";
		if (*pfAttribs & SFGAO_STORAGEANCESTOR) oss << L"STORAGEANCESTOR | ";
		if (*pfAttribs & SFGAO_STORAGECAPMASK) oss << L"STORAGECAPMASK | ";
		if (*pfAttribs & SFGAO_PKEYSFGAOMASK) oss << L"PKEYSFGAOMASK | ";
		return oss.str();
	}

	std::wstring SHCONTFToString(const SHCONTF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wostringstream oss;
		if (*pfAttribs & SHCONTF_CHECKING_FOR_CHILDREN) oss << L"CHECKING_FOR_CHILDREN | ";
		if (*pfAttribs & SHCONTF_FOLDERS) oss << L"FOLDERS | ";
		if (*pfAttribs & SHCONTF_NONFOLDERS) oss << L"NONFOLDERS | ";
		if (*pfAttribs & SHCONTF_INCLUDEHIDDEN) oss << L"INCLUDEHIDDEN | ";
		if (*pfAttribs & SHCONTF_INIT_ON_FIRST_NEXT) oss << L"INIT_ON_FIRST_NEXT | ";
		if (*pfAttribs & SHCONTF_NETPRINTERSRCH) oss << L"NETPRINTERSRCH | ";
		if (*pfAttribs & SHCONTF_SHAREABLE) oss << L"SHAREABLE | ";
		if (*pfAttribs & SHCONTF_STORAGE) oss << L"STORAGE | ";
		if (*pfAttribs & SHCONTF_NAVIGATION_ENUM) oss << L"NAVIGATION_ENUM | ";
		if (*pfAttribs & SHCONTF_FASTITEMS) oss << L"FASTITEMS | ";
		if (*pfAttribs & SHCONTF_FLATLIST) oss << L"FLATLIST | ";
		if (*pfAttribs & SHCONTF_ENABLE_ASYNC) oss << L"ENABLE_ASYNC | ";
		if (*pfAttribs & SHCONTF_INCLUDESUPERHIDDEN) oss << L"INCLUDESUPERHIDDEN | ";
		return oss.str();
	}

	std::wstring SHGDNFToString(const SHGDNF *pfAttribs) {
		if (pfAttribs == NULL) return L"<null>";
		std::wostringstream oss;
		if (*pfAttribs & SHGDN_NORMAL) oss << L"NORMAL | ";
		if (*pfAttribs & SHGDN_INFOLDER) oss << L"INFOLDER | ";
		if (*pfAttribs & SHGDN_FOREDITING) oss << L"FOREDITING | ";
		if (*pfAttribs & SHGDN_FORADDRESSBAR) oss << L"FORADDRESSBAR | ";
		if (*pfAttribs & SHGDN_FORPARSING) oss << L"FORPARSING | ";
		return oss.str();
	}

	std::wstring HRESULTToString(HRESULT hr) {
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
#endif
