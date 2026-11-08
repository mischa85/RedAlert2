/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/IDATA.CPP 3     3/16/97 10:16p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : IDATA.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 15, 1994                                              *
 *                                                                                             *
 *                  Last Update : July 19, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   InfantryTypeClass::As_Reference -- Fetches a reference to the infantry type specified.    *
 *   InfantryTypeClass::Create_And_Place -- Creates and places infantry object onto the map.   *
 *   InfantryTypeClass::Create_One_Of -- Creates an infantry object.                           *
 *   InfantryTypeClass::Display -- Displays a generic infantry object.                         *
 *   InfantryTypeClass::From_Name -- Converts an ASCII name into an infantry type number.      *
 *   InfantryTypeClass::Full_Name -- Fetches the full name text number.                        *
 *   InfantryTypeClass::Get_Cameo_Data -- Fetches the small cameo shape for sidebar strip.     *
 *   InfantryTypeClass::InfantryTypeClass -- Constructor for infantry type class objects.      *
 *   InfantryTypeClass::Init_Heap -- Initialize the infantry type class heap.                  *
 *   InfantryTypeClass::Occupy_List -- Returns with default infantry occupation list.          *
 *   InfantryTypeClass::One_Time -- Performs any one time processing for infantry system.      *
 *   InfantryTypeClass::Prep_For_Add -- Prepares the scenario editor for adding of infantry obj*
 *   InfantryTypeClass::Read_INI -- Fetches infantry override values from the INI database.    *
 *   InfantryTypeClass::operator delete -- Frees an infantry type class object.                *
 *   InfantryTypeClass::operator new -- Allocate an infanty type class object.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define INCLUDE_COM
#include "always.h"

#include "infatype.h"
#include "animtype.h"
#include "swizzle.h"

#include "_map.h"
#include "_rules.h"
#include "cell.h"
#include "findmake.h"
#include "globals.h"
#include "infantry.h"
#include "mouse.h"
#include "rules.h"
#include "sun.h"
#include "tracker.h"

#include <cstdio>

/*
**	RA2: the number of sequence controls each infantry type allocates.
*/
enum { DOINFO_COUNT = 38 };

static inline void Init_Controls(InfantryTypeClass * type)
{
	type->DoControls = new DoInfoStruct[DOINFO_COUNT];
	for (int i = 0; i < DOINFO_COUNT; i++) {
		((DoInfoStruct *)type->DoControls)[i].Frame = 0;
		((DoInfoStruct *)type->DoControls)[i].Count = 0;
		((DoInfoStruct *)type->DoControls)[i].Jump = 0;
		((DoInfoStruct *)type->DoControls)[i].Facing = FACING_NONE;
	}
}


/***********************************************************************************************
 * InfantryTypeClass::InfantryTypeClass -- Constructor for infantry type class objects.        *
 *                                                                                             *
 *    This routine will construct the infantry type objects. It is use to create the static    *
 *    infantry types that are used to give each of the infantry objects their characteristics. *
 *                                                                                             *
 * INPUT:   see below...                                                                       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/24/1994 JLB : Created.                                                                 *
 *   02/16/1996 JLB : Greatly simplified.                                                      *
 *=============================================================================================*/
InfantryTypeClass::InfantryTypeClass(char const * ininame) :
	BASECLASS(ininame, SPEED_FOOT),
	HeapID(INFANTRY_NONE),
	Pip(PIP_GREEN),
	DoControls(NULL),
	FireLaunch(0),
	ProneLaunch(0),
	DeadBodies(),
	DeathAnims(),
	VoiceComment(),
	EnterWaterSound(-1),
	LeaveWaterSound(-1),
	IsCyborg(false),
	NotHuman(false),
	Ivan(false),
	DetectionDistance(0),
	Occupier(false),
	Assaulter(false),
	IsFearless(false),
	IsCrawling(true),
	IsCapture(false),
	IsFraidyCat(false),
	IsTiberiumProof(false),
	IsCivilian(false),
	IsBomber(false),
	IsEngineer(false),
	IsAgent(false),
	IsThief(false),
	IsVehicleThief(false),
	IsDoggie(false),
	Deployer(false)
{
	Create_ID();
	InfantryTypes.Add(this);
	HeapID = (InfantryType)InfantryTypes.ID(this);


	Rotation = FACING_COUNT;
	IsRadarVisible = false;
	IsCrushable = true;
	IsRepairable = false;
	IsCrew = false;
	ImmuneToPsionics = false;
	Parasiteable = true;
	Organic = true;
	ConsideredAircraft = false;

	Init_Controls(this);
}


/// <summary>
/// Constructs an infantry type without initializing it.
/// This constructor is used by the load process, which fills the object in from the save
/// stream rather than building it up from scratch.
/// </summary>
InfantryTypeClass::InfantryTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	DeadBodies(x),
	DeathAnims(x),
	VoiceComment(x)
{
}


/// <summary>
/// Removes this infantry type from the game.
/// Everything that refers to this infantry type is detached from it, and its animation
/// sequence controls are freed, before it is dropped from the infantry type heap.
/// </summary>
InfantryTypeClass::~InfantryTypeClass(void)
{
	Detach_This_From_All(this);
	InfantryTypes.Delete(this);
	delete (DoInfoStruct *)DoControls;
	DoControls = NULL;
}


/***********************************************************************************************
 * InfantryTypeClass::Create_One_Of -- Creates an infantry object.                             *
 *                                                                                             *
 *    This creates an infantry object, but does not attempt to place it on the map. It is      *
 *    typically used by the scenario editor when an object is needed, but the location has     *
 *    not yet been specified for where it should appear on the map.                            *
 *                                                                                             *
 * INPUT:   house -- The owner of the infantry object.                                         *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the created infantry object. If an object could not be   *
 *          created, then NULL is returned.                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/24/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
ObjectClass * InfantryTypeClass::Create_One_Of(HouseClass * house) const
{
	return(new InfantryClass(this, house));
}


/***********************************************************************************************
 * InfantryTypeClass::Create_And_Place -- Creates and places infantry object onto the map.     *
 *                                                                                             *
 *    This routine is used by the scenario editor to create and place an infantry object onto  *
 *    the map at the location specified.                                                       *
 *                                                                                             *
 * INPUT:   cell     -- The cell location to place the infantry object at.                     *
 *                                                                                             *
 *          house    -- The owner of the infantry object.                                      *
 *                                                                                             *
 * OUTPUT:  bool; Was the infantry object successfully created and placed at the location      *
 *                specified?                                                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/24/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
bool InfantryTypeClass::Create_And_Place(Cell const & cell, HouseClass * house) const
{
	InfantryClass * i = new InfantryClass(this, house);
	if (i != NULL) {
		Coord cell_coord = cell;
		Coord coord = Map[cell_coord].Closest_Free_Spot(cell_coord);
		if (coord != COORD_NONE) {
			return(i->Unlimbo(coord, DIR_E));
		} else {
			delete i;
		}
	}
	return(false);
}


/***********************************************************************************************
 * InfantryTypeClass::Occupy_List -- Returns with default infantry occupation list.            *
 *                                                                                             *
 *    This routine will return with a cell offset occupation list for a generic infantry       *
 *    object. This is typically just a single cell since infantry are never bigger than one    *
 *    cell and this routine presumes the infantry is located in the center of the cell.        *
 *                                                                                             *
 * INPUT:   placement   -- Is this for placement legality checking only? The normal condition  *
 *                         is for marking occupation flags.                                    *
 *                                                                                             *
 * OUTPUT:  Returns with a cell offset list for the infantry object as if it were located      *
 *          in the center of a cell.                                                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/24/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
Cell const * InfantryTypeClass::Occupy_List(bool) const
{
	static Cell const _list[] = {Cell(0, 0), REFRESH_EOL};

	return(&_list[0]);
}


/***********************************************************************************************
 * InfantryTypeClass::From_Name -- Converts an ASCII name into an infantry type number.        *
 *                                                                                             *
 *    This routine is used to convert the infantry ASCII name as specified into an infantry    *
 *    type number. This is called from the INI reader routine in the process if creating the   *
 *    infantry objects needed for the scenario.                                                *
 *                                                                                             *
 * INPUT:   name  -- The ASCII name to convert into an infantry type number.                   *
 *                                                                                             *
 * OUTPUT:  Returns with the infantry type number that corresponds to the infantry ASCII name  *
 *          specified. If no match could be found, then INFANTRY_NONE is returned.             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/24/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
InfantryType InfantryTypeClass::From_Name(char const * name)
{
	if (strcmpi(name, "<none>") != 0 && strcmpi(name, "none") != 0) {
		for (int classid = INFANTRY_FIRST; classid < InfantryTypes.Count(); classid++) {
			if (stricmp(InfantryTypes[classid]->Name(), name) == 0) {
				return((InfantryType)classid);
			}
		}
	}
	return(INFANTRY_NONE);
}


char const * SequenceName[DO_COUNT] = {
	"Ready",
	"Guard",
	"Prone",
	"Walk",
	"FireUp",
	"Down",
	"Crawl",
	"Up",
	"FireProne",
	"Idle1",
	"Idle2",
	"Die1",
	"Die2",
	"Die3",
	"Die4",
	"Die5",
	"Hover",
	"Fly",
	"Tumble",
	"FireFly",
	"Struggle"
};


/// <summary>
/// Fetches the animation sequence controls for this infantry type from the art database.
/// Every do-type the soldier can perform -- walking, firing, dying, going prone -- gets its
/// starting frame, frame count, per facing jump, and fixed facing from the sequence section
/// named by the artwork. An infantry type with no sequence section keeps whatever controls
/// it already had.
/// </summary>
void InfantryTypeClass::Read_Sequence_INI(void)
{
	char seqname[32];

	if (ArtINI.Get_String(Graphic_Name(), "Sequence", "", seqname, sizeof(seqname)) > 0) {

		DoInfoStruct * control = (DoInfoStruct *)DoControls; // RA2: allocated by the constructor

		for (int i = 0; i < DOINFO_COUNT; i++) {
			char dostring[32];
			char facing[10] = {""};

			if (ArtINI.Get_String(seqname, SequenceName[i], "", dostring, sizeof(dostring)) > 0) {
				DoInfoStruct * doinfo = &control[i];

				sscanf(dostring, "%d,%d,%d,%s", &doinfo->Frame, &doinfo->Count, &doinfo->Jump, facing);
				if (strcmp("N", facing) == 0) {
					doinfo->Facing = FACING_N;
				} else if (strcmp("NE", facing) == 0) {
					doinfo->Facing = FACING_NE;
				} else if (strcmp("E", facing) == 0) {
					doinfo->Facing = FACING_E;
				} else if (strcmp("SE", facing) == 0) {
					doinfo->Facing = FACING_SE;
				} else if (strcmp("S", facing) == 0) {
					doinfo->Facing = FACING_S;
				} else if (strcmp("SW", facing) == 0) {
					doinfo->Facing = FACING_SW;
				} else if (strcmp("W", facing) == 0) {
					doinfo->Facing = FACING_W;
				} else if (strcmp("NW", facing) == 0) {
					doinfo->Facing = FACING_NW;
				}
			}
		}
	}
}


/***********************************************************************************************
 * InfantryTypeClass::Read_INI -- Fetches infantry override values from the INI database.      *
 *                                                                                             *
 *    This routine will retrieve the override values for this infantry type class object from  *
 *    the INI database specified.                                                              *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to retrieve the data from.                  *
 *                                                                                             *
 * OUTPUT:  bool; Was the infantry section for this type found and data retrieved from it?     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/19/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool InfantryTypeClass::Read_INI(CCINIClass const & ini)
{
	IsDamageSparks = false;

	if (BASECLASS::Read_INI(ini)) {
		Pip = ini.Get_PipEnum(IniName, "Pip", Pip);
		VoiceComment = ini.Get_VocType_List(ini, IniName, "VoiceComment", VoiceComment);
		DeadBodies = TGet_TypeList<AnimTypeClass>(ini, IniName, "DeadBodies", DeadBodies);
		DeathAnims = TGet_TypeList<AnimTypeClass>(ini, IniName, "DeathAnims", DeathAnims);
		IsCyborg = ini.Get_Bool(IniName, "Cyborg", IsCyborg);
		NotHuman = ini.Get_Bool(IniName, "NotHuman", NotHuman);
		if (IsCyborg) IsDamageSparks = true;
		EnterWaterSound = ini.Get_VocType(IniName, "EnterWaterSound", (VocType)EnterWaterSound);
		LeaveWaterSound = ini.Get_VocType(IniName, "LeaveWaterSound", (VocType)LeaveWaterSound);
		IsFearless = ini.Get_Bool(IniName, "Fearless", IsFearless);
		IsFraidyCat = ini.Get_Bool(IniName, "Fraidycat", IsFraidyCat);
		IsCapture = ini.Get_Bool(IniName, "Infiltrate", IsCapture);
		Ivan = ini.Get_Bool(IniName, "Ivan", Ivan);
		Occupier = ini.Get_Bool(IniName, "Occupier", Occupier);
		Assaulter = ini.Get_Bool(IniName, "Assaulter", Assaulter);
		DetectionDistance = ini.Get_Int(IniName, "DetectionDistance", DetectionDistance);
		IsBomber = ini.Get_Bool(IniName, "C4", IsBomber);
		IsCivilian = ini.Get_Bool(IniName, "Civilian", IsCivilian);
		IsEngineer = ini.Get_Bool(IniName, "Engineer", IsEngineer);
		IsTiberiumProof = ini.Get_Bool(IniName, "TiberiumProof", IsTiberiumProof);
		IsAgent = ini.Get_Bool(IniName, "Agent", IsAgent);
		IsThief = ini.Get_Bool(IniName, "Thief", IsThief);
		IsVehicleThief = ini.Get_Bool(IniName, "VehicleThief", IsVehicleThief);
		IsDoggie = ini.Get_Bool(IniName, "Doggie", IsDoggie);
		Deployer = ini.Get_Bool(IniName, "Deployer", Deployer);
		if (IsBomber) IsCapture = true;
		if (IsEngineer) IsCapture = true;
		if (IsAgent) IsCapture = true;
		IsCrawling = ArtINI.Get_Bool(Graphic_Name(), "Crawls", IsCrawling);
		FireLaunch = ArtINI.Get_Int(Graphic_Name(), "FireUp", FireLaunch);
		ProneLaunch = ArtINI.Get_Int(Graphic_Name(), "FireProne", ProneLaunch);
		Read_Sequence_INI();
		return(true);
	}
	return(false);
}


/// <summary>
/// Fetches the bounding size of this infantry type, in leptons.
/// This is the volume that collision and targeting logic treats the soldier as filling.
/// </summary>
/// <returns>Returns with the width, height and depth of the infantry, in leptons.</returns>
Point3D InfantryTypeClass::Lepton_Dimensions(void) const
{
	return(Point3D(CELL_LEPTON_W / 3, CELL_LEPTON_H / 3, LEVEL_LEPTON_H * 2));
}


/// <summary>
/// Fetches the strength restored by one repair step of this infantry type.
/// </summary>
/// <returns>Returns with the number of strength points gained per repair step.</returns>
int InfantryTypeClass::Repair_Step(void) const
{
	return(Rule->IRepairStep);
}


/// <summary>
/// Fetches the credit cost of one repair step for this infantry type.
/// Infantry are healed rather than repaired, so the service is free.
/// </summary>
/// <returns>Returns with the cost, in credits, of a single repair step.</returns>
int InfantryTypeClass::Repair_Cost(void) const
{
	return(0);
}


// RA2: as the base class.
bool InfantryTypeClass::CanUseWaypoint(void) const
{
	return(BASECLASS::CanUseWaypoint());
}


// RA2: as the base class.
bool InfantryTypeClass::CanAttackMove(void) const
{
	return(BASECLASS::CanAttackMove());
}


/// <summary>
/// Adjusts a coordinate to be legal for this infantry type.
/// Infantry cannot occupy a position below the ground, so a coordinate that has sunk beneath
/// the terrain is lifted back up to it.
/// </summary>
/// <param name="coord">The coordinate to adjust.</param>
/// <returns>Returns with the adjusted coordinate.</returns>
Coord const InfantryTypeClass::Coord_Fixup(Coord const & coord) const
{
	Coord fix = coord;
	if (fix.Z < Map.Get_Height_GL(fix)) {
		fix.Z = Map.Get_Height_GL(fix);
	}
	return(fix);
}


/// <summary>
/// Submits this infantry type to the game state checksum.
/// The network sync check uses this routine to prove that every machine in the game is
/// running with identical infantry rules.
/// </summary>
void InfantryTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(Pip);
	crc(DoControls, sizeof(*DoControls) * DOINFO_COUNT);
	crc(FireLaunch);
	crc(ProneLaunch);
	crc(VoiceComment.Count());
	crc(DeadBodies.Count());
	crc(DeathAnims.Count());
	crc(IsFearless);
	crc(IsCrawling);
	crc(IsCapture);
	crc(IsFraidyCat);
	crc(IsTiberiumProof);
	crc(IsCivilian);
	crc(IsBomber);
	crc(IsEngineer);
	crc(IsAgent);
	crc(IsThief);
	crc(IsVehicleThief);
}


/// <summary>
/// Loads this infantry type from the stream specified.
/// The shape images are re-fetched and the voice comment list and animation sequence
/// controls are rebuilt, since none of those survive a raw read of the object.
/// </summary>
/// <returns>Returns with S_OK if the infantry type was read, otherwise a failure code.</returns>
HRESULT STDMETHODCALLTYPE InfantryTypeClass::Load(IStream * stream)
{
	if (DoControls != NULL) { // RA2: drop the controls allocated by the constructor
		delete (DoInfoStruct *)DoControls;
		DoControls = NULL;
	}

	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) InfantryTypeClass(NoInitClass());

		Fetch_Voxel_Image();
		Fetch_Normal_Image();

		VoiceComment.Load(stream); // RA2: the lists are loaded element by element
		DeadBodies.Load(stream);
		for (int index = 0; index < DeadBodies.Count(); index++) {
			Swizzle_Pointer(&DeadBodies[index]);
		}
		DeathAnims.Load(stream);
		for (index = 0; index < DeathAnims.Count(); index++) {
			Swizzle_Pointer(&DeathAnims[index]);
		}

		DoControls = new DoInfoStruct[DOINFO_COUNT];
		stream->Read((void *)DoControls, sizeof(DoInfoStruct) * DOINFO_COUNT, NULL);
	}
	return(result);
}


/// <summary>
/// Saves this infantry type to the stream specified.
/// The voice comment list and the animation sequence controls hang off this object rather
/// than living inside it, so they are written out after the object itself.
/// </summary>
/// <returns>Returns with S_OK if the infantry type was written, otherwise a failure
/// code.</returns>
HRESULT STDMETHODCALLTYPE InfantryTypeClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		int index;
		int count = VoiceComment.Count();
		stream->Write(&count, sizeof(count), NULL);
		for (index = 0; index < count; index++) {
			stream->Write(&VoiceComment[index], sizeof(int), NULL);
		}

		count = DeadBodies.Count();
		stream->Write(&count, sizeof(count), NULL);
		for (index = 0; index < count; index++) {
			stream->Write(&DeadBodies[index], sizeof(AnimTypeClass *), NULL);
		}

		count = DeathAnims.Count();
		stream->Write(&count, sizeof(count), NULL);
		for (index = 0; index < count; index++) {
			stream->Write(&DeathAnims[index], sizeof(AnimTypeClass *), NULL);
		}

		stream->Write((void *)DoControls, sizeof(DoInfoStruct) * DOINFO_COUNT, NULL);
	}
	return(result);
}


/// <summary>
/// Fetches the class identifier of this object.
/// The save game system stores this identifier so that the object can be recreated as the
/// correct class when the game is loaded.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no return pointer was supplied.</returns>
HRESULT STDMETHODCALLTYPE InfantryTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_InfantryTypeClass;
	return(S_OK);
}


/// <summary>
/// Fetches the infantry type of the name specified, creating it if need be.
/// This routine is used while processing the rules and scenario INI databases, where an
/// infantry type can be mentioned before it has been declared.
/// </summary>
/// <param name="name">The internal INI name of the infantry type wanted.</param>
/// <returns>Returns with a pointer to the matching infantry type.</returns>
InfantryTypeClass * InfantryTypeClass::Find_Or_Make(char const * name)
{
	return(TFind_Or_Make<InfantryTypeClass>(name, InfantryTypes));
}


/// <summary>
/// Fetches the RTTI type of this object.
/// </summary>
/// <returns>Returns with RTTI_INFANTRYTYPE.</returns>
RTTIType InfantryTypeClass::Fetch_RTTI(void) const
{
	return(RTTI_INFANTRYTYPE);
}


/// <summary>
/// Fetches the number of bytes this infantry type occupies when saved.
/// Save games written by an earlier version of the game store a shorter record, so the size
/// is trimmed accordingly when such a save is being processed.
/// </summary>
/// <param name="oldsave">Is the size wanted for an older format save game?</param>
/// <returns>Returns with the size, in bytes, of this infantry type.</returns>
int InfantryTypeClass::Fetch_Object_Size(void) const // RA2: no argument
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the heap index of this infantry type.
/// </summary>
/// <returns>Returns with the position of this type within the infantry type heap.</returns>
int InfantryTypeClass::Fetch_Heap_ID(void) const
{
	return(HeapID);
}
