/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "always.h"
#include "abstract.h"

#include "point.h"
#include "sun.h"
#include "win.h"

class ShapeSet;
class ConvertClass;
class ColorScheme; // RA2
class Surface; // RA2

class ProgressScreenClass : public INoticeSource // RA2: INoticeSource
{
	public:
		ProgressScreenClass(void);
		~ProgressScreenClass(void);

		virtual void Unknown_Virtual_0(int callback) override {Callback = (INoticeSink *)callback;} // RA2: sets Callback (0x6203E0)

		void Initialize(double progress, char count, HWND dialog = NULL); // RA2: 0x61ED80, a dialog window instead of a flag
		void End(void);
		void Free_Shape(void); // RA2: 0x61EE00
		void Set_Side(int side); // RA2: 0x61EE30
		int Get_Side(void); // RA2: 0x61EE40
		ColorScheme * Side_Scheme(void); // RA2: 0x61EE50
		ColorScheme * Text_Scheme(void); // RA2: 0x61EE80
		ColorScheme * Player_Scheme(int index); // RA2: 0x61EED0
		void Set_Graphic_Data(char const * progbar, char const * background = NULL, ConvertClass * drawer = NULL); // RA2: 0x61EF40, a drawer instead of the caption and position
		void Set_Layout(Point2D pt, wchar_t const * string, bool flags, bool outline); // RA2: 0x61EFA0
		void Set_Row_Width(int width); // RA2: 0x61F150
		void Bar_Size(int * width, int * height); // RA2: 0x61F160
		void Flag_Size(int * width, int * height); // RA2: 0x61F1D0
		static void Text_Size(int * width, int * height); // RA2: 0x61F210
		void Box_Size(int * width, int * height, int rows, bool flags); // RA2: 0x61F280
		int Row_Y(int y, int row, bool flags); // RA2: 0x61F3B0
		int Bar_Offset(bool flags); // RA2: 0x61F4B0
		int Flag_Offset(bool flags); // RA2: 0x61F630
		int Text_Offset(bool flags); // RA2: 0x61F770
		double Get_Current_Progress(void) const;
		void Draw_Bar(Point2D pt, double fraction, Surface * surface, ColorScheme * scheme); // RA2: 0x61F8F0
		void Draw_Flag(Point2D pt, Surface * flag); // RA2: 0x61FAB0
		void Draw_Name(Point2D pt, wchar_t const * name, ColorScheme * scheme, int width); // RA2: 0x61FB70
		void Draw_Player(ColorScheme * scheme, wchar_t const * name, Point2D & pt, int player, bool flags); // RA2: 0x61FC20
		void Display_Progress(Point2D pt = Point2D(-1,-1));
		void Set_Progress_Percent(int index, double value, Point2D pt=Point2D(-1,-1));
		void Add_Progress_Percent(int index, double value, Point2D pt=Point2D(-1,-1));
		bool Has_Dialog(void); // RA2: 0x6203C0
		void Clear_Dialog(void); // RA2: 0x6203D0

		double Get_Current_Progress(int index) const
		{
			if (index >= PlayerCount) { // RA2: past the last player, the job itself (0x6203F0)
				return(MainProgress); // RA2: as above
			} // RA2: as above
			return(PlayerProgress[index] / MainProgress);
		}

	public:
		/*
		 * These are the amounts of the job that each player being tracked has finished so
		 * far, one slot per player. A slot is measured against MainProgress rather than
		 * against a hundred, so the bar drawn for it is the ratio of the two.
		 */
		INoticeSink * Callback; // RA2: +0x04

		double PlayerProgress[MAX_PLAYERS];

		/*
		 * This is the total amount of work that the job under way amounts to. Every player's
		 * progress is measured against it, so whatever units the caller counts its work in
		 * will do.
		 */
		double MainProgress;

		/*
		 * Pointer to the caption printed above the progress bars, or NULL if the screen
		 * carries no caption. The text is not copied, so it must outlive the job.
		 */
		wchar_t const * String; // RA2: +0x50, wide

		/*
		 * Pointer to the progress bar artwork. A bar is drawn by clipping the first frame of
		 * this shape down to the fraction of the job finished, so the frame's width is also
		 * the width of a full bar.
		 */
		ShapeSet * Shape;
		bool BufferOwned; // RA2: +0x58, Shape was allocated and must be freed

		/*
		 * Pointer to the name of the title page loaded to sit behind the bars, or NULL if
		 * the caller asked for no backdrop.
		 */
		const char * Background;

		/*
		 * If a job is under way and progress updates should be drawn, then this flag will be
		 * true. It is cleared when the job ends, so that a stray progress request arriving
		 * outside a job is quietly ignored.
		 */
		bool IsActive;

		/*
		 * This is the number of players whose progress is being tracked, and so the number
		 * of bars drawn. It is never less than one -- a single player job draws one centered
		 * bar, while a multiplayer job stacks a bar per player with the names alongside.
		 */
		char PlayerCount;
		bool Unused_62; // RA2: only ever set to false

		/*
		 * Handle of the progress dialog, or NULL when the progress is presented on the full
		 * screen instead. The dialog is used where the game must keep a window up while it
		 * works rather than take the screen over.
		 */
		HWND Dialog;

		/*
		 * This is the center of the progress bar display, expressed in screen pixels. A job
		 * that names no spot of its own is centered on the hidden surface.
		 */
		Point2D Pos;

		bool ShowFlags; // RA2: +0x70, the rows show the players' country flags
		bool IsFilled; // RA2: +0x71, fill each row's rectangle before drawing it
		ConvertClass * Drawer; // RA2: +0x74, replaces the bar's palette when not NULL
		int RowWidth; // RA2: +0x78, overrides the computed row width unless -1

		/*
		 * This is the highest loading message threshold already announced (0 - 100), or -1
		 * before any has been. Each message is printed and its notification sound played
		 * only when the progress first passes its threshold, so that none of them repeat.
		 */
		int Percentage;

		int Side; // RA2: +0x80
};

extern ProgressScreenClass Progress;
