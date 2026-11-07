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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Library/wwmouse.h                            $*
 *                                                                                             *
 *                      $Author:: Byon_g                                                      $*
 *                                                                                             *
 *                     $Modtime:: 8/11/97 10:11a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "win.h"
#include "xmouse.h"

class BSurface;
class ShapeSet;

/*
**	Handles the mouse as it relates to the C&C game engine. It is expected that only
**	one object of this type will be created during the lifetime of the game.
*/
class WWMouseClass : public Mouse {
		typedef Mouse BASECLASS;

	public:
		/*
		**	Private constructor.
		*/
		WWMouseClass(Surface * surfaceptr, HWND window);
		virtual ~WWMouseClass(void) override;

		/*
		**	Maintenance callback routine.
		*/
		void Process_Mouse(void);

		/*
		**	Sets the game-drawn mouse imagery.
		*/
		virtual void Set_Cursor(Point2D const & hotspot, ShapeSet const * cursor, int shape) override;

		/*
		**	Controls visibility of the game-drawn mouse.
		*/
		virtual void Hide_Mouse(void) override;
		virtual void Show_Mouse(void) override;

		/*
		**	Takes control of and releases control of the mouse with
		**	respect to the operating system. The mouse must be released
		**	during operations with the operating system. When the mouse is
		**	relased, it may move outside of the confining rectangle and its
		**	shape is controlled by the operating sytem.
		*/
		virtual void Release_Mouse(void) override;
		virtual void Capture_Mouse(void) override;
		virtual bool Is_Captured(void) const override {return(IsCaptured);}

		/*
		**	Hide the mouse if it falls within this game screen region.
		*/
		virtual void Conditional_Hide_Mouse(Rect region) override;
		virtual void Conditional_Show_Mouse(void) override;

		/*
		**	Query about the mouse visiblity state and location.
		*/
		virtual int Get_Mouse_State(void) const override;
		virtual int Get_Mouse_X(void) const override {return(MouseX);}
		virtual int Get_Mouse_Y(void) const override {return(MouseY);}
		virtual Point2D Get_Mouse_Point(void) const override {return(Point2D(MouseX, MouseY));}

		/*
		**	The following two routines can be used to render the mouse onto an alternate
		**	surface.
		*/
		virtual void Draw_Mouse(Surface * scr, bool issidebarsurface = false) override;
		virtual void Erase_Mouse(Surface * scr, bool issidebarsurface = false) override;

		/*
		**	Converts O/S screen coordinates into game coordinates.
		*/
		virtual void Convert_Coordinate(int & x, int & y) const override;

		/*
		**	Recalculate the confining rectangle from the window.
		*/
		void Calc_Confining_Rect(void);

	private:

		/*
		**	This specifies the mouse shape data. It records the shape set
		**	data as well as the particular image contained within.
		*/
		ShapeSet const * MouseShape;
		int ShapeNumber;

		/*
		**	Used to prevent conflict between the processing callback and
		**	the normal mouse processing routines. The only potential conflict
		**	would be with the maintenance callback routine. Since this callback
		**	and the mouse class maintain a strict master/slave relationship, a
		**	simple critial section flag is all that is needed.
		*/
		HANDLE MouseMutex;

		/*
		**	Mouse hide/show state. If zero or greater, the mouse is visible. Otherwise
		**	it is invisible.
		*/
		int MouseState;

		/*
		**	If the mouse is being managed by this class (for the game), then this flag
		**	will be true. When the mouse has been released to be managed by the operating
		**	system, this flag will be false. However, this class will still track the mouse
		**	position.
		*/
		bool IsCaptured;

		/*
		**	This is the last recorded mouse position that it was drawn to.
		*/
		int MouseX;
		int MouseY;

		/*
		**	Points to the main display surface that the mouse will be drawn
		**	to as it moves.
		*/
		Surface * SurfacePtr;

		/*
		**	This is the window handle that is used to bind and bias the mouse
		**	position and drawing procedures.
		*/
		HWND Window;

		/*
		**	This specifies the rectangle that the game oriented mouse will be
		**	confined to on the visible surface. If the mouse is manually drawn
		**	on another surface, then this rectangle cooresponds to the hidden
		**	surface area where the mouse is to be drawn.
		*/
		Rect ConfiningRect;

		/*
		**	Specifies the hot spot where the image click maps to. This is an
		**	offset from the upper left corner of the shape image.
		*/
		int MouseXHot;
		int MouseYHot;

		/*
		**	Holds the background image behind the mouse to be used for
		**	restoring the surface pixels.
		*/
		BSurface * Background;
		Rect SavedRegion;

		/*
		**	This is the alternate mouse background buffer to be used when the
		**	mouse is manually drawn to an alternate surface by the Draw_Mouse()
		**	function.
		*/
		BSurface * Alternate;
		Rect AltRegion;

		/*
		**	This is another alternate buffer for drawing the mouse pointer across a second, adjoining
		**	offscreen buffer.
		*/
		BSurface * SidebarAlternate;
		Rect SidebarAltRegion;

		/*
		**	Conditional hide mouse bounding rectangle and mouse state
		**	flag.
		*/
		Rect ConditionalRect;
		int ConditionalState;

		/*
		**	Maintenance timer handle.
		*/
		MMRESULT TimerHandle;

		// Determines if there is valid mouse shape data available.
		bool Is_Data_Valid(void) const;
		bool Validate_Copy_Buffer(void);

		void Save_Background(void);
		void Restore_Background(void);

		Rect Matching_Rect(void) const;
		void Raw_Draw_Mouse(Surface * surface, int xoffset, int yoffset);
		void Get_Bounded_Position(int & x, int & y) const;
		void Update_Mouse_Position(int x, int y, bool forced);

		void Low_Show_Mouse(void);
		void Low_Hide_Mouse(void);

		void Block_Mouse(void);
		void Unblock_Mouse(void);
		//bool Is_Blocked(void) const {return(Blocked != 0);}

		virtual bool Is_Hidden(void) const override {return(MouseState < 0);}
};
