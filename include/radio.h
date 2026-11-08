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

/* $Header: /CounterStrike/RADIO.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RADIO.H                                                      *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 23, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "globals.h"
#include "mission.h"
#include "vector.h" // RA2: VectorClass

class ObjectClass;
class TechnoClass;


/****************************************************************************
**	Radio contact is controlled by this class. It handles the mundane chore
**	of keeping the radio contact alive as well as broadcasting messages
**	to the receiving radio. Radio contact is primarily used when one object
**	is in "command" of another.
*/
class RadioClass : public MissionClass
{
		typedef MissionClass BASECLASS;

	private:

		/*
		**	This is a record of the last message received by this receiver.
		*/
		RadioMessageType Old[3];

		/*
		**	This is the object that radio communication has been established
		**	with. Although is is only a one-way reference, it is required that
		**	the receiving radio is also tuned to the object that contains this
		**	radio set.
		*/
		VectorClass<TechnoClass *> RadioLinks; // RA2: replaces RadioClass * Radio; +0xC8, one entry when constructed

#ifdef _DEBUG
		/*
		**	This is a text representation of all the possible radio messages. This
		**	text is used for monochrome debug printing.
		*/
		static char const * Messages[RADIO_COUNT];
#endif

	public:

		/*---------------------------------------------------------------------
		**	Constructors, Destructors, and overloaded operators.
		*/
		RadioClass(void);
		RadioClass(NoInitClass const & x);
		virtual ~RadioClass(void) override {/*Radio=0;*/};

		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override; // RA2

		/*---------------------------------------------------------------------
		**	Member function prototypes.
		*/
		bool In_Radio_Contact(void) const {return(RadioLinks[0] != 0);};
		void Radio_Off(void) {RadioLinks[0] = 0;};
		TechnoClass * Contact_With_Whom(void) const {return(RadioLinks[0]);};
		TechnoClass * Radio_Link(int index); // RA2
		TechnoClass * Radio_Link(int index) const; // RA2
		bool ContainsLink(TechnoClass const * techno) const; // RA2
		int FindLinkIndex(TechnoClass const * techno) const; // RA2
		bool HasFreeLink(void) const; // RA2
		bool HasFreeLink(TechnoClass const * ignore) const; // RA2
		bool HasAnyLink(void) const; // RA2
		void SetLinkCount(int count); // RA2

		// Inherited from base class(es).
		virtual void Detach(AbstractClass const * target, bool all = true) override;
		virtual void Compute_CRC(CRCEngine &) const override;
		virtual RadioMessageType Receive_Message(RadioClass * from, RadioMessageType message, int & param) override;
		virtual RadioMessageType Transmit_Message(RadioMessageType message, int & param, RadioClass * to=NULL); // RA2: no default param
		virtual RadioMessageType Transmit_Message(RadioMessageType message, RadioClass * to);
		virtual RadioMessageType Transmit_Message(RadioMessageType message); // RA2
		virtual void SendToEachLink(RadioMessageType command); // RA2
#ifdef _DEBUG
		virtual void Debug_Dump(MonoClass *mono) const override;
#endif
		virtual bool Limbo(void) override;
};
