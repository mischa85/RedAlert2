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

#include "cd.h"

#include "_keyboar.h"
#include "_mixfile.h"
#include "_pk.h"
#include "_xmouse.h"
#include "ccfile.h"
#include "cdcntrl.h"
#include "cdfile.h"
#include "data.h"
#include "dbgprint.h"
#include "globals.h"
#include "keyboard.h"
#include "language\language.h"
#include "mixfile.h"
#include "msgbox.h"
#include "theme.h"
#include "volinfo.h"
#include "stringtable.h"

/*
**	RA2: the original path of this file, passed with each string table lookup (see stringtable.h).
*/
#define RA2_SOURCE "D:\\Ra2\\CD.CPP"

DiskID CD::RequiredCD = DISK_ANY;

bool CD::_OverrideSwap;


/// <summary>
/// Fetches the disc that is currently in the CD drive.
/// This routine is used whenever the game needs to know which disc the player has loaded,
/// such as when deciding whether a swap prompt is called for.
/// </summary>
/// <returns>Returns with the ID of the disc found, or DISK_ANY if it is not a game disc.</returns>
DiskID CD::Get_Current_Disk(int timeout)
{
	return(BASECLASS::Get_Disk_ID(CDFileClass::Get_CD_Drive(), timeout));
}


/// <summary>
/// Sets the disc that the game will require from now on.
/// This routine is used by the loading code to record which disc the data it is about to
/// need lives on. When the swap override is in effect, the requirement is pinned to the
/// local hard drive instead, so that no disc is ever asked for.
/// </summary>
/// <param name="disk">The disc that should be required.</param>
/// <returns>bool; Is the swap override in effect, such that the disc was ignored?</returns>
bool CD::Set_Required_Disk(DiskID disk)
{
	RequiredCD = DISK_LOCAL;

	if (_OverrideSwap != true) {
		RequiredCD = disk;
	}

	return(_OverrideSwap);
}


/// <summary>
/// Ensures that the specified disc is available.
/// Call this routine when the disc must genuinely be present before proceeding. A disc
/// that resides on the local hard drive is always considered available, so the player is
/// never bothered about it.
/// </summary>
/// <param name="disk">The disc that must be made available.</param>
/// <returns>bool; Is the disc available? A false return means the player cancelled.</returns>
bool CD::Force_Available(DiskID disk)
{
	if (disk == DISK_LOCAL) {
		return(true);
	}
	if (_OverrideSwap == true) {
		return(true); // RA2
	}
	ThemePlaying = THEME_NONE;
	return(BASECLASS::Force_Available(disk));
}


/// <summary>
/// Prompts the player to insert the disc that is needed.
/// This routine is called by the disc searching logic when the disc could not be found in
/// any drive. The music is stopped for the duration of the prompt (and resumed if the
/// player gives up), the mouse is forced visible, and the CD tray is unlocked so that the
/// disc can actually be changed.
/// </summary>
/// <param name="disk">The disc to ask the player for.</param>
/// <returns>bool; Should the search continue? A false return means the player cancelled.</returns>
bool CD::Request_Disk(DiskID disk)
{
	wchar_t buf[256]; // RA2: wide text
	wchar_t name[128];

	/*
	**	RA2: the message names the side's disk; it is built but the prompt below shows
	**	its own text.
	*/
	if (disk == DISK_ANY) {
		swprintf(buf, StringTable::LoadString("TXT_CD_DIALOG_1", NULL, RA2_SOURCE, 146));
	} else if (disk == DISK_GDI) {
		swprintf(buf, StringTable::LoadString("TXT_CD_DIALOG_A", NULL, RA2_SOURCE, 151));
	} else if (disk == DISK_NOD) {
		swprintf(buf, StringTable::LoadString("TXT_CD_DIALOG_S", NULL, RA2_SOURCE, 156));
	} else {
		swprintf(name, StringTable::LoadString("TXT_DISK_NAME", NULL, RA2_SOURCE, 160), disk + 1);
		swprintf(buf, StringTable::LoadString("TXT_CD_DIALOG_2", NULL, RA2_SOURCE, 161), disk + 1, name);
	}

	if (ThemePlaying == THEME_NONE) {
		ThemePlaying = Theme.What_Is_Playing();
	}

	Theme.Stop();

	int hidden = Get_Mouse_State() < 0;

	while (Get_Mouse_State() < 0) {
		Show_Mouse();
	}

	Keyboard->Clear();

	bool res = true;

	if (disk == DISK_GDI) {
		if (!Prompt_Dialog(StringTable::LoadString("TXT_CD_DIALOG_A", NULL, RA2_SOURCE, 194))) res = false;
	} else if (disk == DISK_NOD) {
		if (!Prompt_Dialog(StringTable::LoadString("TXT_CD_DIALOG_S", NULL, RA2_SOURCE, 202))) res = false;
	} else {
		if (!Prompt_Dialog(StringTable::LoadString("TXT_CD_DIALOG_1", NULL, RA2_SOURCE, 209))) res = false;
	}

	Keyboard->Clear();

	while (hidden--) {
		Hide_Mouse();
	}

	if (res == false && ThemePlaying != THEME_NONE) {
		Theme.Queue_Song(ThemePlaying);
		ThemePlaying = THEME_NONE;
	}

	return(res);
}


/// <summary>
/// Rebinds the mix file set to the disc just inserted.
/// This routine is called by the disc swapping logic once the required disc has been
/// located. Every mix file that lives on the disc is released and reopened from the new
/// disc, and the theme list is rescanned so that the music now available can be played.
/// </summary>
/// <param name="disk">The disc that has been swapped in.</param>
/// <returns>bool; Were the mix files reopened successfully?</returns>
bool CD::Swap(DiskID disk)
{
	DebugString("***** Swapping to disk %d *****\n", disk);

	if (ThemePlaying == THEME_NONE) {
		ThemePlaying = Theme.What_Is_Playing();
	}

	Theme.Stop();

	if (MoviesMix) {
		DebugString("     Releasing %s\n", MoviesMix->Filename);
		delete MoviesMix;
		MoviesMix = NULL;
	}

	if (ScoresMix) {
		DebugString("     Releasing %s\n", ScoresMix->Filename);
		delete ScoresMix;
		ScoresMix = NULL;
	}

	if (MultiMix) {
		DebugString("     Releasing %s\n", MultiMix->Filename);
		delete MultiMix;
		MultiMix = NULL;
	}

	if (MapsMix) {
		DebugString("     Releasing %s\n", MapsMix->Filename);
		delete MapsMix;
		MapsMix = NULL;
	}

	if (MainMix) {
		DebugString("     Releasing %s\n", MainMix->Filename);
		delete MainMix;
		MainMix = NULL;
	}

	if (!VolumeCheck()) {
		return(true);
	}

	char mixname[64];

	MainMix = new MixFileClass("MAIN.MIX", &FastKey);
	DebugString("     Initialized %s\n", MainMix->Filename);

	disk = DiskID(disk + 1);

	sprintf(mixname, "MAPS%02d.MIX", disk);
	DebugString("     Initializing %s\n", mixname);
	MapsMix = new MixFileClass(mixname, &FastKey);
	if (!MapsMix) {
		DebugString("     FAILED!\n");
		return(false);
	}

	DebugString("     Initializing Multi.Mix\n");
	MultiMix = new MixFileClass("MULTI.MIX", &FastKey);
	if (!MultiMix) {
		DebugString("     FAILED!\n");
		return(false);
	}

	DebugString("     Initializing Themes.MIX\n");
	ScoresMix = new MixFileClass("THEME.MIX", &FastKey); // RA2: the music is in THEME.MIX

	Theme.Scan();

	sprintf(mixname, "MOVIES%02d.MIX", disk);
	DebugString("     Initializing %s\n", mixname);
	MoviesMix = new MixFileClass(mixname, &FastKey);
	if (!MoviesMix) {
		DebugString("     FAILED!\n");
		return(false);
	}

	return(VolumeCheck());
}
