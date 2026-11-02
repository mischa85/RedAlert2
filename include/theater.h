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

#include "theater.hh"

/**********************************************************************
**	The theater mixfiles are cached into a buffer of this size. Ensure
**	that the size specified is at least as large as the largest
**	theater mixfile data block.
*/
#define THEATER_BUFFER_SIZE	4000000L

struct TheaterDataType {
	char		Name[16];
	char		Root[10];
	char		IsoRoot[10];
	char		Suffix[4];
};
