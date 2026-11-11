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

/* $Header: /CounterStrike/BUILDING.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : BUILDING.H                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 14, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 14, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "builtype.h"
#include "rect.h"
#include "techno.h"

#include "banim.hh"
#include "bstate.hh"
#include "check.hh"

#define	MAX_DOOR_STAGE			18	// # of frames of door opening on weapons factory
#define	DOOR_OPEN_STAGE			9	// frame on which the door is entirely open
#define	MAX_REPAIR_ANIM_STAGE	5	// # of stages of anim for repair center cycling
#define BUILDING_UPGRADE_MAX	3
#define MAX_FIRESTORM_WALL_FRAMES 15

class BuildingLightClass;
class LightSourceClass;
class ConvertClass;
template<class T>
class DynamicVectorClass;
class FoggedObjectClass;

/****************************************************************************
**	For each instance of a building in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular building.
*/
extern Cell EmptyCell; // RA2: 0x84ED10, an all-zero cell

class BuildingClass : public TechnoClass
{
		typedef TechnoClass BASECLASS;

	public:

		/*
		**	This points to the control data that gives this building its characteristics.
		*/
		BuildingTypeClass * Class;

		/*
		**	If this building is in the process of producing something, then this
		**	will point to the factory manager.
		*/
		FactoryClass * Factory;

		/*
		**	Special countdown to destruction value. If the building is destroyed,
		**	it won't actually be removed from the map until this value reaches
		**	zero. This delay is for cosmetic reasons.
		*/
		CDTimerClass<FrameTimerClass> CountDown;

		/*
		**	This is the current animation processing state that the building is
		**	in.
		*/
		BStateType BState;
		BStateType QueueBState;

		/*
		**	For multiplayer games, this keeps track of the last house to damage
		**	this building, so if it burns to death or otherwise gradually dies,
		**	proper credit can be given for the kill.
		*/
		HousesType WhoLastHurtMe;

		/*
		**	This is the saboteur responsible for this building's destruction.
		*/
		AbstractClass * WhomToRepay;

		/*
		**	This is a record of the last strength of the building. Every so often,
		**	it will compare this strength to the current strength. If there is a
		**	discrepancy, then the owner power is adjusted accordingly.
		*/
		int LastStrength;

		/*
		**	This is a target id of an animation we're keeping track of.  Examples
		**	of this usage are the advanced tech center, which needs to know
		**	when the sputdoor animation has reached a certain stage.
		*/
		AbstractClass * AnimToTrack;

		AnimClass * PsiWarnAnim; // RA2: +0x444

		/*
		**	This is the countdown timer that regulates placement retry logic
		**	for factory type buildings.
		*/
		CDTimerClass<FrameTimerClass> PlacementDelay;

		/*
		 * These are the animations this building currently has running, one slot per BAnimType.
		 */
		AnimClass * Anims[18]; // RA2: eighteen, not BANIM_COUNT

		AnimClass * DamageFireAnims[8]; // RA2: +0x49C
		bool RequiresDamageFires; // RA2: +0x4BC

		/*
		 * These are the building types installed in this building as upgrades, in the order
		 * they were added. Each carries the extra weapon, power, and super weapon it grants.
		 */
		BuildingTypeClass const * Upgrades[BUILDING_UPGRADE_MAX];

		/*
		 * This is the super weapon being launched from this building, so that a silo serving
		 * both the nuclear and the chemical missile fires the right one. If -1, none has fired.
		 */
		int LastSuperWeaponIndex;

		/*
		 * This is the lettered turret variant that this building displays. A turret drawn as
		 * artwork rather than as a voxel has one variant per facing group, and this picks
		 * between them. If -1, then no turret is up at all.
		 */
		int TurretIndex;

		/*
		 * If this building carries a spotlight, then this points to the light that sweeps the
		 * ground in front of it. Its sweep behavior can be changed by a trigger.
		 */
		BuildingLightClass *BuildingLight;

		/*
		 * This is the delay that a gate holds itself open for. It is restarted while anything
		 * stands in the gateway, so that the gate cannot close on what is passing through.
		 */
		ProgressTimerClass<FrameTimerClass> GateTimer;

		/*
		 * If this building tints the terrain around it, then this points to the light source
		 * doing so. It is switched off whenever the building loses power.
		 */
		LightSourceClass *LightSource;

		/*
		 * This is the shape frame that a laser fence section displays. Frames 8 and 12 are the
		 * dormant fence, which can be walked and shot through; the rest are live and kill
		 * whatever shares the cell. On a fence post it is instead a bitfield of the sides that
		 * carried fence, so that the runs can be laid down again afterwards.
		 */
		int LaserFenceFrame;

		/*
		 * This is the shape frame that a firestorm wall section displays. The low bits pick the
		 * artwork joining the section to its neighbors, and 32 is added while the house has its
		 * firestorm defense switched on -- which is also what makes the wall lethal.
		 */
		int FirestormWallFrame;

		/*
		 * This is a second animation stager, for sequences the building's own stager is already
		 * busy with -- an electric turret charging up, or the tick timing of a repair bay.
		 */
		StageClass BuildingStage;

		/*
		 * These record the rectangle that Get_Render_Rect last handed out, along with the
		 * building coordinate and tactical scroll offset it was computed for. The rectangle is
		 * asked for repeatedly, so it is only recomputed once one of those inputs has moved.
		 */
		Rect LastRenderRect;
		Coord LastRenderCoord;
		Point2D LastRenderOffset;

		/*
		 * If this building is switched on, then this flag will be true. A building switched off
		 * draws no power and performs no service until it is switched back on.
		 */
		bool IsOn;

		bool IsOverpowered; // RA2: +0x531
		int SupportingPrisms; // RA2: +0x534
		bool HasExtraPowerBonus; // RA2: +0x538
		bool HasExtraPowerDrain; // RA2: +0x539
		DynamicVectorClass<InfantryClass *> Overpowerers; // RA2: +0x53C
		DynamicVectorClass<InfantryClass *> Occupants; // RA2: +0x554
		int FiringOccupantIndex; // RA2: +0x56C
		AudioController BuildupSoundController; // RA2: +0x570; plays the BuildupSound while under construction
		AudioController WorkingSoundController; // RA2: +0x584; plays WorkingSound or NotWorkingSound as the power changes
		bool WasOnline; // RA2: +0x598

		/*
		 * If this building should always be identified by its real name, then this flag will be
		 * true. An enemy structure is otherwise glossed over with generic help text.
		 */
		bool IsNominal;

		/*
		**	This building should be rebuilt if it is destroyed. This is in spite
		**	of the condition of the prebuilt base list.
		*/
		bool IsToRebuild;

		/*
		**	Is the building allowed to repair itself?
		*/
		bool IsToRepair;

		bool HasEngineer; // RA2: +0x59C
		CDTimerClass<FrameTimerClass> CashProductionTimer; // RA2: +0x5A0

		/*
		**	If the computer owns this building, then it is allowed to sell it if
		**	the situation warrants it. In the other case, it cannot sell the
		**	building regardless of conditions.
		*/
		bool IsAllowedToSell;

		/*
		**	If the building is at a good point to change orders, then this
		**	flag will be set to true.
		*/
		bool IsReadyToCommence;

		/*
		**	If repair is currently in progress and this flag is true, then a wrench graphic
		**	will be overlaid on the building to give visual feedback for the repair process.
		*/
		bool IsWrenchVisible;

		/*
		**	This flag is set when a commando has raided the building and planted
		**	plastic explosives.  When the CommandoCountDown timer expires, the
		**	building takes massive damage.
		*/
		bool IsGoingToBlow;

		/*
		**	If this building was destroyed by some method that would prevent
		**	survivors, then this flag will be true.
		*/
		bool IsSurvivorless;

		/*
		**	These state control variables are used by the obelisk for the charging
		**	animation.
		*/
		bool IsCharging;
		bool IsCharged;

		/*
		**	A building that has been captured will not contain the full compliment
		**	of crew. This is true even if it subsequently gets captured back.
		*/
		bool IsCaptured;

		/*
		**	If Grand_Opening was already called for this building, then this
		**	flag will be true. By utilizing this flag, multiple inadvertant
		**	calls to Grand_Opening won't cause problems.
		*/
		bool HasOpened;

		/// Unused
		bool UnusedBuildingBool1;

		/*
		 * If the animations this building is running are the damaged versions, then this flag
		 * will be true. Taking damage then restarts them only if the form must actually change.
		 */
		bool IsDamagedAnims;

		/*
		 * If this building is being shown as a remembered "fogged" picture rather than drawn
		 * live, then this flag will be true. The building itself must not then draw, be
		 * selected, or plot on the radar, since the player is only seeing what was last seen.
		 */
		bool IsFogged;

		/*
		**	If this building is currently spending money to repair itself, then
		**	this flag is true. It will automatically be set to false when the building
		**	has reached full strength, when money is exhausted, or if the player
		**	specifically stops the repair process.
		*/
		bool IsRepairing;

		/*
		 * If this building's type has buildup animation data, then this flag will be true. A
		 * building without it can never be sold, since there is no animation to play the sale
		 * through. It is answered once, when the building is created, to spare the file read.
		 */
		bool HasBuildupData;

		/*
		 * If this building is on the house power grid, then this flag will be true. It is
		 * cleared while the building is switched off or stunned by an EM pulse, and its light
		 * source, cloaking field, laser fence, and powered animations then all stay shut down.
		 */
		bool IsPoweredOn;

		/*
		 * This is which way a cloak generator's field is moving -- 1 while it grows outward, -1
		 * while it collapses back, and 0 once it has settled. It moves one ring of cells a frame.
		 */
		char CloakGeneratorState;

		/*
		 * This is how far a cloak generator's field currently reaches, expressed in cells.
		 */
		char CurrentCloakRadius;

		/*
		 * This is how far this building has faded from sight, from 0 (solid) to 15 (invisible).
		 * A cloaking building steps it up a level per frame and an uncloaking one steps it back.
		 */
		char TranslucencyLevel;

		int StorageFilledSlots; // RA2: +0x5C0

		/*
		 * This is the lighting level the building was last drawn at (1000 is normal light).
		 * It is kept so that the animations attached to the building can be drawn to match.
		 */
		unsigned short Brightness;

		/*
		 * This is the number of upgrades installed in this building. It doubles as the cursor
		 * into the Upgrades list, so the last upgrade added is the first to be sold or lost.
		 */
		char UpgradeLevel;

		/*
		 * This is the gate animation frame that was last displayed. A gate asks for a redraw
		 * only when this changes, rather than on every frame of the door's travel.
		 */
		char GateFrame;

		int PrismStage; // RA2: +0x5C8
		Coord PrismTargetCoords; // RA2: +0x5CC
		int DelayBeforeFiring; // RA2: +0x5D8

		/*---------------------------------------------------------------------
		**	Constructors, Destructors, and overloaded operators.
		*/
		BuildingClass(BuildingTypeClass const * type = NULL, HouseClass * house = NULL);
		BuildingClass(NoInitClass const & x);
		virtual ~BuildingClass(void) override;

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;

		/*---------------------------------------------------------------------
		**	Member function prototypes.
		*/

		BuildingTypeClass::AnimControlType const * Fetch_Anim_Control(void) {return(&Class->Anims[BState]);};

		/*
		**	Query functions.
		*/
		virtual RTTIType Fetch_RTTI(void) const override;
		virtual int Fetch_Object_Size(void) const override; // RA2: no argument
		virtual void Compute_CRC(CRCEngine &) const override;
		virtual void const * Get_Image_Data(void) const override;
		virtual int How_Many_Survivors(void) const override;
		void Scatter_Incoming_Infantry(void) const; // RA2: not a virtual
		int Get_Collateral_Damage(void) const; // RA2: not a virtual
		virtual int Get_Z_Adjust(void) const override;
		virtual bool Is_Move_Override(void) const override;
		virtual DirType Turret_Facing(void) const override;
		virtual bool Is_Ready_To_Cloak(void) const override;
		virtual bool Should_Uncloak(void) const override;
		virtual DirType Barrel_Pitch(AbstractClass * target) const override;
		virtual Cell Find_Exit_Cell(TechnoClass const * techno); // RA2: not const, so it is a virtual of its own (+0x460)
		virtual Coord Turret_Coord(int which=0) const override;
		virtual InfantryTypeClass const * Crew_Type(void) const override;
		virtual int Pip_Count(void) const override;
		virtual bool Can_Player_Move(void) const override;
		virtual ActionType What_Action(ObjectClass const * target, bool disallow_force = false) const override;
		virtual ActionType What_Action(Cell const & cell, bool check_fog = false, bool disallow_force = false) const override;
		virtual bool Can_Repair(void) const override;
		virtual bool Is_In_Team(void) const override {return(false);} // RA2: 0x453B50
		virtual bool Is_In_Same_Zone_As(ObjectClass const *) const override {return(true);} // RA2: 0x453B60
		virtual ZGradientType Get_Z_Gradient(void) const override {return(ZGRAD_90DEG);} // RA2: 0x453B70
		virtual Cell GetLastFlightMapCoords(void) const override {return(EmptyCell);} // RA2: 0x453B80
		virtual void SetLastFlightMapCoords(Cell) override {} // RA2: 0x453B90
		virtual bool Is_Allowed_To_Leave_Map(void) const override {return(false);} // RA2: 0x453BA0
		virtual void Advance_Waypoint_Path(void) override {} // RA2: 0x453BB0
		virtual bool IsParalyzed(void) const override {return(false);} // RA2: 0x453BC0
		virtual bool Crash(ObjectClass *) override {return(false);} // RA2: 0x453BD0
		virtual void Draw_Action_Line(bool, int) const override {} // RA2: 0x453BE0
		virtual bool Can_Demolish(void) const override;
		bool Can_Upgrade(BuildingTypeClass const * upgrade, HouseClass const * upgrader) const;
		bool Add_Upgrade(void);
		bool Remove_Upgrade(void);
		void Sell_All_Upgrades(void);
		virtual ObjectTypeClass const * Class_Of(void) const override;
		virtual char const * Full_Name(void) const override;
		virtual DirType Fire_Direction(void) const override;
		virtual bool Considered_Vehicle(void) const override;
		virtual bool Is_Radar_Visible(DetectedType & detected) const override;
		int Set_Turret_Frame(void);
		void Set_Turret_Index(int index);
		int Shape_Number(void) const;
		int Power_Output(void) const;
		int Power_Drain(void) const;
		Cell Check_Point(CheckPointType cp) const;
		virtual bool IsPowerOnline(void) const override; // RA2: Is_Powered_On is the TechnoClass virtual, at 0x450360
		SuperWeaponType Fetch_Super_Weapon(void) const;
		SuperWeaponType Fetch_Super_Weapon2(void) const;
		bool Should_Fog(void) const;
		Matrix3D Get_Barrel_Matrix(void) const;

		/*
		**	Coordinate inquiry functions. These are used for both display and
		**	combat purposes.
		*/
		virtual Coord Target_Coord(void) const override;
		virtual Coord Docking_Coord(TechnoClass * docker) const override; // RA2: the docking object
		virtual Coord Render_Coord(void) const override;
		virtual Coord Fire_Coord(int which) const override;
		virtual Coord Center_Coord(void) const override;
		virtual Coord Destination_Coord(TechnoClass * docker = NULL) const override; // RA2: the AbstractClass virtual takes the docking object
		virtual int Sort_Y(void) const override;
		virtual Coord Exit_Coord(int index) const override; // RA2: an index

		Coord Voxel_Fire_Coord(int which, bool just_fired) const;

		/*
		**	Object entry and exit from the game system.
		*/
		virtual void Init(void) override;
		virtual void Detach(AbstractClass const * target, bool all) override;
		virtual void Detach_All(bool all=true) override;
		virtual int DistanceToDockingCoord(TechnoClass * docker) const; // RA2
		virtual void Grand_Opening(bool captured = false);
		void Eject_Occupants(bool flag); // RA2: 0x452F50, empties a garrisoned building
		virtual void Update_Buildables(void);
		virtual MoveType Can_Enter_Cell(CellClass const * cell, FacingType dir = FACING_NONE, int cell_height = -1, CellClass const * = 0, bool = true) const override;
		virtual bool Unlimbo(Coord const & , Dir256 dir = DIR_N) override;
		virtual bool Limbo(void) override;
		void Reserve_Base_Area(bool=false);
		void Release_Base_Area(void);

		/*
		**	Display and rendering support functionality. Supports imagery and how
		**	object interacts with the map and thus indirectly controls rendering.
		*/
		virtual VisualType Visual_Character(bool raw = false, HouseClass const * = NULL) const override;
		virtual void Set_Occupy_Bit(Coord const & coord) override;
		virtual void Clear_Occupy_Bit(Coord const & coord) override;
		virtual int Exit_Object(TechnoClass * base) override;
		virtual bool Render(Rect &, bool forced, bool extras_only) const override;
		virtual void Draw_It(Point2D const & point, Rect const & cliprect) const override;
		virtual void Editor_Draw_It(Point2D const & point, Rect const & cliprect) const override;
		virtual bool Mark(MarkType mark=MARK_CHANGE) override;
		virtual Rect Get_Render_Rect(void) override;
		virtual void Draw_Radial_Indicator(DWORD) override; // RA2: takes an argument; not const
		int GetRadialIndicatorRange(void) const; // RA2: 0x451930
		int Tinted_Brightness(int brightness); // RA2: 0x4521E0
		void Rotate_Overpowerer(InfantryClass * infantry); // RA2: 0x44DA20
		bool Is_In_Psychic_Range(AbstractClass const * target); // RA2: 0x44DAA0
		bool Is_Cell_In_Psychic_Range(CellClass const * cell); // RA2: 0x44DB40
		virtual void IronCurtain(int duration) override; // RA2: 0x452E60
		bool CanBeOccupiedBy(InfantryClass * infantry) const; // RA2: 0x452EB0
		int GetOccupantCount(void) const; // RA2: 0x4532F0
		void Update_Garrison_Owner(void); // RA2: 0x453300
		void Refresh_State_Animations(void); // RA2: 0x453450
		void KillOccupants(TechnoClass * killer); // RA2: 0x4536D0
		void Produce_Cash(void); // RA2: 0x453850
		bool Exit_New_Object(ObjectTypeClass const * type); // RA2: 0x4538D0
		int Storage_Level(void) const; // RA2: 0x453AB0
		bool Is_Impassable_Cell(CellClass const * cell); // RA2: 0x453AF0
		void Draw_Overlays(Point2D const & point, Rect const & cliprect) const;
		void Begin_Mode(BStateType bstate);
		virtual void Draw_Extras(Point2D &point, Rect &rect);
		virtual DirType Aim_Direction(AbstractClass * target) const;
		void Make_Fogged(DynamicVectorClass<FoggedObjectClass *> * fogged_objects = NULL, CellClass * cellptr = NULL, bool fade = false);
		void Charge_Turret(void);
		void Discharge_Turret(void);

		/*
		**	User I/O.
		*/
		virtual bool Active_Click_With(ActionType action, ObjectClass * object, bool) override;
		virtual bool Active_Click_With(ActionType action, Cell const & cell, Cell const &, bool) override; // RA2: a second cell
		virtual void Clicked_As_Target(int = 7) override;

		/*
		**	Combat related.
		*/
		virtual void Death_Announcement(TechnoClass const * source=0) const override;
		virtual FireErrorType Can_Fire(AbstractClass *, int which, bool ignoreRange=false) const override; // RA2: ignoreRange
		virtual AbstractClass * Greatest_Threat(ThreatType threat, Coord const & coord, bool) const override;
		virtual ResultType Take_Damage(int & damage, int distance, WarheadTypeClass const * warhead, TechnoClass * source=0, bool forced=false, bool=false) override;
		virtual bool Captured(HouseClass * newowner, bool announce = true) override; // RA2: announce
		virtual WeaponDataStruct const * Get_Class_Weapon_Data(int which=0) const override;
		virtual bool Is_Turret_Equipped() const override;
		void Update_Radar_Spied(void);
		void Spied_By(HouseClass * house);

		void Update_FS_Wall_State(void);
		bool Crossing_Firestorm(ObjectClass * object, bool do_damage);

		int Anti_Air_Defense_Value(void);
		int Anti_Armor_Defense_Value(void);
		int Anti_Infantry_Defense_Value(void);

		/*
		**	AI.
		*/
		void Charging_AI(void);
		void Rotation_AI(void) {}
		void Factory_AI(void);
		void Repair_AI(void);
		void Animation_AI(void);
		virtual bool Revealed(HouseClass * house) override;
		virtual void Repair(int control) override;
		virtual void Sell_Back(int control) override;
		virtual RadioMessageType Receive_Message(RadioClass * from, RadioMessageType message, int & param) override;
		virtual void AI(void) override;
		virtual void Cloaking_AI(bool fast) override;
		virtual void Assign_Target(AbstractClass * target) override;
		virtual void Do_Destruction(TechnoClass * last_contact, TechnoClass * source, bool forced, Cell const * offset);
		virtual bool Toggle_Primary(void);
		virtual void DisguiseDetectorActivate(Cell cell = Cell()); // RA2: my name; pairs with DisguiseDetectorDeactivate
		virtual void SensorArrayDeactivate(Cell cell = Cell()); // RA2
		virtual void SensorArrayActivate(Cell cell = Cell()); // RA2
		virtual void DisguiseDetectorDeactivate(Cell cell = Cell()); // RA2
		virtual unsigned entry_380(void); /// Returns 0. A reserved vtable slot that nothing implements.
		void Turn_On(void);
		void Turn_Off(void);
		void Power_On(void);
		void Power_Off(void);
		bool Open_Gate(void);
		bool Is_Gate_Open(void) const;
		bool Is_Blocked_By_Occupier(void);
		int Flush_For_Placement(TechnoClass * techno, Cell const & cell);
		void Enable_Cloak_Generator(void);
		void Disable_Cloak_Generator(void);
		void Disable_Sensor_Array(void);
		void Enable_Sensor_Array(void);

		virtual bool Ready_To_Commence(void) override;
		virtual int Do_MISSION_UNLOAD(void) override;
		virtual int Do_MISSION_REPAIR(void) override;
		virtual int Do_MISSION_ATTACK(void) override;
		virtual int Do_MISSION_HARVEST(void) override;
		virtual int Do_MISSION_GUARD(void) override;
		virtual int Do_MISSION_GUARD_AREA(void) override;
		virtual int Do_MISSION_CONSTRUCTION(void) override;
		virtual int Do_MISSION_DECONSTRUCTION(void) override;
		virtual int Do_MISSION_MISSILE(void) override;
		virtual int Do_MISSION_OPEN(void) override;
		bool Can_Be_Undeployed(void);
		virtual int Apparent_Brightness(int brightness = 1000) const override;
		virtual void Assign_Destination(AbstractClass * target, bool = true) override;
		void Assign_Rally_Point(Cell const & cell);
		virtual bool Enter_Idle_Mode(bool initial=false,  bool = true) override;
		virtual void Radar_Track(void) override;
		virtual void Radar_Untrack(void) override;
		virtual void Plot_On_Radar(void) const override;

		bool Clear_Weapons_Factory_Bib(void);

		void Init_Laser_Fence(void);
		void Update_Laser_Fence_Connections(int explode);
		BuildingClass * Find_Laser_Fence_Post(FacingType dir, bool connected, int range);
		static void Init_All_Laser_Fences(void);
		void Connect_Laser_Fence(FacingType dir);
		void Disconnect_Laser_Fence(FacingType dir, bool explode);
		static void Unlimbo_Laser_Fence_Helper(Cell const & cell);
		void Init_Laser_Fence_Frame(void);
		void Toggle_Laser_Fence_Post(bool force);

		int Get_Firestorm_Wall_Frame(void);

		void Begin_Anim(BAnimType anim, bool damaged, int delay = 0);
		void Set_Anim_Coords(void);
		void Create_Anim(char const * name, BAnimType anim, bool damaged, bool garrisoned, int delay); // RA2: garrisoned
		void Detach_Anim(AnimClass * anim);
		void End_Anim(BAnimType anim);
		void Set_Anim_Damage_State(bool damaged);
		void Set_Anim_Drawer(ConvertClass * drawer, int brightness);
		void Update_Anim_Appearance(void);
		void Set_Anim_Translucency(int translucency);
		bool Anim_Active(BAnimType anim) { return(Anims[anim] != NULL); }

		/*
		**	Scenario and debug support.
		*/
#ifdef _DEBUG
		virtual void Debug_Dump(MonoClass *mono) const override;
#endif

		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass const & ini);
		static void Write_All(CCINIClass & ini);
		void Write_INI(CCINIClass & ini);

	private:
		void Drop_Debris(AbstractClass * source = NULL);

		/*
		 * This is the scenario INI section that lists the buildings to place upon the map.
		 */
		static char const * const INI_NAME;
};

void Adjust_House_Power(HouseClass * house);
BuildingClass * Find_Unit_Repair_Facility(HouseClass * house, TechnoClass * obj);


inline BuildingClass * AbstractClass::As_BuildingClass(void)
{
	return(dynamic_cast<BuildingClass *>(this));
}


inline BuildingClass const * AbstractClass::As_BuildingClass(void) const
{
	return(dynamic_cast<BuildingClass const *>(this));
}
