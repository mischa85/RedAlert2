// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#define RA2_GEOMETRY_H // geometry.h follows the includes
#include "always.h"
#include <math.h>
#include "rect.h"

#include "mpsiege.h"
#include "session.h"
#include <windows.h>
#undef LoadString
#include <stddef.h>
#include "stringtable.h"

const Rect RECT_NONE_2(0, 0, 0, 0);

#include "geometry.h"

MultiplayerSiegeDefenderTeam::MultiplayerSiegeDefenderTeam(void) :
	MultiplayerTeam(StringTable::LoadString("MP:DefenderTeam", NULL, "D:\\Ra2\\MPSiegeTeam.cpp", 6), 1)
{
}

MultiplayerSiegeDefenderTeam::~MultiplayerSiegeDefenderTeam(void)
{
}

// RA2: true when no player other than self is on the defender team
bool __stdcall Is_Last_Defender(int self)
{
	for (int i = 0; i < Session.Players.Count(); i++) {
		if (self != i && Session.Players[i]->Slot.SpectatorFlag == 1) {
			return(false);
		}
	}
	return(true);
}

MultiplayerSiegeAttackerTeam::MultiplayerSiegeAttackerTeam(void) :
	MultiplayerTeam(StringTable::LoadString("MP:AttackerTeam", NULL, "D:\\Ra2\\MPSiegeTeam.cpp", 22), 1)
{
}

MultiplayerSiegeAttackerTeam::~MultiplayerSiegeAttackerTeam(void)
{
}

