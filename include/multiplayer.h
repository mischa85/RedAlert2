// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
// Changed to RA2 1.006's layout by RedAlert2 contributors, 2026.
#pragma once

#include "always.h"
#include "vector.h"
#include "widewstring.h"
#include "wstring.h"
#include "coord.h"

#include <windows.h>

class CCINIClass;
class NoInitClass;
class HouseClass;
class MultiMission;

class MultiplayerTeam
{
	public:
		MultiplayerTeam(WideWstring const & title, int index);
		virtual ~MultiplayerTeam(void);
		virtual bool IsTeamIncluded(int index);
		virtual bool SetPlayerTeam(int player);
		int CountMembers(void);
		int AddToList(HWND list);

		WideWstring Title;
		int Index;
};

class MultiplayerGameMode
{
	public:
		class InitializerBase
		{
			public:
				InitializerBase(void);
				virtual ~InitializerBase(void);
				virtual MultiplayerGameMode * Allocate(WideWstring const & title, WideWstring const & tooltip, Wstring const & inifile, Wstring const & mapfilter, bool aiallowed, int modeindex) = 0;
		};

		static MultiplayerGameMode * __fastcall Get(int index);
		CCINIClass * GetINI(void);
		bool IsMapAllowed(MultiMission const * map);
		MultiplayerTeam * GetTeam(int index);
		void DeleteTeams(void);
		bool GenerateUnits(void); // RA2: 0x005B7CE0, gives every house its base unit and starting units

		MultiplayerGameMode(NoInitClass const &) {}
		MultiplayerGameMode(WideWstring const & title, WideWstring const & tooltip, Wstring const & inifile, Wstring const & mapfilter, bool aiallowed, int modeindex);
		virtual ~MultiplayerGameMode(void);
		virtual bool vt_entry_04(void) { return(false); }
		virtual bool vt_entry_08(void) { return(false); }
		virtual bool vt_entry_0C(unsigned a) { return(true); }
		virtual bool AssignTeams(void);
		virtual bool CheckStartConditions(WideWstring * message);
		virtual bool AssignTeamsAtStart(void);
		virtual bool IsCountryAllowed(int country);
		virtual void vt_entry_20(void) {}
		virtual void vt_entry_24(void) {}
		virtual int DefaultCountry(void);
		virtual bool IsAIAllowed(void);
		virtual bool IsSkirmishAllowed(void); // RA2: Fill_Game_Mode_List lists only the modes returning true in a skirmish; FreeForAll does
		virtual int FirstValidMapIndex(void);
		virtual void PopulateTeamDropdown(HWND hwnd, DynamicVectorClass<MultiplayerTeam *> * teams, MultiplayerTeam * team);
		virtual void DrawTeamDropdown(HWND hwnd, DynamicVectorClass<MultiMission const *> * maps, MultiMission const * map);
		virtual void PopulateTeamDropdownForPlayer(HWND hwnd, int index);
		virtual bool vt_entry_44(int a, DynamicVectorClass<int> picks, int b) { return(false); }
		virtual bool vt_entry_48(int a, DynamicVectorClass<int> picks, char const * map, char const * name, int b) { return(false); }
		virtual bool vt_entry_4C(unsigned a, unsigned b, unsigned c) { return(true); }
		virtual bool vt_entry_50(void);
		virtual bool vt_entry_54(void);
		virtual bool vt_entry_58(void) { return(true); }
		virtual int RandomHumanCountryIndex(void);
		virtual int RandomAICountryIndex(void);
		virtual void vt_entry_64(unsigned a, unsigned b) {}
		virtual bool UnfixAlliances(void);
		virtual bool StartingPositionsToHouseBaseCells(char unused);
		virtual bool vt_entry_70(void);
		virtual bool OnGameStart(void);
		virtual void vt_entry_78(void) {}
		virtual void vt_entry_7C(void) {}
		virtual int vt_entry_80(void) { return(-1); }
		virtual void vt_entry_84(void) {}
		virtual void vt_entry_88(unsigned a) {}
		virtual void vt_entry_8C(unsigned a) {}
		virtual bool vt_entry_90(void) { return(false); }
		virtual void vt_entry_94(void) {}
		virtual void vt_entry_98(void) {}
		virtual bool vt_entry_9C(DynamicVectorClass<int> list, int a, int b) { return(false); }
		virtual bool vt_entry_A0(void) { return(true); }
		virtual void CreateMPTeams(DynamicVectorClass<MultiplayerTeam *> * teams);
		virtual Cell * AssignStartingPositionsToHouse(Cell * result, int house, DynamicVectorClass<Cell> * coords, unsigned char * satisfied);
		virtual bool SpawnBaseUnits(HouseClass * house, unsigned unused);
		virtual bool GenerateStartingUnits(HouseClass * house, int & amount);

		bool unknown_4;
		DynamicVectorClass<MultiplayerTeam *> MPTeams;
		WideWstring UIName;
		WideWstring CSFTooltip;
		int MPModeIndex;
		Wstring INIFilename;
		Wstring MapFilter;
		bool AIAllowed;
		CCINIClass * INI;
		int wolTourney; // RA2: +0x3C, an int read from WonlineTournamentAllowed (default 1); no AlliesAllowed, wolClanTourney or MustAlly
};
