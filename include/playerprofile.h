// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
// Changed to RA2 1.006's layout by RedAlert2 contributors, 2026.
#pragma once

#include <windows.h>
#include "refcount.h"
#include "widewstring.h"
#include "wstring.h"


struct Ladder;

class PlayerProfile : public ReferenceCounted
{
	public:
		PlayerProfile(Wstring const & name, int locale, bool verified, int status, int country, int color, int rank, int points, int wins, int losses, int disconnects); // RA2
		virtual ~PlayerProfile(void);
		void SetName(Wstring const * name);
		void SetLocale(int locale, int state);
		void SetCountry(int country);
		void SetMatchByRes(bool value);
		void SetUpdateStamp(void);
		void SetField20(int value);
		void UpdateStats(Ladder const * record);
		bool StatsDiffer(Ladder const * record) const;
		void AddField2C(int delta);
		void AddField30(int delta);
		void AddField34(int delta);
		void Save(Wstring const * name);
		void FUN_0077A9F0(HWND window, Wstring const * name, bool flag);

		bool IsDirty; // RA2: +0x08, set by every change; Save writes only then and clears it
		Wstring Name; // RA2: from the Server key of wolinfo.ini
		int Locale; // RA2: +0x10
		bool IsLocaleVerified; // RA2: +0x14
		int LocaleStatus; // RA2: +0x18, 1 when verified, 3 when not; a lower status does not replace the locale
		int Country; // RA2: +0x1C
		int Color; // RA2: +0x20
		int Rank; // RA2: +0x24
		int Points; // RA2: +0x28
		int Wins; // RA2: +0x2C
		int Losses; // RA2: +0x30
		int Disconnects; // RA2: +0x34
};
