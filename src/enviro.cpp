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

#include "enviro.h"

#include "globals.h"
#include "house.h"
#include "scenario.h"

EnvironmentClass Environment;


/// <summary>
/// Creates the carry over environment in its empty state.
/// This routine ensures that a scenario played without a predecessor inherits nothing
/// from a previous mission.
/// </summary>
EnvironmentClass::EnvironmentClass(void):
	CarryOverMoney(0),
	MissionTimer(0),
	Difficulty(DIFF_NORMAL),
	Stage(0)
{
	for (int i = 0; i < 50; i++) {
		Globals[i] = false;
	}
}


/// <summary>
/// Destroys the carry over environment object.
/// </summary>
EnvironmentClass::~EnvironmentClass(void)
{

}


/// <summary>
/// Captures the state that carries over into the next mission.
/// This routine is called as a campaign mission is won, before the next scenario is
/// started. The global flags, spare money, mission timer, difficulty and stage are
/// remembered here so that Restore can hand them to the new mission.
/// </summary>
void EnvironmentClass::Store(void)
{
	for (int i = 0; i < 50; i++) {
		Globals[i] = Scen->GlobalFlags[i].Value;
	}

	CarryOverMoney = PlayerPtr->Available_Money();
	MissionTimer = Scen->MissionTimer;
	Difficulty = PlayerPtr->Difficulty;
	Stage = Scen->Stage;
}


/// <summary>
/// Applies the remembered carry over state to the new mission.
/// This routine is called once the next scenario in the campaign has been started. It
/// restores the global flags, grants the player whatever share of the previous mission's
/// money the scenario allows, re-applies the difficulty handicap and resumes an
/// inherited mission timer.
/// </summary>
void EnvironmentClass::Restore(void)
{
	for (int i = 0; i < 50; i++) {
		Scen->Set_Global_To(i, Globals[i]);
	}

	int cap = Scen->CarryOverCap;
	double money = (double)CarryOverMoney * Scen->CarryOverPercent;

	if (cap != -1) {
		money = MIN(money, cap);
	}

	PlayerPtr->Refund_Money((int)money);
	PlayerPtr->Control.InitialCredits += (int)money;
	PlayerPtr->Assign_Handicap(Difficulty);

	if (Scen->IsInheritTimer) {
		if (MissionTimer > 0) {
			Scen->MissionTimer = MissionTimer;
			Scen->MissionTimer.Start();
		}
	}

	Scen->Stage = Stage;
}


/// <summary>
/// Reads the carry over environment back in from a save game.
/// This routine is the counterpart to Save and is called while the save game is being
/// restored, before the scenario itself is brought back.
/// </summary>
/// <returns>Returns with the result reported by the stream read.</returns>
HRESULT EnvironmentClass::Load(IStream * stream)
{
	return(stream->Read(this, sizeof(*this), NULL));
}


/// <summary>
/// Writes the carry over environment out to a save game.
/// This routine is called by the save game code and simply dumps the object wholesale,
/// since the carry over data is plain values with no pointers in it.
/// </summary>
/// <returns>Returns with the result reported by the stream write.</returns>
HRESULT EnvironmentClass::Save(IStream * stream)
{
	return(stream->Write(this, sizeof(*this), NULL));
}
