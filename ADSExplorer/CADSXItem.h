#pragma once

#include "stdafx.h"  // MUST be included first

#include <comutil.h>


struct CADSXItem {
	UINT32 SIGNATURE = 'ADSX';
	LONGLONG m_Filesize;
	_bstr_t m_Name;

	// Check if a PIDL contains a CADSXItem.
	// CADSXItems are always the last part of the PIDL (i.e. the child).
	// FUTURE: pseudofolders may open this up to be any relative PIDL.
	static bool IsOwn(PCUIDLIST_RELATIVE pidl);

	static CADSXItem *Get(PCUITEMID_CHILD pidl);

	// Create a new one-item-long PIDL containing this item in the abID.
	// Ownership of the return value belongs to the caller.
	// Return value must be freed with CoTaskMemFree.
	PITEMID_CHILD ToPidl() const;
};
