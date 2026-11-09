// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#define INCLUDE_COM
#include "always.h"
#include "geometry.h"
#include <math.h>
#include "empulse.h"
#include "_map.h"
#include "_rules.h"
#include "aircraft.h"
#include "anim.h"
#include "building.h"
#include "ccrand.h"
#include "cell.h"
#include "crc.h"
#include "globals.h"
#include "house.h"
#include "infantry.h"
#include "infatype.h"
#include "mouse.h"
#include "rules.h"
#include "sun.h"
#include "tag.h"
#include "tracker.h"
#include "unit.h"
#include "unittype.h"

DynamicVectorClass<EMPulseClass *> EMPulseClass::EMPulses;

EMPulseClass::EMPulseClass(Cell cell, int spread, int duration, TechnoClass * source) :
	BASECLASS(),
	CellID(cell),
	Spread(spread),
	CreationFrame(Frame),
	Duration(duration)
{
	Create_ID();
	EMPulses.Add(this);
	Create(source);
}

EMPulseClass::EMPulseClass(void) :
	BASECLASS(),
	CellID(CELL_NONE),
	Spread(0),
	CreationFrame(0),
	Duration(0)
{
}

EMPulseClass::EMPulseClass(NoInitClass const & x) :
	BASECLASS(x)
{
}

EMPulseClass::~EMPulseClass(void)
{
	if (GameActive) {
		Destroy();
	}
	Detach_This_From_All(this, true);
	EMPulses.Delete(this);
}

void EMPulseClass::Reset(void)
{
	while (EMPulses.Count()) {
		delete EMPulses[0];
	}
}

void EMPulseClass::Update_All(void)
{
	for (int i = EMPulses.Count() - 1; i >= 0; i--) {
		EMPulseClass * pulse = EMPulses[i];
		if (Frame >= pulse->CreationFrame + pulse->Duration) {
			delete pulse;
		}
	}
}

void EMPulseClass::Create(TechnoClass * source)
{
	if (Duration + CreationFrame > Frame) {
		int spread_sq = Spread * Spread;

		for (int i = 0; i < Aircraft.Count(); i++) {
			AircraftClass * aircraft = Aircraft[i];
			if (aircraft->IsDown && !aircraft->IsInLimbo && !aircraft->In_Air() && aircraft->Strength > 0) {
				if (aircraft->Center_Coord().Distance_To(CellID.As_Coord()) < Spread * CELL_LEPTON) {
					((TechnoClass *)aircraft)->Crash(source); // RA2: virtual TechnoClass::Crash
				}
			}
		}

		for (int y = -Spread; y <= Spread; y++) {
			for (int x = -Spread; x <= Spread; x++) {
				Cell cell = Cell(x, y) + CellID;
				if (cell.X >= 0 && cell.X < MAP_CELL_W && cell.Y >= 0 && cell.Y < MAP_CELL_H) {
					if (Map.Is_Valid(cell)) {
						int dx = cell.X - CellID.X;
						int dy = cell.Y - CellID.Y;
						if (dx * dx + dy * dy <= spread_sq) {
							CellClass & cellptr = Map[cell];

							BuildingClass * building = cellptr.Cell_Building();
							if (building != NULL) {
								if (building->Center_Coord().As_Cell() == cell) {
									if (!building->Class->IsInvisibleInGame) {
										building->Power_Off(); // RA2: 0x44D6C0
										building->StunDuration = Duration;
										if (building->Class->IsRadar) {
											building->House->RecalcRadar = true;
										}
									}
								}
							} else {
								TechnoClass * techno = cellptr.Cell_Techno();
								if (techno != NULL) {
									if (techno->RTTI == RTTI_UNIT || techno->RTTI == RTTI_AIRCRAFT) {
										if (techno->Is_Foot() && ((FootClass *)techno)->Locomotion != NULL) {
											if (techno->RTTI != RTTI_UNIT || !((UnitClass *)techno)->Class->IsLargeVisceroid || !((UnitClass *)techno)->Class->IsSmallVisceroid) {
												FootClass * foot = (FootClass *)techno;
												foot->Locomotion->Power_Off();
												if (foot->Locomotion->Is_Moving()) {
													foot->Locomotion->Stop_Moving();
												}
												foot->StunDuration = Duration;
												AnimClass * sparks = new AnimClass(Rule->EMPulseSparkles, foot->Center_Coord(), Random_Pick(0, 25));
												if (sparks != NULL) {
													sparks->Attach_To(foot);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void EMPulseClass::Destroy(void)
{
	int spread_sq = Spread * Spread;
	for (int y = -Spread; y <= Spread; y++) {
		for (int x = -Spread; x <= Spread; x++) {
			Cell cell = Cell(x, y) + CellID;
			if (cell.X >= 0 && cell.X < MAP_CELL_W && cell.Y >= 0 && cell.Y < MAP_CELL_H) {
				if (Map.Is_Valid(cell)) {
					int dx = cell.X - CellID.X;
					int dy = cell.Y - CellID.Y;
					if (dx * dx + dy * dy <= spread_sq) {
						CellClass & cellptr = Map[cell];
						cellptr.IsAffectedByEMP = false;
					}
				}
			}
		}
	}
}

void EMPulseClass::Compute_CRC(CRCEngine &crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(CellID.X);
	crc(CellID.Y);
	crc(Spread);
	crc(CreationFrame);
	crc(Duration);
}

HRESULT STDMETHODCALLTYPE EMPulseClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_EMPulseClass;
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE EMPulseClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) EMPulseClass(NoInitClass());

		result = S_OK;
	}
	return(result);
}

HRESULT STDMETHODCALLTYPE EMPulseClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		result = S_OK;
	}
	return(result);
}


int EMPulseClass::Fetch_Object_Size(void) const
{
	return(sizeof(EMPulseClass));
}


RTTIType EMPulseClass::Fetch_RTTI(void) const
{
	return(RTTI_EMPULSE);
}
