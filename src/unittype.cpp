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

/* $Header: /CounterStrike/UDATA.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : UDATA.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : July 19, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   UnitTypeClass::As_Reference -- Fetches a reference to the unit type class specified.      *
 *   UnitTypeClass::Create_And_Place -- Creates and places a unit object onto the map.         *
 *   UnitTypeClass::Create_One_Of -- Creates a unit in limbo.                                  *
 *   UnitTypeClass::Dimensions -- Determines the unit's pixel dimensions.                      *
 *   UnitTypeClass::Display -- Displays a generic unit shape.                                  *
 *   UnitTypeClass::From_Name -- Fetch class pointer from specified name.                      *
 *   UnitTypeClass::Init_Heap -- Initialize the unit type class heap.                          *
 *   UnitTypeClass::Max_Pips -- Fetches the maximum pips allowed for this unit.                *
 *   UnitTypeClass::One_Time -- Performs one time processing for unit type class objects.      *
 *   UnitTypeClass::Prep_For_Add -- Prepares scenario editor to add unit.                      *
 *   UnitTypeClass::Read_INI -- Fetch the unit type data from the INI database.                *
 *   UnitTypeClass::Turret_Adjust -- Turret adjustment routine for MLRS and MSAM units.        *
 *   UnitTypeClass::UnitTypeClass -- Constructor for unit types.                               *
 *   UnitTypeClass::operator delete -- Return a unit type class object back to the pool.       *
 *   UnitTypeClass::operator new -- Allocates an object from the unit type class heap.         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define INCLUDE_COM
#include "always.h"

#include "unittype.h"

#include "_map.h"
#include "_mixfile.h"
#include "_palette.h"
#include "_rules.h"
#include "bsurface.h"
#include "convert.h"
#include "face.h"
#include "findmake.h"
#include "globals.h"
#include "inline.h"
#include "mixfile.h"
#include "mouse.h"
#include "rules.h"
#include "sun.h"
#include "tracker.h"
#include "unit.h"

#include "color.hh"


Surface * EightBitSurface = NULL;
ConvertClass * EightBitDrawer = NULL;

/***************************************************************************
**	These are the pointers to the special shape data that the units may need.
*/
void const * UnitTypeClass::SmallVisceroidShapes = NULL;
void const * UnitTypeClass::LargeVisceroidShapes = NULL;

/***********************************************************************************************
 * UnitTypeClass::UnitTypeClass -- Constructor for unit types.                                 *
 *                                                                                             *
 *    This is the constructor for the unit types. It is used to initialize the unit type class *
 *    structure. The unit type class is used to control the behavior of the various types      *
 *    of units in the game. This constructor is called for every unique unit type as it        *
 *    exists in the array of unit types.                                                       *
 *                                                                                             *
 * INPUT:   bla bla bla... see below                                                           *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/20/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
UnitTypeClass::UnitTypeClass(char const * ininame) :
	BASECLASS(ininame, SPEED_NONE),
	MovementRestrictedTo(LAND_NONE),
	HalfDamageSmokeLocation(0, 0, 0),
	IsPassive(false),
	IsCrateGoodie(false),
	IsToHarvest(false),
	IsToVeinHarvest(false),
	IsFireAnim(false),
	IsTilter(true),
	DeathFrameRate(1),
	IsLockTurret(false),
	IsNoFireWhileMoving(false),
	IsDeployToFire(false),
	IsUseTurretShadow(false),
	IsTooBigToFitUnderBridge(false),
	IsSmallVisceroid(false),
	IsLargeVisceroid(false),
	IsCarriesCrate(false),
	AltImageData(NULL),
	IsNonVehicle(false),
	IsJellyfish(false),
	IsLimpetDrone(false),
	IsMobileEMP(false),
	IsCoreDefender(false),
	StandingFrames(0),
	DeathFrames(0),
	MaxCharge(0),
	StartCharge(0),
	StartStandFrame(-1),
	StartWalkFrame(-1),
	StartFiringFrame(-1),
	StartDeathFrame(-1),
	MaxDeathCounter(-1),
	Facings(FACING_COUNT),
	WalkFrames(12),
	FiringFrames(0),
	HeapID(UNIT_NONE),
	AltImageFile()
{
	Create_ID();
	Rotation = 32;
	UnitTypes.Add(this);
	HeapID = (UnitType)UnitTypes.ID(this);
	FiringSyncFrame[0] = FiringSyncFrame[1] = -1;
}


/// <summary>
/// Constructor for unit type objects that skips initialization.
/// This routine is used when the object is about to be filled in from a save game, so
/// every member is deliberately left as it lies.
/// </summary>
UnitTypeClass::UnitTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	AltImageFile(x)
{

}


/// <summary>
/// Destructor for the unit type objects.
/// This routine severs every reference the game holds to this unit type before removing
/// it from the unit type heap.
/// </summary>
UnitTypeClass::~UnitTypeClass(void)
{
	Detach_This_From_All(this);
	UnitTypes.Delete(this);
}


/***********************************************************************************************
 * UnitTypeClass::From_Name -- Fetch class pointer from specified name.                        *
 *                                                                                             *
 *    This routine converts an ASCII representation of a unit class and                        *
 *    converts it into a real unit class number.                                               *
 *                                                                                             *
 * INPUT:   name  -- ASCII name representing a unit class.                                     *
 *                                                                                             *
 * OUTPUT:  Returns with the actual unit class number that the string                          *
 *          represents.                                                                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/07/1992 JLB : Created.                                                                 *
 *   05/02/1994 JLB : Converted to member function.                                            *
 *=============================================================================================*/
UnitType UnitTypeClass::From_Name(char const * name)
{
	if (name != NULL) {
		for (int classid = UNIT_FIRST; classid < UnitTypes.Count(); classid++) {
			if (stricmp(UnitTypes[classid]->Name(), name) == 0) {
				return(UnitType(classid));
			}
		}
	}
	return(UNIT_NONE);
}


/***********************************************************************************************
 * UnitTypeClass::One_Time -- Performs one time processing for unit type class objects.        *
 *                                                                                             *
 *    This routine is used to perform the action necessary only once for the unit type class.  *
 *    It loads unit shapes and brain files.   This routine should only be called once.         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only call this routine once.                                                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/28/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void UnitTypeClass::One_Time(void)
{
	if (EightBitSurface == NULL) {
		EightBitSurface = new BSurface(160, 160, 1);
		EightBitSurface->Fill(TBLACK);
	}

	if (EightBitDrawer == NULL) {
		EightBitDrawer = new ConvertClass(GamePalette, GamePalette, *EightBitSurface);
	}

	/*
	**	Load any custom shapes at this time.
	*/
	SmallVisceroidShapes = MFCD::Retrieve("VISC_SML.SHP");
	LargeVisceroidShapes = MFCD::Retrieve("VISC_LRG.SHP");
}


/***********************************************************************************************
 * UnitTypeClass::Create_And_Place -- Creates and places a unit object onto the map.           *
 *                                                                                             *
 *    This routine is used by the scenario editor to create and place a unit object of this    *
 *    type onto the map.                                                                       *
 *                                                                                             *
 * INPUT:   cell     -- The cell that the unit is to be placed into.                           *
 *                                                                                             *
 *          house    -- The house that the unit belongs to.                                    *
 *                                                                                             *
 * OUTPUT:  bool; Was the unit created and placed successfully?                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/28/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
bool UnitTypeClass::Create_And_Place(Cell const & cell, HouseClass * house) const
{
	UnitClass * unit = new UnitClass(this, house);
	if (unit != NULL) {
		DirType dir = Random_Dir(DIR_N, DIR_MAX);
		return(unit->Unlimbo(cell, dir.As_Dir256()));
	}
	return(false);
}


/***********************************************************************************************
 * UnitTypeClass::Create_One_Of -- Creates a unit in limbo.                                    *
 *                                                                                             *
 *    This function creates a unit of this type and keeps it in limbo. A pointer to the        *
 *    created unit object is returned. It is presumed that this object will later be           *
 *    unlimboed at the correct time and place.                                                 *
 *                                                                                             *
 * INPUT:   house -- Pointer to the house that is to own the unit.                             *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the created unit object. If the unit object              *
 *          could not be created, then NULL is returned.                                       *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/07/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
ObjectClass * UnitTypeClass::Create_One_Of(HouseClass * house) const
{
	return(new UnitClass(this, house));
}


/***********************************************************************************************
 * UnitTypeClass::Dimensions -- Determines the unit's pixel dimensions.                        *
 *                                                                                             *
 *    This routine will fill in the width and height for this unit type. This width and        *
 *    height are used to render the selection rectangle and the positioning of the health      *
 *    bargraph.                                                                                *
 *                                                                                             *
 * INPUT:   width    -- Reference to the width of the unit (to be filled in).                  *
 *                                                                                             *
 *          height   -- Reference to the height of the unit (to be filled in).                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/23/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
Point3D UnitTypeClass::Pixel_Dimensions(void) const
{
	int width;
	int height;

	width = MaxSize-(MaxSize/4);
	width = min(width, CELL_PIXEL_H);
	height = MaxSize-(MaxSize/4);
	height = min(height, CELL_PIXEL_H);
	return(Point3D(width, width, height));
}


/// <summary>
/// Determines the unit's dimensions in leptons.
/// This routine gives the volume the unit is considered to occupy in the world, which
/// is used for the intersection tests that decide whether a shot or an effect touches
/// the unit.
/// </summary>
/// <returns>Returns with the width, depth and height of the unit, in leptons.</returns>
Point3D UnitTypeClass::Lepton_Dimensions(void) const
{
	return(Point3D(CELL_LEPTON, CELL_LEPTON, 200 + (IsCoreDefender ? 500 : 0)));
}


/***********************************************************************************************
 * UnitTypeClass::Turret_Adjust -- Turret adjustment routine for MLRS and MSAM units.          *
 *                                                                                             *
 *    This routine adjusts the pixel coordinates specified to account for the displacement of  *
 *    the turret on the MLRS and MSAM vehicles.                                                *
 *                                                                                             *
 * INPUT:   dir   -- The direction of the body of the vehicle.                                 *
 *                                                                                             *
 *          x,y   -- References to the turret center pixel position. These will be modified as *
 *                   necessary.                                                                *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/08/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
TPoint2D<int> UnitTypeClass::Turret_Adjust(Dir256 dir, TPoint2D<int> const & xy) const
{
	return(xy);
}


/***********************************************************************************************
 * UnitTypeClass::Read_INI -- Fetch the unit type data from the INI database.                  *
 *                                                                                             *
 *    This routine will find the section in the INI database for this unit type object and     *
 *    then fill in the override values specified.                                              *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that will be examined.                      *
 *                                                                                             *
 * OUTPUT:  bool; Was the section for this unit found in the database and the data extracted?  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/19/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool UnitTypeClass::Read_INI(CCINIClass const & ini)
{
	if (BASECLASS::Read_INI(ini)) {
		IsCrateGoodie = ini.Get_Bool(Name(), "CrateGoodie", IsCrateGoodie);
		IsNoFireWhileMoving = ini.Get_Bool(Name(), "NoMovingFire", IsNoFireWhileMoving);
		IsDeployToFire = ini.Get_Bool(Name(), "DeployToFire", IsDeployToFire);
		IsToHarvest = ini.Get_Bool(Name(), "Harvester", IsToHarvest);
		IsToVeinHarvest = ini.Get_Bool(Name(), "Weeder", IsToVeinHarvest);

		/*
		**	If this unit can drive over walls, then mark it as recognizing the crusher zone.
		*/
		if (Speed == SPEED_NONE) {
			Speed = IsCrusher ? SPEED_TRACK : SPEED_WHEEL;
		}

		Speed = ini.Get_SpeedType(Name(), "SpeedType", Speed);

		IsTilter = ini.Get_Bool(Name(), "IsTilter", IsTilter);
		IsCarriesCrate = ini.Get_Bool(Name(), "CarriesCrate", IsCarriesCrate);
		IsLockTurret = !IsTurretEquipped;
		IsTooBigToFitUnderBridge = ini.Get_Bool(Name(), "TooBigToFitUnderBridge", IsTooBigToFitUnderBridge);

		HalfDamageSmokeLocation = ini.Get_Point(Name(), "HalfDamageSmokeLocation", HalfDamageSmokeLocation);

		InitialMission = MISSION_HUNT;
		if (IsToHarvest || IsToVeinHarvest) {
			InitialMission = MISSION_HARVEST;
		}

		IsUseTurretShadow = ArtINI.Get_Bool(Graphic_Name(), "UseTurretShadow", IsUseTurretShadow);
		WalkFrames = ArtINI.Get_Int(Graphic_Name(), "WalkFrames", WalkFrames);
		FiringFrames = ArtINI.Get_Int(Graphic_Name(), "FiringFrames", FiringFrames);
		IsPassive = ini.Get_Bool(Name(), "Passive", IsPassive);
		MovementRestrictedTo = ini.Get_LandType(Name(), "MovementRestrictedTo", MovementRestrictedTo);
		IsSmallVisceroid = ini.Get_Bool(Name(), "SmallVisceroid", IsSmallVisceroid);
		IsLargeVisceroid = ini.Get_Bool(Name(), "LargeVisceroid", IsLargeVisceroid);
		IsJellyfish = ini.Get_Bool(Name(), "Jellyfish", IsJellyfish);
		IsNonVehicle = ini.Get_Bool(Name(), "NonVehicle", IsNonVehicle);

		if (IsSmallVisceroid || IsLargeVisceroid) {
			IsNonVehicle = true;
		}

		IsLimpetDrone = ini.Get_Bool(Name(), "IsLimpetDrone", IsLimpetDrone);
		IsCoreDefender = ini.Get_Bool(Name(), "IsCoreDefender", IsCoreDefender);
		IsMobileEMP = ini.Get_Bool(Name(), "IsMobileEMP", IsMobileEMP);
		MaxCharge = ini.Get_Int(Name(), "MaxCharge", MaxCharge);
		StartCharge = ini.Get_Int(Name(), "StartCharge", StartCharge);

		if (FiringFrames > 0) {
			StandingFrames = 1;
		}

		StandingFrames = ArtINI.Get_Int(Graphic_Name(), "StandingFrames", StandingFrames);
		DeathFrames = ArtINI.Get_Int(Graphic_Name(), "DeathFrames", DeathFrames);
		DeathFrameRate = ArtINI.Get_Int(Graphic_Name(), "DeathFrameRate", DeathFrameRate);

		if (DeathFrameRate < 1) {
			DeathFrameRate = 1;
		}

		if (!FiringFrames && !IsTurretEquipped) {
			Facings = 1;
		}

		Facings = ArtINI.Get_Int(Graphic_Name(), "Facings", Facings);

		if (StartWalkFrame == -1) {
			StartWalkFrame = 0;
		}

		if (StartStandFrame == -1) {
			StartStandFrame = StandingFrames == 0 ? StartWalkFrame : Facings * WalkFrames;
		}

		if (StartFiringFrame == -1) {
			StartFiringFrame = FiringFrames == 0 ? StartStandFrame : Facings * (StandingFrames + WalkFrames);
		}

		if (StartDeathFrame == -1) {
			StartDeathFrame = DeathFrames ? Facings * (FiringFrames + WalkFrames + 1) : -1;
			MaxDeathCounter = StartDeathFrame + DeathFrames;
		}

		StartStandFrame = ArtINI.Get_Int(Graphic_Name(), "StartStandFrame", StartStandFrame);
		StartWalkFrame = ArtINI.Get_Int(Graphic_Name(), "StartWalkFrame", StartWalkFrame);
		StartFiringFrame = ArtINI.Get_Int(Graphic_Name(), "StartFiringFrame", StartFiringFrame);
		StartDeathFrame = ArtINI.Get_Int(Graphic_Name(), "StartDeathFrame", StartDeathFrame);
		MaxDeathCounter = ArtINI.Get_Int(Graphic_Name(), "MaxDeathCounter", MaxDeathCounter);

		FiringSyncFrame[0] = ArtINI.Get_Int(Graphic_Name(), "FiringSyncFrame1", FiringSyncFrame[0]);
		FiringSyncFrame[1] = ArtINI.Get_Int(Graphic_Name(), "FiringSyncFrame2", FiringSyncFrame[1]);

		char defvalue[25];
		strcpy(defvalue, AltImageFile);
		ini.Get_String(Name(), "AltImage", defvalue, AltImageFile, sizeof(AltImageFile));

		char filename[_MAX_PATH];
		_makepath(filename, NULL, NULL, AltImageFile, ".SHP");
		AltImageData = (ShapeSet const *)MFCD::Retrieve(filename);
		return(true);
	}
	return(false);
}


/// <summary>
/// Adjusts a coordinate so that the unit does not sink below the ground.
/// This routine is used when a unit is being placed or moved, to ensure that its
/// altitude is never lower than the terrain beneath it.
/// </summary>
/// <param name="coord">The coordinate that needs checking.</param>
/// <returns>Returns with the coordinate, raised to ground level if it was below.</returns>
Coord const UnitTypeClass::Coord_Fixup(Coord const & coord) const
{
	Coord fix = coord;
	if (fix.Z < Map.Get_Height_GL(fix)) {
		fix.Z = Map.Get_Height_GL(fix);
	}
	return(fix);
}


/// <summary>
/// Fetches the amount of strength restored by one repair step.
/// </summary>
/// <returns>Returns with the number of hit points a single repair tick will restore.</returns>
int UnitTypeClass::Repair_Step(void) const
{
	return(Rule->RepairStep);
}


/// <summary>
/// Fetches the persistent class identifier for the unit type.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE UnitTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_UnitTypeClass;
	return(S_OK);
}


/// <summary>
/// Submits the unit type's state to the game CRC.
/// This routine is used by the multiplayer synchronization check to prove that every
/// machine is playing with the same rules for this unit type.
/// </summary>
void UnitTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(MovementRestrictedTo);
	crc(IsPassive);
	crc(IsCrateGoodie);
	crc(IsToHarvest);
	crc(IsToVeinHarvest);
	crc(IsFireAnim);
	crc(IsLockTurret);
	crc(IsNoFireWhileMoving);
	crc(IsTilter);
	crc(IsUseTurretShadow);
}


/// <summary>
/// Reads the unit type back in from the save game stream.
/// This routine copes with save games written before the animation frame members were
/// added, shuffling the older layout into place and inventing sensible values for the
/// members that were missing. The artwork is then re-fetched, since image pointers
/// cannot survive a save game.
/// </summary>
/// <returns>Returns with S_OK if the unit type was read successfully.</returns>
HRESULT STDMETHODCALLTYPE UnitTypeClass::Load(IStream *stream)
{
	int size = Fetch_Object_Size(false);
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		if (IsOldSaveGame) {
			memmove(&WalkFrames, &IsNonVehicle, size - offsetof(UnitTypeClass, WalkFrames));
			int facings = 1;
			IsNonVehicle = false;
			IsJellyfish = false;
			IsLimpetDrone = false;
			IsCoreDefender = false;
			IsMobileEMP = false;
			MaxCharge = 0;
			StartCharge = 0;
			DeathFrames = 0;
			DeathFrameRate = 1;
			if (FiringFrames != 0 || IsTurretEquipped) {
				facings = FACING_COUNT;
			}
			Facings = facings;
			StartStandFrame = facings * WalkFrames;
			StartWalkFrame = 0;
			StartFiringFrame = facings * (WalkFrames + 1);
			StartDeathFrame = facings * (WalkFrames + 1);
			MaxDeathCounter = facings * (WalkFrames + 1);
			FiringSyncFrame[0] = FiringSyncFrame[1] = -1;
		}

		new (this) UnitTypeClass(NoInitClass());

		Fetch_Voxel_Image();
		Fetch_Normal_Image();

		if (AltImageData != NULL) {
			char filename[_MAX_PATH];
			_makepath(filename, NULL, NULL, AltImageFile, ".SHP");
			AltImageData = (ShapeSet const *)MFCD::Retrieve(filename);
		}

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Writes the unit type out to the save game stream.
/// </summary>
/// <param name="cleardirty">Should the object be marked as clean once it is written?</param>
/// <returns>Returns with S_OK if the unit type was written successfully.</returns>
HRESULT STDMETHODCALLTYPE UnitTypeClass::Save(IStream *stream, int cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Fetches the unit type of the name specified, creating it if necessary.
/// This routine is used while the rules are being parsed, so that a unit type may be
/// referred to before its own section has been reached.
/// </summary>
/// <param name="name">The name of the unit type to look for.</param>
/// <returns>Returns with a pointer to the unit type of that name.</returns>
UnitTypeClass * UnitTypeClass::Find_Or_Make(char const * name)
{
	return(TFind_Or_Make<UnitTypeClass>(name, UnitTypes));
}


/// <summary>
/// Fetches the size of this object as it appears in a save game.
/// </summary>
/// <param name="oldsave">Is this for a save game written in the older format?</param>
/// <returns>Returns with the number of bytes this object occupies in the save game.</returns>
int UnitTypeClass::Fetch_Object_Size(bool oldsave) const
{
	int delta = oldsave ? Get_Object_Size_Delta() : 0;
	return(sizeof(*this) - delta);
}


/// <summary>
/// Determines how much this object has grown since the old save game format.
/// This routine is used by the save game loader so that an object written before these
/// members existed can still be read into the tail of the current object.
/// </summary>
/// <returns>Returns with the number of bytes by which this object has grown.</returns>
int UnitTypeClass::Get_Object_Size_Delta(void) const
{
	return	(BASECLASS::Get_Object_Size_Delta() +
			sizeof(IsNonVehicle) +
			sizeof(IsJellyfish) +
			sizeof(IsLimpetDrone) +
			sizeof(IsMobileEMP) +
			sizeof(IsCoreDefender) +
			sizeof(StandingFrames) +
			sizeof(DeathFrames) +
			sizeof(DeathFrameRate) +
			sizeof(MaxCharge) +
			sizeof(StartCharge) +
			sizeof(FiringSyncFrame) +
			sizeof(StartStandFrame) +
			sizeof(StartWalkFrame) +
			sizeof(StartFiringFrame) +
			sizeof(StartDeathFrame) +
			sizeof(MaxDeathCounter) +
			sizeof(Facings));
}


/// <summary>
/// Fetches the RTTI identifier of this class.
/// </summary>
RTTIType UnitTypeClass::Fetch_RTTI(void) const
{
	return(RTTI_UNITTYPE);
}


/// <summary>
/// Fetches the heap identifier of this unit type.
/// </summary>
/// <returns>Returns with the index of this unit type within the unit type heap.</returns>
int UnitTypeClass::Fetch_Heap_ID(void) const
{
	return(HeapID);
}
