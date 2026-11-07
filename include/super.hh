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

#pragma once

/**********************************************************************
**	These are the special weapons that can be used in the game. The common thread
**	with these weapons is that they are controlled through the sidebar
**	mechanism.
*/
enum SuperWeaponType {
	SUPER_NONE=-1,

	SUPER_MULTI_MISSILE,
	SUPER_IRON_CURTAIN,			// RA2: the super weapon types are RA2's (YR names)
	SUPER_LIGHTNING_STORM,
	SUPER_CHRONO_SPHERE,
	SUPER_CHRONO_WARP,
	SUPER_PARA_DROP,
	SUPER_AMER_PARA_DROP,

	SUPER_COUNT,
	SUPER_FIRST=0,

	SUPER_ANY=1
};
