#include "stdafx.h"  // MUST be included first

#include "PidlMgr.h"
#include <shtypes.h>


// Returned object should be freed with CoTaskMemFree
PITEMID_CHILD PidlMgr::Create(const IPidlData &Data) {
	UINT cbSizeItem = sizeof(SHITEMID) - sizeof(BYTE) + Data.GetSize();
	UINT cbSizeItemList = cbSizeItem + sizeof(SHITEMID);

	// Allocate memory for this SHITEMID plus the final null SHITEMID.
	auto pidlNew = (PITEMID_CHILD) CoTaskMemAlloc(cbSizeItemList);
	if (pidlNew == NULL) return NULL;

	// Fill the PIDL
	Data.CopyTo(&pidlNew->mkid.abID);
	pidlNew->mkid.cb = cbSizeItem;

	// A sentinel PIDL at the end of the list as the ITEMIDLIST spec ordains
	PUITEMID_CHILD pidlEnd = (PUITEMID_CHILD) ILNext(pidlNew);
	pidlEnd->mkid.cb = 0;
	pidlEnd->mkid.abID[0] = 0;

	return pidlNew;
}
