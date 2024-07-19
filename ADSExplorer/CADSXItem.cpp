#include "stdafx.h"  // MUST be included first

#include "CADSXItem.h"


CADSXItem::CADSXItem() : m_Path("PlaceholderPath") {}
bool CADSXItem::IsOwn(PCUIDLIST_RELATIVE pidl) {
  return pidl != NULL &&
		 pidl->mkid.cb == sizeof(ITEMIDLIST) + sizeof(CADSXItem) - sizeof(BYTE) &&
		 ILIsChild(pidl) &&
		 CADSXItem::Get((PCUITEMID_CHILD) pidl)->SIGNATURE == 'ADSX';
}


CADSXItem *CADSXItem::Get(PCUITEMID_CHILD pidl) {
	return (CADSXItem *) &pidl->mkid.abID;
}


ULONG CADSXItem::GetSize() const {
	return sizeof(CADSXItem);
}


void CADSXItem::CopyTo(void *pTarget) const {
	new (pTarget) CADSXItem();
	auto p = (CADSXItem *) pTarget;
	p->m_Filesize = m_Filesize;
	p->m_Path = m_Path;
	p->m_Name = m_Name;
}
