#include "stdafx.h"	 // MUST be included first

#include "PidlMgr.h"
#include <shtypes.h>


PITEMID_CHILD PidlMgr::Create(const IPidlData &Data) {
	// Total size of the PIDL, including SHITEMID
	// TODO(garlic-os): one byte larger than it needs to be?
	UINT cbTotalSize = sizeof(ITEMIDLIST) + Data.GetSize() - sizeof(BYTE);
	// UINT cbTotalSize = sizeof(ITEMIDLIST) + Data.GetSize();

	// Allocate memory for this SHITEMID plus the final null SHITEMID.
	auto pidlNew = (PITEMID_CHILD) CoTaskMemAlloc(cbTotalSize + sizeof(ITEMIDLIST));
	if (pidlNew) {
		// Prepares the PIDL to be filled with actual data
		pidlNew->mkid.cb = cbTotalSize;

		// Fill the PIDL
		Data.CopyTo(&pidlNew->mkid.abID);

		// Set an empty PIDL at the end
		LPITEMIDLIST pidlEnd = PidlMgr::GetNextItem(pidlNew);
		pidlEnd->mkid.cb = 0;
		pidlEnd->mkid.abID[0] = 0;
	}

	return pidlNew;
}

LPITEMIDLIST PidlMgr::Copy(LPCITEMIDLIST pidlSrc) {
	if (pidlSrc == NULL) return NULL;

	// Allocate memory for the new PIDL.
	UINT cbSize = PidlMgr::GetSize(pidlSrc);
	LPITEMIDLIST pidlTarget = (LPITEMIDLIST) CoTaskMemAlloc(cbSize);

	if (pidlTarget == NULL) return NULL;

	// Copy the source PIDL to the target PIDL.
	CopyMemory(pidlTarget, pidlSrc, cbSize);

	return pidlTarget;
}

LPITEMIDLIST PidlMgr::GetNextItem(LPCITEMIDLIST pidl) {
	ATLASSERT(pidl != NULL);
	if (!pidl) return NULL;
	return LPITEMIDLIST(LPBYTE(pidl) + pidl->mkid.cb);
}

PITEMID_CHILD PidlMgr::GetLastItem(LPCITEMIDLIST pidl) {
	PITEMID_CHILD pidlLast = NULL;

	// get the PIDL of the last item in the list
	while (pidl && pidl->mkid.cb) {
		pidlLast = (PITEMID_CHILD) pidl;
		pidl = PidlMgr::GetNextItem(pidl);
	}
	return pidlLast;
}

void PidlMgr::Delete(LPITEMIDLIST pidl) {
	if (pidl) {
		CoTaskMemFree(pidl);
	}
}

UINT PidlMgr::GetSize(LPCITEMIDLIST pidl) {
	UINT cbSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;

	ATLASSERT(pidl != NULL);
	if (!pidl) return 0;

	while (pidlTemp->mkid.cb != 0) {
		cbSize += pidlTemp->mkid.cb;
		pidlTemp = PidlMgr::GetNextItem(pidlTemp);
	}

	// add the size of the NULL terminating ITEMIDLIST
	cbSize += sizeof(ITEMIDLIST);

	return cbSize;
}

bool PidlMgr::IsChild(LPCITEMIDLIST pidl) {
	return PidlMgr::GetNextItem(pidl)->mkid.cb == 0;
}
