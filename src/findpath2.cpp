// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "rect.h"
#include "cell.h"
#include "bullettype.h"
#include "weapon.h"
#include "warhead.h"
#include "house.h"
#include "overtype.h"
#include "globals.h"
#include "_rules.h"
#include "rules.h"
#include "trajhelp.h"
#include "_map.h"
#include "inline.h"

#include <math.h>

CellClass * TrajectoryHelper::FindFirstObstacle(Coord const & source, Coord const & target, BulletTypeClass const * type, HouseClass const * owner)
{
	if (type->SubjectToCliffs || type->SubjectToWalls) {
		Cell scell = source.As_Cell();
		Cell tcell = target.As_Cell();
		int dx = abs(scell.X - tcell.X);
		int dy = abs(scell.Y - tcell.Y);
		int steps = dx;
		if (dx <= dy) {
			steps = dy;
		}
		Coord delta = target - source;
		Coord step;
		if (steps == 0) {
			step = Coord(0, 0, 0);
		} else {
			step = Coord(delta.X / steps, delta.Y / steps, delta.Z / steps);
		}
		CellClass * start = &Map[scell];
		CellClass * end = &Map[tcell];
		CellClass * prev = start;
		for (int i = 0; i < steps; i++) {
			Coord offset = step * i;
			Coord current = source + offset;
			CellClass * obstacle = GetObstacle(start, end, prev, current, type, owner);
			if (obstacle != NULL) {
				return(obstacle);
			}
			prev = &Map[current.As_Cell()];
		}
	}
	return(NULL);
}

CellClass * TrajectoryHelper::FindFirstImpenetrableObstacle(Coord const & source, Coord const & target, WeaponTypeClass const * weapon, HouseClass const * owner)
{
	BulletTypeClass const * bullet = weapon->Bullet;
	CellClass * cell = FindFirstObstacle(source, target, bullet, owner);
	if (cell != NULL && cell->Has_Wall_Or_Gate() && weapon->WarheadPtr->IsWallDestroyer) {
		return(NULL);
	}
	return(cell);
}

CellClass * TrajectoryHelper::GetObstacle(CellClass const * source, CellClass const * target, CellClass const * bullet, Coord current, BulletTypeClass const * type, HouseClass const * owner)
{
	Cell cell = current.As_Cell();
	CellClass * first = &Map[cell];
	if ((type->SubjectToCliffs && IsCliffHit(source, bullet, first)) ||
		(type->SubjectToWalls && IsWallHit(source, first, target, owner))) {
		Coord center = first->Center_Coord();
		if (current.Distance_To(center) > CELL_LEPTON_W / 3) {
			Coord from = source->Center_Coord();
			Coord to = target->Center_Coord();
			int dx = abs(from.X - to.X);
			int dy = abs(from.Y - to.Y);
			int ox = abs(current.X - center.X);
			int oy = abs(current.Y - center.Y);
			if ((dx > dy && oy > ox) || (dy > dx && ox > oy)) {
				CellClass * check = &Map[cell];
				if (type->SubjectToCliffs && IsCliffHit(source, bullet, check)) {
					return(first);
				}
				if (type->SubjectToWalls && IsWallHit(source, check, target, owner)) {
					return(first);
				}
				return(NULL);
			}
			return(first);
		}
		return(first);
	}
	return(NULL);
}

bool TrajectoryHelper::IsCliffHit(CellClass const * source, CellClass const * before, CellClass const * after)
{
	return(after->GetLevel() - before->GetLevel() >= 4 && after->GetLevel() - source->GetLevel() > 0);
}

bool TrajectoryHelper::IsWallHit(CellClass const * source, CellClass const * check, CellClass const * target, HouseClass const * owner)
{
	bool iswall = check->Overlay != OVERLAY_NONE && OverlayTypes[check->Overlay]->IsWall;
	if (check != target && iswall && source->Height <= target->Height) {
		if (Rule->AlliedWallTransparency && Houses[check->Owner] != NULL && Houses[check->Owner]->Is_Ally(owner)) {
			return(false);
		}
		return(true);
	}
	return(false);
}
