// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// RA2: a static of four zeroed ints (initializer at the start of the module) that several modules share.
#pragma once

struct GeoZeroVars {
	int Var1;
	int Var2;
	int Var3;
	int Var4;
	GeoZeroVars(void) { Var1 = 0; Var2 = 0; Var3 = 0; Var4 = 0; }
};
static GeoZeroVars GeoZero;
