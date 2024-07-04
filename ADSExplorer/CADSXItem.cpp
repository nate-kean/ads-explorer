#include "stdafx.h"  // MUST be included first

#include "CADSXItem.h"


ULONG CADSXItem::GetSize() {
	return sizeof(CADSXItem);
}


void CADSXItem::CopyTo(void *pTarget) {
	new (pTarget) CADSXItem();
	auto p = (CADSXItem *) pTarget;
	p->m_Path = m_Path;
	p->m_Name = m_Name;
}


bool CADSXItem::IsOwn(LPCITEMIDLIST pidl) {
  return pidl != NULL &&
		 pidl->mkid.cb == sizeof(CADSXItem) &&
		 pidl->mkid.abID[0] == 'A' &&
		 pidl->mkid.abID[1] == 'D' &&
		 pidl->mkid.abID[2] == 'S' &&
		 pidl->mkid.abID[3] == 'X';
}


CADSXItem *CADSXItem::Get(LPCITEMIDLIST pidl) {
	return (CADSXItem *) &pidl->mkid.abID;
}
