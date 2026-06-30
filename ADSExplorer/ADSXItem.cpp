#include "StdAfx.h"  // Precompiled header; include first

#include "ADSXItem.h"

namespace ADSX {


bool CItem::IsOwn(PCUIDLIST_RELATIVE pidlr) {
	return (
		// Not null of course
		pidlr != NULL &&
		// Is of expected size
		pidlr->mkid.cb == sizeof(ITEMIDLIST) + sizeof(CItem) - sizeof(BYTE) &&
		// Is a child PIDL as are all ADSX::CItems
		ILIsChild(pidlr) &&
		// Pronounces shibboleth correctly
		CItem::Get(static_cast<PCUITEMID_CHILD>(pidlr))->SIGNATURE == 'ADSX'
	);
}


CItem *CItem::Get(PUITEMID_CHILD pidlc) {
	return reinterpret_cast<CItem *>(&pidlc->mkid.abID);
}

const CItem *CItem::Get(PCUITEMID_CHILD pidlc) {
	return reinterpret_cast<const CItem *>(&pidlc->mkid.abID);
}

PADSXITEMID_CHILD CItem::NewPidl() {
	auto adsxpidlc = static_cast<PADSXITEMID_CHILD>(
		CoTaskMemAlloc(sizeof(ADSXITEMID_CHILD))
	);
	if (adsxpidlc == NULL) return NULL;
	new (reinterpret_cast<CItem *>(&adsxpidlc->mkid.abID)) CItem();
	return adsxpidlc;
}

}  // namespace ADSX
