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

#include "always.h"

#include "dbgprint.h"
#include "playcd.h"
#include "win.h"


/// <summary>
/// Constructor for the CD drive list object.
/// This routine asks the system about every logical drive letter and records the ones
/// that turn out to be CD-ROM drives. The disc searching code walks this list when it
/// hunts for the game CD, so an empty list means there is nowhere to look.
/// </summary>
GetCDClass::GetCDClass(void)
{
	char path[] = "a:\\";

	CDCount = 0;
	CDIndex = 0;

	DebugString("GetCDClass constructor\n");

	memset(CDDrives, NO_CD_DRIVE, MAX_CD_DRIVES);

	for (char i = 'c'; i <= 'z'; i++) {
		path[0] = i;
		if (GetDriveType(path) == DRIVE_CDROM) {
			CDDrives[CDCount++] = int(i - 'a');
			DebugString("CD drive found - %c:\n", (i +'A'-'a'));
		}
	}

	if (CDCount == 0) {
		for (char i = 'a'; i <= 'b'; i++) {
			path[0] = i;
			if (GetDriveType(path) == DRIVE_CDROM) {
				CDDrives[CDCount++] = int(i - 'a');
			}
		}
	}

	if (CDCount == 0) {
		DebugString("No CD drives found\n");
	}
}


/// <summary>
/// Destructor for the CD drive list object.
/// </summary>
GetCDClass::~GetCDClass(void)
{
}
