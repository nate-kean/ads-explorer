#include "stdafx.h"  // MUST be included first

#include "ShellItems.h"
#include "ads.h"

// Return value of TRUE indicates success
// Return value of FALSE indicates failure; check GetLastError() for reason
BOOL ADS::Enumerate(
	/* [in] */ const BSTR path,
	/* [out][in] */ CADSXItemList *list
) {
	CADSXItem item;
	WIN32_FIND_STREAM_DATA first_fsd;

	HANDLE finder =
		FindFirstStreamW(path, FindStreamInfoStandard, &first_fsd, 0);
	if (finder == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	item.SetFilesize(first_fsd.StreamSize.QuadPart);
	item.SetName(first_fsd.cStreamName);
	list->Add(item);
	if (GetLastError() == ERROR_HANDLE_EOF) {
		FindClose(finder);
		return TRUE;
	}

	bool done = false;
	while (true) {
		WIN32_FIND_STREAM_DATA fsd;
		done = !FindNextStreamW(finder, &fsd);
		if (done) {
			if (GetLastError() != ERROR_HANDLE_EOF) {
				// Finder has stopped for some reason other than it found all
				// the streams
				FindClose(finder);
				return FALSE;
			}
			break;
		}
		item.SetFilesize(fsd.StreamSize.QuadPart);
		item.SetName(fsd.cStreamName);
		list->Add(item);
	}
	FindClose(finder);
	return TRUE;
}
