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

#include "theater.hh"

/**********************************************************************
**	The theater mixfiles are cached into a buffer of this size. Ensure
**	that the size specified is at least as large as the largest
**	theater mixfile data block.
*/
#define THEATER_BUFFER_SIZE	4000000L

struct TheaterDataType {
	char		Name[16];
	char		UIName[32];				// RA2: the string table name (YR name)
	char		Root[10];
	char		IsoRoot[10];
	char		Suffix[4];
	char		MMExtension[4];			// RA2 (YR name)
	char		Letter[2];				// RA2 (YR name)
	float		RadarTerrainBrightness;	// RA2: +0x50 (YR name)
	float		Unknown_54;				// RA2: 1.6 temperate/urban, 1.1 snow; use not known
	double		Unknown_58;				// RA2: 1.0 temperate/urban, 0.75 snow; use not known
	int			Unknown_60;				// RA2: -1 temperate/snow, 0 urban; use not known
	int			Unknown_64;				// RA2: 0 in each theater; use not known
};										// RA2: 0x68 bytes
