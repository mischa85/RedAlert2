// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include <windows.h>
#undef LoadString
#include "_keyboar.h"
#include "_map.h"
#include "ccfile.h"
#include "ccini.h"
#include "globals.h"
#include "keyboard.h"
#include "ownrdraw.h"
#include "scenario.h"
#include "stringtable.h"

char const * MissionININame = "MISSION.INI"; // RA2
char const * BriefingKey = "Briefing"; // RA2
static wchar_t const * RestateText; // RA2: the briefing text the dialog shows

int CALLBACK Restate_Mission_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void __fastcall Restate_Mission(ScenarioClass * scen) // RA2: shows the mission briefing in a dialog
{
	bool active = ScenarioActive;
	ScenarioActive = false;

	CCFileClass file;
	CCINIClass ini;
	file.Close();
	file.Set_Name(MissionININame);
	if (file.Is_Available(0)) {
		ini.Load(file, false, false);
		char const * section = scen->ScenarioName;
		bool found;
		if (BriefingKey == NULL) {
			found = ini.Is_Present(section);
		} else {
			found = ini.Is_Present(section, BriefingKey);
		}
		wchar_t const * text;
		if (found) {
			char buffer[256];
			ini.Get_String(section, BriefingKey, "", buffer, sizeof(buffer));
			text = StringTable::LoadString(buffer, NULL, "D:\\Ra2\\Restate.cpp", 0x87);
		} else {
			text = StringTable::LoadString("Brief:Error", NULL, "D:\\Ra2\\Restate.cpp", 0x8a);
		}
		RestateText = text;

		int result;
		HWND dialog = OwnerDraw::Begin_Dialog(0x10c, Restate_Mission_Dialog_Proc, 0);
		IgnoreInput = true;
		Keyboard->WWKeyboardClass::Clear();
		if (dialog != NULL) {
			SetWindowLongA(dialog, DWL_USER, (LONG)&result);
			OwnerDraw::Display_Dialog(dialog);
			result = 0;
			while (true) {
				if (OwnerDraw::Dialog_Message_Handler()) {
					result = 0x423;
					break;
				}
				if (result != 0) {
					break;
				}
			}
			OwnerDraw::End_Dialog(dialog);
		}
		Keyboard->WWKeyboardClass::Clear();
		IgnoreInput = Scen->IsInputLocked;
		Map.Flag_To_Redraw(GS_REDRAW_ALL);
	}
	ScenarioActive = active;
}

int CALLBACK Restate_Mission_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc != 0) {
		return rc;
	}
	switch (message) {
		case WM_COMMAND:
			if (GetDlgCtrlID((HWND)lparam) == 0x423 && HIWORD(wparam) == 0) {
				*(int *)GetWindowLong(window, DWL_USER) = 0x423;
			}
			break;
		case 0x497:
			{
				HWND item = GetDlgItem(window, 0x6e7);
				SendMessage(item, 0x4b2, 0, (LPARAM)RestateText);
			}
			break;
	}
	return 0;
}
