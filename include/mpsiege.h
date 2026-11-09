// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#pragma once

#include "multiplayer.h"

class MultiplayerObserverTeam : public MultiplayerTeam
{
	public:
		MultiplayerObserverTeam(void);
		virtual bool IsTeamIncluded(int index) override;
};

class MultiplayerSiegeDefenderTeam : public MultiplayerTeam
{
	public:
		MultiplayerSiegeDefenderTeam(void);
		virtual ~MultiplayerSiegeDefenderTeam(void);
};

class MultiplayerSiegeAttackerTeam : public MultiplayerTeam
{
	public:
		MultiplayerSiegeAttackerTeam(void);
		virtual ~MultiplayerSiegeAttackerTeam(void);
};

class MultiplayerSiege : public MultiplayerGameMode
{
	public:
		MultiplayerSiege(NoInitClass const & x) : MultiplayerGameMode(x) {}
		MultiplayerSiege(WideWstring const & title, WideWstring const & tooltip, Wstring const & inifile, Wstring const & mapfilter, bool aiallowed, int modeindex);
		virtual ~MultiplayerSiege(void);
		virtual bool AssignTeams(void) override;
		virtual bool CheckStartConditions(WideWstring * message) override;
		virtual bool AssignTeamsAtStart(void) override;
		virtual bool IsAIAllowed(void) override;
		virtual bool UnfixAlliances(void) override;
		virtual bool StartingPositionsToHouseBaseCells(char unused) override;
		virtual void CreateMPTeams(DynamicVectorClass<MultiplayerTeam *> * teams) override;
		virtual bool SpawnBaseUnits(HouseClass * house, unsigned unused) override;
};
