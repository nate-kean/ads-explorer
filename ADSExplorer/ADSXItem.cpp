#include "StdAfx.h"  // Precompiled header; include first

#include "ADSXItem.h"

namespace ADSX {


bool CItem::IsOwn(PCUIDLIST_RELATIVE pidl) {
  return pidl != NULL &&
		 pidl->mkid.cb == sizeof(ITEMIDLIST) + sizeof(CItem) - sizeof(BYTE) &&
		 ILIsChild(pidl) &&
		 CItem::Get(static_cast<PCUITEMID_CHILD>(pidl))->SIGNATURE == 'ADSX';
}


CItem *CItem::Get(PUITEMID_CHILD pidl) {
	return reinterpret_cast<CItem *>(&pidl->mkid.abID);
}

const CItem *CItem::Get(PCUITEMID_CHILD pidl) {
	return reinterpret_cast<const CItem *>(&pidl->mkid.abID);
}

PADSXITEMID_CHILD CItem::NewPidl() {
	auto pidl = static_cast<PADSXITEMID_CHILD>(
		CoTaskMemAlloc(sizeof(ADSXITEMID_CHILD))
	);
	if (pidl == NULL) return NULL;
	new (CItem::Get(pidl)) CItem();
	return pidl;
}

}  // namespace ADSX
