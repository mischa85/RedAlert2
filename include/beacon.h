/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

#include "coord.h"
#include "rect.h"

class BeaconClass;
class Surface;

/*
**	RA2: the beacons that players place on the map in a multiplayer game.
*/
class BeaconManagerClass
{
	public:
		BeaconClass * FindBeaconAt(Coord coord);
		void Draw_On_Radar(Surface * surface, Rect bounds); // RA2: 0x0042EED0; marks the beacons of allies on the radar
		bool Is_Flashing(void); // RA2: 0x0042EFD0; a visible beacon of an ally is in its lit phase
};

extern BeaconManagerClass BeaconManager;
