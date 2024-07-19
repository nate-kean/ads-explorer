/*
 * Copyright (c) 2004 Pascal Hurni
 * Copyright (c) 2024 Nate Kean
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//==============================================================================
// Greatly inspired from several examples from: Microsoft, Michael Dunn, ...

#pragma once

#include "stdafx.h"  // MUST be included first

class IPidlData {
   public:
	//	IPidlData();
	//	IPidlData(const IPidlData &src);
	//	virtual IPidlData &operator=(const IPidlData &src);

	virtual ULONG GetSize() const = 0;
	virtual void CopyTo(void *pTarget) const = 0;
};

namespace PidlMgr {
	PITEMID_CHILD Create(const IPidlData &Data);
};
