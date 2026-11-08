// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "mssfx.h"
#include <string.h>
#include <stdlib.h>

int __fastcall AudioDeviceAvailable(void); // RA2: whether the audio library has a device

MSSfx::MSSfx(char const * name, char const * file_name) :
	Name(NULL),
	Sample(NULL),
	Loaded(false)
{
	if (AudioDeviceAvailable()) {
		Name = strdup(name);
	}
}

MSSfx::~MSSfx(void)
{
	if (Name != NULL) {
		free(Name);
	}
}

void MSSfx::Play(int volume)
{
}
