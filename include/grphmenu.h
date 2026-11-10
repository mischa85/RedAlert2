/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "keyboard.h"
#include "msanim.h"
#include "msengine.h"
#include "rect.h"
#include "wstring.h" // RA2: the menu's names are Wstrings

class GraphicMenuItem;
class INIClass;
class MSSfxEntry;
template<class T> class DynamicVectorClass;

class GraphicMenu
{
	friend GraphicMenu * _Graphic_Menu(INIClass & ini, Wstring const & name); // RA2: Wstring name

	typedef DynamicVectorClass<GraphicMenuItem *> ITEM_LIST;
	typedef VectorCursor<GraphicMenuItem *, ITEM_LIST> ITEM_ITER;

	public:
		GraphicMenu(void);
		virtual ~GraphicMenu(void);

		void Set_Animation(MSAnim * anim);
		void Set_Theme_Name(Wstring const & name); // RA2: Wstring name
		void Set_Background_Name(Wstring const & name); // RA2: sets the background PCX name

		void Add_Item(GraphicMenuItem * item);

		void Set_Item_Enabled(int item_id, bool enabled);
		int Presentation(void);
		GraphicMenuItem * Get_Item_Under_Mouse(Point2D const & mouse);
		GraphicMenuItem * Get_Item_For_Key(KeyNumType key);

	public:
		/*
		 * This is the animation engine that drives the menu. It owns the backdrop and the
		 * items' animations, paces the display, and is handed to the chosen item so that
		 * the item can act upon the screen.
		 */
		MSEngine Engine;

		/*
		 * This is the name of the still picture that backs the menu, which is "Title.PCX"
		 * until the menu description supplies another. The shell menu takes a copy of it so
		 * that the same backdrop can stay up after this menu has gone.
		 */
		Wstring BackgroundName; // RA2: a Wstring, not GraphicMenuString

		/*
		 * This is the name of the music theme played while the menu is up. It starts when
		 * the menu takes control and is faded out again once the player picks something.
		 */
		Wstring ThemeName; // RA2: a Wstring, not GraphicMenuString

	private:
		/*
		 * Pointer to the animation playing behind the menu. The engine owns it -- this is
		 * kept only so that a fresh backdrop can take its place and so that the menu can
		 * let it play out before drawing over it.
		 */
		MSAnim * CurrentAnim;

		/*
		 * These are the items the player may pick from. The menu owns them and destroys
		 * them along with itself.
		 */
		ITEM_LIST Items;
};


GraphicMenu * Do_Graphic_Menu(Wstring const & ini, Wstring const & name); // RA2: Wstring names
