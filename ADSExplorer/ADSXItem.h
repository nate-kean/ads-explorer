#pragma once

#include "pch.h"  // Precompiled header; include first

namespace ADSX {

struct _ADSXITEMID_CHILD;
using PADSXITEMID_CHILD = _ADSXITEMID_CHILD *;

struct CItem {
	// Identifying marker a la file signatures
	UINT32 SIGNATURE = 'ADSX';

	// The actual content of the item
	LONGLONG llFilesize;
	PWSTR pszName;  // Allocated with CoTaskMemAlloc

	// Static: these are functions associated with ADSX::CItems but are not
	// kept on the objects; at runtime, it's just the above data members in a
	// CItem struct.

	// Check whether a PIDL of any type is a ADSXITEMID_CHILD, which contains a
	// ADSX::CItem.
	// ADSX::CItems are always the last part of the PIDL (i.e. the child).
	// FUTURE: pseudofolders may open this up to be any relative PIDL.
	static bool IsOwn(PCUIDLIST_RELATIVE pidl);

	// Helper functions for accessing the data member of a child item ID
	static CItem *Get(PUITEMID_CHILD pidl);
	static const CItem *Get(PCUITEMID_CHILD pidl);

	// Allocates and constructs a new child item ID holding an ADSX::CItem
	static PADSXITEMID_CHILD NewPidl();
};


// A new subclass of ITEMID_CHILD for our purposes:
// An ITEMID_CHILD that always holds an ADSX::CItem.
#include <pshpack1.h>
	typedef struct _ADSXITEMID {
		// This ITEMID has a known, fixed size!
		// So write it down here.
		USHORT cb = sizeof(USHORT) + sizeof(CItem);
		// Always ADSX::CItem.
		CItem abID;
		// Sentinel "null" item at the end that all properly-formed
		// ITEMIDLISTs, including children, have to have.
		// (So something that doesn't KNOW this is a child can seek
		// past for the next item and safely ("safely"...) find null.)
		USHORT cbNull = 0;
		BYTE abIDNull = NULL;
	} ADSXITEMID;
	typedef struct _ADSXITEMID_CHILD : ITEMID_CHILD {
		ADSXITEMID mkid;
	} ADSXITEMID_CHILD;
#include <poppack.h>

// Boilerplate for the rest of the variants to fit the Windows typedef naming scheme
typedef /* [wire_marshal] */ ADSXITEMID_CHILD *PADSXITEMID_CHILD;
typedef /* [wire_marshal] */ const ADSXITEMID_CHILD *PCADSXITEMID_CHILD;
typedef /* [wire_marshal] */ ADSXITEMID_CHILD __unaligned *PUADSXITEMID_CHILD;
typedef /* [wire_marshal] */ const ADSXITEMID_CHILD __unaligned *PCUADSXITEMID_CHILD;


}  // namespace ADSX
