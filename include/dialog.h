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

#pragma once

#include <stddef.h> // RA2: wchar_t for the wide text routines

#include "point.h"

#include "color.hh"
#include "dialog.hh"

class FontClass;
class Surface;
class ColorScheme;
template<class T> class TRect;
typedef TRect<int> Rect;

int Format_Window_String(char * string, FontClass const * font, int maxlinelen, int & width, int & height);
Point2D Conquer_Clip_Text_Print(wchar_t const *, // RA2: wide
 Surface & surface, Rect const &rect, Point2D const & pt, ColorScheme * fore, int back=TBLACK, TextPrintType flag=TextPrintType(TPF_8POINT|TPF_DROPSHADOW), int width=-1, int const * tabs=0);
void Draw_Box(Rect const & rect, BoxStyleEnum up, bool filled);
Point2D __cdecl Fancy_Text_Print(char const * text, Surface & surface, Rect const & rect, Point2D const &pt, ColorScheme * fore, int back, TextPrintType flag, ...);
Point2D __cdecl Fancy_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const &pt, ColorScheme * fore, int back, TextPrintType flag, ...); // RA2: wide text
Point2D __cdecl Fancy_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const &pt, unsigned fore, unsigned back, TextPrintType flag, ...); // RA2: wide text in a pixel colour
Point2D __cdecl Fancy_Text_Print(int text, Surface & surface, Rect const & rect, Point2D const & pt, ColorScheme * fore, int back, TextPrintType flag, ...);
Point2D Simple_Text_Print(wchar_t const * text, // RA2: wide
	 Surface & surface, Rect const & rect, Point2D const & pt, ColorScheme * scheme, int back, TextPrintType flag, int fore);
Point2D Simple_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int unused); // RA2: pixel colour (0x497830)
Point2D Conquer_Clip_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int width, int const * tabs); // RA2: pixel colour (0x497CE0)
Point2D Plain_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int scheme, int unused); // RA2: wide (0x497F40)
Point2D Plain_Text_Print(wchar_t const * text, Surface * surface, Rect const * rect, Point2D const * pt, unsigned fore, unsigned back, TextPrintType flag, int scheme, int unused); // RA2: by pointers (0x498050)
int Format_Window_String(wchar_t const * string, int, int columns, int & width, int & height); // RA2: wide, an unused second argument, width in columns of 'W' (0x4977D0)
Point2D Plain_Text_Print(int text, Surface & surface, Rect const &rect, Point2D const & xy, int /*fore*/, int back, TextPrintType flag, int scheme, int fore);
Point2D Plain_Text_Print(char const * text, Surface & surface, Rect const &rect, Point2D const & pt, int /*fore*/, int back, TextPrintType flag, int scheme, int fore);

class BitFont; // RA2: Font_From_TPF returns a BitFont
BitFont *Font_From_TPF(TextPrintType flags); // RA2: text is drawn with BitFont
bool __fastcall Draw_Text_Field(Surface & surface, Rect const & rect, wchar_t const * text, int caret, BitFont * font, int color, int & firstchar, int focused, int unknown, int show_caret, int shown); // RA2: wide text with scrolling, caret and IME composition
unsigned char * Font_Palette(int color);
