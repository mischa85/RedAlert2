/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "droppod.h"

#include "_map.h"
#include "_rand.h"
#include "_rules.h"
#include "anim.h"
#include "cell.h"
#include "combat.h"
#include "coord.h"
#include "foot.h"
#include "globals.h"
#include "house.h"
#include "map.h"
#include "noinit.h"
#include "rules.h"
#include "sun.h"
#include "weapon.h"

#include "layer.hh"

#include <new.h>


/// <summary>
/// Constructs a drop pod locomotor.
/// The pod has no landing site until Move_To assigns one.
/// </summary>
DropPodLocomotionClass::DropPodLocomotionClass(void) :
	BASECLASS(),
	Direction(DPOD_DIR_NE),
	DestinationCoord(COORD_NONE),
	Piggybacker(NULL)
{
}


/// <summary>
/// Destroys the drop pod locomotor.
/// </summary>
DropPodLocomotionClass::~DropPodLocomotionClass(void)
{
}


/// <summary>
/// Is the drop pod in motion?
/// A pod exists only for the duration of its fall, so it always reports movement.
/// </summary>
boolean STDMETHODCALLTYPE DropPodLocomotionClass::Is_Moving(void)
{
	return(true);
}


/// <summary>
/// Fetches the coordinate the pod is falling toward.
/// </summary>
/// <returns>Returns with the landing coordinate, or COORD_NONE if no destination has
/// been assigned yet.</returns>
CoordStruct STDMETHODCALLTYPE DropPodLocomotionClass::Destination(void)
{
	return(DestinationCoord);
}


/// <summary>
/// Handles the drop pod's descent for one game logic frame.
/// The pod slides in along its fixed approach angle, trailing smoke and shelling the
/// landing zone as it comes. On touchdown the carried locomotor is handed back and the
/// passenger is unlimboed, or destroyed along with its surroundings if there is nowhere
/// for it to stand.
/// </summary>
boolean STDMETHODCALLTYPE DropPodLocomotionClass::Process(void)
{
	Coord coord = LinkedTo->PositionCoord;
	Coord smoke_coord = coord;

	int speed = LinkedTo->HeightAGL / 10 + 2;
	if (Rule->DropPodSpeed > speed) {
		speed = Rule->DropPodSpeed;
	}

	double horspeed = fastmath::cos(Rule->DropPodAngle) * speed;

	switch (Direction) {
		case DPOD_DIR_NE:
		default:
			coord.X -= horspeed;
			break;
		case DPOD_DIR_NW:
			coord.X += horspeed;
			break;
		case DPOD_DIR_SE:
			coord.Y -= horspeed;
			break;
		case DPOD_DIR_SW:
			coord.Y += horspeed;
			break;
	}

	coord.Z -= fastmath::sin(Rule->DropPodAngle) * speed;

	int height = LinkedTo->HeightAGL;
	FootClass * linked = LinkedTo;

	if (height <= 0) {
		linked->HeightAGL = 0;
		coord = linked->PositionCoord;
		linked->Limbo();

		AddRef();
		End_Piggyback(&LinkedTo->Locomotion);

		if (!linked->Unlimbo(coord, DIR_N)) {
			Explosion_Damage(coord, 100, LinkedTo, Rule->C4Warhead);
			new AnimClass(Combat_Anim(100, Rule->C4Warhead, LAND_CLEAR, coord), coord, 0, 1, ShapeFlags_Type(SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_ZGRAD), Get_Explosion_Z(coord));
			linked->Delete_Me();
		} else {
			new AnimClass(Rule->DropPod[Direction % Rule->DropPod.Count()], coord);
			linked->Mark(MARK_DOWN);
			linked->HeightAGL = 0;
			linked->Enter_Idle_Mode();
			linked->Commence();
			linked->Scatter(COORD_NONE);
		}
		Release();
	} else {
		LinkedTo->PositionCoord = coord;
		WeaponTypeClass const * weapon = Rule->DropPodWeapon;

		if (weapon != NULL) {
			if (Frame % 6 == 0) {
				new AnimClass(AnimTypes[AnimTypeClass::From_Name("SMOKEY")], smoke_coord);
			}
			if (Frame % 3 == 0) {
				if (!LinkedTo->House->Is_Ally(Map[DestinationCoord].Cell_Techno())) {
					Coord scattered_coord = Coord_Scatter(DestinationCoord, CELL_LEPTON / 3, false);
					Sound_Effect((VocType)weapon->Sound.Pick(NonCriticalRandomNumber), coord);
					Explosion_Damage(scattered_coord, 2 * weapon->Attack, LinkedTo, weapon->WarheadPtr);
					new AnimClass(Combat_Anim(2 * weapon->Attack, weapon->WarheadPtr, LAND_CLEAR, scattered_coord), scattered_coord, 0, 1, ShapeFlags_Type(SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_ZGRAD), Get_Explosion_Z(scattered_coord));
				}
			}
		}
	}
	return(true);
}


/// <summary>
/// Sets the ground position that the pod is to land on.
/// This routine picks the compass direction the pod will fall in, preferring one whose
/// approach begins on screen, then lifts the object to the drop height above that
/// approach and unlimbos it there with the atmosphere entry effect. A pod that already
/// has a destination ignores any later request.
/// </summary>
/// <param name="to">The coordinate the pod should land on.</param>
void STDMETHODCALLTYPE DropPodLocomotionClass::Move_To(CoordStruct to)
{
	if (DestinationCoord == COORD_NONE) {

		DestinationCoord = to;
		DestinationCoord.Z = Map.Get_Height_GL(DestinationCoord);

		Coord dropcoord = DestinationCoord;
		dropcoord.Z += Rule->DropPodHeight;
		double dropradius = (double)Rule->DropPodHeight / fastmath::tan(Rule->DropPodAngle);
		dropcoord.X += dropradius;

		CellClass * cptr = &Map[dropcoord];

		if (Map.In_Local_Radar(dropcoord)) {
			Direction = DPOD_DIR_NE;
		} else {
			dropcoord.X = DestinationCoord.X - dropradius;
			cptr = &Map[dropcoord];

			if (Map.In_Local_Radar(dropcoord)) {
				Direction = DPOD_DIR_NW;
			} else {
				dropcoord.X = DestinationCoord.X;
				dropcoord.Y = DestinationCoord.Y + dropradius;
				cptr = &Map[dropcoord];

				if (Map.In_Local_Radar(dropcoord)) {
					Direction = DPOD_DIR_SE;
				} else {
					dropcoord.Y = DestinationCoord.Y - dropradius;
					cptr = &Map[dropcoord];
					Direction = DPOD_DIR_SW;
				}
			}
		}

		LinkedTo->PositionCoord = dropcoord;

		if (LinkedTo->Unlimbo(dropcoord, DIR_S)) {
			LinkedTo->PositionCoord = dropcoord;
			LinkedTo->PrimaryFacing.Set(DIR_S << 8);
			new AnimClass(Rule->AtmosphereEntry, dropcoord);
		} else {
			LinkedTo->Unlimbo(dropcoord, DIR_S);
		}
	}
}


/// <summary>
/// Fetches the class ID that this locomotor is persisted under.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no return pointer was supplied.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_BallisticLocomotion;
	return(S_OK);
}


/// <summary>
/// Loads the drop pod locomotor from a stream.
/// The pod is reconstructed in place and the locomotor it was carrying, if any, is
/// restored along with it.
/// </summary>
/// <returns>Returns with S_OK, or the failure code reported by the stream.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) DropPodLocomotionClass(NoInitClass());

		bool haspiggy;
		stream->Read(&haspiggy, sizeof(haspiggy), NULL);

		if (haspiggy) {
			result = OleLoadFromStream(stream, IID_ILocomotion, (void**)&Piggybacker);
		}
	}
	return(result);
}


/// <summary>
/// Saves the drop pod locomotor to a stream.
/// The carried locomotor is written out along with the pod itself, so that a saved game
/// can restore an object that was still falling when the game was saved.
/// </summary>
/// <param name="cleardirty">Should the dirty flag be cleared after saving?</param>
/// <returns>Returns with S_OK, or the failure code reported by the stream.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		bool haspiggy;
		if (Piggybacker != NULL) {
			haspiggy = true;
			stream->Write(&haspiggy, sizeof(haspiggy), NULL);
			IPersistStreamPtr ptr(Piggybacker);
			result = OleSaveToStream(ptr, stream);
		} else {
			haspiggy = false;
			result = stream->Write(&haspiggy, sizeof(haspiggy), NULL);
		}
	}
	return(result);
}


/// <summary>
/// Stops the pod's descent.
/// A pod cannot be halted in mid air, so this request is quietly ignored.
/// </summary>
void STDMETHODCALLTYPE DropPodLocomotionClass::Stop_Moving(void)
{
	// empty
}


/// <summary>
/// Takes charge of the object's existing locomotor.
/// The drop pod holds on to the locomotor it displaces so that the object can be given
/// it back when the pod touches down.
/// </summary>
/// <param name="pointer">The locomotor to carry.</param>
/// <returns>Returns with S_OK, or E_FAIL if something is already being carried.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::Begin_Piggyback(ILocomotion * pointer)
{
	if (pointer == NULL) {
		return(E_POINTER);
	}
	if (Piggybacker == NULL) {
		Piggybacker = pointer;
		return(S_OK);
	}
	return(E_FAIL);
}


/// <summary>
/// Hands the carried locomotor back to the caller.
/// The pod gives up its hold without destroying the locomotor, so the object can resume
/// using it once the pod has landed.
/// </summary>
/// <param name="pointer">Pointer to the location that receives the carried locomotor.</param>
/// <returns>Returns with S_OK, or S_FALSE if nothing was being carried.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::End_Piggyback(ILocomotion ** pointer)
{
	if (pointer == NULL) {
		return(E_POINTER);
	}
	if (Piggybacker != NULL) {
		*pointer = Piggybacker;
		Piggybacker.Detach();
		return(S_OK);
	}
	return(S_FALSE);
}


/// <summary>
/// Can the piggyback be ended now?
/// The carried locomotor may only be given control back once the pod has come to rest.
/// </summary>
/// <returns>bool; May the carried locomotor take over again?</returns>
boolean STDMETHODCALLTYPE DropPodLocomotionClass::Is_Ok_To_End(void)
{
	if (!Is_Moving() && Piggybacker != NULL) {
		return(true);
	}
	return(false);
}


/// <summary>
/// Fetches an interface pointer from the drop pod locomotor.
/// This routine extends the base locomotor's interface set with IPiggyback, which is how
/// the pod carries the object's real locomotor while it falls.
/// </summary>
/// <returns>Returns with S_OK, or E_NOINTERFACE if this object does not offer the
/// interface asked for.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::QueryInterface(REFIID riid, LPVOID * ppvObject)
{
	HRESULT result = BASECLASS::QueryInterface(riid, ppvObject);

	if (result == E_NOINTERFACE) {
		if (riid == IID_IPiggyback) {
			*ppvObject = (IPiggyback*)this;
		}
		if (*ppvObject == NULL) {
			result = E_NOINTERFACE;
		} else {
			AddRef();
			result = S_OK;
		}
	}
	return(result);
}


/// <summary>
/// Determines which display layer the pod belongs in.
/// A pod is always falling, so it draws along with the other airborne objects right up
/// until it lands and gives its object back.
/// </summary>
LayerType STDMETHODCALLTYPE DropPodLocomotionClass::In_Which_Layer(void)
{
	return(LAYER_AIR);
}


/// <summary>
/// Fetches the class ID of the locomotor being carried.
/// The save system uses this to record which locomotor is to be restored underneath the
/// drop pod. When nothing is being carried, the pod supplies its own class ID instead.
/// </summary>
/// <returns>Returns with S_OK, or an error code if the class ID could not be
/// determined.</returns>
HRESULT STDMETHODCALLTYPE DropPodLocomotionClass::Piggyback_CLSID(GUID * classid)
{
	if (classid == NULL) {
		return(E_POINTER);
	}

	if (Piggybacker != NULL) {
		IPersistPtr ptr(Piggybacker);
		if (ptr == NULL) {
			return(E_FAIL);
		}
		return(ptr->GetClassID(classid));
	}

	IPersistPtr ptr(this);
	if (ptr == NULL) {
		return(E_FAIL);
	}
	return(ptr->GetClassID(classid));
}


/// <summary>
/// Fetches the drawing code for the drop pod.
/// The renderer uses this to choose the artwork that suits the pod's approach.
/// </summary>
int STDMETHODCALLTYPE DropPodLocomotionClass::Drawing_Code(void)
{
	return((unsigned)Direction % 2);
}
