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

/* $Header: /CounterStrike/WEAPON.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WEAPON.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 05/20/96                                                     *
 *                                                                                             *
 *                  Last Update : September 9, 1996 [JLB]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Armor_From_Name -- Convert ASCII name into armor type number.                             *
 *   WeaponTypeClass::As_Pointer -- Give a weapon type ID, fetch pointer to weapon type object.*
 *   WeaponTypeClass::Read_INI -- Fetch the weapon data from the INI database.                 *
 *   WeaponTypeClass::WeaponTypeClass -- Default constructor for weapon type objects.          *
 *   WeaponTypeClass::operator delete -- Returns weapon type object back to special heap.      *
 *   WeaponTypeClass::operator new -- Allocates a weapon type object form the special heap.    *
 *   WeaponTypeClass::~WeaponTypeClass -- Destructor for weapon type class objects.            *
 *   Weapon_From_Name -- Conver ASCII name to weapon type ID number.                           *
 *   WeaponTypeClass::Allowed_Threats -- Determine what threats this weapon can address.       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define INCLUDE_COM
#include "always.h"

#include "weapon.h"

#include "_rules.h"
#include "_weapon.h"
#include "animtype.h"
#include "bullettype.h"
#include "combat.h"
#include "findmake.h"
#include "globals.h"
#include "incdec.h"
#include "psystype.h"
#include "rules.h"
#include "session.h"
#include "sun.h"
#include "swizzle.h"
#include "tracker.h"
#include "warhead.h"

#include <cstring>


/***********************************************************************************************
 * WeaponTypeClass::WeaponTypeClass -- Default constructor for weapon type objects.            *
 *                                                                                             *
 *    This default constructor will initialize all the values of the weapon to the default     *
 *    state. Thus, if any of these settings are not specifically overridden by the rules.ini   *
 *    file, they will remain this value in the game.                                           *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
WeaponTypeClass::WeaponTypeClass(char const * ininame) :
	BASECLASS(ininame),
	AmbientDamage(0),
	Burst(1),
	Bullet(NULL),
	Attack(0),
	MaxSpeed(MPH_IMMOBILE),
	WarheadPtr(NULL),
	ROF(0),
	Range(0),
	MinimumRange(0),
	Sound(),
	Anim(),
	AssaultAnim(NULL),
	AttachedParticleSystem(NULL),
	LaserInnerColor(0, 0, 0),
	LaserOuterColor(0, 0, 0),
	LaserOuterSpread(0, 0, 0),
	UseFireParticles(false),
	UseSparkParticles(false),
	OmniFire(false),
	DistributedWeaponFire(false),
	IsRailgun(false),
	IsLobber(false),
	IsBright(false),
	IsSonic(false),
	Spawner(false),
	LimboLaunch(false),
	DecloakToFire(true),
	CellRangefinding(false),
	FireOnce(false),
	NeverUse(false),
	RevealOnFire(true),
	TerrainFire(false),
	SabotageCursor(false),
	DisguiseFireOnly(false),
	DisguiseFakeBlinkTime(0),
	Suicide(false),
	IsTurboBoosted(false),
	IsSupressed(false),
	IsCamera(false),
	IsElectric(false),
	IsLaser(false),
	IsBigLaser(false),
	IsHouseColor(false),
	LaserDuration(10),
	IsIonSensitive(false),
	AreaFire(false),
	IsElectricBolt(false),
	DrawBoltAsLaser(false),
	IsAlternateColor(false),
	IsRadBeam(false),
	IsRadEruption(false),
	RadLevel(0)
{
	Create_ID();
	Weapons.Add(this);
}


/// <summary>
/// Constructs a weapon type object, leaving its data untouched.
/// This constructor is used by the load process, which is about to overwrite the object
/// with the data it reads from the save game file.
/// </summary>
WeaponTypeClass::WeaponTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	Sound(x),
	Anim(x),
	LaserInnerColor(x),
	LaserOuterColor(x),
	LaserOuterSpread(x)
{
}


/***********************************************************************************************
 * WeaponTypeClass::~WeaponTypeClass -- Destructor for weapon type class objects.              *
 *                                                                                             *
 *    This destructor really doesn't do anything but set the pointers to NULL. This is a       *
 *    general purposes safety tactic but is otherwise useless.                                 *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
WeaponTypeClass::~WeaponTypeClass(void)
{
	Detach_This_From_All(this, true);
	Bullet = NULL;
	WarheadPtr = NULL;

	Weapons.Delete(this);
}


/***********************************************************************************************
 * WeaponTypeClass::Read_INI -- Fetch the weapon data from the INI database.                   *
 *                                                                                             *
 *    This routine will fetch the weapon data for this weapon type object from the INI         *
 *    database specified.                                                                      *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that the weapon data will be fetched        *
 *                   from.                                                                     *
 *                                                                                             *
 * OUTPUT:  bool; Was this weapon type described in the database and the values retrieved?     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/19/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool WeaponTypeClass::Read_INI(CCINIClass const & ini)
{
	if (ini.Is_Present(IniName)) {
		AmbientDamage = ini.Get_Int(IniName, "AmbientDamage", AmbientDamage);
		IsSonic = ini.Get_Bool(IniName, "IsSonic", IsSonic);
		Spawner = ini.Get_Bool(IniName, "Spawner", Spawner);
		LimboLaunch = ini.Get_Bool(IniName, "LimboLaunch", LimboLaunch);
		DecloakToFire = ini.Get_Bool(IniName, "DecloakToFire", DecloakToFire);
		CellRangefinding = ini.Get_Bool(IniName, "CellRangefinding", CellRangefinding);
		FireOnce = ini.Get_Bool(IniName, "FireOnce", FireOnce);
		NeverUse = ini.Get_Bool(IniName, "NeverUse", NeverUse);
		RevealOnFire = ini.Get_Bool(IniName, "RevealOnFire", RevealOnFire);
		TerrainFire = ini.Get_Bool(IniName, "TerrainFire", TerrainFire);
		SabotageCursor = ini.Get_Bool(IniName, "SabotageCursor", SabotageCursor);
		DisguiseFireOnly = ini.Get_Bool(IniName, "DisguiseFireOnly", DisguiseFireOnly);
		DisguiseFakeBlinkTime = ini.Get_Int(IniName, "DisguiseFakeBlinkTime", DisguiseFakeBlinkTime);
		Suicide = ini.Get_Bool(IniName, "Suicide", Suicide);
		IsSupressed = ini.Get_Bool(IniName, "Supress", IsSupressed);
		Burst = ini.Get_Int(IniName, "Burst", Burst);
		Attack = ini.Get_Int(IniName, "Damage", Attack);
		MaxSpeed = ini.Get_MPHType(IniName, "Speed", MaxSpeed);
		ROF = ini.Get_Int(IniName, "ROF", ROF);
		Range = ini.Get_Lepton(IniName, "Range", Range);
		MinimumRange = ini.Get_Lepton(IniName, "MinimumRange", MinimumRange);

		Sound = ini.Get_VocType_List(ini, IniName, "Report", Sound);
		Anim = TGet_TypeList<AnimTypeClass>(ini, IniName, "Anim", Anim);
		AssaultAnim = TGet_Class(ini, IniName, "AssaultAnim", AssaultAnim);

		IsCamera = ini.Get_Bool(IniName, "Camera", IsCamera);
		IsLaser = ini.Get_Bool(IniName, "IsLaser", IsLaser);
		IsHouseColor = ini.Get_Bool(IniName, "IsHouseColor", IsHouseColor);
		IsElectric = ini.Get_Bool(IniName, "Charges", IsElectric);
		IsTurboBoosted = ini.Get_Bool(IniName, "TurboBoost", IsTurboBoosted);

		UseFireParticles = ini.Get_Bool(IniName, "UseFireParticles", UseFireParticles);
		UseSparkParticles = ini.Get_Bool(IniName, "UseSparkParticles", UseSparkParticles);
		OmniFire = ini.Get_Bool(IniName, "OmniFire", OmniFire);
		DistributedWeaponFire = ini.Get_Bool(IniName, "DistributedWeaponFire", DistributedWeaponFire);
		IsRailgun = ini.Get_Bool(IniName, "IsRailgun", IsRailgun);
		IsLobber = ini.Get_Bool(IniName, "Lobber", IsLobber);

		LaserInnerColor = ini.Get_RGBClass(IniName, "LaserInnerColor", LaserInnerColor);
		LaserOuterColor = ini.Get_RGBClass(IniName, "LaserOuterColor", LaserOuterColor);
		LaserOuterSpread = ini.Get_RGBClass(IniName, "LaserOuterSpread", LaserOuterSpread);
		LaserDuration = ini.Get_Int(IniName, "LaserDuration", LaserDuration);
		IsBigLaser = ini.Get_Bool(IniName, "IsBigLaser", IsBigLaser);

		IsBright = ini.Get_Bool(IniName, "Bright", IsBright);
		IsIonSensitive = ini.Get_Bool(IniName, "IonSensitive", IsIonSensitive);
		AreaFire = ini.Get_Bool(IniName, "AreaFire", AreaFire);
		IsElectricBolt = ini.Get_Bool(IniName, "IsElectricBolt", IsElectricBolt);
		DrawBoltAsLaser = ini.Get_Bool(IniName, "DrawBoltAsLaser", DrawBoltAsLaser);
		IsAlternateColor = ini.Get_Bool(IniName, "IsAlternateColor", IsAlternateColor);
		IsRadBeam = ini.Get_Bool(IniName, "IsRadBeam", IsRadBeam);
		IsRadEruption = ini.Get_Bool(IniName, "IsRadEruption", IsRadEruption);
		RadLevel = ini.Get_Int(IniName, "RadLevel", RadLevel);

		char buffer[20];
		buffer[0] = '\0';
		ini.Get_String(IniName, "AttachedParticleSystem", buffer, buffer, sizeof(buffer));
		ParticleSystemType ps = ParticleSystemTypeClass::From_Name(buffer);
		if (ps != PARTSYS_NONE) {
			AttachedParticleSystem = ParticleSystemTypes[ps];
		}

		WarheadPtr = TGet_Class(ini, IniName, "Warhead", WarheadPtr);
		Bullet = TGet_Class(ini, IniName, "Projectile", Bullet);

		return(true);
	}
	return(false);
}


/// <summary>
/// Sets the launch speed for a ballistic weapon.
/// A weapon that throws an unguided projectile must launch it hard enough to carry it the
/// weapon's full range, so the speed is worked out from that range and the gravity the
/// projectile falls under. A guided projectile steers itself and is left alone.
/// </summary>
/// <remarks>Only call this routine once the rules have been read, since it leans on them.</remarks>
void WeaponTypeClass::Init_Max_Speed(void)
{
	if (Bullet != NULL && Bullet->ROT == 0) {
		double gravity = Rule->Gravity;
		if (Bullet->IsFloater) {
			gravity = Get_Floater_Gravity();
		}
		MaxSpeed = Calculate_Projectile_Speed(Range, gravity);
	}
}


/***********************************************************************************************
 * Armor_From_Name -- Convert ASCII name into armor type number.                               *
 *                                                                                             *
 *    This will find the armor type that matches the ASCII name specified and then will return *
 *    the armor ID number of it.                                                               *
 *                                                                                             *
 * INPUT:   name  -- Pointer to the ASCII name of the armor to find.                           *
 *                                                                                             *
 * OUTPUT:  Returns with the armor ID number of the armor that matches the name specified. If  *
 *          no match could be found, then ARMOR_NONE is returned.                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
ArmorType Armor_From_Name(char const * name)
{
	if (!name) return(ARMOR_NONE);

	for (ArmorType index = ARMOR_FIRST; index < ARMOR_COUNT; index++) {
		if (stricmp(ArmorName[index], name) == 0) {
			return(index);
		}
	}

	return(ARMOR_NONE);
}


/***********************************************************************************************
 * WeaponTypeClass::Allowed_Threats -- Determine what threats this weapon can address.         *
 *                                                                                             *
 *    This routine will examine the capabilities of this weapon and return with the threat     *
 *    types that it can address.                                                               *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the threat types that this weapon can address.                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/09/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
ThreatType WeaponTypeClass::Allowed_Threats(void) const
{
	ThreatType threat = THREAT_NORMAL;
	if (Bullet->IsAntiAircraft) {
		threat = ThreatType(threat | THREAT_AIR);
	}
	if (Bullet->IsAntiGround) {
		threat = ThreatType(threat | THREAT_INFANTRY|THREAT_VEHICLES|THREAT_BOATS|THREAT_BUILDINGS);
	}
	return(threat);
}


/// <summary>
/// Can this weapon knock down a wall?
/// The answer comes from the warhead the weapon fires, and it is used when deciding whether
/// an object should bother attacking a wall that is in its way.
/// </summary>
/// <returns>bool; Will this weapon destroy walls?</returns>
bool WeaponTypeClass::Is_Wall_Destroyer(void) const
{
	if (WarheadPtr != NULL && WarheadPtr->IsWallDestroyer) {
		return(true);
	}
	return(false);
}


/// <summary>
/// Adds this weapon type's data to the CRC calculation.
/// This routine is used by the multiplayer sync checker to prove that every machine agrees
/// about the weapon, so the projectile and warhead it refers to are submitted by identifier
/// rather than by pointer.
/// </summary>
/// <param name="crc">The checksum engine to submit the data to.</param>
void WeaponTypeClass::Compute_CRC(CRCEngine &crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(AmbientDamage);
	crc(IsSonic);
	crc(IsTurboBoosted);
	crc(IsSupressed);
	crc(IsCamera);
	crc(IsLaser);
	crc(IsHouseColor);
	crc(Burst);
	if (Bullet != NULL) crc(Bullet->Fetch_ID());
	crc(Attack);
	crc(MaxSpeed);
	if (WarheadPtr != NULL) crc(WarheadPtr->Fetch_ID());
	crc(ROF);
	crc(Range);
	crc(Sound.Count());
	crc(Anim.Count());
	crc(UseFireParticles);
	crc(IsLobber);
	crc((char)LaserDuration);
	crc(IsBigLaser);
	crc(IsRailgun);
	crc(IsElectric);
	crc(IsBright);
	crc(UseSparkParticles);
	crc(OmniFire);
	crc(DistributedWeaponFire);
}


/// <summary>
/// Fetches the class identifier of this object.
/// This routine is used by the persistence machinery to recognize what kind of object it
/// is about to load back.
/// </summary>
/// <param name="retval">Pointer to the identifier to fill in.</param>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE WeaponTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_WeaponTypeClass;
	return(S_OK);
}


/// <summary>
/// Loads this weapon type from the save game stream.
/// A weapon type saved by an older version of the game lacks the projectile range and
/// burst delay data, so such a save is patched up to the current layout as it is read. The
/// object is then rebuilt in place, its sound and animation lists are read back, and the
/// saved pointers are remapped to the objects of this session.
/// </summary>
/// <returns>Returns with S_OK if the weapon type was loaded, otherwise the failure code.</returns>
HRESULT STDMETHODCALLTYPE WeaponTypeClass::Load(IStream * stream)
{
	Sound.Clear();
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) WeaponTypeClass(NoInitClass());

		Anim.Load(stream);
		Sound.Load(stream);

		Swizzle_Pointer(&WarheadPtr);
		Swizzle_Pointer(&Bullet);
		Swizzle_Pointer(&AttachedParticleSystem);
		Swizzle_Pointer(&AssaultAnim);

		for (int i = 0; i < Anim.Count(); i++) {
			Swizzle_Pointer(&Anim[i]);
		}

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Writes this weapon type out to the save game stream.
/// The base class writes the raw object image, and this routine then writes out the
/// report sound and firing animation lists that hang off it.
/// </summary>
/// <param name="cleardirty">Should the object be marked as clean once it has been written?</param>
/// <returns>Returns with S_OK if the weapon type was written, otherwise the failure code.</returns>
HRESULT STDMETHODCALLTYPE WeaponTypeClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		Anim.Save(stream);
		Sound.Save(stream);

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Fetches the weapon of the name specified, creating it if it does not exist yet.
/// Use this routine while processing the rules so that a weapon mentioned before its own
/// section has been read will still resolve to a real object.
/// </summary>
/// <param name="name">The INI name of the weapon desired.</param>
/// <returns>Returns with a pointer to the weapon type of that name.</returns>
WeaponTypeClass * WeaponTypeClass::Find_Or_Make(const char *name)
{
	return(TFind_Or_Make<WeaponTypeClass>(name, Weapons));
}


/***********************************************************************************************
 * Weapon_From_Name -- Conver ASCII name to weapon type ID number.                             *
 *                                                                                             *
 *    This will find the weapon whos name matches that specified and then it will return the   *
 *    weapon ID number associated with it.                                                     *
 *                                                                                             *
 * INPUT:   name  -- Pointer to the ASCII name of the weapon type.                             *
 *                                                                                             *
 * OUTPUT:  Returns with the weapon type ID number that matches the name specified. If no      *
 *          match could be found, then WEAPON_NONE is returned.                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/17/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
WeaponType WeaponTypeClass::From_Name(char const * name)
{
	for (int index = 0; index < Weapons.Count(); index++) {
		if (stricmp(Weapons[index]->IniName, name) == 0) {
			return(WeaponType(index));
		}
	}
	return(WEAPON_NONE);
}


/**
 *  RA2: The speed of the projectile for the range given. Ballistic projectiles (no rate of
 *  turn) are given the speed needed to cover the range under gravity.
 */
int WeaponTypeClass::Get_Speed(int range) const
{
	if (Bullet != NULL && Bullet->ROT == 0) {
		double gravity = Bullet->IsFloater ? Get_Floater_Gravity() : (double)Rule->Gravity;
		return(Calculate_Projectile_Speed(range, gravity));
	}
	return(MaxSpeed);
}


/// <summary>
/// Fetches the size this object occupies in a save game file.
/// This routine is used by the load process, which must know how much smaller a weapon type
/// was in save games written before the projectile range and burst delay data was added.
/// </summary>
/// <param name="oldsave">Is the size wanted for the older save game layout?</param>
/// <returns>Returns with the number of bytes this object occupies in the save file.</returns>
int WeaponTypeClass::Fetch_Object_Size(void) const // RA2: no argument
{
	return(sizeof(*this));
}
