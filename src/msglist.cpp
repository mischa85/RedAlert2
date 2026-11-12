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

/* $Header: /CounterStrike/MSGLIST.CPP 2     3/04/97 2:52p Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MSGLIST.CPP                              *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 05/22/95                                 *
 *                                                                         *
 *                  Last Update : March 4, 1997 [JLB]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MessageListClass::MessageListClass -- constructor                     *
 *   MessageListClass::~MessageListClass -- destructor                     *
 *   MessageListClass::Init -- Inits message system, sets options          *
 *   MessageListClass::Add_Message -- displays the given message           *
 *   MessageListClass::Get_Message -- retrieves given message              *
 *   MessageListClass::Get_Label -- retrieves given text label             *
 *   MessageListClass::Concat_Message -- concats the given message         *
 *   MessageListClass::Add_Edit -- Adds editable string to message list    *
 *   MessageListClass::Remove_Edit -- removes the edit field               *
 *   MessageListClass::Get_Edit_Buf -- gets edit buffer                    *
 *   MessageListClass::Set_Edit_Color -- sets color of edit gizmo          *
 *   MessageListClass::Manage -- Manages multiplayer messages              *
 *   MessageListClass::Input -- Handles input for sending messages         *
 *   MessageListClass::Draw -- Draws the messages                          *
 *   MessageListClass::Num_Messages -- returns # messages in the list      *
 *   MessageListClass::Set_Width -- sets allowable width of messages       *
 *   MessageListClass::Trim_Message -- trims chars off start of message    *
 *   MessageListClass::Compute_Y -- recomputes y-coord for all messages    *
 *   MessageListClass::Reset -- Reset so no messages are visible.          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "always.h"

#include "msglist.h"
#include "session.h" // RA2: the scroll-text options
#include "bitfont.h"

#include "_keyboar.h"
#include "_rules.h"
#include "_surface.h"
#include "_timer.h"
#include "_xmouse.h"
#include "dbgprint.h"
#include "dialog.h"
#include "font.h"
#include "keyboard.h"
#include "rules.h"
#include "scheme.h"
#include "surface.h"
#include "txtlabel.h"
#include "vector.h"

#include <cstdio>


/**************************** Globals **************************************/


/************************************************************************** *
 * MessageListClass::MessageListClass -- constructor                        *
 *                                                                          *
 * INPUT:                                                                   *
 *    x,y         coord of upper-left of top message                        *
 *    max_msg      max messages allowed, including edit message             *
 *                                                                          *
 * OUTPUT:                                                                  *
 *    none.                                                                 *
 *                                                                          *
 * WARNINGS:                                                                *
 *    none.                                                                 *
 *                                                                          *
 * HISTORY:                                                                 *
 *   05/21/1995 BRR : Created.                                              *
 *=========================================================================*/
MessageListClass::MessageListClass(void)
{
	int i;

	//------------------------------------------------------------------------
	// Init all data members
	//------------------------------------------------------------------------
	MessageList = 0;
	MessageX = 0;
	MessageY = 0;
	MaxMessages = 0;
	MaxChars = 0;
	Height = 0;

	EnableOverflow = 0;
	IsEdit = 0;
	AdjustEdit = 0;
	EditX = 0;
	EditY = 0;
	EditLabel = 0;
	EditCurPos = 0;
	EditInitPos = 0;
	CursorChar = 0;
	OverflowStart = 0;
	OverflowEnd = 0;
	Width = 0;

	EditBuf[0] = 0;
	OverflowBuf[0] = 0;

	for (i = 0; i < MAX_NUM_MESSAGES; i++) {
		BufferAvail[i] = 1;
	}

}


/***************************************************************************
 * MessageListClass::~MessageListClass -- destructor                       *
 *                                                                         *
 * INPUT:                                                                  *
 *    x,y         coord of upper-left of top message                       *
 *    max_msg      max messages allowed, including edit message            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    none.                                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/21/1995 BRR : Created.                                             *
 *=========================================================================*/
MessageListClass::~MessageListClass(void)
{
	Init(0,0,0,0,0,0,0,0,0,0);
}


/***************************************************************************
 * MessageListClass::Init -- Inits message system, sets options            *
 *                                                                         *
 * INPUT:                                                                  *
 *    x,y            coord of upper-left of top message                    *
 *    max_msg         max messages allowed, NOT including edit message     *
 *    maxchars         max # characters allowed per message                *
 *      height         pixel height of a line of text                      *
 *      edit_x         x-coord of edit field; -1 = put at the top of the   *
 *                     other messages                                      *
 *      edit_y         y-coord of edit field; -1 = put at the top of the   *
 *                     other messages                                      *
 *      overflow_on      true = enable the overflow typing feature         *
 *      over_start      start index for overflow processing                *
 *      over_end         end index for overflow processing                 *
 *     width          pixel width of message buffer                        *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    none.                                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/21/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Init(int x, int y, int max_msg, int maxchars,
	int height, int edit_x, int edit_y, bool overflow_on, int over_start,
	int over_end, int width)
{
	TextLabelClass * txtlabel;
	int i;

	Width = width - 8;
	DebugString("MessageListClass::Init(Width: %d)\n", Width);

	//------------------------------------------------------------------------
	// Remove every entry in the list
	//------------------------------------------------------------------------
	txtlabel = MessageList;
	while (txtlabel) {
		MessageList = (TextLabelClass *)txtlabel->Remove();
		delete txtlabel;
		txtlabel = MessageList;
	}

	//------------------------------------------------------------------------
	// Mark all buffers as available
	//------------------------------------------------------------------------
	for (i = 0; i < MAX_NUM_MESSAGES; i++) {
		BufferAvail[i] = 1;
	}

	//------------------------------------------------------------------------
	// Remove the editable message
	//------------------------------------------------------------------------
	if (IsEdit) {
		delete EditLabel;
		EditLabel = 0;
	}

	//------------------------------------------------------------------------
	// Init variables
	//------------------------------------------------------------------------
	MessageList = 0;
	MessageX = x;
	MessageY = y;

	MaxMessages = max_msg;
	if (MaxMessages > MAX_NUM_MESSAGES)
		MaxMessages = MAX_NUM_MESSAGES;

	MaxChars = maxchars;
	if (MaxChars > MAX_MESSAGE_LENGTH)
		MaxChars = MAX_MESSAGE_LENGTH;

	Height = height;

	//------------------------------------------------------------------------
	// Init the edit field variables.  If edit_x or edit_y is -1, place the
	// edit field above the other messages; otherwise, place it at the desired
	// coords.
	//------------------------------------------------------------------------
	EnableOverflow = overflow_on;
	IsEdit = 0;
	if (edit_x == -1 || edit_y == -1) {
		AdjustEdit = 1;
		EditX = x;
		EditY = y;
	}
	else {
		AdjustEdit = 0;
		EditX = edit_x;
		EditY = edit_y;
	}
	EditLabel = 0;
	EditBuf[0] = 0;
	OverflowBuf[0] = 0;
	EditCurPos = 0;
	EditInitPos = 0;
	CursorChar = 0;

	//------------------------------------------------------------------------
	// Init the overflow processing indices
	//------------------------------------------------------------------------
	OverflowStart = over_start;
	OverflowEnd = over_end;
	if (OverflowEnd >= MaxChars) {
		OverflowEnd = MaxChars - 1;
	}
	if (OverflowStart >= OverflowEnd) {
		OverflowStart = OverflowEnd - 1;
	}

}


/***************************************************************************
 * MessageListClass::Add_Message -- displays the given message             *
 *                                                                         *
 * INPUT:                                                                  *
 *      name         name of sender, NULL = none                           *
 *      id            numerical ID for this message                        *
 *    txt         text to display                                          *
 *    color       color to draw text in                                    *
 *    style       style to use                                             *
 *    timeout     # of ticks the thing is supposed to last (-1 = forever)  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    ptr to new TextLabelClass object.                                    *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/05/1995 BRR : Created.                                             *
 *   10/16/1996 JLB : Audio feedback added.                                *
 *=========================================================================*/
TextLabelClass * MessageListClass::Add_Message(wchar_t const * name, int id, wchar_t const * txt, // RA2: YR's wide-text form
	int color, TextPrintType style, int timeout, bool singleplayer)
{
	BitFont * font = Font_From_TPF(style);
	if (!txt) return NULL;
	if (!font) return NULL;
	wchar_t message[MAX_MESSAGE_LENGTH + 50];
	message[0] = 0;
	if (name) {
		wcscpy(message, name);
		wcscat(message, L":");
	}
	int namelen = wcslen(message);
	int namewidth = font->String_Pixel_Width(message, 0);
	int textlen = wcslen(txt);
	int width = Width - namewidth - 8;
	if (width <= 0) return NULL;
	int print_this_pass = font->Chars_In_Width(txt, width, MAX_MESSAGE_LENGTH - 1, 1);
	if (print_this_pass < 0) return NULL;
	wcsncat(message, txt, print_this_pass);
	message[namelen + print_this_pass] = 0;
	TextLabelClass * txtlabel;
	int i;
	if (MaxMessages > 0 && Num_Messages() + 1 > MaxMessages) {
		txtlabel = MessageList;
		if (!txtlabel) return NULL;
		MessageList = (TextLabelClass *)txtlabel->Remove();
		for (i = 0; i < MAX_NUM_MESSAGES; i++) {
			if (txtlabel->Text == MessageBuffers[i]) BufferAvail[i] = 1;
		}
		delete txtlabel;
	}
	txtlabel = new TextLabelClass(message, MessageX, MessageY, color, (TextPrintType)(style | 0x8000));
	if (Session.Type == GAME_INTERNET) txtlabel->Animate = !singleplayer && Session.WOLScrollText;
	else if (Session.Type == GAME_IPX) txtlabel->Animate = !singleplayer && Session.LANScrollText;
	else txtlabel->Animate = !singleplayer;
	if (timeout == -1) txtlabel->UserData1 = 0;
	else txtlabel->UserData1 = TickCount + timeout;
	txtlabel->UserData2 = id;
	txtlabel->PixWidth = Width;
	int found = 0;
	for (i = 0; i < MAX_NUM_MESSAGES; i++) {
		if (BufferAvail[i]) {
			BufferAvail[i] = 0;
			memset(MessageBuffers[i], 0, MAX_MESSAGE_LENGTH + 50);
			wcscpy(MessageBuffers[i], message);
			txtlabel->Text = MessageBuffers[i];
			found = 1;
			break;
		}
	}
	if (!found) {
		delete txtlabel;
		return NULL;
	}
	if (!singleplayer) VocClass::PlayGlobal(Rule->IncomingMessage, 1.0f, 0x2000, NULL);
	if (MessageList) txtlabel->Add_Tail(*MessageList);
	else MessageList = txtlabel;
	Compute_Y();
	if (print_this_pass < textlen) {
		wchar_t const * msg = txt + print_this_pass;
		while (*msg && *msg < L' ') msg++;
		if (*msg) Add_Message(name, id, msg, color, style, timeout, true);
	}
	return txtlabel;
}


/***************************************************************************
 * MessageListClass::Get_Message -- retrieves given message                *
 *                                                                         *
 * INPUT:                                                                  *
 *      id         ID of message to get                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      ptr to message text, NULL if not found                             *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1995 BRR : Created.                                             *
 *=========================================================================*/
wchar_t * MessageListClass::Get_Message(int id) // RA2: Get_Message at 0x5B5160 matches only so
{
	TextLabelClass * gadg;

	if (MessageList) {
		gadg = MessageList;
		while (gadg) {
			if ((int)gadg->UserData2 == id) {
				return(gadg->Text);
			}
			gadg = (TextLabelClass *)gadg->Get_Next();
		}
	}

	return(NULL);
}


/***************************************************************************
 * MessageListClass::Get_Label -- retrieves given text label               *
 *                                                                         *
 * INPUT:                                                                  *
 *      id         ID of message to get                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      ptr to message text, NULL if not found                             *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1995 BRR : Created.                                             *
 *=========================================================================*/
TextLabelClass * MessageListClass::Get_Label(int id)
{
	TextLabelClass * gadg;

	if (MessageList) {
		gadg = MessageList;
		while (gadg) {
			if ((int)gadg->UserData2 == id) {
				return(gadg);
			}
			gadg = (TextLabelClass *)gadg->Get_Next();
		}
	}

	return(NULL);
}


/***************************************************************************
 * MessageListClass::Concat_Message -- concats the given message           *
 *                                                                         *
 * INPUT:                                                                  *
 *      name         name of sender; NULL = none                           *
 *      id            ID of message to concatenate to                      *
 *      txt         text to concatenate onto existing message              *
 *      timeout      new timeout for message                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = OK, 0 = error (id or name not found)                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      If the required message doesn't exist, this routine does nothing.  *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1995 BRR : Created.                                             *
 *=========================================================================*/
int MessageListClass::Concat_Message(wchar_t const * name, int id, wchar_t const * txt, int timeout) // RA2: YR's wide-text form
{
	int min_chars;
	int max_chars;
	wchar_t * msg;
	TextLabelClass * tlabel = NULL;
	int found;

	if (name == NULL || txt == NULL || !EnableOverflow) {
		return(0);
	}

	found = 0;
	if (MessageList) {
		tlabel = MessageList;
		while (tlabel) {
			if ((int)tlabel->UserData2 == id &&
				!memcmp(tlabel->Text, name, wcslen(name) * 2)) {
				found = 1;
				break;
			}
			tlabel = (TextLabelClass *)tlabel->Get_Next();
		}
	}
	if (!found) {
		return(0);
	}

	msg = tlabel->Text + wcslen(name) + 1;

	if ((wcslen(msg) + wcslen(txt)) < (unsigned)MaxChars) {
		wchar_t * concat_test = new wchar_t [MaxChars + 1];
		BitFont * font = Font_From_TPF(tlabel->Style);
		int name_width = font->String_Pixel_Width(tlabel->Text, 0) - font->String_Pixel_Width(msg, 0);
		int width;

		wcscpy(concat_test, msg);
		wcscat(concat_test, txt);
		width = font->String_Pixel_Width(concat_test, 0) + name_width;
		min_chars = 10;
		while (width >= Width) {
			max_chars = wcslen(msg);
			if (max_chars < min_chars) {
				max_chars = min_chars;
			}
			Trim_Message(NULL, msg, min_chars, max_chars, 0);
			wcscpy(concat_test, msg);
			wcscat(concat_test, txt);
			width = font->String_Pixel_Width(concat_test, 0) + name_width;
		}
		delete [] concat_test;
		wcscat(msg, txt);
	}
	else {
		min_chars = (wcslen(msg) + wcslen(txt)) - MaxChars;
		max_chars = wcslen(msg);
		if (max_chars < min_chars) {
			max_chars = min_chars;
		}
		Trim_Message(NULL, msg, min_chars, max_chars, 0);
		wcscat(msg, txt);
	}

	if (timeout == -1) {
		tlabel->UserData1 = 0;
	}
	else {
		tlabel->UserData1 = TickCount + timeout;
	}
	return(1);
}


/***********************************************************************************************
 * MessageListClass::Set_Edit_Focus -- Give the gadget system focus to the edit box            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/19/96 4:41PM ST : Created                                                             *
 *=============================================================================================*/
void MessageListClass::Set_Edit_Focus (void)
{
	if (IsEdit) EditLabel->Set_Focus();
}


/***********************************************************************************************
 * MessageListClass::Has_Edit_Focus -- Find out if the edit box has the input focus            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/19/96 4:41PM ST : Created                                                             *
 *=============================================================================================*/
bool MessageListClass::Has_Edit_Focus (void)
{
	if (IsEdit){
		return(EditLabel->Has_Focus());
	}else{
		return(false);
	}
}


/***************************************************************************
 * MessageListClass::Add_Edit -- Adds editable string to message list      *
 *                                                                         *
 * INPUT:                                                                  *
 *    color         color of edit message                                  *
 *    style         style of edit message                                  *
 *    to            string: who to send to; NULL = none                    *
 *      cursor      character to use as a cursor; 0 = none                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    ptr to new TextLabelClass                                            *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/22/1995 BRR : Created.                                             *
 *=========================================================================*/
TextLabelClass * MessageListClass::Add_Edit(int color, // RA2: YR's wide-text form
	TextPrintType style, wchar_t * to, char cursor, int width)
{
	int i;
	TextLabelClass * txtlabel;

	if (IsEdit) {
		EditLabel->Set_Focus();
		return(NULL);
	}

	if (AdjustEdit && ((Num_Messages() + 1) > MaxMessages)) {
		txtlabel = MessageList;
		MessageList = (TextLabelClass *)txtlabel->Remove();
		for (i = 0; i < MAX_NUM_MESSAGES; i++) {
			if (txtlabel->Text == MessageBuffers[i])
				BufferAvail[i] = 1;
		}
		delete txtlabel;
	}

	if (!to) {
		to = (wchar_t *)L"";
	}
	CursorChar = cursor;
	memset(EditBuf, 0, sizeof(EditBuf));
	wcscpy(EditBuf, to);
	OverflowBuf[0] = 0;
	EditCurPos = EditInitPos = wcslen(to);
	EditLabel = new TextLabelClass(EditBuf, EditX, EditY,
		color, (TextPrintType)(style | 0x8000));
	if (width > 0) {
		EditLabel->PixWidth = width;
	} else {
		EditLabel->PixWidth = Width;
	}
	if (EditLabel) {
		IsEdit = 1;
		EditLabel->Set_Focus();
	}
	else {
		IsEdit = 0;
	}
	if (AdjustEdit) {
		Compute_Y();
		MaxMessages--;
	}
	return(EditLabel);
}


/***************************************************************************
 * MessageListClass::Remove_Edit -- removes the edit field                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Remove_Edit(void)
{
	//------------------------------------------------------------------------
	// If the edit field is active, delete it
	//------------------------------------------------------------------------
	if (IsEdit) {
		IsEdit = 0;
		delete EditLabel;

		//.....................................................................
		// If the edit field appears over the message list, recompute the
		// y-value for all messages.  Adjust MaxMessages back up, since there
		// is now a new available slot.
		//.....................................................................
		if (AdjustEdit) {
			Compute_Y();
			MaxMessages++;
		}
	}

}


/***************************************************************************
 * MessageListClass::Get_Edit_Buf -- gets edit buffer                      *
 *                                                                         *
 * INPUT:                                                                  *
 *    none.                                                                *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    ptr to edit buffer, minus the "To:" header                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/21/1995 BRR : Created.                                             *
 *=========================================================================*/
wchar_t * MessageListClass::Get_Edit_Buf(void) // RA2: Get_Edit_Buf at 0x5B55E0 matches only so
{
	return(EditBuf + EditInitPos);
}


/***************************************************************************
 * MessageListClass::Set_Edit_Color -- sets color of edit gizmo            *
 *                                                                         *
 * INPUT:                                                                  *
 *      color      color to set edit label to                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/08/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Set_Edit_Color(int color)
{
	if (IsEdit) {
		EditLabel->Color = color;
	}
}


/***************************************************************************
 * MessageListClass::Manage -- Manages multiplayer messages                *
 *                                                                         *
 * If this routine returns TRUE, the caller should update the display.     *
 *                                                                         *
 * INPUT:                                                                  *
 *    none.                                                                *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    none.                                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *    0 = no change has occurred, 1 = changed                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int MessageListClass::Manage(void) // RA2: Manage at 0x5B5610 matches only so
{
	TextLabelClass * txtlabel;
	TextLabelClass * next;
	int changed = 0;
	int i;

	txtlabel = MessageList;
	while (txtlabel) {
		if (txtlabel->UserData1 != 0 && TickCount > (int)txtlabel->UserData1) {
			next = (TextLabelClass *)txtlabel->Get_Next();
			MessageList = (TextLabelClass *)txtlabel->Remove();
			for (i = 0; i < MAX_NUM_MESSAGES; i++) {
				if (txtlabel->Text == MessageBuffers[i]) {
					BufferAvail[i] = 1;
				}
			}
			delete txtlabel;
			changed = 1;
			txtlabel = next;
		}
		else {
			txtlabel = (TextLabelClass *)txtlabel->Get_Next();
		}
	}

	if (changed) {
		Compute_Y();
	}
	return(changed);
}


/***************************************************************************
 * MessageListClass::Input -- Handles input for sending messages           *
 *                                                                         *
 * INPUT:                                                                  *
 *    input         key value to process                                   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    1 = caller should redraw the message list (no need to complete       *
 *        refresh, though)                                                 *
 *    2 = caller should completely refresh the display.                    *
 *    3 = caller should send the edit message.                             *
 *        (sets 'input' to 0 if it processes it.)                          *
 *      4 = caller should send the Overflow buffer                         *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int MessageListClass::Input(KeyNumType &input, wchar_t definput) // RA2: YR's wide-text form
{
	int retcode = 0;
	int ascii;
	int numchars;
	BitFont * font;
	int width;

	if (definput == 0) {
		if (input == KN_NONE) {
			return(0);
		}

		if ((input & (~KN_RLSE_BIT)) == KN_LMOUSE ||
			(input & (~KN_RLSE_BIT)) == KN_RMOUSE) {
			if ((input & 2) && IsEdit) {
				Remove_Edit();
				return(2);
			}
			return(0);
		}
	}

	if (IsEdit) {
		if (definput == 0) {
			ascii = Keyboard->To_ASCII(input) & 0x00ff;

			if ((input & WWKEY_VK_BIT) && ascii >= '0' && ascii <= '9') {
				input = (KeyNumType)(input & ~WWKEY_VK_BIT);
			} else {
				if ((!(input & WWKEY_VK_BIT) && !(input & KN_BUTTON) && ascii >= ' ')
					|| (input & 0xff) == (KN_RETURN & 0xff)
					|| (input & 0xff) == (KN_BACKSPACE & 0xff)
					|| (input & 0xff) == (KN_ESC & 0xff)
					|| (char)ascii == 8) {
				} else {
					input = KN_NONE;
					return(0);
				}
			}
		} else {
			ascii = definput & 0xffff;
		}

		switch (ascii) {
			case KA_ESC & 0xff:
				Remove_Edit();
				retcode = 2;
				input = KN_NONE;
				break;

			case KA_RETURN & 0xff:
				if (EditCurPos < EditInitPos) {
					EditCurPos = EditInitPos;
				}
				while (EditBuf[EditCurPos] != 0 && (EditCurPos - EditInitPos) < (MaxChars - 1)) {
					EditCurPos++;
				}
				if (EditCurPos == EditInitPos) {
					Remove_Edit();
					retcode = 2;
					input = KN_NONE;
					break;
				}
				if ((EditCurPos - EditInitPos) < (MaxChars - 1)) {
					EditBuf[EditCurPos] = ' ';
					EditCurPos++;
					EditBuf[EditCurPos] = 0;
				}
				Remove_Edit();
				retcode = 3;
				input = KN_NONE;
				break;

			case KA_BACKSPACE & 0xff:
				if (input != 8) {
					if (EditCurPos > EditInitPos) {
						EditCurPos--;
						EditBuf[EditCurPos] = 0;
						retcode = 2;
					}
				}
				input = KN_NONE;
				EditLabel->Set_Focus();
				break;

			default:
				EditLabel->Set_Focus();
				if (ascii > 0x1f && ascii < 0x10000) {
					while (EditBuf[EditCurPos] != 0 && (EditCurPos - EditInitPos) < (MaxChars - 1)) {
						EditCurPos++;
					}
					if ((EditCurPos - EditInitPos) < (MaxChars - 1)) {
						EditBuf[EditCurPos] = (wchar_t)ascii;
						EditCurPos++;
						EditBuf[EditCurPos] = 0;
						retcode = 1;
						font = Font_From_TPF(EditLabel->Style);
						width = font->String_Pixel_Width(EditBuf, 0);
						if (width < Width - 10) {
							goto key_done;
						}
						EditCurPos--;
						EditBuf[EditCurPos] = 0;
					}
					if (EnableOverflow) {
						numchars = Trim_Message(OverflowBuf, EditBuf + EditInitPos,
							OverflowStart, OverflowEnd, 1);
						EditCurPos -= numchars;
						EditBuf[EditCurPos] = (wchar_t)ascii;
						EditCurPos++;
						EditBuf[EditCurPos] = 0;
						retcode = 4;
					} else {
						retcode = 0;
					}
				}
key_done:
				input = KN_NONE;
				break;
		}
	}
	return(retcode);
}


/***************************************************************************
 * MessageListClass::Draw -- draws messages                                *
 *                                                                         *
 * INPUT:                                                                  *
 *    none                                                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    none.                                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/22/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Draw(void)
{
	wchar_t txt[2] = {0,0}; // RA2: Draw at 0x5B5B30 matches only so

	if (IsEdit) {
		Hide_Mouse();
		EditLabel->Draw_Me(true);
		BitFont * font = Font_From_TPF(EditLabel->Style); // RA2: as above
		int textwidth = font->String_Pixel_Width(EditLabel->Text, 0); // RA2: as above
		if (CursorChar && (EditCurPos - EditInitPos) < (MaxChars - 1) && EditLabel->Has_Focus()) {
			txt[0] = CursorChar;
			Point2D pt(EditLabel->X + textwidth, EditLabel->Y); // RA2: as above
			Fancy_Text_Print(txt, *LogicalSurface, LogicalSurface->Get_Rect(), pt, // RA2: as above
				ColorSchemes[EditLabel->Color], 0, EditLabel->Style); // RA2: as above
		}
		Show_Mouse();
	}

	if (MessageList) { // RA2: Draw at 0x5B5B30 has no Show_Mouse();
		MessageList->Draw_All();
	}
}


/***************************************************************************
 * MessageListClass::Num_Messages -- returns # messages in the list        *
 *                                                                         *
 * INPUT:                                                                  *
 *    none.                                                                *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    # of messages, including the edit field if it's above the messages   *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/26/1995 BRR : Created.                                             *
 *=========================================================================*/
int MessageListClass::Num_Messages(void)
{
	GadgetClass * gadg;
	int num;

	num = 0;

	if (MessageList) {
		gadg = MessageList;
		while (gadg) {
			num++;
			gadg = (GadgetClass *)gadg->Get_Next();
		}
	}

	if (IsEdit && AdjustEdit) {
		num++;
	}

	return(num);
}


/***************************************************************************
 * MessageListClass::Set_Width -- sets allowable width of messages         *
 *                                                                         *
 * INPUT:                                                                  *
 *    width      pixel width                                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *    none.                                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *    none.                                                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/26/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Set_Width(int width)
{
	GadgetClass * gadg;

	width = width - 8;
	Width = width;
	DebugString("MessageListClass::Set_Width(%d)\n", width);

	if (MessageList) {
		gadg = MessageList;
		while (gadg) {
			((TextLabelClass *)gadg)->PixWidth = width;
			gadg = (GadgetClass *)gadg->Get_Next();
		}
	}
	if (IsEdit) {
		EditLabel->PixWidth = width;
	}
}


/***************************************************************************
 * MessageListClass::Trim_Message -- trims chars off start of message      *
 *                                                                         *
 * INPUT:                                                                  *
 *      dest            buffer to store removed characters in; NULL = none *
 *      src            text buffer to trim                                 *
 *      min_chars      min # chars that must be trimmed off                *
 *      max_chars      max # chars allowed to trim                         *
 *      scandir         0 = left-to-right, 1 = right-to-left               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      # characters removed                                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1995 BRR : Created.                                             *
 *=========================================================================*/
int MessageListClass::Trim_Message(wchar_t * dest, wchar_t * src, int min_chars, // RA2: YR's wide-text form
	int max_chars, int scandir)
{
	int i;
	int len;
	int found;

	if (min_chars <= 0) {
		return(0);
	}

	len = wcslen(src);
	if (max_chars > len) {
		max_chars = len;
	}

	found = 0;
	if (scandir == 0) {
		for (i = min_chars; i <= max_chars; i++) {
			if (src[i - 1] != 0 && (unsigned)src[i - 1] <= _CONTROL) {
				found = 1;
				break;
			}
		}
	}
	else {
		for (i = max_chars; i >= min_chars; i--) {
			if (src[i - 1] != 0 && (unsigned)src[i - 1] <= _CONTROL) {
				found = 1;
				break;
			}
		}
	}
	if (!found) {
		i = min_chars;
	}

	if (dest) {
		memcpy (dest, src, i * sizeof(dest[0]));
		dest[i] = 0;
	}

	memmove (src, src + i, (len - i + 1) * sizeof(src[0]));

	return(i);
}


/***************************************************************************
 * MessageListClass::Compute_Y -- recomputes y-coord for all messages      *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/07/1995 BRR : Created.                                             *
 *=========================================================================*/
void MessageListClass::Compute_Y(void)
{
	GadgetClass * gadg;
	int y;

	//------------------------------------------------------------------------
	// If the editable message is attached to the message list, 'AdjustEdit'
	// will be set; so, adjust all y-values downward one line.  Otherwise,
	// the editable message has its own screen coordinates.
	//------------------------------------------------------------------------
	if (IsEdit && AdjustEdit) {
		y = MessageY + Height;
	}
	else {
		y = MessageY;
	}
	if (MessageList) {
		gadg = MessageList;
		while (gadg) {
			gadg->Y = y;
			gadg = (GadgetClass *)gadg->Get_Next();
			y += Height;
		}
	}

}



/*************************** end of msglist.cpp ****************************/
