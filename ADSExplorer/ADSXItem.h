#pragma once

#include "StdAfx.h"  // Precompiled header; include first

struct CADSXItem {
	UINT32 SIGNATURE = 'ADSX';
	LONGLONG llFilesize;
	LPWSTR pszName;  // Allocated with CoTaskMemAlloc

	// Check if a PIDL contains a CADSXItem.
	// CADSXItems are always the last part of the PIDL (i.e. the child).
	// FUTURE: pseudofolders may open this up to be any relative PIDL.
	static bool IsOwn(PCUIDLIST_RELATIVE pidl);

	static CADSXItem *Get(PUITEMID_CHILD pidl);
	static const CADSXItem *Get(PCUITEMID_CHILD pidl);

	// Create a new one-item-long PIDL containing this item in the abID.
	// Ownership of the return value belongs to the caller.
	// Return value must be freed with CoTaskMemFree.
	PITEMID_CHILD ToPidl() const;
};
