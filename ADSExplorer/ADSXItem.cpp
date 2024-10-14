#include "StdAfx.h"  // Precompiled header; include first

#include "ADSXItem.h"


bool CADSXItem::IsOwn(PCUIDLIST_RELATIVE pidl) {
  return pidl != NULL &&
		 pidl->mkid.cb == sizeof(ITEMIDLIST) + sizeof(CADSXItem) - sizeof(BYTE) &&
		 ILIsChild(pidl) &&
		 CADSXItem::Get(static_cast<PCUITEMID_CHILD>(pidl))->SIGNATURE == 'ADSX';
}


CADSXItem *CADSXItem::Get(PUITEMID_CHILD pidl) {
	return reinterpret_cast<CADSXItem *>(&pidl->mkid.abID);
}

const CADSXItem *CADSXItem::Get(PCUITEMID_CHILD pidl) {
	return reinterpret_cast<const CADSXItem *>(&pidl->mkid.abID);
}


/**
 * Allocate a PIDL (PITEMID_CHILD) and shallow copy into its mkid this item.
 * @post: The caller owns the return value and must free it with CoTaskMemFree.
 */
PITEMID_CHILD CADSXItem::ToPidl() const {
	// The PIDL is manually allocated, as opposed to using `new`,
	// because COM requires memory to be allocated with CoTaskMemAlloc.
	const UINT cbItem = FIELD_OFFSET(SHITEMID, abID[sizeof(CADSXItem)]);
	const UINT cbItemList = cbItem + sizeof(SHITEMID);

	// Allocate memory for this SHITEMID plus the final null SHITEMID.
	auto pidlNew = static_cast<PITEMID_CHILD>(CoTaskMemAlloc(cbItemList));
	if (pidlNew == NULL) return NULL;

	// Put the data object in the PIDL
	new (pidlNew->mkid.abID) CADSXItem();
	// shallow copy (and that's enough since CADSXItem holds no raw pointers)
	(CADSXItem &) pidlNew->mkid.abID = *this;
	pidlNew->mkid.cb = cbItem;

	// A sentinel PIDL at the end of the list as the ITEMIDLIST spec ordains
	PUITEMID_CHILD pidlEnd = static_cast<PUITEMID_CHILD>(ILNext(pidlNew));
	pidlEnd->mkid.abID[0] = 0;
	pidlEnd->mkid.cb = 0;

	return pidlNew;
}
