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

/* $Header: /CounterStrike/WARHEAD.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WARHEAD.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 05/17/96                                                     *
 *                                                                                             *
 *                  Last Update : May 17, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "abstype.h"
#include "typelist.h"

#include "armor.hh"

class AnimTypeClass;
class ParticleSystemTypeClass;

/**********************************************************************
**	Each of the warhead types has specific characteristics. This structure
**	holds these characteristics.
*/
class WarheadTypeClass : public AbstractTypeClass
{
		typedef AbstractTypeClass BASECLASS;

	public:
		WarheadTypeClass(char const * ininame = NULL);
		WarheadTypeClass(NoInitClass const & x);
		virtual ~WarheadTypeClass(void) override;

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;

		virtual void Detach(AbstractClass const * target, bool all = true) override;

		virtual RTTIType Fetch_RTTI(void) const override;
		virtual int Fetch_Object_Size(void) const override; // RA2: no argument

		virtual void Compute_CRC(CRCEngine &) const override;

		bool Read_INI(CCINIClass const & ini);

		static WarheadTypeClass *Find_Or_Make(const char *name);
		static WarheadTypeClass *From_Name(char const * name);

		/*
		 * This is the chance, per point of damage inflicted, that the blast will crater the
		 * ground beneath it. The damage must first exceed the DeformThreshhold before the
		 * roll is made at all.
		 */
		double Deform;

		/*
		**	The warhead damage is reduced depending on the the type of armor the
		**	defender has. This table is what gives weapons their "character".
		*/
		double Modifier[11]; // RA2: eleven armor types, not ARMOR_COUNT

		/*
		 * This is the fraction of the damage that reaches an infantryman who is lying prone,
		 * where one means full damage. A prone infantryman still always takes at least one
		 * point of damage, however small the fraction.
		 */
		double ProneDamage;

		/*
		 * Damage at or below this amount will never crater the ground, however lucky the
		 * Deform roll would have been.
		 */
		int DeformThreshhold;

		/*
		**	Which explosion set to use for warhead impact.
		*/
		TypeList<AnimTypeClass const *> ExplosionSet;

		/*
		**	This specifies the infantry death animation to use if the infantry dies as
		**	a result of a warhead of this type.
		*/
		int InfantryDeath;

		/*
		**	This value control how damage from this warhead type will reduce
		**	over distance. The larger the number, the less the damage is reduced
		**	the further the distance from the source of the damage.
		*/
		float SpreadFactor; // RA2: a float, read from CellSpread

		float CellInset; // RA2: +0x128
		float PercentAtMax; // RA2: +0x12C
		bool CausesDelayKill; // RA2: +0x130
		int DelayKillFrames; // RA2: +0x134
		float DelayKillAtMax; // RA2: +0x138
		float CombatLightSize; // RA2: +0x13C

		/*
		 * Pointer to the particle system this warhead releases where it detonates. A "webby"
		 * warhead spawns one in every cell it webs. If NULL, then no particles are released.
		 */
		ParticleSystemTypeClass *Particle;

		/*
		**	If this warhead type can destroy walls, then this flag will be true.
		*/
		bool IsWallDestroyer;

		bool WallAbsoluteDestroyer; // RA2: +0x145

		/*
		**	If this warhead can destroy wooden walls, then this flag will be true.
		*/
		bool IsWoodDestroyer;

		/*
		**	Does this warhead damage tiberium?
		*/
		bool IsTiberiumDestroyer;

		/*
		**	Only effective against infantry?
		*/
		bool IsOrganic;

		/*
		 * If this warhead sets what it hits alight, then this flag will be true. A terrain
		 * object struck by it catches fire, and a building shows pieces of flame as it
		 * drops through a damage level.
		 */
		bool IsSparky;

		bool Sonic; // RA2: +0x14A

		/*
		 * If this warhead burns rather than blasts, then this flag will be true. Fire melts
		 * the ice it lands on, but it cannot bring down a destroyable cliff, so the attack
		 * cursor will not offer one as a target.
		 */
		bool IsFire;

		/*
		 * If this warhead throws up a splash when it strikes water, then this flag will be
		 * true. Such a blast picks its animation from the splash list in the rules rather
		 * than from its own explosion set.
		 */
		bool IsConventional;

		/*
		 * If this warhead rocks the vehicles caught in the blast, then this flag will be
		 * true. Everything within three cells is tipped by the explosion, with the force
		 * scaled by the damage -- a weak enough blast rocks nothing at all.
		 */
		bool IsRocker;

		/*
		 * If this warhead lights up the terrain around the blast, then this flag will be
		 * true. The explosion drops a spotlight whose size grows with the damage.
		 */
		bool IsBright;

		bool CLDisableRed; // RA2: +0x14F
		bool CLDisableGreen; // RA2: +0x150
		bool CLDisableBlue; // RA2: +0x151

		/*
		 * If this warhead detonates as an electromagnetic pulse rather than as a blast, then
		 * this flag will be true. Its impact animation is also picked at random rather than
		 * chosen according to the strength of the damage.
		 */
		bool IsEMEffect;

		bool MindControl; // RA2: +0x153
		bool IvanBomb; // RA2: +0x154
		bool ElectricAssault; // RA2: +0x155
		bool Parasite; // RA2: +0x156
		bool Temporal; // RA2: +0x157
		bool BombDisarm; // RA2: +0x158
		int Paralyzes; // RA2: +0x15C
		bool Culling; // RA2: +0x160
		bool MakesDisguise; // RA2: +0x161
		bool NukeMaker; // RA2: +0x162
		bool Radiation; // RA2: +0x163
		bool PsychicDamage; // RA2: +0x164
		bool Bullets; // RA2: +0x165

		/*
		 * If this warhead is what the veins and the veinhole monster attack with, then this
		 * flag will be true. Such damage arrives with no source object, so a unit hurt by it
		 * retaliates against the veinhole that owns the veins it is standing in.
		 */
		bool IsVeinhole;

		int ShakeXlo; // RA2: +0x168
		int ShakeXhi; // RA2: +0x16C
		int ShakeYlo; // RA2: +0x170
		int ShakeYhi; // RA2: +0x174

};
