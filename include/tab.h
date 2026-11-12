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

/* $Header: /CounterStrike/TAB.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : TAB.H                                                        *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 12/15/94                                                     *
 *                                                                                             *
 *                  Last Update : December 15, 1994 [JLB]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "credits.h"
#include "abstract.h" // RA2: INoticeSink
#include "sidebar.h"

extern int const NoAdvancedCommand; // RA2: 0x7F67F4, -1

class TabClass: public SidebarClass, public INoticeSink // RA2: INoticeSink added; its vtable pointer is at +0x4EE8
{
		typedef SidebarClass BASECLASS;

	public:
		TabClass(void);
		TabClass(NoInitClass const & x) : BASECLASS(x), Credits(x), FlasherTimer(x), MoneyFlashTimer(x) {};

		virtual void AI(KeyNumType &input, Point2D const & xy) override;
		virtual void Draw_It(bool complete=false) override;
		void Draw_Credits_Tab(void); // RA2: not static
		static void Hilite_Tab(int tab);
		static int Command_From_Name(char const * name); // RA2: 0x6A0580
		static void Set_Command_Position(int command, int position); // RA2: 0x6A05E0
		static void Set_Button_Count(int count); // RA2: 0x6A0670
		void Flash_Money(void);

		virtual void One_Time(void) override;							// One-time inits
		void Redraw_Tab(void) {IsToRedraw = true;Flag_To_Redraw();};

		virtual void Init_For_House(void) override;

		virtual bool Unknown_Virtual_0(int, int) override; // RA2: TabClass's INoticeSink virtual

		CreditClass Credits;

		CDTimerClass<FrameTimerClass> FlasherTimer;

	private:
		void Set_Active(int select);

		CDTimerClass<FrameTimerClass> MoneyFlashTimer;

	protected:
		bool IsThumbActive; // RA2: +0x4F14; picks which of two buttons the tab bar draws

		/*
		**	If the tab graphic is to be redrawn, then this flag is true.
		*/
		bool IsToRedraw; // RA2: after MoneyFlashTimer, at +0x4F15

	public:
		bool IsDrawn; // RA2: +0x4F16; set once the tab bar has been drawn; public, Update_Visible_Surface reads it

	private:

		static ShapeSet const * TabShape;
};
