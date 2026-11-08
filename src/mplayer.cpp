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

/* $Header: /CounterStrike/MPLAYER.CPP 3     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MPLAYER.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Bill Randolph                                                *
 *                                                                                             *
 *                   Start Date : April 14, 1995                                               *
 *                                                                                             *
 *                  Last Update : November 30, 1995 [BRR]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Clear_Listbox -- clears the given list box                                                *
 *   Clear_Vector -- clears the given NodeNameType vector                                      *
 *   Computer_Message -- "sends" a message from the computer                                   *
 *   Garble_Message -- "garbles" a message                                                     *
 *   Surrender_Dialog -- Prompts user for surrendering                                         *
 *   Abort_Dialog -- Prompts user for confirmation on aborting the mission                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "mplayer.h"

#include "_surface.h"
#include "addon.h"
#include "init.h"
#include "msgbox.h"
#include "nulldlg.h"
#include "ownrdraw.h"
#include "session.h"
#include "worlddom.h"
#include "stringtable.h"

/*
**	RA2: the original path of this file, passed with each string table lookup (see stringtable.h).
*/
#define RA2_SOURCE "D:\\Ra2\\MPLayer.CPP"

class ListClass;

BOOL CALLBACK Select_MPlayer_Game_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

/// <summary>
/// Handles the messages for the multiplayer game type dialog.
/// This routine gives the ownerdraw system first crack at the message and hands the
/// control the player pressed back to the caller.
/// </summary>
/// <returns>Returns with the result of the ownerdraw handler, or false when the message was
/// left unhandled.</returns>
BOOL CALLBACK Select_MPlayer_Game_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	int rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);

	/*
	**	RA2: there is no world domination button to disable.
	*/
	if (rc == 0) {
		if (message == WM_COMMAND) {
			*(int *)GetWindowLong(window, DWL_USER) = LOWORD(wparam);
		}
		return(false);
	}
	return(rc);
}


/***************************************************************************
 * Surrender_Dialog -- Prompts user for surrendering                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = user cancels, 1 = user wants to surrender.                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Surrender_Dialog(wchar_t const * text)
{
	return(WWMessageBox().Process(text, StringTable::LoadString("TXT_OK", NULL, RA2_SOURCE, 108), StringTable::LoadString("TXT_CANCEL", NULL, RA2_SOURCE, 108), NULL) == 0); // RA2: wide text
}


/***************************************************************************
 * Clear_Vector -- clears the given NodeNameType vector                    *
 *                                                                         *
 * INPUT:                                                                  *
 *      vector      ptr to vector to clear                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Clear_Vector(DynamicVectorClass <NodeNameType *> * vector)
{
	int i;

	//------------------------------------------------------------------------
	// Clear the 'Players' Vector
	//------------------------------------------------------------------------
	for (i = 0; i < vector->Count(); i++) {
		delete (*vector)[i];
	}
	vector->Clear();

}	// end of Clear_Vector


/************************** end of mplayer.cpp *****************************/
