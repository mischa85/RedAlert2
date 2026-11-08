/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/RADIO.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RADIO.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : June 5, 1996 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RadioClass::Debug_Dump -- Displays the current status of the radio to the mono monitor.   *
 *   RadioClass::Limbo -- When limboing a unit will always break radio contact.                *
 *   RadioClass::Receive_Message -- Handles receipt of a radio message.                        *
 *   RadioClass::Transmit_Message -- Transmit message from one object to another.              *
 *   RadioClass::Transmit_Message -- Transmits a message to the object specified.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "radio.h"

#include "_rtti.h"
#include "house.h"
#include "mono.h"
#include "swizzle.h"
#include "techno.h"

#include <new.h>


/*
**	RA2: the index of the object among the radio links, or -1.
*/
static inline int Index_Of(VectorClass<TechnoClass *> const & links, TechnoClass * link)
{
	for (int i = 0; i < links.Length(); i++) {
		if (links[i] == link) return i;
	}
	return -1;
}

#ifdef _DEBUG
/*
**	These are the text representations of the radio messages that can be transmitted.
*/
char const * RadioClass::Messages[RADIO_COUNT] = {
	"static (no message)",
	"Roger.",
	"Come in.",
	"Over and out.",
	"Requesting transport.",
	"Attach to transport.",
	"I've got a delivery for you.",
	"I'm performing load/unload maneuver. Be careful.",
	"I'm clear.",
	"You are clear to unload. Driving away now.",
	"Am unable to comply.",
	"I'm starting construction now... act busy.",
	"I've finished construction. You are free.",
	"We bumped, redraw yourself please.",
	"I'm trying to load up now.",
	"May I become a passenger?",
	"Are you ready to receive shipment?",
	"Are you trying to become a passenger?",
	"Move to location X.",
	"Do you need to move?",
	"All right already. Now what?",
	"I'm a passenger now.",
	"Backup into refinery now.",
	"Run away!",
	"Tether established.",
	"Tether broken.",
	"Repair one step.",
	"Are you prepared to fight?",
	"Attack this target please.",
	"Reload one step.",
	"Circumstances prevent success.",
	"All done with the request.",
	"Do you need service depot work?",
	"Are you sitting on service depot?"
};
#endif


/// <summary>
/// Creates a radio set that is not in contact with anyone.
/// The message history is cleared as well, so the first message this object receives will
/// always be treated as a new one rather than a repeat.
/// </summary>
RadioClass::RadioClass(void) :
	BASECLASS(),
	RadioLinks(1)
{
	RadioLinks[0] = NULL;
	for (int i = 0; i < 3; i++) {
		Old[i] = RADIO_STATIC;
	}
}


/// <summary>
/// Constructs a radio set for loading from a save game; nothing is initialized.
/// </summary>
RadioClass::RadioClass(NoInitClass const & x) :
	BASECLASS(x),
	RadioLinks(x)
{
}


#ifdef _DEBUG
/***********************************************************************************************
 * RadioClass::Debug_Dump -- Displays the current status of the radio to the mono monitor.     *
 *                                                                                             *
 *    This displays the radio connection value to the monochrome monitor.                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadioClass::Debug_Dump(MonoClass * mono) const
{
	mono->Set_Cursor(29, 7);mono->Printf("0-%-47s", Messages[Old[0]]);
	mono->Set_Cursor(29, 8);mono->Printf("1-%-47s", Messages[Old[1]]);
	mono->Set_Cursor(29, 9);mono->Printf("2-%-47s", Messages[Old[2]]);
	if (Radio != NULL) {
		mono->Set_Cursor(20, 7);mono->Printf("%08X", Radio);
	}
	BASECLASS::Debug_Dump(mono);
}
#endif


/***********************************************************************************************
 * RadioClass::Receive_Message -- Handles receipt of a radio message.                          *
 *                                                                                             *
 *    This is the base version of what should happen when a radio message is received. It      *
 *    turns the radio off when the "OVER_OUT" message is received. All other messages are      *
 *    merely acknowledged with a "ROGER".                                                      *
 *                                                                                             *
 * INPUT:   from     -- The object that is initiating this radio message (always valid).       *
 *                                                                                             *
 *          message  -- The radio message received.                                            *
 *                                                                                             *
 *          param    -- Reference to optional value that might be used to return more          *
 *                      information than can be conveyed in the simple radio response          *
 *                      messages.                                                              *
 *                                                                                             *
 * OUTPUT:  Returns with the response radio message.                                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/28/1994 JLB : Created.                                                                 *
 *   09/24/1994 JLB : Streamlined to be only a communications carrier.                         *
 *   05/22/1995 JLB : Recognized who is sending the message                                    *
 *   06/05/1996 JLB : Radio message history tracking.                                          *
 *=============================================================================================*/
RadioMessageType RadioClass::Receive_Message(RadioClass * from, RadioMessageType message, int & param)
{
	if (message != Old[0]) {
		Old[2] = Old[1];
		Old[1] = Old[0];
		Old[0] = message;
	}

	if (message == RADIO_OVER_OUT) {
		for (int i = 0; i < RadioLinks.Length(); i++) {
			if ((TechnoClass *)from == RadioLinks[i]) {
				BASECLASS::Receive_Message(from, RADIO_OVER_OUT, param);
				RadioLinks[i] = NULL;
				return RADIO_ROGER;
			}
		}
	} else if (message == RADIO_HELLO && Strength) {
		if (!((TechnoClass *)from)->House->Is_Ally(this)) return RADIO_NEGATIVE;
		if (this != NULL && (AbstractFlags & 1) && !((TechnoClass *)this)->House->Is_Ally(from)) return RADIO_NEGATIVE;
		if (from != NULL) {
			if (Index_Of(RadioLinks, (TechnoClass *)from) != -1) return RADIO_ROGER;
		}
		for (int i = 0; i < RadioLinks.Length(); i++) {
			if (RadioLinks[i] == NULL) {
				RadioLinks[i] = (TechnoClass *)from;
				return RADIO_ROGER;
			}
		}
		return RADIO_NEGATIVE;
	}
	return BASECLASS::Receive_Message(from, message, param);
}


/***********************************************************************************************
 * RadioClass::Transmit_Message -- Transmit message from one object to another.                *
 *                                                                                             *
 *    This routine is used to transmit a radio message from this object to another. Most       *
 *    inter object coordination is handled through this mechanism.                             *
 *                                                                                             *
 * INPUT:   to       -- Pointer to the object that will receive the radio message.             *
 *                                                                                             *
 *          message  -- The message itself (see RadioType).                                    *
 *                                                                                             *
 *          param    -- Optional reference to parameter that might be used to pass or          *
 *                      receive additional information.                                        *
 *                                                                                             *
 * OUTPUT:  Returns with the response radio message from the receiving object.                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/22/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
RadioMessageType RadioClass::Transmit_Message(RadioMessageType message, int & param, RadioClass * to)
{
	if (to == NULL) {
		to = (RadioClass *)RadioLinks[0];
		if (to == NULL) return RADIO_STATIC;
	}

	if (message == RADIO_OVER_OUT) {
		for (int i = 0; i < RadioLinks.Length(); i++) {
			if (RadioLinks[i] == (TechnoClass *)to) {
				RadioLinks[i] = NULL;
			}
		}
	} else if (message == RADIO_HELLO) {
		int slot = -1;
		int i;
		for (i = 0; i < RadioLinks.Length(); i++) {
			if (slot == -1 && RadioLinks[i] == NULL) slot = i;
			if (RadioLinks[i] == (TechnoClass *)to) return RADIO_ROGER;
		}
		if (slot == -1) {
			Transmit_Message(RADIO_OVER_OUT, RadioLinks[0]);
			slot = 0;
		}
		if (to->Receive_Message(Dynamic_Cast<TechnoClass *>(this), RADIO_HELLO, param) == RADIO_ROGER) {
			RadioLinks[slot] = (TechnoClass *)to;
			return RADIO_ROGER;
		}
		return RADIO_NEGATIVE;
	}
	return to->Receive_Message(Dynamic_Cast<TechnoClass *>(this), message, param);
}


/***********************************************************************************************
 * RadioClass::Limbo -- When limboing a unit will always break radio contact.                  *
 *                                                                                             *
 *    This routine will break radio contact as the object is entering limbo state.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Was the object successfully limboed?                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/25/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RadioClass::Limbo(void)
{
	if (!IsInLimbo) {
		SendToEachLink(RADIO_OVER_OUT);
	}
	return BASECLASS::Limbo();
}


/***********************************************************************************************
 * RadioClass::Transmit_Message -- Transmits a message to the object specified.                *
 *                                                                                             *
 *    This routine will transmit the specified message to the object. This routine differs     *
 *    from the normal Transmit_Message in that the LParam value is "faked" into the            *
 *    parameter list. It is presumed that the message sent with this function does not         *
 *    require the LParam.                                                                      *
 *                                                                                             *
 * INPUT:   message  -- The message to transmit.                                               *
 *                                                                                             *
 *          to       -- The requested receiver of this message.                                *
 *                                                                                             *
 * OUTPUT:  Returns with the radio response from the receiver.                                 *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/21/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
RadioMessageType RadioClass::Transmit_Message(RadioMessageType message, RadioClass * to)
{
	return Transmit_Message(message, LParam, to);
}


/// <summary>
/// Removes any reference to the specified object.
/// This routine is called when an object is about to be destroyed or removed from the
/// game. Any radio contact with the doomed object must be broken here, since a stale
/// contact would leave this object talking to freed memory.
/// </summary>
/// <param name="target">Pointer to the object that is going away.</param>
/// <param name="all">Should even a casual reference to the target be severed?</param>
void RadioClass::Detach(AbstractClass const * target, bool all)
{
	BASECLASS::Detach(target, all);
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] == (TechnoClass const *)target && all) {
			RadioLinks[i] = NULL;
		}
	}
}


/// <summary>
/// Adds this radio set to the game state checksum.
/// This routine is used by the multiplayer synchronization checker. The identity of the
/// object this radio is tuned to contributes to the checksum, so a desynchronized radio
/// contact will be caught.
/// </summary>
void RadioClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc(RadioLinks.Length());
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] != NULL) {
			crc(RadioLinks[i]->Fetch_ID());
			crc((int)RadioLinks[i]->Fetch_RTTI());
		}
	}
}


/// <summary>
/// Loads this radio set from the save game stream.
/// This routine chains to the base class and then registers the radio contact pointer
/// with the swizzle manager so that it can be remapped once every object has been read
/// back in.
/// </summary>
/// <returns>Returns with S_OK if the radio set was loaded, otherwise the failure code
/// from the base class.</returns>
HRESULT STDMETHODCALLTYPE RadioClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		int count;
		new (&RadioLinks) VectorClass<TechnoClass *>(0);
		result = stream->Read(&count, sizeof(count), NULL);
		if (SUCCEEDED(result)) {
			RadioLinks.Resize(count);
			int i;
			for (i = 0; i < count; i++) {
				TechnoClass * link;
				result = stream->Read(&link, sizeof(link), NULL);
				if (FAILED(result)) return result;
				RadioLinks[i] = link;
			}
			for (i = 0; i < count; i++) {
				Swizzle_Pointer(&RadioLinks[i]);
			}
			result = S_OK;
		}
	}
	return result;
}


/// <summary>
/// Saves this radio set and its links to the save game stream.
/// </summary>
HRESULT STDMETHODCALLTYPE RadioClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		int count = RadioLinks.Length();
		result = stream->Write(&count, sizeof(count), NULL);
		if (SUCCEEDED(result)) {
			for (int i = 0; i < count; i++) {
				result = stream->Write(&RadioLinks[i], sizeof(TechnoClass *), NULL);
				if (FAILED(result)) return result;
			}
			result = S_OK;
		}
	}
	return result;
}


/// <summary>
/// Transmits a message to the first radio link, if there is one.
/// </summary>
RadioMessageType RadioClass::Transmit_Message(RadioMessageType message)
{
	if (RadioLinks[0] != NULL) {
		return Transmit_Message(message, LParam, (RadioClass *)RadioLinks[0]);
	}
	return RADIO_STATIC;
}


/// <summary>
/// Transmits a message to every radio link.
/// </summary>
void RadioClass::SendToEachLink(RadioMessageType command)
{
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] != NULL) {
			Transmit_Message(command, LParam, (RadioClass *)RadioLinks[i]);
		}
	}
}


/// <summary>
/// Fetches the radio link at the specified index.
/// </summary>
TechnoClass * RadioClass::Radio_Link(int index)
{
	return RadioLinks[index];
}


/// <summary>
/// Fetches the radio link at the specified index.
/// </summary>
TechnoClass * RadioClass::Radio_Link(int index) const
{
	return RadioLinks[index];
}


/// <summary>
/// Checks whether the object is one of the radio links.
/// </summary>
bool RadioClass::ContainsLink(TechnoClass const * techno) const
{
	return FindLinkIndex(techno) != -1;
}


/// <summary>
/// Fetches the index of the object among the radio links, or -1.
/// </summary>
int RadioClass::FindLinkIndex(TechnoClass const * techno) const
{
	if (techno != NULL) {
		for (int i = 0; i < RadioLinks.Length(); i++) {
			if (RadioLinks[i] == techno) return i;
		}
	}
	return -1;
}


/// <summary>
/// Checks whether a radio link is free.
/// </summary>
bool RadioClass::HasFreeLink(void) const
{
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] == NULL) return true;
	}
	return false;
}


/// <summary>
/// Checks whether a radio link is free or held by the object specified.
/// </summary>
bool RadioClass::HasFreeLink(TechnoClass const * ignore) const
{
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] == NULL || RadioLinks[i] == ignore) return true;
	}
	return false;
}


/// <summary>
/// Checks whether any radio link is in use.
/// </summary>
bool RadioClass::HasAnyLink(void) const
{
	for (int i = 0; i < RadioLinks.Length(); i++) {
		if (RadioLinks[i] != NULL) return true;
	}
	return false;
}


/// <summary>
/// Grows the number of radio links; new links are free.
/// </summary>
void RadioClass::SetLinkCount(int count)
{
	int old = RadioLinks.Length();
	if (count > old) {
		RadioLinks.Resize(count, NULL);
		for (int i = old; i < count; i++) {
			RadioLinks[i] = NULL;
		}
	}
}
