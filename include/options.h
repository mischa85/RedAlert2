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

/* $Header: /CounterStrike/OPTIONS.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : OPTIONS.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : June 8, 1994                                                 *
 *                                                                                             *
 *                  Last Update : June 8, 1994   [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "keyboard.h"

#include "diff.hh"

class OptionsClass {
	public:
		enum {
			MAX_SCROLL_SETTING=7,
			MAX_SPEED_SETTING=7,
			MAX_DETAIL_SETTING=3,
			MAX_DIFFICULTY_SETTING=DIFF_COUNT
		};

		OptionsClass(void);

		void One_Time(void);

		void Fixup_Palette(void) const;
		void Set_Shuffle(bool on);
		void Set_Repeat(bool on);
		void Set_Score_Volume(float volume, bool feedback);
		void Set_Sound_Volume(float volume, bool feedback);
		void Set_Voice_Volume(float volume, bool feedback);
		int Normalize_Delay(int delay) const;
		int Normalize_Volume(int volume) const;
		int Get_Scroll_Method(void) const;

		bool Hotkey_Dialog(void);

		/*
		**	File I/O routines
		*/
		void Load_Settings(void);
		void Save_Settings(void);

		void Setup_Dest_Net(void);
		void Set(void);

		/*
		**	This is actually the delay between game frames expressed as 1/60 of
		**	a second. The default value is 4 (1/15 second).
		*/
		int GameSpeed;

		/*
		 * This is the campaign difficulty the player has picked, expressed as the slider
		 * position (0 - 4). It sets both the scenario's difficulty and the computer's when a
		 * single player game starts; multiplay takes its difficulty from the session options.
		 */
		int Difficulty;

		/*
		 * These control how the tactical map scrolls. The method (0 - 2) selects what holding
		 * the right mouse button does: 0 scrolls the map alone, while 1 and 2 also drag the
		 * mouse pointer along so that the map appears to move under it (2 reverses the sense).
		 */
		int ScrollMethod;
		int ScrollRate;					// Distance to scroll.
		bool AutoScroll;				// Does map autoscroll?

		/*
		 * This is how much detail the renderer is allowed to draw (0 - 2). Animations, particle
		 * systems and translucency carry a detail level of their own and are skipped when it
		 * exceeds this one, so a lower setting buys frame rate at the cost of eye candy.
		 */
		int DetailLevel;

		/*
		 * If the sidebar is to be drawn down the right hand edge of the screen, then this flag
		 * will be true. Tiberian Sun offers no way to move it, so it is always true.
		 */
		bool IsSidebarOnRight;

		/*
		 * If each sidebar cameo is to be captioned with the name of what it builds, then this
		 * flag will be true.
		 */
		bool SidebarCameoText;

		/*
		 * If selected objects are to draw a line to their target, then this flag will be true.
		 */
		bool ActionLines;

		/*
		 * If tooltips are to pop up over the user interface, then this flag will be true.
		 */
		bool ToolTips;

		/*
		 * These are the dimensions of the game screen, expressed in pixels. The tactical
		 * view, the sidebar and the tabs are all laid out against them.
		 */
		int ScreenWidth;
		int ScreenHeight;

		/*
		 * If the movies are to be stretched to fill the screen, then this flag will be true.
		 * It is forced off when the display cannot stretch blit.
		 */
		bool StretchMovies;

		/*
		 * If display modes beyond the two the game normally offers are to be selectable, then
		 * this flag will be true. There is no options screen control for it.
		 */
		bool AllowHiResModes;

		float SoundVolume;				// Volume for sound effects.
		float VoiceVolume;				/// Volume for voices.
		float ScoreVolume;				// Volume for scores.
		bool IsScoreRepeat;				// Score should repeat?
		bool IsScoreShuffle;			// Score list should shuffle?

		/*
		 * This is how far ahead of the movie the sound card is assumed to be running, expressed
		 * in VQA time ticks. The audio position is backed off by it so that a software emulated
		 * sound driver stays in step with the video; a real driver has it zeroed instead.
		 */
		unsigned short SoundLatency;

		/*
		 * This is the IPX socket the game communicates over. A socket below 0x4000 is biased up
		 * into the dynamic range before use, and 0xFFFF means to take the default socket.
		 */
		unsigned short Socket;

		/*
		 * This is which network adapter to send over on a machine that has more than one,
		 * expressed as an index into the adapter list. It defaults to zero, the first one.
		 */
		int NetCard;

		/*
		 * This is the address of a network on the far side of a bridge, written as dot
		 * separated hexadecimal bytes. When one is given the game broadcasts there as well as
		 * locally, so that players on either side of a router can find each other.
		 */
		char DestNet[64];

		/*
		**	These are the hotkeys used for keyboard control.
		*/
		KeyNumType KeyForceMove1;
		KeyNumType KeyForceMove2;
		KeyNumType KeyForceAttack1;
		KeyNumType KeyForceAttack2;
		KeyNumType KeySelect1;
		KeyNumType KeySelect2;
		KeyNumType KeyQueueMove1;
		KeyNumType KeyQueueMove2;

	private:

		static char const * const HotkeyName;
};
