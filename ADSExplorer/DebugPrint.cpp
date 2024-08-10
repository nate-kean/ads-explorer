#include "stdafx.h"

#include "DebugPrint.h"

#include "defer.h"


// #define _DEBUG
#ifdef _DEBUG
void DebugPrint(LPCTSTR fmt, ...) {
    va_list args;
    va_start(args, fmt);

    size_t count = _vsctprintf(fmt, args);
    if (count <= 0) {
        OutputDebugString(_T("Invalid format string\n"));
        va_end(args);
        return;
    }
    size_t size = count * sizeof(TCHAR) + sizeof(TCHAR);

    TCHAR *buffer = new TCHAR[size];
    if (buffer == NULL) {
        OutputDebugString(_T("Out of memory\n"));
        va_end(args);
        return;
    }

    _vsntprintf_s(buffer, size, count, fmt, args);
    OutputDebugString(buffer);

    delete[] buffer;
    va_end(args);
}
#endif
