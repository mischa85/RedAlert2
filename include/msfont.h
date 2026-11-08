/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <stddef.h>

class Surface;
class ConvertClass;
class ShapeSet;
template<class T> class TRect;
typedef TRect<int> Rect;

class MSFont
{
	public:
		MSFont(bool use_side_palette = true);
		virtual ~MSFont(void);

		int Get_Font_Width(void) {return(FontWidth);}
		int Get_Font_Height(void) {return(FontHeight);}

		unsigned char Get_Red(void) {return(Red);}
		unsigned char Get_Green(void) {return(Green);}
		unsigned char Get_Blue(void) {return(Blue);}
		int Get_Color(void) {return(Color);}

		bool Init(bool use_side_palette); // RA2

		void Get_String_Rect(wchar_t const * string, Rect & rect); // RA2: wide

		virtual int Get_Character_Width(wchar_t character); // RA2: wide
		virtual int Get_String_Width(wchar_t const * string); // RA2: wide

		virtual void Draw_String(Surface * surface, wchar_t const * string, int x, int y, int frame); // RA2: wide
		virtual void Draw_Character(Surface * surface, wchar_t character, int x, int y, int frame, bool do_sound); // RA2: wide

	private:
		/*
		 * These are the width and height of the font's glyph cell, expressed in pixels and
		 * taken from the shape file as it is loaded. The height is what a newline advances
		 * by, and the width serves as a rough per character measure.
		 */
		int FontWidth;
		int FontHeight;

		/*
		 * These are the red, green and blue components of the color the font prints in, read
		 * out of the palette that colors the glyphs. They let a caller shade a panel or a
		 * rule to match the text that will be drawn over it.
		 */
		unsigned char Red;
		unsigned char Green;
		unsigned char Blue;

		/*
		 * This is that same font color built into a pixel of the display's own format, ready
		 * to be handed to the line and rectangle drawing routines.
		 */
		int Color;

		/*
		 * This points to the glyph shapes for the font. Every printable character owns three
		 * consecutive frames, which the menu print animations step through to fade a
		 * character in, and the last of the three is the one measured for the glyph's width.
		 */
		ShapeSet * FontFile;

		/*
		 * This points to the conversion table the glyphs are drawn through, built from the
		 * font's own palette. It is what remaps the glyph shapes into the colors that the
		 * visible surface is running in.
		 */
		ConvertClass * Drawer;

		/*
		 * If the glyph shapes had to be loaded from disk rather than found already resident
		 * in a mixfile, then this flag will be true. Only shapes loaded that way belong to
		 * the font, so only those are freed when the last font is destroyed.
		 */
		bool AllocLoaded;
};
