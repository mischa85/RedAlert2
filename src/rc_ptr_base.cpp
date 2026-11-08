// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "rcptr.h"

rc_ptr_base::rc_ptr_base(void) : Ptr(0)
{
}

rc_ptr_base::rc_ptr_base(rc_object * ptr) : Ptr(ptr)
{
	if (ptr) {
		ptr->RefCount++;
	}
}

rc_ptr_base::~rc_ptr_base(void)
{
	rc_object * p = Ptr;
	if (p) {
		Ptr = 0;
		p->Release();
	}
}

void rc_ptr_base::Assign(void * ptr)
{
	rc_object * p = (rc_object *)ptr;
	if (p != Ptr) {
		if (p) {
			p->RefCount++;
		}
		rc_object * old = Ptr;
		if (old) {
			Ptr = 0;
			old->Release();
		}
		Ptr = p;
	}
}

rc_ptr_base::rc_ptr_base(rc_ptr_base const & that) : Ptr(that.Ptr)
{
	if (Ptr) {
		Ptr->RefCount++;
	}
}
