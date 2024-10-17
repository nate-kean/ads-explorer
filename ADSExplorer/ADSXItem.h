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
};


#include <pshpack1.h>
	typedef struct _ADSXITEMID {
		USHORT cb = FIELD_OFFSET(_ADSXITEMID, abIDNull);
		CADSXItem abID;
		USHORT cbNull = 0;
		BYTE abIDNull = NULL;
	} ADSXITEMID;
	typedef struct _ADSXITEMID_CHILD: ITEMID_CHILD {
		ADSXITEMID mkid;
	} ADSXITEMID_CHILD;
#include <poppack.h>

typedef /* [wire_marshal] */ ADSXITEMID_CHILD *PADSXITEMID_CHILD;
typedef /* [wire_marshal] */ const ADSXITEMID_CHILD *PCADSXITEMID_CHILD;
typedef /* [wire_marshal] */ ADSXITEMID_CHILD __unaligned *PUADSXITEMID_CHILD;
typedef /* [wire_marshal] */ const ADSXITEMID_CHILD __unaligned *PCUADSXITEMID_CHILD;

PADSXITEMID_CHILD NewADSXPidl();
