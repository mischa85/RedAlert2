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

#include "session.h"

class SerialEdit
{
		friend BOOL CALLBACK Com_Settings_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
	public:
		SerialEdit(void) {};
		SerialEdit(SerialSettingsType *settings);
		bool Dialog(SerialSettingsType *settings);

	private:
		static __forceinline BOOL Handle_Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
		bool Save(void);
		void Init(SerialSettingsType *settings);
		void Build_Port_Listbox(void);
		void Build_Baud_Rate_Listbox(void);
		void Build_Call_Wait_Listbox(void);
		void Build_Init_String_Listbox(void);
		void Set_Dial_Method(void);
		void Set_String(int id);
		void Set_Port(void);
		void Set_Init_String(void);
		void Set_Custom_Port(int id, int port, char *str);
		void Add_Init_String(void);
		void Delete_Init_String(void);

	private:
		/*
		 * This is the window handle of the settings dialog while it is up.
		 */
		HWND _Dialog;

		/*
		 * This is the button the player dismissed the dialog with, IDOK or IDCANCEL. It
		 * stays -1 for as long as the dialog is up, which is what the message pump waits
		 * upon.
		 */
		int RC;

		/*
		 * This is a scratch buffer that the text of a list box or combo box entry is
		 * fetched into before the setting is picked out of it.
		 */
		char TempBuf[128];

		/*
		 * These are the settings being edited. The caller's settings are copied in as the
		 * dialog opens and are only copied back out if the player accepts, so that
		 * cancelling leaves them as they were.
		 */
		SerialSettingsType TempSettings;

		/*
		 * These are the modems found in the registry -- how many there were, and the name
		 * of each. Only the first ten are taken, so anyone with more attached is simply
		 * out of luck.
		 */
		int ModemsFound;
		char ModemNames[10][MODEM_NAME_MAX];
};
