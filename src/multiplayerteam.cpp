// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "rect.h"
#include "multiplayer.h"
#include "session.h"
#include <windows.h>
#undef LoadString
#include <math.h>

MultiplayerTeam::MultiplayerTeam(WideWstring const & title, int index) :
	Title(title),
	Index(index)
{
}

MultiplayerTeam::~MultiplayerTeam(void)
{
}

bool MultiplayerTeam::IsTeamIncluded(int index)
{
	if (index >= 0 && index < Session.Players.Count()) {
		return true;
	}
	return false;
}

bool MultiplayerTeam::SetPlayerTeam(int player)
{
	if (!IsTeamIncluded(player)) {
		return false;
	}
	Session.Players[player]->Slot.SpectatorFlag = Index;
	return true;
}

int MultiplayerTeam::CountMembers(void)
{
	int count = 0;
	for (int i = 0; i < Session.Players.Count(); i++) {
		if (Session.Players[i]->Slot.SpectatorFlag == Index) {
			count++;
		}
	}
	return count;
}

int MultiplayerTeam::AddToList(HWND list)
{
	int item = SendMessage(list, CB_ADDSTRING, 0, (LPARAM)Title.text());
	SendMessage(list, CB_SETITEMDATA, item, Index);
	return item;
}
