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

#include "diskswap.h"

#include "theme.hh"


class CD : public DiskSwap
{
		typedef DiskSwap BASECLASS;

	public:
		virtual bool Force_Available(DiskID disk) override;
		virtual bool Request_Disk(DiskID disk) override;
		virtual bool Swap(DiskID disk) override;

		static DiskID Get_Current_Disk(void);

		static bool Force_Available(void) { return(CD().Force_Available(RequiredCD)); }

		static bool Set_Required_Disk(DiskID disk);
		static DiskID Get_Required_Disk(void) { return(RequiredCD); }

		static void Override_Swap(bool state) { _OverrideSwap = state; }
		static bool Is_Override_Swap(void) { return(_OverrideSwap); }

	private:
		/*
		 * This is the score that was playing when a disc prompt interrupted it. The music
		 * must be stopped while the disc is out of the drive, so this remembers what to put
		 * back on if the player gives up.
		 */
		ThemeType ThemePlaying;

		/*
		 * This is the disc the game needs in the drive, recorded as the loading code moves
		 * from one body of data to the next. DISK_LOCAL means the data lives on the hard
		 * drive and no disc is called for.
		 */
		static DiskID RequiredCD;

		/*
		 * If every disc is to be treated as already present, then this flag will be true.
		 * The demo, the debug build and the modern build all run entirely off the hard
		 * drive and set it, so that the player is never asked to swap discs.
		 */
		static bool _OverrideSwap;
};
