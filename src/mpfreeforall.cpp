// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#define RA2_GEOMETRY_H // geometry.h follows the code
#include "always.h"
#include "rect.h"
#include "mpfreeforall.h"
#include "session.h"
#include <windows.h>
#undef LoadString
#include <math.h>

int __fastcall Clear_ComboBox(HWND list); // RA2: 0x006007F0

FreeForAll::FreeForAll(WideWstring const & title, WideWstring const & tooltip, Wstring const & inifile, Wstring const & mapfilter, bool aiallowed, int modeindex) :
	MultiplayerGameMode(title, tooltip, inifile, mapfilter, aiallowed, modeindex)
{
}

FreeForAll::~FreeForAll(void)
{
}

bool FreeForAll::AssignTeams(void)
{
	return(MultiplayerGameMode::AssignTeams());
}

bool FreeForAll::CheckStartConditions(WideWstring * message)
{
	if (!MultiplayerGameMode::CheckStartConditions(message)) {
		return(false);
	}
	NodeNameType * node;
	for (int i = 0; i < Session.Players.Count(); i++) {
		node = Session.Players[i];
		if (node->Slot.SpectatorFlag != -1) {
			Session.Players[i]->Slot.SpectatorFlag = i;
		}
	}
	return(true);
}

bool FreeForAll::AssignTeamsAtStart(void)
{
	if (!MultiplayerGameMode::AssignTeamsAtStart()) {
		return(false);
	}
	for (int i = 0; i < Session.Players.Count(); i++) {
		if (Session.Players[i]->Slot.SpectatorFlag != -1) {
			Session.Players[i]->Slot.SpectatorFlag = i;
		}
	}
	return(true);
}

void FreeForAll::PopulateTeamDropdownForPlayer(HWND hwnd, int index)
{
	Clear_ComboBox(hwnd);
	MultiplayerTeam * team = GetTeam(index);
	if (team) {
		team->AddToList(hwnd);
	}
	team = GetTeam(-1);
	if (team) {
		team->AddToList(hwnd);
	}
	SendMessage(hwnd, CB_SETCURSEL, 0, 0);
}

bool FreeForAll::IsSkirmishAllowed(void)
{
	return(true);
}

const Rect RECT_NONE_2(0, 0, 0, 0);

#include "geometry.h"
