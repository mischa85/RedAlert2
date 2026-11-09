// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#pragma once

#include "multiplayer.h"

class FreeForAll : public MultiplayerGameMode
{
	public:
		FreeForAll(NoInitClass const & x) : MultiplayerGameMode(x) {}
		FreeForAll(WideWstring const & title, WideWstring const & tooltip, Wstring const & inifile, Wstring const & mapfilter, bool aiallowed, int modeindex);
		virtual ~FreeForAll(void);
		virtual bool AssignTeams(void) override;
		virtual bool CheckStartConditions(WideWstring * message) override;
		virtual bool AssignTeamsAtStart(void) override;
		virtual bool IsSkirmishAllowed(void) override;
		virtual void PopulateTeamDropdownForPlayer(HWND hwnd, int index) override;
};
