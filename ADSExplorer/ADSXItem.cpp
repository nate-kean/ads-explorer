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

PADSXITEMID_CHILD NewADSXPidl() {
	auto pidl = static_cast<PADSXITEMID_CHILD>(
		CoTaskMemAlloc(sizeof(ADSXITEMID_CHILD))
	);
	if (pidl == NULL) return NULL;
	new (CADSXItem::Get(pidl)) CADSXItem();
	return pidl;
}
