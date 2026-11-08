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
**	The various armor types are best suited to defend against a limited
**	kind of warheads. The game strategy revolves around proper
**	combination of armor and weaponry. Each vehicle or building has armor
**	rated according to one of the following types.
*/
enum ArmorType {
	ARMOR_NONE,     // RA2: the armor types are RA2's (YR names).
	ARMOR_FLAK,
	ARMOR_PLATE,
	ARMOR_LIGHT,
	ARMOR_MEDIUM,
	ARMOR_HEAVY,
	ARMOR_WOOD,
	ARMOR_STEEL,
	ARMOR_CONCRETE,
	ARMOR_SPECIAL_1,
	ARMOR_SPECIAL_2,

	ARMOR_COUNT,
	ARMOR_FIRST=0
};
