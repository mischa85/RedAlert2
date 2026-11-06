/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 ra2-decomp contributors
 *
 * Renamed and ordered as in RA2 1.006 by RedAlert2 contributors, 2026.
 ******************************************************************************/

#pragma once

#include <stddef.h>

class BitFont;
class Surface;

/*
**	Draws text in a BitFont into a surface: locks it, prints, unlocks.
*/
class BitText
{
	public:
		BitText(void);
		virtual ~BitText(void);

		int Print_Char(BitFont * font, Surface * surface, unsigned short ch, int x, int y);
		int Print(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int maxchars, int shown);
		int Print_Bold(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int maxchars);
		bool Draw_Text(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade);
		bool Draw_Shadow(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade);
		static BitText * Instance;

		bool Draw_Bold(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade);
};

/*
**	Load and free the game font, GAME.FNT, kept in BitFont::Instance.
*/
void Create_Font(void);
void Destroy_Font(void);
void __stdcall Release_Font(BitFont * font);
