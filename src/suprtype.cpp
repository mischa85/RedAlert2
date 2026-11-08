/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#define INCLUDE_COM
#include "always.h"

#include "suprtype.h"

#include "_map.h"
#include "_mixfile.h"
#include "building.h"
#include "builtype.h"
#include "cell.h"
#include "findmake.h"
#include "globals.h"
#include "mixfile.h"
#include "mouse.h"
#include "noinit.h"
#include "sun.h"
#include "swizzle.h"
#include "weapon.h"


/// <summary>
/// Converts a super weapon behavior name into its type number.
/// The behavior name says which hard coded effect a super weapon delivers -- the ion
/// cannon, the multi missile, and so on -- rather than which rules section declared it.
/// </summary>
/// <returns>Returns with the super weapon type named. Otherwise, SUPER_NONE is
/// returned.</returns>
SuperWeaponType Special_From_Name(char const * name)
{
	static const char * _names[SUPER_COUNT] = {
		"MultiMissile",
		"EMPulse",
		"Firestorm",
		"IonCannon",
		"HunterSeeker",
		"ChemMissile",
		"DropPod"
	};

	for (int i = SUPER_FIRST; i < SUPER_COUNT; i++) {
		if (strcmpi(_names[i], name) == 0) {
			return(SuperWeaponType)(i);
		}
	}
	return(SUPER_NONE);
}


/// <summary>
/// Constructs a super weapon type of the rules name specified.
/// The new type is given a heap identifier and added to the super weapon type heap. Only
/// harmless defaults are installed here; Read_INI supplies the real characteristics.
/// </summary>
/// <param name="ininame">The rules section this super weapon is declared by.</param>
SuperWeaponTypeClass::SuperWeaponTypeClass(char const * ininame) :
	BASECLASS(ininame),
	Weapon(NULL),
	VoxRecharge(VOX_NONE),
	VoxCharging(VOX_NONE),
	VoxImpatient(VOX_NONE),
	VoxSuspend(VOX_NONE),
	RechargeTime(4500),
	Type(SUPER_NONE),
	CameoData(NULL),
	Action(ACTION_NONE),
	AuxBuilding(NULL),
	SidebarImage(),
	UseChargeDrain(false),
	IsPowered(true),
	DisableableFromShell(false),
	PreClick(false),
	PostClick(false),
	PreDependent(-1),
	ShowTimer(false),
	IsManualControl(false)
{
	Create_ID();
	HeapID = (SuperWeaponType)SuperWeaponTypes.Count();
	SuperWeaponTypes.Add(this);
	SidebarImage = IniName;
}


/// <summary>
/// Constructs a super weapon type without initializing it.
/// This constructor is used by the load process, which is about to overwrite the object
/// with the data it reads from the save game stream.
/// </summary>
SuperWeaponTypeClass::SuperWeaponTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	SidebarImage(x),
	CameoData(NULL)
{

}


/// <summary>
/// Removes this super weapon type from the game.
/// The type is unlinked from the super weapon type heap so that nothing can reach it
/// afterwards.
/// </summary>
SuperWeaponTypeClass::~SuperWeaponTypeClass(void)
{
	SuperWeaponTypes.Delete(this);
}


/// <summary>
/// Fetches the class identifier of this object.
/// The save game system uses this to know which class to construct when the object is
/// read back in.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE SuperWeaponTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_SuperWeaponTypeClass;
	return(S_OK);
}


/// <summary>
/// Reads this super weapon type back from the save game stream.
/// The weapon and auxiliary building references are remapped to their new addresses, and
/// the sidebar cameo is fetched from the mix files again since artwork is never written
/// to a save game.
/// </summary>
/// <returns>Returns with S_OK if the object was read successfully.</returns>
HRESULT STDMETHODCALLTYPE SuperWeaponTypeClass::Load(IStream *stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) SuperWeaponTypeClass(NoInitClass());

		Swizzle_Pointer(&Weapon);
		Swizzle_Pointer(&AuxBuilding);

		char fullname[_MAX_FNAME+_MAX_EXT];
		_makepath(fullname, NULL, NULL, SidebarImage, ".SHP");
		CameoData = (ShapeSet *)MFCD::Retrieve(fullname);
		if (CameoData == NULL) {
			CameoData = (ShapeSet *)MFCD::Retrieve("XXICON.SHP");
		}

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Writes this super weapon type to the save game stream.
/// </summary>
/// <param name="cleardirty">Should the object be marked as clean once written?</param>
/// <returns>Returns with S_OK if the object was written successfully.</returns>
HRESULT STDMETHODCALLTYPE SuperWeaponTypeClass::Save(IStream *stream, int cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Fetches the run time type of this object.
/// </summary>
/// <returns>Returns with RTTI_SUPERWEAPONTYPE.</returns>
RTTIType SuperWeaponTypeClass::Fetch_RTTI(void) const
{
	return(RTTI_SUPERWEAPONTYPE);
}


/// <summary>
/// Fetches the size of this object for the save game system.
/// </summary>
/// <returns>Returns with the number of bytes this object occupies.</returns>
int SuperWeaponTypeClass::Fetch_Object_Size(void) const // RA2: no argument
{
	return(sizeof(*this));
}


/// <summary>
/// Submits this super weapon type to the game state checksum.
/// This routine is used by the multiplayer sync check to prove that every machine is
/// playing with the same rules.
/// </summary>
void SuperWeaponTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(HeapID);
	crc(Action);
	crc(VoxRecharge);
	crc(VoxCharging);
	crc(VoxImpatient);
	crc(VoxSuspend);
	crc(IsPowered);
	crc(DisableableFromShell);
	crc(PreClick);
	crc(PostClick);
	crc(PreDependent);
	crc(ShowTimer);
	crc(RechargeTime);
	crc(Type);
	crc(UseChargeDrain);
	crc(IsManualControl);
}


/// <summary>
/// Fetches the heap identifier of this super weapon type.
/// </summary>
/// <returns>Returns with the index of this object within the super weapon type heap.</returns>
int SuperWeaponTypeClass::Fetch_Heap_ID(void) const
{
	return(HeapID);
}


/// <summary>
/// Fetches this super weapon's characteristics from the rules.
/// This routine picks up the weapon, the announcement voices, the recharge delay, and
/// the sidebar cameo artwork. A missing cameo falls back on the generic icon so that the
/// sidebar always has something to draw.
/// </summary>
/// <returns>bool; Was the database entry found and read?</returns>
bool SuperWeaponTypeClass::Read_INI(CCINIClass const & ini)
{
	if (BASECLASS::Read_INI(ini)) {
		Weapon = TGet_Class(ini, IniName, "WeaponType", Weapon);
		Action = ini.Get_ActionType(IniName, "Action", Action);
		IsPowered = ini.Get_Bool(IniName, "IsPowered", IsPowered);
		DisableableFromShell = ini.Get_Bool(IniName, "DisableableFromShell", DisableableFromShell);
		PreClick = ini.Get_Bool(IniName, "PreClick", PreClick);
		PostClick = ini.Get_Bool(IniName, "PostClick", PostClick);
		ShowTimer = ini.Get_Bool(IniName, "ShowTimer", ShowTimer);



		char type_name[40];
		char dependent_name[40];
		ini.Get_String(IniName, "Type", "", type_name, sizeof(type_name));
		if (strlen(type_name) != 0) {
			SuperWeaponType type = Special_From_Name(type_name);
			if (type != SUPER_NONE) Type = type;
		}

		ini.Get_String(IniName, "PreDependent", "", dependent_name, sizeof(dependent_name));
		if (strlen(dependent_name) != 0) {
			SuperWeaponType type = Special_From_Name(dependent_name);
			if (type != SUPER_NONE) PreDependent = type;
		}

		AuxBuilding = TGet_Class(ini, IniName, "AuxBuilding", AuxBuilding);
		UseChargeDrain = ini.Get_Bool(IniName, "UseChargeDrain", UseChargeDrain);
		IsManualControl = ini.Get_Bool(IniName, "ManualControl", IsManualControl);

		double recharge = ini.Get_Float(IniName, "RechargeTime");
		if (recharge != 0.0) {
			RechargeTime = (float)recharge * 900.0f;
		}

		char sidebar_image[256];
		ini.Get_String(IniName, "SidebarImage", (char *)SidebarImage, sidebar_image, sizeof(sidebar_image));
		SidebarImage = sidebar_image;

		char fullname[_MAX_FNAME+_MAX_EXT];
		_makepath(fullname, NULL, NULL, SidebarImage, ".SHP");
		CameoData = (ShapeSet const *)MFCD::Retrieve(fullname);
		if (CameoData == NULL) {
			CameoData = (ShapeSet const *)MFCD::Retrieve("XXICON.SHP");
		}

		return(true);
	}
	return(false);
}


/// <summary>
/// Converts a rules name into a super weapon type number.
/// </summary>
/// <returns>Returns with the super weapon type that bears the name. Otherwise, SUPER_NONE
/// is returned.</returns>
SuperWeaponType SuperWeaponTypeClass::From_Name(char const * name)
{
	if (name != NULL) {
		for (int classid = SUPER_FIRST; classid < SuperWeaponTypes.Count(); classid++) {
			if (stricmp(SuperWeaponTypes[classid]->IniName, name) == 0) {
				return(SuperWeaponType)classid;
			}
		}
	}
	return(SUPER_NONE);
}


/// <summary>
/// Fetches the super weapon type that performs the action specified.
/// This routine is used to work back from a mouse action to the super weapon that
/// offers it.
/// </summary>
/// <returns>Returns with a pointer to the matching super weapon type. Otherwise, NULL is
/// returned.</returns>
SuperWeaponTypeClass * SuperWeaponTypeClass::From_Action(ActionType action)
{
	for (int classid = SUPER_FIRST; classid < SuperWeaponTypes.Count(); classid++) {
		if (SuperWeaponTypes[classid]->Action == action) {
			return(SuperWeaponTypes[classid]);
		}
	}
	return(NULL);
}


/// <summary>
/// Fetches the super weapon type of the name specified, creating it if need be.
/// This routine is used while the rules are being parsed so that a super weapon may be
/// referred to before its own section has been reached.
/// </summary>
/// <returns>Returns with a pointer to the super weapon type object.</returns>
SuperWeaponTypeClass * SuperWeaponTypeClass::Find_Or_Make(const char *name)
{
	return(TFind_Or_Make<SuperWeaponTypeClass>(name, SuperWeaponTypes));
}


/// <summary>
/// Determines the action to perform when this super weapon is aimed at a target.
/// Most super weapons simply report the action their rules entry names. The EM pulse is
/// the exception, since it must be delivered by a powered pulse cannon -- this routine
/// tells the cursor logic whether the player has one that can reach the target.
/// </summary>
/// <param name="object">The object being targeted, or NULL if the cell itself is the
/// target.</param>
/// <returns>Returns with the action to perform, which the caller turns into a mouse
/// shape.</returns>
ActionType SuperWeaponTypeClass::What_Action(Cell const & cell, ObjectClass * object)
{
	return(Action); // RA2: the same action for every target
}
