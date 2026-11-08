/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

#include "coord.h"

class BeaconClass;

/*
**	RA2: the beacons that players place on the map in a multiplayer game.
*/
class BeaconManagerClass
{
	public:
		BeaconClass * FindBeaconAt(Coord coord);
};

extern BeaconManagerClass BeaconManager;
