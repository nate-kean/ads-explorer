#pragma once

#include "ShellItems.h"

namespace ADS {
	// void copy(LPCWSTR src_path, LPCWSTR dst_path);
	// void create(LPCWSTR path);
	// void remove(LPCWSTR path);
	// void move(LPCWSTR src_path, LPCWSTR dst_path);
	// void print(LPCWSTR path);
	// std::string read(LPCWSTR path);
	// bool exists(LPCWSTR path);
	BOOL Enumerate(const BSTR path, CADSXItemList *list);
}
