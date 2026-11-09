// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "msgbox.h"
#include "_xmouse.h"
#include "globals.h"
#include "init.h"
#include "ownrdraw.h"
#include "winfix.h"
#include <windows.h>
#undef LoadString

class Surface;
void __fastcall Darken_Surface(Surface * surface, bool update); // RA2: dims the screen behind the dialog

BOOL CALLBACK Message_Box_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void __fastcall Message_Box_On_WM_COMMAND(HWND window, int id, int control, int notify_code);

int WWMessageBox::Process(wchar_t const * msg, wchar_t const * b1txt, wchar_t const * b2txt, wchar_t const * b3txt, bool preserve)
{
	int retval = -1;
	int numbuttons = 0;

	int mousestate = MouseCursor->Get_Mouse_State();

	bool hasmsg = (msg != NULL && msg[0] != L'\0');
	bool has1 = (b1txt != NULL && b1txt[0] != L'\0');
	bool has2 = (b2txt != NULL && b2txt[0] != L'\0');
	bool has3 = (b3txt != NULL && b3txt[0] != L'\0');

	Darken_Surface(NULL, true);

	int id = 0xCE;
	if (has3) {
		id = 0x121;
	} else if (has2) {
		id = 0x120;
	}

	HWND dialog = OwnerDraw::Begin_Dialog(id, Message_Box_Proc, 0);
	if (dialog != NULL) {
		SetWindowLong(dialog, DWL_USER, (LONG)&retval);

		if (hasmsg) {
			HWND item = GetDlgItem(dialog, 0x5B0);
			SendMessage(item, 0x4B2, 0, (LPARAM)msg);
		}
		if (has1) {
			HWND item = GetDlgItem(dialog, 0x5AE);
			SendMessage(item, 0x4B2, 0, (LPARAM)b1txt);
			numbuttons = 1;
		}
		if (has2) {
			HWND item = GetDlgItem(dialog, 2);
			SendMessage(item, 0x4B2, 0, (LPARAM)b2txt);
			numbuttons = 2;
		}
		if (has3) {
			HWND item = GetDlgItem(dialog, 0x5AF);
			SendMessage(item, 0x4B2, 0, (LPARAM)b3txt);
			numbuttons = 3;
		}

		OwnerDraw::Display_Dialog(dialog);

		if (numbuttons > 0) {
			while (MouseCursor->Get_Mouse_State()) {
				MouseCursor->Show_Mouse();
			}
			while (retval < 0) {
				if (OwnerDraw::Dialog_Message_Handler()) {
					break;
				}
				if (!GameActive) {
					Title_Screen_Restore(false);
				}
			}
		} else {
			retval = 0;
		}

		OwnerDraw::End_Dialog(dialog);
	}

	while (mousestate < MouseCursor->Get_Mouse_State()) {
		MouseCursor->Hide_Mouse();
	}

	return retval;
}

BOOL CALLBACK Message_Box_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	BOOL rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);

	if (rc == FALSE) {
		switch (message) {
			case WM_COMMAND:
				Message_Box_On_WM_COMMAND(window, LOWORD(wparam), 0, HIWORD(wparam));
				rc = FALSE;
				break;

			case WM_MOVING:
				rc = On_WM_MOVING(window, wparam, lparam);
				break;

			default:
				rc = FALSE;
				break;
		}
	}

	return rc;
}

void __fastcall Message_Box_On_WM_COMMAND(HWND window, int id, int control, int notify_code)
{
	int * retval = (int *)GetWindowLong(window, DWL_USER);

	switch (id) {
		case 1:
		case 2:
			if (notify_code == 0) {
				*retval = 1;
			}
			break;

		case 0x5AE:
			if (notify_code == 0) {
				*retval = 0;
			}
			break;

		case 0x5AF:
			if (notify_code == 0) {
				*retval = 2;
			}
			break;
	}
}
