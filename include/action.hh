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

#pragma once


/// TS names derived from FA2/FS

/**********************************************************************
**	Whenever a unit is selected and a click occurs over another object
**	or terrain element, there is some action to initiate. This specifies
**	the different types of actions possible. This also controls how the
**	mouse cursor looks when "hovering" over the spot that clicking would
**	occur at.
*/
enum ActionType {
	ACTION_NONE,			// Either undefined action or "do nothing".
	ACTION_MOVE,			// Can move there or at least try to.
	ACTION_NOMOVE,			// Special case for movable object, but illegal mouse position.
	ACTION_ENTER,			// Special case for infantry->APC or vehicle->Repair facility.
	ACTION_SELF,			// Self select special case.
	ACTION_ATTACK,			// Can attack or fire upon it in some fashion.
	ACTION_HARVEST,			// Special harvest mode.
	ACTION_SELECT,			// Would change selection to specified object.
	ACTION_TOGGLE_SELECT,	// Toggles select state of the object.
	ACTION_CAPTURE,			// The unit will try to capture the object.
	ACTION_REPAIR,			// The target object should be repaired.
	ACTION_EATEN,			// RA2
	ACTION_SELL,			// The target building should be sold back.
	ACTION_SELL_UNIT,		// The target unit should be sold back.
	ACTION_NO_SELL,  		// No sell or no repair.
	ACTION_NO_REPAIR,		// No sell or no repair.
	ACTION_SABOTAGE,		// The unit will try to sabotage/destroy the object.
	ACTION_TOTE,
	ACTION_DONT_USE2,		// RA2: unused
	ACTION_DONT_USE3,		// RA2: unused
	ACTION_NUKE_BOMB,		// That target object should be blasted.
	ACTION_DONT_USE4,		// RA2: unused
	ACTION_DONT_USE5,		// RA2: unused
	ACTION_DONT_USE6,		// RA2: unused
	ACTION_DONT_USE7,		// RA2: unused
	ACTION_DONT_USE8,		// RA2: unused
	ACTION_GUARD_AREA,		// Guard the area/object clicked on.
	ACTION_HEAL,			// Heal the infantryman clicked on.
	ACTION_DAMAGE,			// Enter and damage building.
	ACTION_GREPAIR,			// Enter and complete repair building.
	ACTION_NO_DEPLOY,
	ACTION_NO_ENTER,
	ACTION_NO_GREPAIR,
	ACTION_TOGGLE_POWER,
	ACTION_NO_TOGGLE_POWER,
	ACTION_ENTER_TUNNEL,
	ACTION_NO_ENTER_TUNNEL,
	ACTION_IRON_CURTAIN,	// RA2: that target object should be invulnerable.
	ACTION_LIGHTNING_STORM,	// RA2
	ACTION_CHRONOSPHERE,	// RA2: that target object should be teleported.
	ACTION_CHRONO_WARP,		// RA2: teleport it to the given coordinates now.
	ACTION_PARA_DROP,		// RA2
	ACTION_PLACE_WAYPOINT,
	ACTION_NO_PLACE_WAYPOINT,
	ACTION_ENTER_WAYPOINT_MODE,
	ACTION_FOLLOW_WAYPOINT,
	ACTION_SELECT_WAYPOINT,
	ACTION_LOOP_WAYPOINT_PATH,
	ACTION_DRAG_WAYPOINT,
	ACTION_ATTACK_WAYPOINT,
	ACTION_ENTER_WAYPOINT,
	ACTION_PATROL_WAYPOINT,
	ACTION_AREA_ATTACK,		// RA2
	ACTION_IVAN_BOMB,		// RA2
	ACTION_NO_IVAN_BOMB,	// RA2
	ACTION_DETONATE,		// RA2
	ACTION_DETONATE_ALL,	// RA2
	ACTION_DISARM_BOMB,		// RA2
	ACTION_SELECT_NODE,		// RA2
	ACTION_ATTACK_SUPPORT,
	ACTION_PLACE_BEACON,	// RA2
	ACTION_SELECT_BEACON,	// RA2

	ACTION_COUNT
};
