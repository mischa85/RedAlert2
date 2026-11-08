// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#pragma once

class ReferenceCounted
{
	public:
		ReferenceCounted(void) : Count(0) {}
		virtual ~ReferenceCounted(void);
		virtual bool Unknown_Virtual_4(int); // RA2: vtable +0x4; always false here, its name is not known
		virtual void Release(void); // RA2: drops a reference, deleting the object with the last one

		int Count;
};
