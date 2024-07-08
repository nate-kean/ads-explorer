#pragma once

#include "stdafx.h"  // MUST be included first

#include <comutil.h>

#include "PidlMgr.h"

class CADSXItem : public IPidlData {
   public:
	enum { SIGNATURE = 'ADSX' };

	LONGLONG m_Filesize;
	_bstr_t m_Path;
	_bstr_t m_Name;

	// Check if a PIDL contains a CADSXItem
	static bool IsOwn(LPCITEMIDLIST pidl);

	static CADSXItem *Get(LPCITEMIDLIST pidl);

	// From IPidlData
	ULONG GetSize() const;
	void CopyTo(void *pTarget) const;
};
