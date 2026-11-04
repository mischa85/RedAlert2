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

/* $Header: /CounterStrike/ABSTRACT.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : ABSTRACT.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 01/26/95                                                     *
 *                                                                                             *
 *                  Last Update : July 10, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   AbstractClass::Debug_Dump -- Display debug information to mono screen.                    *
 *   AbstractClass::Distance -- Determines distance to target.                                 *
 *   AbstractTypeClass::AbstractTypeClass -- Constructor for abstract type objects.            *
 *   AbstractTypeClass::Coord_Fixup -- Performs custom adjustments to location coordinate.     *
 *   AbstractTypeClass::Full_Name -- Returns the full name (number) of this object type.       *
 *   AbstractTypeClass::Get_Ownable -- Fetch the ownable bits for this object.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "abstract.h"

#include "_rtti.h"
#include "assert.h"
#include "coord.h"
#include "crc.h"
#include "globals.h"
#include "noinit.h"
#include "scenario.h"
#include "swizzle.h"
#include "vector.h"

/// <summary>
/// Default constructor for abstract objects.
/// This routine puts the object into its neutral state: no unique ID assigned
/// yet, and nothing pending for the save system.
/// </summary>
AbstractClass::AbstractClass(void) :
	ID(-1),
	RefCount(0),
	Dirty(false)
{
}


/// <summary>
/// No-initialization constructor for abstract objects.
/// Use this constructor when the object is about to be overwritten wholesale by
/// the load process. It deliberately leaves every member untouched.
/// </summary>
AbstractClass::AbstractClass(NoInitClass const & x)
{
	x(); /// referenced so the compiler does not warn about it
}


/// <summary>
/// Destructor for abstract objects.
/// </summary>
AbstractClass::~AbstractClass(void)
{
	/// empty
}


/// <summary>
/// Fetches the RTTI type of this object.
/// The engine carries its own run-time type scheme in place of the C++ feature.
/// Only instantiable classes report a type of their own; the intermediate layers
/// of the hierarchy report RTTI_NONE.
/// </summary>
/// <returns>Returns with the RTTI type constant for this object.</returns>
int STDMETHODCALLTYPE AbstractClass::What_Am_I(void) const
{
	return(RTTI);
}


/// <summary>
/// Fetches this object's unique identifier.
/// </summary>
/// <returns>Returns with the unique ID, or -1 if none has been assigned yet.</returns>
int STDMETHODCALLTYPE AbstractClass::Fetch_ID(void) const
{
	return(ID);
}


/// <summary>
/// Assigns this object a unique identifier.
/// The ID is drawn from the scenario's running counter, so objects created
/// before a scenario exists are all given an ID of zero.
/// </summary>
VOID STDMETHODCALLTYPE AbstractClass::Create_ID(void)
{
	ID = (Scen == NULL ? 0 : Scen->Get_Unique_ID());
}


/// <summary>
/// Fetches a COM interface pointer from this object.
/// This is the IUnknown implementation shared by every game object. Abstract
/// objects expose IUnknown, IPersistStream, IPersist and IRTTITypeInfo; the save
/// game system reaches the whole object hierarchy through them.
/// </summary>
/// <param name="riid">The identifier of the interface being asked for.</param>
/// <param name="ppvObject">Receives the interface pointer, or NULL when the
/// interface is not supported.</param>
/// <returns>
/// Returns with S_OK when the interface was supplied. Otherwise E_NOINTERFACE is
/// returned for an unsupported interface, or E_POINTER when no output pointer was given.
/// </returns>
HRESULT STDMETHODCALLTYPE AbstractClass::QueryInterface(REFIID riid, LPVOID * ppvObject)
{
	if (ppvObject == NULL) {
		return(E_POINTER);
	}

	*ppvObject = NULL;

	if (riid == IID_IUnknown) {
		*ppvObject = (IUnknown *)(IPersistStream *)this;
	}
	if (riid == IID_IPersistStream) {
		*ppvObject = (IPersistStream *)this;
	}
	if (riid == IID_IPersist) {
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
/// Satisfies the IUnknown reference count contract.
/// The game owns its objects outright and they outlive any interface pointer
/// handed out, so nothing is actually counted.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE AbstractClass::AddRef(void)
{
	return(1);
}


/// <summary>
/// Satisfies the IUnknown release contract.
/// Releasing an interface never destroys a game object -- see AddRef.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE AbstractClass::Release(void)
{
	return(1);
}


/// <summary>
/// Writes this object to the save stream.
/// The object's address goes out first as its swizzle identity, followed by the
/// raw object image. Load reverses the process.
/// </summary>
/// <param name="stream">The stream to write to.</param>
/// <param name="cleardirty">Should the object be marked clean once it has been written?</param>
/// <returns>
/// Returns with S_OK when the object was written, E_POINTER when no stream was
/// supplied, or the stream's own failure code.
/// </returns>
HRESULT STDMETHODCALLTYPE AbstractClass::Save(IStream * stream, BOOL cleardirty)
{
	if (stream == NULL) {
		return(E_POINTER);
	}

	ULONG id = (ULONG)this;

	HRESULT result = stream->Write(&id, sizeof(id), NULL);
	if (SUCCEEDED(result)) {
		result = stream->Write(this, Fetch_Object_Size(false), NULL);
		if (SUCCEEDED(result)) {

			if (cleardirty) {
				Dirty = false;
			}
		}
		return(result);
	}
	return(result);
}


/// <summary>
/// Reads this object back from the save stream.
/// The saved address is handed to the swizzle system so that pointers elsewhere
/// in the save game can be remapped onto this object's new location, and then the
/// raw object image is read back over this one.
/// </summary>
/// <param name="stream">The stream to read from.</param>
/// <returns>
/// Returns with S_OK when the object was read, E_POINTER when no stream was
/// supplied, or the stream's own failure code.
/// </returns>
HRESULT STDMETHODCALLTYPE AbstractClass::Load(IStream * stream)
{
	if (stream == NULL) {
		return(E_POINTER);
	}

	ULONG id;

	HRESULT result = stream->Read(&id, sizeof(id), NULL);
	if (SUCCEEDED(result)) {
		Swizzle_Here_I_Am(id, this);
		int ref = RefCount;

		result = stream->Read(this, Fetch_Object_Size(IsOldSaveGame), NULL);

		RefCount = ref;
	}
	return(result);
}


/// <summary>
/// Fetches the number of bytes that Save will write.
/// </summary>
/// <param name="pcbSize">Receives the maximum size, in bytes.</param>
/// <returns>Returns with S_OK, or E_POINTER when no output pointer was given.</returns>
HRESULT STDMETHODCALLTYPE AbstractClass::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	if (pcbSize == NULL) {
		return(E_POINTER);
	}

	pcbSize->QuadPart = sizeof(int) + Fetch_Object_Size(false);
	return(S_OK);
}


/// <summary>
/// Folds this object's state into a running CRC.
/// The multiplayer sync check walks every object each frame and accumulates its
/// state; a CRC that differs between machines means the simulations have drifted
/// apart. Derived classes add their own state on top of this.
/// </summary>
/// <param name="crc">The running CRC to fold this object's state into.</param>
void AbstractClass::Compute_CRC(CRCEngine & crc) const
{
	assert(this != NULL);

	crc(ID);
	crc(Dirty);
}


/// <summary>
/// Determines if this object is inactive.
/// The bare abstract object is never a participant in the game, so it always
/// reports inactive. Objects that can be placed in the world override this.
/// </summary>
/// <returns>bool; Is the object inactive?</returns>
bool AbstractClass::Is_Inactive(void) const
{
	assert(this != NULL);

	return(true);
}


/// <summary>
/// Determines if this object is a techno object.
/// Techno objects are the ownable and targetable ones -- infantry, units,
/// aircraft and buildings.
/// </summary>
/// <returns>bool; Is this object derived from TechnoClass?</returns>
bool AbstractClass::Is_Techno(void) const
{
	return(::Dynamic_Cast<TechnoClass const *>(this) != NULL);
}


/// <summary>
/// Determines if this object has changed since it was last saved.
/// </summary>
/// <returns>Returns with S_OK when the object is dirty, or S_FALSE when it is not.</returns>
HRESULT AbstractClass::IsDirty(void)
{
	/*
	 * Per IPersistStream::IsDirty specifications this method returns S_OK to indicate that the object has changed.
	 * Otherwise, it returns S_FALSE.
	 */
	if (Dirty) {
		return(S_OK);
	}
	return(S_FALSE);
}


/// <summary>
/// Resets this object to its start of scenario state.
/// The bare abstract object carries no scenario state, so there is nothing to do.
/// </summary>
void AbstractClass::Init(void)
{
	assert(this != NULL);
}


/// <summary>
/// Severs any references this object holds to the target.
/// This routine is called on every object when one of them is about to be
/// destroyed, so that nothing is left pointing at the corpse.
/// </summary>
/// <param name="target">The object that is about to disappear.</param>
/// <param name="all">Should every reference be severed, including cargo and radio links?</param>
void AbstractClass::Detach(AbstractClass const * target, bool all)
{
	assert(this != NULL);

}


/// <summary>
/// Fetches the save image size adjustment for this object.
/// A save game written by an earlier version can store an object at a different
/// size than the current build uses. Fetch_Object_Size adds this difference in when
/// it is loading an old save, so the image is read back at the size it was written.
/// </summary>
/// <returns>Returns with the size difference in bytes; zero when the object has
/// not changed size.</returns>
int AbstractClass::Get_Object_Size_Delta(void) const
{
	assert(this != NULL);

	return(0);
}


/// <summary>
/// Fetches the house that owns this object.
/// The bare abstract object belongs to nobody. Ownable objects override this.
/// </summary>
/// <returns>Returns with the owning house, or HOUSE_NONE when the object is unowned.</returns>
HousesType AbstractClass::Owner(void) const
{
	assert(this != NULL);

	return(HOUSE_NONE);
}


/// <summary>
/// Fetches the house object that owns this object.
/// </summary>
/// <returns>Returns with a pointer to the owning house, or NULL when the object
/// is unowned.</returns>
HouseClass *AbstractClass::Owner_HouseClass(void) const
{
	assert(this != NULL);

	return(NULL);
}


/// <summary>
/// Fetches this object's index within its heap.
/// </summary>
/// <returns>Returns with the heap index, or zero for an object that does not
/// live in a heap.</returns>
int AbstractClass::Fetch_Heap_ID(void) const
{
	assert(this != NULL);

	return(0);
}


/***********************************************************************************************
 * ObjectClass::Center_Coord -- Fetches the center coordinate for the object.                  *
 *                                                                                             *
 *    This routine will return the center coordinate for the object. The center coordinate is  *
 *    typically the coordinate recorded in the object structure. Exceptions to this include    *
 *    the trees and other terrain elements.                                                    *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns the coordinate that is considered the center point of this object.         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/21/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
Coord AbstractClass::Center_Coord(void) const
{
	assert(this != NULL);

	return(COORD_NONE);
}


/// <summary>
/// Fetches the coordinate that others should move toward.
/// Objects with a distinguished approach point -- a building's entry cell, say --
/// override this. By default an object is approached at its center.
/// </summary>
/// <returns>Returns with the coordinate another object should head for.</returns>
Coord AbstractClass::Destination_Coord(void) const
{
	assert(this != NULL);

	return(Center_Coord());
}


/// <summary>
/// Determines if this object is resting on the ground.
/// </summary>
/// <returns>bool; Is the object on the ground?</returns>
bool AbstractClass::On_Ground(void) const
{
	return(false);
}


/// <summary>
/// Determines if this object is airborne.
/// </summary>
/// <returns>bool; Is the object in the air?</returns>
bool AbstractClass::In_Air(void) const
{
	return(false);
}


/// <summary>
/// Fetches this object's coordinate.
/// Defaults to the center coordinate; objects that are located by some other
/// reference point override this.
/// </summary>
/// <returns>Returns with the object's coordinate.</returns>
Coord AbstractClass::As_Coord(void) const
{
	assert(this != NULL);

	return(Center_Coord());
}


/// <summary>
/// Handles the per frame logic for this object.
/// This routine is called once per game frame for every active object. The bare
/// abstract object has no logic of its own.
/// </summary>
void AbstractClass::AI(void)
{
	assert(this != NULL);
}
