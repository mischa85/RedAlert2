// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#define INCLUDE_COM
#include "always.h"
#include <math.h> // RA2: Draw_It's asin
#include "rect.h" // RA2: Draw_It's rects


#include "blight.h"

#include "_logic.h"
#include "_map.h"
#include "_rect.h"
#include "_rules.h"
#include "_surface.h"
#include "_tactica.h"
#include "building.h"
#include "cell.h"
#include "globals.h"
#include "house.h"
#include "inline.h"
#include "logic.h"
#include "mouse.h"
#include "ovrlight.h"
#include "rules.h"
#include "sun.h"
#include "swizzle.h"
#include "tactical.h"
#include "techno.h"
#include "wave.h"
#include "xsurface.h"

BuildingLightClass::BuildingLightClass(TechnoClass * owner) :
	BASECLASS(),
	Speed(0),
	RotationPivot(COORD_NONE),
	RotationTarget(COORD_NONE),
	Acceleration(0),
	IsOppositeDirection(false),
	Behavior(0),
	Target(NULL),
	Owner(owner)
{
	BuildingLights.Add(this);
	if (owner != NULL) {
		Init_Rotation_Arc(owner);

		Unlimbo(PositionCoord);
		Set_Behavior_Type(LIGHT_BEHAVIOR_SWEEP);
		IsOppositeDirection = BuildingLights.ID(this) % 2 != 0;
	}
}


/// <summary>
/// Destroys the spotlight.
/// The light is taken back out of the world and dropped from the global light list.
/// </summary>
BuildingLightClass::~BuildingLightClass(void)
{
	Limbo();
	BuildingLights.Delete(this);
}


/// <summary>
/// Draws the spotlight and the beam that casts it.
/// This routine renders the pool of light on the ground and the two glowing edges that
/// run back up to the caster. Nothing is drawn when the owning building is destroyed,
/// unpowered, or hidden away under the fog of war.
/// </summary>
void BuildingLightClass::Draw_It(Point2D const & point, Rect const & cliprect) const
{
	if (!Behavior || Owner == NULL || Owner->RTTI != RTTI_BUILDING) return;

	BuildingClass * building = (BuildingClass *)Owner;
	if (building == NULL || !building->IsActive || !building->IsPowerOnline() || building->IsFogged) return; // RA2: returns early unless the building is active, powered and not fogged

	if (Scen->Special.IsFogOfWar && Map.Is_Fogged(Get_Coord())) return;

	SpotLightClass * spotlight = new SpotLightClass(Get_Coord(), 16);

	int planar_dist = Distance(Owner->Center_Coord());
	int stage = Sweep_Stage();

	if (planar_dist > Rule->SpotlightLocationRadius && Behavior == LIGHT_BEHAVIOR_FOLLOW) {
		int radius = stage + 80;
		radius = (radius > 0) ? radius : 0; // RA2: radius clamped at 0
		spotlight->Set_Radius(min(radius, 89)); // RA2: and at 89
	} else {
		spotlight->Set_Radius(80);
	}

	spotlight->Draw_It();
	delete spotlight;

	Coord here = PositionCoord;
	Coord there = Owner->Center_Coord();

	int distance = Distance(there);
	int detection = Detection_Radius();

	if (distance < detection) return; // RA2: no draw when the target is inside the detection radius

	double angle = fastmath::asin((double)detection / (double)distance); // RA2: the cone's half angle from the detection radius

	Matrix3D mat; // RA2: the cone's two edges are the target offset rotated by +angle
	mat.Make_Identity(); // RA2: from the identity
	mat.Rotate_Z(angle); // RA2: by +angle
	Vector3 v(here.X - there.X, here.Y - there.Y, here.Z - there.Z); // RA2: the offset from the caster to the target
	Vector3 vec = mat * v; // RA2: rotated
	Coord arc1 = Coord(there.X + vec.X, there.Y + vec.Y, there.Z + vec.Z); // RA2: the first edge's end

	mat.Make_Identity(); // RA2: and by -angle
	mat.Rotate_Z(-angle); // RA2: by -angle
	Vector3 vec2 = mat * v; // RA2: rotated
	Coord arc2 = Coord(there.X + vec2.X, there.Y + vec2.Y, there.Z + vec2.Z); // RA2: the second edge's end

	Point2D arc1_px; // RA2: screen points for the edges and the caster
	Point2D arc2_px; // RA2: screen point of the second edge
	Point2D caster_px; // RA2: screen point of the caster
	TacticalMap->Coord_To_Pixel(arc1, arc1_px); // RA2: edges to pixels
	TacticalMap->Coord_To_Pixel(arc2, arc2_px); // RA2: edges to pixels
	TacticalMap->Coord_To_Pixel(there + Coord(0, 0, 430), caster_px); // RA2: the caster 430 leptons up

	arc1_px += TacticalRect.TopLeft; // RA2: shifted by the tactical origin
	arc2_px += TacticalRect.TopLeft; // RA2: shifted by the tactical origin
	caster_px += TacticalRect.TopLeft; // RA2: shifted by the tactical origin
	Point2D caster_px2 = caster_px; // RA2: a copy for the second line

	int zstart = -Tactical::Z_Lepton_To_Pixel(there.Z + 400); // RA2: depth from 400 leptons above the caster
	int zend = -Tactical::Z_Lepton_To_Pixel((Get_Coord() + Coord(0, 0, 250)).Z); // RA2: to 250 leptons above the light

	if (Clip_Line_To_Rect(caster_px, arc1_px, TacticalRect)) { // RA2: each edge clipped,
		LogicalSurface->Draw_Depth_Glow_Line(LogicalSurface->Get_Rect(), caster_px, arc1_px, 75 - 6 * stage, zstart, zend); // RA2: and drawn as a depth glow line
	}
	if (Clip_Line_To_Rect(caster_px2, arc2_px, TacticalRect)) { // RA2: each edge clipped,
		LogicalSurface->Draw_Depth_Glow_Line(LogicalSurface->Get_Rect(), caster_px2, arc2_px, 75 - 6 * stage, zstart, zend); // RA2: and drawn as a depth glow line
	}
}


/// <summary>
/// Handles the per frame logic for the spotlight.
/// This routine walks the beam along according to the behavior it was given -- sweeping
/// back and forth across its arc, circling its owner, or chasing a target -- and springs
/// the owner's spotlight triggers whenever an enemy is caught in the light. A light
/// whose owner has left the game deletes itself here.
/// </summary>
void BuildingLightClass::AI(void)
{
	if (Owner == NULL || !Owner->IsActive) {
		Delete_Me();
		return;
	}

	Coord coord;
	ObjectClass * following; // RA2: the followed object
	switch (Behavior) {
		default:
			coord = PositionCoord;
			break;

		case LIGHT_BEHAVIOR_SWEEP: {
			Speed += Acceleration;
			if (IsOppositeDirection) {
				if (Speed > Rule->SpotlightAngle / 2) {
					Acceleration -= Rule->SpotlightAcceleration;
					if (Acceleration < 0) {
						Acceleration = 0;
						IsOppositeDirection = false;
					}
				} else if (Acceleration < Rule->SpotlightSpeed) {
					Acceleration += Rule->SpotlightAcceleration;
				}
			} else {
				if (Speed < Rule->SpotlightAngle / -2) {
					Acceleration += Rule->SpotlightAcceleration;
					if (Acceleration > 0.0) {
						Acceleration = 0.0;
						IsOppositeDirection = true;
					}
				} else if (-Rule->SpotlightSpeed < Acceleration) {
					Acceleration -= Rule->SpotlightAcceleration;
				}
			}

			Matrix3D mtx;
			mtx.Make_Identity();
			mtx.Rotate_Z(Speed);

			Vector3 v(RotationTarget.X - RotationPivot.X, RotationTarget.Y - RotationPivot.Y, RotationTarget.Z - RotationPivot.Z);
			Vector3 vec = mtx * v;
			coord = Coord(RotationPivot.X + vec.X, RotationPivot.Y + vec.Y, RotationPivot.Z + vec.Z);
		}
		break;
		case LIGHT_BEHAVIOR_CIRCLE: { // RA2: circling: the light turns around the owner
			Coord owner_coord = Owner->PositionCoord; // RA2: the owner's position
			Speed += Rule->SpotlightSpeed * 4; // RA2: four times the spotlight speed

			if (Speed > DEG_TO_RAD(360)) { // RA2: wrapped at 360 degrees
				Speed -= DEG_TO_RAD(360); // RA2: wrapped at 360 degrees
			}

			Matrix3D mtx; // RA2: rotation about Z
			mtx.Make_Identity(); // RA2: from the identity
			mtx.Rotate_Z(Speed); // RA2: by Speed

			Vector3 v(RotationTarget.X - owner_coord.X, RotationTarget.Y - owner_coord.Y, RotationTarget.Z - owner_coord.Z); // RA2: the offset from the owner
			Vector3 vec = mtx * v; // RA2: rotated
			coord = Coord(owner_coord.X + vec.X, owner_coord.Y + vec.Y, owner_coord.Z + vec.Z); // RA2: the new position
		}
		break;

		case LIGHT_BEHAVIOR_FOLLOW: // RA2: after the sweep and circle cases
			following = Target; // RA2: follows its Target
			if (following != NULL && following->IsActive) { // RA2: while it is active
				int dist = following->Center_Coord().Distance_YZX(Owner->Center_Coord()); // RA2: distance summed y, z, x
				if (dist < Rule->SpotlightMovementRadius) { // RA2: within the movement radius
					coord = Lerp(Get_Coord(), following->Get_Coord(), 0.25f); // RA2: a quarter of the way there
					break;
				}
			}
			Set_Behavior_Type(LIGHT_BEHAVIOR_SWEEP); // RA2: otherwise back to sweeping
			break;
	}

	Set_Coord(coord); // RA2: through the virtual

	BuildingClass * owner_building = (Owner->RTTI == RTTI_BUILDING) ? (BuildingClass *)Owner : NULL;

	if (Behavior == LIGHT_BEHAVIOR_SWEEP) {
		if (owner_building != NULL && owner_building->IsActive && owner_building->IsPowerOnline() && owner_building->Tag != NULL) { // RA2: needs an active, powered building with a tag
			bool found = false;
			Cell cell = Get_Cell(); // RA2: the light's own cell
			int detection = Detection_Radius() + 30;
			HouseClass * house = Owner->House;

			for (int x = -1; x < 2; x++) {
				for (int y = -1; y < 2; y++) {
					ObjectClass * occupier = Map[cell + Cell(x, y)].Cell_Occupier();
					while (occupier != NULL) {
						if ((occupier->RTTI == RTTI_INFANTRY || occupier->RTTI == RTTI_UNIT) && !house->Is_Ally(occupier)) {
							int dist = occupier->Center_Coord().Distance_To(Get_Coord());
							if (dist < detection) {
								found = true;
							}
						}
						occupier = occupier->Next;
					}

				}
			}

			if (found) {
				owner_building->Tag->Spring(TEVENT_ENEMY_IN_SPOTLIGHT, owner_building); // RA2: one event
			}
		}
	}
}

int BuildingLightClass::Fetch_Object_Size(void) const // RA2: takes no argument
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the class identifier of this object.
/// This routine is used by the save game code so that an object of the right kind can
/// be created when the game is loaded back in.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE BuildingLightClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_BuildingLightClass;
	return(S_OK);
}


/// <summary>
/// Loads this spotlight from the save game stream.
/// The owner and target references are remembered as raw identifiers in the save file,
/// so they are handed to the swizzler to be turned back into pointers once every object
/// has been read in.
/// </summary>
/// <returns>Returns with the result code of the load operation.</returns>
HRESULT STDMETHODCALLTYPE BuildingLightClass::Load(IStream *stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) BuildingLightClass(NoInitClass());

		Swizzle_Pointer(&Owner);
		Swizzle_Pointer(&Target);
	}
	return(result);
}


/// <summary>
/// Saves this spotlight to the save game stream.
/// </summary>
/// <returns>Returns with the result code of the save operation.</returns>
HRESULT STDMETHODCALLTYPE BuildingLightClass::Save(IStream * stream, BOOL cleardirty)
{
	return(BASECLASS::Save(stream, cleardirty));
}


/// <summary>
/// Fetches the object type that this light was created from.
/// A spotlight is conjured up by the building that owns it rather than built from any
/// object type, so there is nothing to hand back.
/// </summary>
/// <returns>Returns with a pointer to the object type, which is always NULL.</returns>
ObjectTypeClass const * BuildingLightClass::Class_Of(void) const
{
	return(NULL);
}


/// <summary>
/// Fetches the display layer that this light belongs to.
/// </summary>
/// <returns>Returns with the layer that the light should be rendered in.</returns>
LayerType BuildingLightClass::In_Which_Layer(void) const
{
	return(LAYER_AIR);
}


/// <summary>
/// Removes any reference this light holds to the specified object.
/// This routine is called when an object is about to leave the game, so that the light
/// does not go on tracking, or claiming to be owned by, something that no longer exists.
/// </summary>
void BuildingLightClass::Detach(AbstractClass const * target, bool all)
{
	BASECLASS::Detach(target, all);
	if (Target == target) {
		Target = NULL;
	}
	if (Owner == target) {
		Owner = NULL;
	}
}


/// <summary>
/// Sets up the arc that the spotlight will sweep through.
/// The pivot and target points are taken from the owner's facing, so the beam swings
/// out in front of whatever building the light is mounted on.
/// </summary>
/// <param name="owner">The object that this light is mounted upon.</param>
void BuildingLightClass::Init_Rotation_Arc(TechnoClass * owner)
{
	Coord target = Move_Coord(owner->PositionCoord, owner->PrimaryFacing.Current(), Rule->SpotlightLocationRadius);
	RotationTarget = target;

	RotationPivot = Move_Coord(owner->PositionCoord, owner->PrimaryFacing.Current(), -Rule->SpotlightMovementRadius);

	PositionCoord = target;
}


/// <summary>
/// Sets the manner in which the spotlight moves.
/// This routine restarts the beam's travel and, when the light is told to follow, picks
/// out the nearest enemy in the neighborhood for it to track.
/// </summary>
void BuildingLightClass::Set_Behavior_Type(LightBehaviorType type)
{
	Behavior = type;
	Speed = 0;

	if (Behavior == LIGHT_BEHAVIOR_FOLLOW) {
		Cell cell = PositionCell;

		int mindist = 9999999;
		ObjectClass * closest = NULL;

		HouseClass * house = Owner->House;

		for (int x = -1; x < 2; x++) {
			for (int y = -1; y < 2; y++) {
				ObjectClass * occupier = Map[Cell(TPoint2D<short>(x + cell.X, y + cell.Y))].Cell_Occupier(); // RA2: the occupier of each cell in the box
				while (occupier != NULL) {
					if ((occupier->RTTI == RTTI_INFANTRY || occupier->RTTI == RTTI_UNIT) && !house->Is_Ally(occupier)) {
						int dist = occupier->Center_Coord().Distance_To(Get_Coord());
						if (dist < mindist) {
							closest = occupier;
							mindist = dist;
						}
					}
					occupier = occupier->Next;
				}

			}
		}

		if (closest != NULL) {
			Target = (TechnoClass *)closest;
		}
	}
}


/// <summary>
/// Fetches the radius within which the spotlight notices an enemy.
/// The radius grows as the beam sweeps further out, so a light reaching for the far end
/// of its arc will catch an intruder that one pointing at its own feet would miss.
/// </summary>
/// <returns>Returns with the radius to scan when looking for enemies.</returns>
int BuildingLightClass::Detection_Radius(void) const
{
	/// Not 112.0 / 15.0 -- that quotient is one ULP higher, and the difference is
	/// visible here because the radius truncates: stages 15 and 30 would give 112
	/// and 224 where the game gives 111 and 223.
	static const double _scale = 5.973333333333333; // RA2
	int radius = (int)(Sweep_Stage() * _scale) + Rule->SpotlightRadius; // RA2: the sweep stage scaled by 5.9733
	return(radius);
}


/// <summary>
/// Fetches how far out from its owner the beam has swung.
/// The stage rises as the light travels away from the building it is mounted on, and
/// it is used to scale the beam's radius, its brightness, and how far it can see.
/// </summary>
/// <returns>Returns with the sweep stage, which is zero while the beam is still close
/// to home.</returns>
int BuildingLightClass::Sweep_Stage(void) const
{
	int dist = Center_Coord().Distance_To(Owner->Center_Coord());
	int radius = Rule->SpotlightLocationRadius;
	if (dist < radius) {
		return(0);
	}
	return((dist - radius) / ((Rule->SpotlightMovementRadius - radius) / 10));
}


/// <summary>
/// Adds the state of this spotlight to the running game checksum.
/// This routine is used by the multiplayer sync checking code to prove that every
/// machine agrees about where this light is pointing and who it is watching.
/// </summary>
void BuildingLightClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(Speed);
	crc(RotationPivot.X);
	crc(RotationPivot.Y);
	crc(RotationPivot.Z);
	crc(RotationTarget.X);
	crc(RotationTarget.Y);
	crc(RotationTarget.Z);
	crc(Acceleration);
	crc(IsOppositeDirection);
	crc(Behavior);
	if (Target != NULL) {
		crc(Target->Fetch_ID());
	}
	if (Owner != NULL) {
		crc(Owner->Fetch_ID());
	}
}


/// <summary>
/// Removes the spotlight from the game world.
/// This routine will limbo the light and pull it back out of the logic list so that it
/// stops being processed.
/// </summary>
/// <returns>bool; Was the light removed from the world?</returns>
bool BuildingLightClass::Limbo(void)
{
	if (BASECLASS::Limbo()) {
		Logic.Remove(this);
		return(true);
	}
	return(false);
}


/// <summary>
/// Places the spotlight into the game world.
/// This routine will unlimbo the light and submit it to the logic list so that it
/// begins receiving its per frame processing.
/// </summary>
/// <returns>bool; Was the light placed into the world?</returns>
bool BuildingLightClass::Unlimbo(Coord const & coord, Dir256 dir)
{
	if (BASECLASS::Unlimbo(coord, dir)) {
		Logic.Submit(this);
		return(true);
	}
	return(false);
}


template class TPoint3D<int>; // RA2: TPoint3D<int> instantiated here (0x00437160)
