/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 ra2-decomp contributors
 *
 * Renamed and ordered as in RA2 1.006 by RedAlert2 contributors, 2026.
 ******************************************************************************/

#pragma once

#include <stddef.h>

class Surface;

/*
**	A rectangle given by its edges, inclusive.
*/
struct LTRBStruct
{
	int Left;
	int Top;
	int Right;
	int Bottom;
};

/*
**	A bitmap font: one bit per pixel glyphs drawn in a single colour into a locked
**	16 bit surface.
*/
class BitFont
{
	public:
		/*
		**	The glyphs as loaded from a font file.
		*/
		struct InternalData
		{
			int FontWidth;
			int Stride;
			int FontHeight;
			int Lines;
			int Count;
			int SymbolDataSize;
			unsigned short * SymbolTable;
			char * Bitmaps;
			int ValidSymbolCount;

			void Destroy(void);
		};

		BitFont(char const * filename);
		virtual ~BitFont(void);

		static InternalData * Load_Data(char const * filename);
		bool Load(char const * filename);

		unsigned short Set_Color(unsigned short color);
		unsigned short Set_Shadow_Color(unsigned short color);
		bool Set_Clip(bool clipped);
		void Set_Bounds(LTRBStruct * bounds);
		bool Set_Bold(bool bold);

		bool Text_Dimensions(wchar_t const * text, int * width, int * height, int maxwidth);
		int String_Pixel_Width(wchar_t const * text, int maxwidth);
		int Bold_String_Pixel_Width(wchar_t const * text, int maxwidth);
		int Chars_In_Width(wchar_t const * text, int maxwidth, int maxchars, int wrap);
		bool Bold_Text_Dimensions(wchar_t const * text, int * width, int * height, int maxwidth);

		int Set_Tab_Width(int width);
		void Set_Tab_Origin(int x);

		int Print_Char(unsigned short ch, int x, int y, int color);
		int Print_Typed(wchar_t const * text, int x, int y, int maxchars, int shown);

		unsigned char * Char_Data(wchar_t ch);
		void Make_Missing_Char(void);
		void * Missing_Char(unsigned short * color);
		void Widen_Space(void);
		void Set_Wide_Space(void);
		bool Clip_Rect(LTRBStruct * out, LTRBStruct * rect, LTRBStruct * bounds);

		void Lock(Surface * surface);
		void Unlock(Surface * surface);
		int Char_Width(wchar_t ch);
		void Reset(void);

		InternalData * Data;
		void * MissingChar;
		short * Buffer;
		int Pitch;
		int Unknown_14;
		int Stride;
		int LineHeight;
		int TabOrigin;
		unsigned short Color;
		unsigned short ShadowColor;
		int TabWidth;
		int Spacing;
		LTRBStruct Bounds;
		bool IsBold;
		bool IsClipped;
		bool Unknown_42;
		bool Unknown_43;
};
