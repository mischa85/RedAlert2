/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/COMBAT.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : COMBAT.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 19, 1994                                           *
 *                                                                                             *
 *                  Last Update : July 26, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Combat_Anim -- Determines explosion animation to play.                                    *
 *   Explosion_Damage -- Inflict an explosion damage affect.                                   *
 *   Modify_Damage -- Adjusts damage to reflect the nature of the target.                      *
 *   Wide_Area_Damage -- Apply wide area damage to the map.                                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "combat.h"

#include "_map.h"
#include "_rtti.h"
#include "_rules.h"
#include "_tactica.h"
#include "aircraft.h"
#include "anim.h"
#include "bound.h"
#include "building.h"
#include "builtype.h"
#include "ccrand.h"
#include "cell.h"
#include "globals.h"
#include "incdec.h"
#include "infantry.h"
#include "infatype.h"
#include "inline.h"
#include "isotype.h"
#include "mouse.h"
#include "overtype.h"
#include "ovrlight.h"
#include "partsys.h"
#include "psystype.h"
#include "rules.h"
#include "scenario.h"
#include "stimer.h"
#include "tactical.h"
#include "team.h"
#include "teamtype.h"
#include "tiberium.h"
#include "unit.h"
#include "unittype.h"
#include "vanim.h"
#include "vein.h"
#include "warhead.h"

#include "mph.hh"

#include <windef.h>
inline static int Explosion_Distance(Coord const & coord1, Coord const & coord2); // RA2: defined after its first caller

namespace FPSCounter { extern unsigned CurrentFrameRate; }
namespace Detail { unsigned GetMinFrameRate(void); }



/***********************************************************************************************
 * Modify_Damage -- Adjusts damage to reflect the nature of the target.                        *
 *                                                                                             *
 *    This routine is the core of combat tactics. It implements the                            *
 *    affect various armor types have against various weapon types. By                         *
 *    careful exploitation of this table, tactical advantage can be                            *
 *    obtained.                                                                                *
 *                                                                                             *
 * INPUT:   damage   -- The damage points to process.                                          *
 *                                                                                             *
 *          warhead  -- The source of the damage points.                                       *
 *                                                                                             *
 *          armor    -- The type of armor defending against the damage.                        *
 *                                                                                             *
 *          distance -- The distance (in leptons) from the source of the damage.               *
 *                                                                                             *
 * OUTPUT:  Returns with the adjusted damage points to inflict upon the                        *
 *          target.                                                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/16/1994 JLB : Created.                                                                 *
 *   04/17/1994 JLB : Always does a minimum of damage.                                         *
 *   01/01/1995 JLB : Takes into account distance from damage source.                          *
 *   04/11/1996 JLB : Changed damage fall-off formula for less damage fall-off.                *
 *=============================================================================================*/
int Modify_Damage(int damage, WarheadTypeClass const * warhead, ArmorType armor, int distance)
{
	if (!damage || Scen->Special.IsInert || warhead == NULL) return(0);

	if (damage < 0) {
		if (distance < 8) return(damage); // RA2: Modify_Damage at 0x47E1C0 matches only so
		return(0);
	}

	float fdamage = (float)damage; // RA2: as above
	float fmin = fdamage * warhead->PercentAtMax; // RA2: as above
	int spread = (int)(warhead->SpreadFactor * 256.0f); // RA2: as above
	if (fmin != fdamage && spread) { // RA2: as above
		damage = (int)((fdamage - fmin) * (spread - distance) / spread + fmin); // RA2: as above
	}

	damage = MAX(damage, 0); // RA2: as above
	damage = (int)(damage * warhead->Modifier[armor]); // RA2: as above
	return(MIN(damage, Rule->MaxDamage)); // RA2: as above
} // RA2: Modify_Damage at 0x47E1C0 has no Bound, MAX, Rule and 3 more


/// <summary>
/// Handles the chain reaction of a tiberium field.
/// This routine is called when a cell of chain reactive growth is disturbed. The tiberium
/// there goes up, damaging whatever is standing in the blast, and the neighboring growth
/// is set off in turn so that the reaction ripples across the field.
/// </summary>
/// <param name="cell">The cell whose growth should be set off.</param>
void Chain_Reaction_Damage(Cell const & cell)
{
	// RA2: empty in RA2 (a bare ret at 0x47E2B0)
}


/***********************************************************************************************
 * Explosion_Damage -- Inflict an explosion damage affect.                                     *
 *                                                                                             *
 *    Processes the collateral damage affects typically caused by an                           *
 *    explosion.                                                                               *
 *                                                                                             *
 * INPUT:   coord    -- The coordinate of ground zero.                                         *
 *                                                                                             *
 *          strength -- Raw damage points at ground zero.                                      *
 *                                                                                             *
 *          source   -- Source of the explosion (who is responsible).                          *
 *                                                                                             *
 *          warhead  -- The kind of explosion to process.                                      *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine can consume some time and will affect the AI                       *
 *             of nearby enemy units (possibly).                                               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/16/1991 JLB : Created.                                                                 *
 *   11/30/1991 JLB : Uses coordinate system.                                                  *
 *   12/27/1991 JLB : Radius of explosion damage effect.                                       *
 *   04/13/1994 JLB : Streamlined.                                                             *
 *   04/16/1994 JLB : Warhead damage type modifier.                                            *
 *   04/17/1994 JLB : Cleaned up.                                                              *
 *   06/20/1994 JLB : Uses object pointers to distribute damage.                               *
 *   06/20/1994 JLB : Source is a pointer.                                                     *
 *   06/18/1996 JLB : Strength could be negative for healing effects.                          *
 *=============================================================================================*/
void Explosion_Damage(Coord const & coord, int strength, TechnoClass * source, WarheadTypeClass const * warhead, bool dochainreaction)
{
	Cell								cell;		// Cell number under explosion.
	ObjectClass *						object;		// Working object pointer.
	DynamicVectorClass<ObjectClass *>	objects;	// Maximum number of objects that can be damaged.
	int									distance;	// Distance to unit.
	int									range;		// Damage effect radius.

	if (Scen->Special.IsInert || warhead == NULL) return;


	range = CELL_LEPTON_W + (CELL_LEPTON_W >> 1); // RA2: no IsWebby (not in RA2's layout)
	cell = coord.As_Cell();

	CellClass * cellptr = &Map[cell];

	if (Map.Get_Height_GL(cell) < coord.Z) {
		int index;

		for (index = 0; index < Aircraft.Count(); index++) {
			AircraftClass * aircraft = Aircraft[index];

			if (aircraft->IsActive) {
				if (aircraft->IsDown && aircraft->Strength > 0) {
					distance = coord.Distance_To(aircraft->PositionCoord);
					if (distance < CELL_LEPTON_W) {
						objects.Delete(aircraft);
						objects.Add(aircraft);
					}
				}
			}
		}

		for (index = 0; index < Infantry.Count(); index++) {
			InfantryClass * infantry = Infantry[index];

		} // RA2: no IsJumpJet (not in RA2's layout)

		for (index = 0; index < Units.Count(); index++) {
			UnitClass * unit = Units[index];

		} // RA2: no IsJellyfish (not in RA2's layout)

	}

	CellClass *cptr = cellptr;
	ObjectClass *impacto;
	bool isbridge = false;
	if (cptr->IsUnderBridge && coord.Z > Map.Get_Height_GL(coord) + BRIDGE_LEPTON_HEIGHT / 2) {
		impacto = cptr->Cell_Bridge_Occupier();
		isbridge = true;
	} else {
		impacto = cptr->Cell_Occupier(false);
	}

	/*
	**	Fill the list of unit IDs that will have damage
	**	assessed upon them. The units can be lifted from
	**	the cell data directly.
	*/
	for (FacingType i = FACING_NONE; i < FACING_COUNT; i++) {

		/*
		**	Fetch a pointer to the cell to examine. This is either
		**	an adjacent cell or the center cell. Damage never spills
		**	further than one cell away.
		*/
		if (i != FACING_NONE) {
			cptr = &Map[cell].Adjacent_Cell(i);
		}

		/*
		**	Add all objects in this cell to the list of objects to possibly apply
		**	damage to. The list stops building when the object pointer list becomes
		**	full.  Do not include overlapping objects; selection state can affect
		**	the overlappers, and this causes multiplayer games to go out of sync.
		*/
		object = cptr->Cell_Occupier(isbridge);
		while (object) {
			if (object != source) {
				if (object->RTTI != RTTI_UNIT || !Scen->Special.IsHarvesterImmune || !Rule->HarvesterUnit.Is_In_List((UnitTypeClass *)object->Class_Of())) {
					objects.Delete(object);
					objects.Add(object);
				}
			}
			object = object->Next;
		}

		if (cptr->Overlay != OVERLAY_NONE) {
			if (OverlayTypes[cptr->Overlay]->IsVeinholeMonster) {
				VeinholeMonsterClass * veinhole = VeinholeMonsterClass::Get_Monster_At(cell);
				if (veinhole != NULL) {
					objects.Add(veinhole);
				}
			}
		}
	}

	/*
	**	Sweep through the units to be damaged and damage them. When damaging
	**	buildings, consider a hit on any cell the building occupies as if it
	**	were a direct hit on the building's center.
	*/
	for (int index = 0; index < objects.Count(); index++) {
		object = objects[index];

		if (object->IsActive && (object->RTTI != RTTI_BUILDING || !((BuildingClass *)object)->Class->IsInvisibleInGame)) { // RA2: no IsToDamage (TibSun's use does not compile against RA2's declaration)
			if (object->RTTI == RTTI_BUILDING && impacto == object) {
				distance = 0;
			} else {
				distance = Explosion_Distance(coord, object->Target_Coord());
				if (object->RTTI == RTTI_AIRCRAFT) {
					distance /= 2;
				}
			}
			if (object->Strength > 0 && object->IsDown && !object->IsInLimbo && distance < range) {
				int damage = strength;
			} // RA2: no IonStormWarhead (not in RA2's layout)
		}
	}

	double rocking_force = MIN(strength * 0.01, 4.0);
	if (warhead->IsRocker && rocking_force > 0.3) {
		for (int x = cell.X - 3; x <= cell.X + 3; x++) {
			for (int y = cell.Y - 3; y <= cell.Y + 3; y++) {
				object = isbridge ? Map[Cell(x, y)].Cell_Occupier(true) : Map[Cell(x, y)].Cell_Occupier(false);

				while (object) {
					TechnoClass * techno = Dynamic_Cast<TechnoClass *>(object);
					if (techno != NULL) {
						if (Cell(x, y) == cell && source) {
							Coord tcoord = techno->PositionCoord;

							Coord scoord = source->PositionCoord;
							TPoint3D<float> rockdirf((float)(scoord.X - tcoord.X), (float)(scoord.Y - tcoord.Y), (float)(scoord.Z - tcoord.Z));
							rockdirf = rockdirf.Normalize() * 10;

							tcoord += Coord(rockdirf.X, rockdirf.Y, rockdirf.Z);
							techno->Rock(tcoord, rocking_force);
						} else {
							techno->Rock(coord, rocking_force);
						}
					}
					object = object->Next;
				}
			}
		}
	}

	/*
	**	If there is a wall present at this location, it may be destroyed. Check to
	**	make sure that the warhead is of the kind that can destroy walls.
	*/
	cellptr = &Map[cell];
	if (cellptr->Overlay != OVERLAY_NONE) {
		OverlayTypeClass const * optr = OverlayTypes[cellptr->Overlay];

		if (optr->IsChainReaction && (!optr->IsTiberium || warhead->IsTiberiumDestroyer) && dochainreaction) {
			Chain_Reaction_Damage(cell);
			cellptr->Reduce_Tiberium(strength / 10);
		}

		if (optr->IsWall && (warhead->IsWallDestroyer || (warhead->IsWoodDestroyer && optr->Armor == ARMOR_WOOD))) {
			cellptr->Reduce_Wall(strength);
		}

		if (cellptr->Overlay == OVERLAY_NONE) {
			TechnoClass::Remove_Target(cellptr);
		}
	}

	/*
	**	If there is a bridge at this location, then it may be destroyed by the
	**	combat damage.
	*/
	bool ion_cannon = warhead == Rule->IonCannonWarhead;
	if (Scen->Special.IsDestroyBridges && warhead->IsWallDestroyer) {

		IsometricTileType ittype1 = IsometricTileType(cellptr->ITType - IsometricTileTypeClass::BridgeSet + 1);
		CellClass * bridge_deck = cellptr->Get_Bridge_Deck_CellClass();

		if (bridge_deck && bridge_deck->Is_Overlay_Bridge() ||
			ittype1 == IsometricTileTypeClass::BridgeMiddle1 + 0 || ittype1 == IsometricTileTypeClass::BridgeMiddle1 + 1 ||
			ittype1 == IsometricTileTypeClass::BridgeMiddle1 + 2 || ittype1 == IsometricTileTypeClass::BridgeMiddle1 + 3 ||
			ittype1 == IsometricTileTypeClass::BridgeMiddle2 + 0 || ittype1 == IsometricTileTypeClass::BridgeMiddle2 + 1 ||
			ittype1 == IsometricTileTypeClass::BridgeMiddle2 + 2 || ittype1 == IsometricTileTypeClass::BridgeMiddle2 + 3) {
			if (!cellptr->IsUnderBridge || coord.Z <= BRIDGE_LEPTON_HEIGHT + LEVEL_LEPTON_H * (cellptr->Height + 1) && coord.Z > BRIDGE_LEPTON_HEIGHT + LEVEL_LEPTON_H * (cellptr->Height - 2)) {
				if (warhead->IsWallDestroyer && (warhead == Rule->IonCannonWarhead || Random_Pick(1, Rule->BridgeStrength) < strength)) {
					bool bridge_damaged = false;
					int tries=4;
					do {
						bridge_damaged = Map.Damage_Bridge(cell);
						tries--;
						if (bridge_damaged) {
							break;
						}
					} while (ion_cannon && tries > 0);

					if (bridge_damaged) {
						TechnoClass::Remove_Target(cellptr);
					}
					Point2D point;
					TacticalMap->Coord_To_Pixel(coord, point);
					TacticalMap->Register_Dirty_Area(Rect(point.X - 128, point.Y - 128, 256, 256), false);
				}
			}
		}

		IsometricTileType ittype2 = IsometricTileType(cellptr->ITType - IsometricTileTypeClass::TrainBridgeSet + 1);

		if (bridge_deck && bridge_deck->Is_Overlay_Rail_Bridge() ||
			ittype2 == IsometricTileTypeClass::BridgeMiddle1 + 0 || ittype2 == IsometricTileTypeClass::BridgeMiddle1 + 1 ||
			ittype2 == IsometricTileTypeClass::BridgeMiddle1 + 2 || ittype2 == IsometricTileTypeClass::BridgeMiddle1 + 3 ||
			ittype2 == IsometricTileTypeClass::BridgeMiddle2 + 0 || ittype2 == IsometricTileTypeClass::BridgeMiddle2 + 1 ||
			ittype2 == IsometricTileTypeClass::BridgeMiddle2 + 2 || ittype2 == IsometricTileTypeClass::BridgeMiddle2 + 3) {
			if (!cellptr->IsUnderBridge || coord.Z <= BRIDGE_LEPTON_HEIGHT + LEVEL_LEPTON_H * (cellptr->Height + 1) && coord.Z > BRIDGE_LEPTON_HEIGHT + LEVEL_LEPTON_H * (cellptr->Height - 2)) {
				if (warhead->IsWallDestroyer && (warhead == Rule->IonCannonWarhead || Random_Pick(1, Rule->BridgeStrength) < strength)) {
					bool bridge_damaged = false;
					int tries=4;
					do {
						bridge_damaged = Map.Damage_Bridge(cell);
						tries--;
						if (bridge_damaged) {
							break;
						}
					} while (ion_cannon && tries > 0);

					if (bridge_damaged) {
						TechnoClass::Remove_Target(cellptr);
					}
					Point2D point;
					TacticalMap->Coord_To_Pixel(coord, point);
					TacticalMap->Register_Dirty_Area(Rect(point.X - 96, point.Y - 96, 192, 192), false);
				}
			}
		}

		if (cellptr->Overlay >= OVERLAY_LOWBRIDGE_01 && cellptr->Overlay <= OVERLAY_LOWBRIDGE_26) {
			if (warhead == Rule->IonCannonWarhead || Random_Pick(1, Rule->BridgeStrength) < strength) {
				bool destroyed = Map.Damage_Low_Bridge(cell);
				Map.Damage_Low_Bridge(cell);
				if (destroyed) {
					TechnoClass::Remove_Target(cellptr);
				}
			}
		}
	}

	if (cellptr->Overlay != OVERLAY_NONE && OverlayTypes[cellptr->Overlay]->IsExplosive) {
		cellptr->Register_For_Redraw();
		cellptr->Overlay = OVERLAY_NONE;
		cellptr->Recalc_Attributes();
		Map.Update_Cell_Zone(cellptr->CellID);
		Map.Update_Cell_Subzones(cellptr->CellID);
		TechnoClass::Remove_Target(cellptr);

		new AnimClass(Rule->BarrelExplode, coord);
		Explosion_Damage(coord, Rule->AmmoCrateDamage, NULL, Rule->C4Warhead, true);
		for (int i = 0; i < Rule->BarrelDebris.Count(); i++) {
			if (Percent_Chance(15)) {
				new VoxelAnimClass(Rule->BarrelDebris[i], coord);
				break;
			}
		}
		if (Percent_Chance(25)) {
			ParticleSystemClass * psys = new ParticleSystemClass(Rule->BarrelParticle, coord);
			psys->Spawn_Held_Particle(coord, coord);
		}

		static FacingType _part_facings[] = { FACING_N, FACING_E, FACING_S, FACING_W };

		for (int f = 0; f < ARRAY_SIZE(_part_facings); f++) {
			Coord adjacent = Adjacent_Coord_With_Height(coord, _part_facings[f]);
			if (Map[adjacent].Overlay != OVERLAY_NONE && OverlayTypes[Map[adjacent].Overlay]->IsExplosive) {
				new AnimClass(AnimTypes[AnimTypeClass::From_Name("FIRE3")], adjacent, Random_Pick(1, 3) + 3);
			}
		}
	}

	if (strength > warhead->DeformThreshhold) {
		if (Percent_Chance((strength * 0.01) * warhead->Deform * 100.0) && !(Map[coord].IsUnderBridge && coord.Z >= BRIDGE_LEPTON_HEIGHT + Map.Get_Height_GL(coord))) {
			Map.Deform_Terrain(coord.As_Cell(), false);
		}
	}

	if (cellptr->Is_Tile_Destroyable_Cliff()) {
		if (Percent_Chance(Rule->CollapseChance)) {
			Map.Collapse_Cliff(cellptr);
		}
	}

	if (warhead->Particle) {
		ParticleSystemClass * psys;
		if (warhead->Particle->BehavesLike == PSYS_BEHAVIOR_GAS) {
			psys = GasSystem;
		} else {
			psys = new ParticleSystemClass(warhead->Particle, coord);
		}
		psys->Spawn_Held_Particle(coord, coord);
	}

	if ((warhead->IsWallDestroyer || warhead->IsFire) && !(Map[coord].IsUnderBridge && coord.Z >= BRIDGE_LEPTON_HEIGHT + Map.Get_Height_GL(coord))) {
		if (Map.Crack_Ice(cellptr, NULL)) { // RA2: no DirtyIceCells (TibSun's use does not compile against RA2's declaration)
			Map.Recalc_Ice_Cells();
		}
	}
}


/***********************************************************************************************
 * Combat_Anim -- Determines explosion animation to play.                                      *
 *                                                                                             *
 *    This routine is called when a projectile impacts. This routine will determine what       *
 *    animation should be played.                                                              *
 *                                                                                             *
 * INPUT:   damage   -- The amount of damage this warhead possess (warhead size).              *
 *                                                                                             *
 *          warhead  -- The type of warhead.                                                   *
 *                                                                                             *
 *          land     -- The land type that this explosion is over. Sometimes, this makes       *
 *                      a difference (especially over water).                                  *
 *                                                                                             *
 * OUTPUT:  Returns with the animation to play. If no animation is to be played, then          *
 *          ANIM_NONE is returned.                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/19/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
AnimTypeClass const * Combat_Anim(int damage, WarheadTypeClass const * warhead, LandType land, Coord const & coord)
{
	if (damage == 0 || warhead == NULL) { // RA2: Combat_Anim at 0x47F4D0 has no enum {
		return(NULL);
	}

	if (land == LAND_WATER && warhead->IsConventional && !Map[coord].IsUnderBridge && coord.Z < Map.Get_Height_GL(coord) + LEVEL_LEPTON_H * 2) { // RA2: Combat_Anim at 0x47F4D0 matches only so
		if (Rule->SplashList.Count()) {
			int val = MIN(damage, 35 * Rule->SplashList.Count() - 1); // RA2: as above
			return(Rule->SplashList[val / 35]); // RA2: as above
		}
		return(NULL);
	}

	if (warhead == Rule->LightningWarhead) { // RA2: as above
		return(Rule->WeatherConBoltExplosion); // RA2: as above
	}

	if (warhead->ExplosionSet.Count()) {
		if (warhead->IsEMEffect) {
			return(warhead->ExplosionSet[Random_Pick(0, warhead->ExplosionSet.Count() - 1)]);
		}
		int val = MIN(damage, 25 * warhead->ExplosionSet.Count() - 1); // RA2: as above
		return(warhead->ExplosionSet[val / 25]); // RA2: as above
	}

	return(NULL);
}


/// <summary>
/// Creates the lighting flash of an explosion.
/// This routine is called alongside the explosion animation so that a bright warhead
/// briefly lights up the terrain around the blast.
/// </summary>
/// <param name="coord">The coordinate the blast is centered upon.</param>
/// <param name="damage">The strength of the blast, which governs the size of the flash.</param>
/// <param name="forced">Should the flash occur even if the warhead is not a bright one?</param>
void Combat_Lighting(Coord coord, int damage, WarheadTypeClass const * warhead, bool forced)
{
	if (forced || warhead != NULL && warhead->IsBright) {
		damage *= 64;
		int size = damage >> 8;
		if (size >= 63) {
			size = 63;
		} else if (size <= 21) {
			size = 21;
		}

		new SpotLightClass(coord, size);
	}
}


/***********************************************************************************************
 * Wide_Area_Damage -- Apply wide area damage to the map.                                      *
 *                                                                                             *
 *    This routine will apply damage to a very wide area on the map. The damage will be        *
 *    spread out from the coordinate specified by the radius specified. The amount of damage   *
 *    will attenuate according to the distance from center.                                    *
 *                                                                                             *
 * INPUT:   coord    -- The coordinate that the explosion damage will center about.            *
 *                                                                                             *
 *          radius   -- The radius of the explosion.                                           *
 *                                                                                             *
 *          damage   -- The amount of damage to apply at the center location.                  *
 *                                                                                             *
 *          source   -- Pointer to the purpetrator of the damage (if any).                     *
 *                                                                                             *
 *          warhead  -- The type of warhead that is causing the damage.                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/26/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
void Wide_Area_Damage(Coord const & coord, LEPTON radius, int rawdamage, TechnoClass * source, WarheadTypeClass const * warhead)
{
	if (warhead == NULL) return;

	int cell_radius = (radius + CELL_LEPTON_W-1) / CELL_LEPTON_W;
	Cell cell = coord.As_Cell();

	for (int x = -cell_radius; x <= cell_radius; x++) { // RA2: Wide_Area_Damage at 0x47F6E0 has no DeformThreshhold, Deform, Map
		for (int y = -cell_radius; y <= cell_radius; y++) {
			int xpos = cell.X + x;
			int ypos = cell.Y + y;
			if ((unsigned)xpos > MAP_CELL_W) {
				continue;
			}
			if ((unsigned)ypos > MAP_CELL_H) {
				continue;
			}
			Cell tcell = Cell(xpos, ypos);
			if (!Map.In_Radar(tcell)) continue;

			int dist_from_center = Distance(Coord(x+cell_radius, y+cell_radius), Coord(cell_radius, cell_radius));
			double damagemod = 1;
			if (dist_from_center > 0) {
				damagemod = 1.0 / ((double)cell_radius / dist_from_center);
			}
			Coord tcoord(tcell, coord.Z); // RA2: Wide_Area_Damage at 0x47F6E0 matches only so
			Explosion_Damage(tcoord, rawdamage * damagemod, source, warhead);
		}
	}
}


/// <summary>
/// Determines the pitch to launch a ballistic projectile at.
/// This is the routine the weapon code calls when it wants to lob something at a target.
/// It takes the solution from Calculate_Projectile_Angle and turns it into a pitch the launcher
/// can be aimed with, correcting the flat solution when it would send the shot skyward.
/// </summary>
/// <param name="high_arc">Should the shot be lobbed rather than fired flat?</param>
/// <param name="speed">The speed the projectile will be launched at.</param>
/// <param name="distance">The horizontal distance to the target.</param>
/// <param name="height">The height of the target relative to the launch point.</param>
/// <param name="gravity">The gravity that will act upon the projectile.</param>
/// <param name="pitch">Reference to the pitch to be filled in.</param>
/// <returns>bool; Was a firing solution found?</returns>
bool Calculate_Projectile_Pitch(bool high_arc, int speed, int distance, int height, double gravity, DirType & pitch)
{
	double angle;
	if (Calculate_Projectile_Angle(high_arc, speed, distance, height, gravity, angle)) {
		if (high_arc) {
			pitch = DirType(angle);
			return(true);
		}

		double test;
		Calculate_Projectile_Angle(false, speed, distance + 1, height, gravity, test);
		if (test < angle) {
			pitch = DirType(-angle);
		} else {
			pitch = DirType(angle);
		}
		return(true);
	}
	return(false);
}


/// <summary>
/// Determines the elevation angle for a ballistic shot.
/// This routine solves the projectile equation for the angle that will drop the shot onto
/// the target. A reachable target usually has two answers -- a flat fast one and a lobbed
/// one -- and the caller says which of them it wants.
/// </summary>
/// <param name="high_arc">Should the lobbed solution be returned rather than the flat one?</param>
/// <param name="speed">The speed the projectile will be launched at.</param>
/// <param name="distance">The horizontal distance to the target.</param>
/// <param name="height">The height of the target relative to the launch point.</param>
/// <param name="gravity">The gravity that will act upon the projectile.</param>
/// <param name="angle">Reference to the angle, in radians, to be filled in.</param>
/// <returns>bool; Was a firing solution found?</returns>
bool Calculate_Projectile_Angle(bool high_arc, int speed, int distance, int height, double gravity, double & angle)
{
	if (height > 0) { // RA2: Calculate_Projectile_Angle at 0x47F9B0 matches only so
		angle = 0.7853822499999998; // RA2: as above
		return(true); // RA2: as above
	}

	if ((double)distance == 0.0) {
		distance = 0;
	}

	double dx = (double)distance;
	double velocity = (double)speed; // RA2: as above
	double dy = (double)height;
	double v = velocity; // RA2: as above

	double vsq = v * v; // RA2: Calculate_Projectile_Angle at 0x47F9B0 has no double dy2 = dy * dy;
	double base = vsq - ((double)height * gravity); // RA2: as above
	double g2 = gravity * gravity;
	double dx2 = (dx * dx); // RA2: as above

	double value = (v * v * vsq)
				 - (2 * vsq * dy * gravity)
				 - (g2 * dx2);

	double denominator = (((dy * dy) / dx2) + 1.0) * 2; // RA2: as above

	if (value < 0.0 || denominator == 0.0) {
		return(false); // RA2: as above
	}

	double numerator = !high_arc ? base + fastmath::sqrt(value)
								  : base - fastmath::sqrt(value);

	value = numerator / denominator;

	if (value >= 0.0) {
		angle = fastmath::acos(fastmath::sqrt(value) / v);
		return(true);
	}
	return(false);
}


/// <summary>
/// RA2
/// </summary>
bool Calculate_Projectile_Dir(int speed, int distance, int height, double gravity, DirType & dir)
{
	double angle;
	if (Calculate_Projectile_Angle(false, speed, distance, height, gravity, angle)) {
		if (angle < 0.0) {
			angle += M_PI * 2;
		}
		dir = DirType(angle);
		return(true);
	}
	return(false);
}


/// <summary>
/// Determines the launch speed needed to cover a range.
/// This routine is used by artillery style weapons to pick a muzzle velocity that will
/// carry the shot out as far as the target sits.
/// </summary>
/// <param name="range">The distance the projectile must cover.</param>
/// <param name="gravity">The gravity that will act upon the projectile.</param>
/// <returns>Returns with the speed to launch the projectile at.</returns>
MPHType Calculate_Projectile_Speed(int range, double gravity)
{
	return((MPHType)(int)fastmath::sqrt((double)range * gravity * 1.2));
}


/// <summary>
/// Can a ballistic projectile reach the target at all?
/// This routine is used before a lobbed weapon is allowed to fire so that the object does
/// not line up a shot that gravity will never let it complete.
/// </summary>
/// <param name="speed">The speed the projectile would be launched at.</param>
/// <param name="distance">The horizontal distance to the target.</param>
/// <param name="height">The height of the target relative to the launch point.</param>
/// <param name="gravity">The gravity that will act upon the projectile.</param>
/// <returns>bool; Does a firing solution exist?</returns>
bool Is_Projectile_Trajectory_Valid(int speed, int distance, int height, double gravity)
{
	double v = (double)speed;
	double dx = (double)distance;
	if (dx == 0.0) dx = 0.001; // RA2: Is_Projectile_Trajectory_Valid at 0x47FBA0 matches only so
	double dy = (double)height;

	double y2 = dy * dy;
	double vsq = v * v;
	double g2 = gravity * gravity;
	double x2 = dx * dx;

	double discriminant = (v * v * vsq)
						- (2 * vsq * dy * gravity)
						- (g2 * x2);

	if (discriminant < 0.0) {
		return(false);
	}

	double denominator = 2 * ((y2 / x2) + 1.0);
	if (denominator == 0.0) {
		return(false); // RA2: as above
	}

	double base = vsq - (dy * gravity);

	if (((base + fastmath::sqrt(discriminant)) / denominator) >= 0.0 ||
		((base - fastmath::sqrt(discriminant)) / denominator) >= 0.0) {
		return(true); // RA2: as above
	}
	return(false); // RA2: as above
}


/// <summary>
/// Fetches the Z adjustment to display an explosion with.
/// Explosion animations are nudged toward the viewer so that the blast draws over the
/// object it is consuming instead of disappearing behind it.
/// </summary>
/// <returns>Returns with the Z adjustment to hand to the animation.</returns>
int Get_Explosion_Z(Coord coord)
{
	return(-15);
}


/// <summary>
/// Fetches the gravity that acts upon a floating projectile.
/// Bullets marked as floaters drift toward their target rather than plummeting, so the
/// firing and trajectory routines use this gentler pull in place of the rule gravity.
/// </summary>
/// <returns>Returns with the gravity to use for a floating projectile.</returns>
double Get_Floater_Gravity(void)
{
	return(Rule->Gravity * 0.5);
}


/// <summary>
/// Determines the distance between two points, ignoring a slight height difference.
/// This routine is used when explosion damage is being dealt out, so that a victim
/// perched a fraction of a level above or below the blast is not judged to be further
/// away than one standing level with it.
/// </summary>
/// <returns>Returns with the distance between the two coordinates.</returns>
inline static int Explosion_Distance(Coord const & coord1, Coord const & coord2)
{
	int z1 = coord1.Z;
	int z2 = coord2.Z;
	if (abs(z2 - z1) < LEVEL_LEPTON_H) {
		z2 = coord1.Z;
	}
	return(Coord(coord1.X - coord2.X, coord1.Y - coord2.Y, z1 - z2).Length());
}

