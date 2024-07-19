#pragma once

#include "stdafx.h"  // MUST be included first

#include <comutil.h>

#include "PidlMgr.h"

class CADSXItem : public IPidlData {
   public:
	const UINT32 SIGNATURE = 'ADSX';
	LONGLONG m_Filesize;
	_bstr_t m_Name;

	// Check if a PIDL contains a CADSXItem.
	// CADSXItems are always the last part of the PIDL (i.e. the child).
	// FUTURE: pseudofolders may open this up to be any relative PIDL.
	static bool IsOwn(PCUIDLIST_RELATIVE pidl);

	static CADSXItem *Get(PCUITEMID_CHILD pidl);

	// From IPidlData
	ULONG GetSize() const;
	void CopyTo(void *pTarget) const;
};
