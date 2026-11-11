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

/* $Header: /CounterStrike/HDATA.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : HDATA.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : May 22, 1994                                                 *
 *                                                                                             *
 *                  Last Update : September 4, 1996 [JLB]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   HouseTypeClass::As_Reference -- Fetches a reference to the house specified.               *
 *   HouseTypeClass::From_Name -- Fetch house pointer from its name.                           *
 *   HouseTypeClass::HouseTypeClass -- Constructor for house type objects.                     *
 *   HouseTypeClass::Init_Heap -- Allocate all heap objects for the house types.               *
 *   HouseTypeClass::One_Time -- One-time initialization                                       *
 *   HouseTypeClass::Read_INI -- Fetch the house control values from ini database.             *
 *   HouseTypeClass::Remap_Table -- Fetches the remap table for this house.                    *
 *   HouseTypeClass::operator delete -- Returns a house type object back to the heap.          *
 *   HouseTypeClass::operator new -- Allocates a house type class object from special heap.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define INCLUDE_COM
#include "always.h"

#include "houstype.h"

#include "ccini.h"
#include "crc.h"
#include "findmake.h"
#include "globals.h"
#include "side.h"
#include "sun.h"
#include "swizzle.h"
#include "tracker.h"
#include "vector.h"

/***********************************************************************************************
 * HouseTypeClass::HouseTypeClass -- Constructor for house type objects.                       *
 *                                                                                             *
 *    This is the constructor for house type objects. This object holds the constant data      *
 *    for the house type.                                                                      *
 *                                                                                             *
 * INPUT:   see below...                                                                       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/21/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
HouseTypeClass::HouseTypeClass(char const * ininame) :
	BASECLASS(ininame),
	House(HOUSE_NONE),
	HeapID(HOUSE_NONE),
	Side(SIDE_NONE),
	FirepowerBias(1.0),
	GroundspeedBias(1.0),
	AirspeedBias(1.0),
	ArmorBias(1.0),
	ROFBias(1.0),
	CostBias(1.0),
	BuildSpeedBias(1.0),
	Scheme(0),
	Prefix('A'),
	IsMultiplay(false),
	IsMultiplayPassive(false),
	IsWallOwner(true),
	IsSmartAI(false)
{
	Create_ID();
	Suffix[0] = '\0';
	HouseTypes.Add(this);
	House = (HousesType)HouseTypes.ID(this);
	HeapID = (HousesType)HouseTypes.ID(this);
}


/// <summary>
/// Constructs a house type without initializing it.
/// This constructor is used by the load process, which fills the object in from the save
/// stream rather than building it up from scratch.
/// </summary>
HouseTypeClass::HouseTypeClass(NoInitClass const & x) :
	BASECLASS(x)
{
}


/// <summary>
/// Removes this house type from the game.
/// Everything that refers to this house type is detached from it before it is dropped from
/// the house type heap.
/// </summary>
HouseTypeClass::~HouseTypeClass(void)
{
	Detach_This_From_All(this, true);
	HouseTypes.Delete(this);
}


/***********************************************************************************************
 * HouseTypeClass::From_Name -- Fetch house pointer from its name.                             *
 *                                                                                             *
 *    This routine will convert the ASCII house name specified into a                          *
 *    real house number. Typically, this is used when processing a                             *
 *    scenario INI file.                                                                       *
 *                                                                                             *
 * INPUT:   name  -- ASCII name of house to process.                                           *
 *                                                                                             *
 * OUTPUT:  Returns with actual house number represented by the ASCII                          *
 *          name specified.                                                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/07/1992 JLB : Created.                                                                 *
 *   05/21/1994 JLB : Converted to member function.                                            *
 *=============================================================================================*/
HousesType HouseTypeClass::From_Name(char const * name)
{
	if (name != NULL) {
		for (int house = HOUSE_FIRST; house < HouseTypes.Count(); house++) {
			HouseTypeClass *ptr = HouseTypes[house];
			if (stricmp(ptr->Full_Name(), name) == 0 || stricmp(ptr->Name(), name) == 0) {
				return(ptr->House);
			}
		}
	}
	return(HOUSE_NONE);
}


/***********************************************************************************************
 * HouseTypeClass::Read_INI -- Fetch the house control values from ini database.               *
 *                                                                                             *
 *    This routine will fetch the rules controllable values for the house type from the        *
 *    INI database specified.                                                                  *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database to fetch the house control values from.     *
 *                                                                                             *
 * OUTPUT:  bool; Was the house section found and processed?                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/04/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool HouseTypeClass::Read_INI(CCINIClass const & ini)
{
	if (BASECLASS::Read_INI(ini)) {
		char buffer[32];
		ini.Get_String(Name(), "Suffix", "", buffer, sizeof(Suffix));
		if (strlen(buffer) != 0) {
			strcpy(Suffix, buffer);
		}

		Scheme = ini.Get_Scheme_Index(Name(), "Color", Scheme);

		char prefix[2];
		buffer[0] = Prefix;
		buffer[1] = '\0';
		ini.Get_String(Name(), "Prefix", buffer, prefix, sizeof(prefix));
		Prefix = prefix[0];

		FirepowerBias = ini.Get_Float(Name(), "Firepower", FirepowerBias);
		GroundspeedBias = ini.Get_Float(Name(), "Groundspeed", GroundspeedBias);
		AirspeedBias = ini.Get_Float(Name(), "Airspeed", AirspeedBias);
		ArmorBias = ini.Get_Float(Name(), "Armor", ArmorBias);
		ROFBias = ini.Get_Float(Name(), "ROF", ROFBias);
		CostBias = ini.Get_Float(Name(), "Cost", CostBias);
		BuildSpeedBias = ini.Get_Float(Name(), "BuildTime", BuildSpeedBias);

		IsMultiplay = ini.Get_Bool(Name(), "Multiplay", IsMultiplay);
		IsMultiplayPassive = ini.Get_Bool(Name(), "MultiplayPassive", IsMultiplayPassive);
		IsWallOwner = ini.Get_Bool(Name(), "WallOwner", IsWallOwner);
		IsSmartAI = ini.Get_Bool(Name(), "SmartAI", IsSmartAI);

		SideType oldside = Side;
		Side = ini.Get_Side(Name(), "Side", Side);

		if (Side != oldside) {
			int & house = (int &)House;
			if (oldside != SIDE_NONE) {
				Sides[oldside]->Houses.Delete_Index(house);
			}
			if (Side != SIDE_NONE) {
				Sides[Side]->Houses.Add(house);
			}
		}
		return(true);
	}
	return(false);
}


/// <summary>
/// Submits this house type to the game state checksum.
/// The network sync check uses this routine to prove that every machine in the game is
/// running with identical house rules.
/// </summary>
void HouseTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(House);
	crc(Side);
	crc(Scheme);
	crc(FirepowerBias);
	crc(GroundspeedBias);
	crc(AirspeedBias);
	crc(ArmorBias);
	crc(ROFBias);
	crc(CostBias);
	crc(BuildSpeedBias);
	crc(Suffix, strlen(Suffix));
	crc(Prefix);
	crc(IsMultiplay);
}


/// <summary>
/// Determines if this house type has been changed since it was last saved.
/// House types are written out wholesale rather than on demand, so the answer never varies.
/// </summary>
/// <returns>Returns with S_OK.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::IsDirty(void)
{
	return(false);
}


/// <summary>
/// Loads this house type from the stream specified.
/// The object is read back in raw form and then reconstructed in place, after which it
/// announces itself to the swizzler so that saved references can be resolved to it.
/// </summary>
/// <returns>Returns with S_OK if the house type was read, otherwise a failure code.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::Load(IStream * stream)
{
	HRESULT result;

	if (stream == NULL) {
		result = E_POINTER;
	} else {
		LONG id;

		result = stream->Read(&id, sizeof(id), NULL);
		if (SUCCEEDED(result)) {
			result = stream->Read(this, sizeof(*this), NULL);
			if (SUCCEEDED(result)) {

				new (this) HouseTypeClass(NoInitClass());

				Swizzle_Here_I_Am(id, this);
				result = S_OK;
			}
		}
	}
	return(result);
}


/// <summary>
/// Saves this house type to the stream specified.
/// The swizzle identifier is written ahead of the object itself so that the load process can
/// repoint everything that referred to this house type.
/// </summary>
/// <returns>Returns with S_OK if the house type was written, otherwise a failure code.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result;

	if (stream == NULL) {
		result = E_POINTER;
	} else {
		LONG id;
		Swizzler.Fetch_Swizzle_ID(this, &id);
		void *ptr = (void *)id;
		result = stream->Write(&ptr, sizeof(ptr), NULL);
		if (SUCCEEDED(result)) {
			result = stream->Write(this, sizeof(*this), NULL);
		}
	}
	return(result);
}


/// <summary>
/// Fetches the largest number of bytes this house type will need when saved.
/// The save game system calls this routine to reserve room in the stream before asking the
/// object to save itself.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no size pointer was supplied.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	if (pcbSize == NULL) {
		return(E_POINTER);
	}

	pcbSize->HighPart = 0;
	pcbSize->LowPart = sizeof(int) + sizeof(*this);

	return(S_OK);
}


/// <summary>
/// Fetches the requested interface from this house type.
/// House types serve up the persistence and RTTI interfaces that the save game system asks
/// them for.
/// </summary>
/// <returns>Returns with S_OK, or E_NOINTERFACE if the interface is not supported.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::QueryInterface(REFIID riid, LPVOID * ppvObject)
{
	if (ppvObject == NULL) {
		return(E_POINTER);
	}

	*ppvObject = NULL;

	if (riid == IID_IUnknown) {
		*ppvObject = (IUnknown *)(IPersistStream *)this;
	}
	if (riid == IID_IPersist) {
		*ppvObject = (IPersistStream *)this;
	}
	if (riid == IID_IPersistStream) {
		*ppvObject = (IPersist *)this;
	}
	if (riid == IID_IRTTITypeInfo) {
		*ppvObject = (IRTTITypeInfo *)this;
	}
	if (*ppvObject == NULL) {
		return(E_NOINTERFACE);
	}

	AddRef();
	return(S_OK);
}


/// <summary>
/// Fetches the class identifier of this object.
/// The save game system stores this identifier so that the object can be recreated as the
/// correct class when the game is loaded.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no return pointer was supplied.</returns>
HRESULT STDMETHODCALLTYPE HouseTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_HouseTypeClass;
	return(S_OK);
}


/// <summary>
/// Fetches the house type of the name specified, creating it if need be.
/// This routine is used while processing the rules and scenario INI databases, where a house
/// can be mentioned before it has been declared.
/// </summary>
/// <param name="ininame">The internal INI name of the house type wanted.</param>
/// <returns>Returns with a pointer to the matching house type.</returns>
HouseTypeClass * HouseTypeClass::Find_Or_Make(char const * ininame)
{
	return(TFind_Or_Make<HouseTypeClass>(ininame, HouseTypes));
}


/// <summary>
/// Fetches the RTTI type of this object.
/// </summary>
/// <returns>Returns with RTTI_HOUSETYPE.</returns>
RTTIType HouseTypeClass::Fetch_RTTI(void) const
{
	return(RTTI_HOUSETYPE);
}


/// <summary>
/// Fetches the number of bytes this house type occupies when saved.
/// The save game system uses this routine to size the block it reads or writes for this
/// object.
/// </summary>
/// <returns>Returns with the size, in bytes, of this house type.</returns>
int HouseTypeClass::Fetch_Object_Size(bool oldsave) const
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the heap index of this house type.
/// </summary>
/// <returns>Returns with the position of this house type within the house type heap.</returns>
int HouseTypeClass::Fetch_Heap_ID(void) const
{
	return(HeapID);
}


/// <summary>
/// Adds a reference to this house type.
/// House types are not reference counted -- they live for the duration of the game, so this
/// routine exists only to satisfy the IUnknown contract.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE HouseTypeClass::AddRef(void)
{
	return(1);
}


/// <summary>
/// Releases a reference to this house type.
/// House types are not reference counted -- they live for the duration of the game, so this
/// routine exists only to satisfy the IUnknown contract.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE HouseTypeClass::Release(void)
{
	return(1);
}
