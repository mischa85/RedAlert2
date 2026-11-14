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

/* $Header: /CounterStrike/DIALOG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : DIALOG.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : July 31, 1996 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Clip_Text_Print -- Prints text with clipping and <TAB> support.                           *
 *   Dialog_Box -- draws a dialog background box                                               *
 *   Display_Place_Building -- Displays the "place building" dialog box.                       *
 *   Display_Select_Target -- Displays the "choose target" prompt.                             *
 *   Display_Status -- Display the player scenario status box.                                 *
 *   Draw_Box -- Displays a highlighted box.                                                   *
 *   Draw_Caption -- Draws a caption on a dialog box.                                          *
 *   Fancy_Text_Print -- Prints text with a drop shadow.                                       *
 *   Plain_Text_Print -- Prints text without using a color scheme                              *
 *   Redraw_Needed -- Determine if sidebar needs to be redrawn.                                *
 *   Render_Bar_Graph -- Renders a specified bargraph.                                         *
 *   Simple_Text_Print -- Prints text with a drop shadow.                                      *
 *   Window_Box -- Draws a fancy box over the specified window.                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "dsurface.h" // RA2: wide BitFont text
#include <stdarg.h> // RA2: wide BitFont text
#include <stdio.h> // RA2: wide BitFont text
#include <wchar.h> // RA2: wide BitFont text
#include "dialog.h"
#include "bitfont.h" // RA2: text is drawn with BitFont

#include "_font.h"
#include "_surface.h"
#include "data.h"
#include "font.h"
#include "gadget.h"
#include "lightcon.h"
#include "scheme.h"
#include "surface.h"
#include "vector.h"

#include "color.hh"
#include "bittext.h" // RA2: as above


/// <summary>
/// RA2
/// </summary>
Rect * Drawing_GetTextDimensions(Rect * out, wchar_t const * text, Point2D loc, unsigned short flags, int marginX, int marginY) // RA2: not in TibSun, at 0x497360 (YR's name)
{
	BitFont * font = BitFont::Instance;
	int width = font->String_Pixel_Width(text, 0);
	int height = font->LineHeight;
	int x = loc.X;

	if (flags & 0x100) {
		x -= width / 2;
	} else if (flags & 0x200) {
		x -= width;
	}
	*out = Rect(x - marginX, loc.Y - marginY, width + marginX * 2, height + marginY * 2);
	return out;
}


/***********************************************************************************************
 * Draw_Box -- Displays a highlighted box.                                                     *
 *                                                                                             *
 *    This will draw a highlighted box to the logicpage. It can                                *
 *    optionally fill the box with a color as well. This is a low level                        *
 *    function and thus, it doesn't do any graphic mode color adjustments.                     *
 *                                                                                             *
 * INPUT:   x,y   -- Upper left corner of the box to be drawn (pixels).                        *
 *                                                                                             *
 *          w,h   -- Width and height of box (in pixels).                                      *
 *                                                                                             *
 *          up    -- Is the box rendered in the "up" stated?                                   *
 *                                                                                             *
 *          filled-- Is the box to be filled.                                                  *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/28/1991 JLB : Created.                                                                 *
 *   05/30/1992 JLB : Embedded color codes.                                                    *
 *   07/31/1992 JLB : Depressed option added.                                                  *
 *=============================================================================================*/
void Draw_Box(Rect const & rect, BoxStyleEnum up, bool filled)
{
	ColorScheme * scheme = ColorSchemes[GadgetClass::Get_Color_Scheme()];

	// Filler, Shadow, Hilite, Corner colors

	BoxStyleType const ButtonColors[BOXSTYLE_COUNT] = {
		{ scheme->Background, scheme->Highlight, scheme->Shadow,  scheme->Corners}, // Down
		{ scheme->Background, scheme->Shadow, scheme->Highlight,  scheme->Corners}, // Raised
		{ DKGREY, WHITE,  BLACK,  DKGREY},                                          // Disabled down
		{ DKGREY, BLACK,  LTGREY, DKGREY},                                          // Disabled up
		{ scheme->Background,  scheme->Box, scheme->Box,  scheme->Background},      // List box
		{ scheme->Background,  scheme->Box, scheme->Box,  scheme->Background},      // Dialog box
	};

	//w--;
	//h--;
	BoxStyleType const &style = ButtonColors[up];

	if (filled) {
		LogicalSurface->Fill_Rect(rect, scheme->Converter->Convert_Pixel(style.Filler));
	}

	switch (up) {
		case (BOXSTYLE_BOX):
			LogicalSurface->Draw_Rect(rect, scheme->Converter->Convert_Pixel(style.Highlight));
			break;

		case (BOXSTYLE_BORDER):
			LogicalSurface->Draw_Rect(Rect(rect.X+1, rect.Y+1, rect.Width-1, rect.Height-1), scheme->Converter->Convert_Pixel(style.Highlight));
			break;

		default:
			LogicalSurface->Draw_Line(Point2D(rect.X, rect.Y+rect.Height), Point2D(rect.X+rect.Width, rect.Y+rect.Height), scheme->Converter->Convert_Pixel(style.Shadow));
			LogicalSurface->Draw_Line(Point2D(rect.X+rect.Width, rect.Y), Point2D(rect.X+rect.Width, rect.Y+rect.Height), scheme->Converter->Convert_Pixel(style.Shadow));

			LogicalSurface->Draw_Line(Point2D(rect.X, rect.Y), Point2D(rect.X+rect.Width, rect.Y), scheme->Converter->Convert_Pixel(style.Highlight));
			LogicalSurface->Draw_Line(Point2D(rect.X, rect.Y), Point2D(rect.X, rect.Y+rect.Height), scheme->Converter->Convert_Pixel(style.Highlight));

			LogicalSurface->Put_Pixel(Point2D(rect.X, rect.Y+rect.Height), scheme->Converter->Convert_Pixel(style.Corner));
			LogicalSurface->Put_Pixel(Point2D(rect.X+rect.Width, rect.Y), scheme->Converter->Convert_Pixel(style.Corner));
			break;
	}
}


/***********************************************************************************************
 * Format_Window_String -- Separates a String into Lines.                                      *
 *   This function will take a long string and break it up into lines                          *
 *   which are not longer then the window width. Any character < ' ' is                        *
 *   considered a new line marker and will be replaced by a NULL.                              *
 *                                                                                             *
 * INPUT:      char *String - string to be formated.                                           *
 *             int maxlinelen - Max length of any line in pixels.                              *
 *                                                                                             *
 * OUTPUT:     int - number of lines string is.                                                *
 *                                                                                             *
 * WARNINGS:    The string passed in will be modified - NULLs will be put                      *
 *                into each position that will be a new line.                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/27/1992  SB : Created.                                                                 *
 *   05/18/1995 JLB : Greatly revised for new font system.                                     *
 *     09/04/1996 BWG : Added '@' is treated as a carriage return for width calculations.      *
 *=============================================================================================*/
/// <summary>
/// Counts the lines a wide string takes when wrapped to the given number of columns.
/// </summary>
int Format_Window_String(wchar_t const * string, int, int columns, int & width, int & height) // RA2: at 0x4977D0, wide; an unused second argument (ret 12); columns of 'W'
{
	width = 0;
	height = 0;
	BitFont * font = BitFont::Instance;
	if (string != NULL && font != NULL) {
		font->Text_Dimensions(string, &width, &height, font->Char_Width('W') * columns);
		return(height / font->LineHeight);
	}
	return(0);
}


/// <summary>
/// Fetches the font that a text print style calls for.
/// This routine is used by the gadget drawing and text printing routines to turn the point
/// size portion of a print style into the font that should actually be used. A style that
/// names no recognized font falls back to the small six point font, so there is always a
/// font to draw with.
/// </summary>
/// <param name="flags">The text print style whose point size selects the font.</param>
/// <returns>Returns with a pointer to the font this style should be printed with.</returns>
Point2D Simple_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int) // RA2: at 0x497830
{
	BitFont * font = BitFont::Instance;
	int x = rect.X + pt.X;
	int y = rect.Y + pt.Y;
	int width;
	int height;

	font->Text_Dimensions(text, &width, &height, rect.Width);
	if (flag & TPF_CENTER) {
		x -= width / 2;
	} else if (flag & TPF_RIGHT) {
		x -= width;
	}

	if (flag & TPF_FADED_BACKGROUND) {
		Rect box(x - 1, y - 1, width + 2, height + 2);
		surface.Fill_Rect_Trans(box, RGBClass(0, 0, 0), 60);
	} else if (flag & TPF_BLACK_BACKGROUND) {
		Rect box(x - 1, y - 1, width + 2, height + 2);
		surface.Fill_Rect(box, 0);
	}

	LTRBStruct bounds = {rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height};
	font->Set_Clip(true);
	font->Set_Bounds(&bounds);
	font->Set_Color((unsigned short)fore);
	BitText::Instance->Print(font, &surface, text, x, y, 0, 0);
	return(Point2D(x, y));
}


/***********************************************************************************************
 * Fancy_Text_Print -- Prints text with a drop shadow.                                         *
 *                                                                                             *
 *    This routine functions like Text_Print, but will render a drop                           *
 *    shadow (in black).                                                                       *
 *                                                                                             *
 * INPUT:   text  -- Text number to print.                                                     *
 *                                                                                             *
 *          x,y   -- Pixel coordinate for to print text.                                       *
 *                                                                                             *
 *          fore  -- Foreground color.                                                         *
 *                                                                                             *
 *          back  -- Background color.                                                         *
 *                                                                                             *
 *          flag  -- Text print control flags.                                                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine is much slower than normal text print and                          *
 *             if rendered to the SEENPAGE, the intermediate rendering                         *
 *             steps could be visible.                                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   11/29/1994 JLB : Created                                                                  *
 *=============================================================================================*/
/// <summary>
/// Prints wide text in a colour scheme's colour.
/// </summary>
Point2D Simple_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, ColorScheme * scheme, int back, TextPrintType flag, int fore) // RA2: at 0x497990
{
	RGBClass rgb = scheme->HSV;
	unsigned color = ((rgb.Get_Red() >> DSurface::RedLeft) << DSurface::RedRight) | ((rgb.Get_Green() >> DSurface::GreenLeft) << DSurface::GreenRight) | ((rgb.Get_Blue() >> DSurface::BlueLeft) << DSurface::BlueRight);
	return(Simple_Text_Print(text, surface, rect, pt, color, back, flag, fore));
}


/***********************************************************************************************
 * Fancy_Text_Print -- Prints text with a drop shadow.                                         *
 *                                                                                             *
 *    This routine functions like Text_Print, but will render a drop                           *
 *    shadow (in black).                                                                       *
 *                                                                                             *
 * INPUT:   text  -- Pointer to text to render.                                                *
 *                                                                                             *
 *          x,y   -- Pixel coordinate for to print text.                                       *
 *                                                                                             *
 *          fore  -- Foreground color.                                                         *
 *                                                                                             *
 *          back  -- Background color.                                                         *
 *                                                                                             *
 *          flag  -- Text print control flags.                                                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine is much slower than normal text print and                          *
 *             if rendered to the SEENPAGE, the intermediate rendering                         *
 *             steps could be visible.                                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/24/1991 JLB : Created.                                                                 *
 *   10/26/94   JLB : Handles font X spacing in a more friendly manner.                        *
 *   11/29/1994 JLB : Separated actual draw action.                                            *
 *=============================================================================================*/
/// <summary>
/// Fetches the font to use for the text print flags given (RA2 has the one).
/// </summary>
BitFont * Font_From_TPF(TextPrintType) // RA2: at 0x497A50
{
	return(BitFont::Instance); // RA2: the one BitFont
}


/***********************************************************************************************
 * Clip_Text_Print -- Prints text with clipping and <TAB> support.                             *
 *                                                                                             *
 *    Use this routine to print text that that should be clipped at an arbitrary right margin  *
 *    as well as possibly recognizing <TAB> characters. Typical users of this routine would    *
 *    be list boxes.                                                                           *
 *                                                                                             *
 * INPUT:   text  -- Reference to the text to print.                                           *
 *                                                                                             *
 *          x,y   -- Pixel coordinate of the upper left corner of the text position.           *
 *                                                                                             *
 *          fore  -- The foreground color to use.                                              *
 *                                                                                             *
 *          back  -- The background color to use.                                              *
 *                                                                                             *
 *          flag  -- The text print flags to use.                                              *
 *                                                                                             *
 *          width -- The maximum pixel width to draw the text. Extra characters beyond this    *
 *                   point will not be printed.                                                *
 *                                                                                             *
 *          tabs  -- Optional pointer to a series of pixel tabstop positions.                  *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/21/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
/// <summary>
/// Prints formatted wide text in a pixel colour.
/// </summary>
Point2D __cdecl Fancy_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, ...) // RA2: at 0x497A60
{
	wchar_t buffer[512];

	if (text != NULL) {
		va_list args;
		va_start(args, flag);
		vswprintf(buffer, text, args);
		va_end(args);
		return(Simple_Text_Print(buffer, surface, rect, pt, fore, back, flag, 1));
	}
	return(Simple_Text_Print(text, surface, rect, pt, fore, back, flag, 1));
}


/// <summary>
/// Prints formatted wide text in a colour scheme's colour.
/// </summary>
Point2D __cdecl Fancy_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, ColorScheme * fore, int back, TextPrintType flag, ...) // RA2: at 0x497B40
{
	wchar_t buffer[512];

	if (text != NULL) {
		va_list args;
		va_start(args, flag);
		vswprintf(buffer, text, args);
		va_end(args);
		return(Simple_Text_Print(buffer, surface, rect, pt, fore, back, flag, 1));
	}
	return(Simple_Text_Print(text, surface, rect, pt, fore, back, flag, 1));
}


/// <summary>
/// Prints wide text in a pixel colour, honouring tabs and clipping it to the width given.
/// </summary>
Point2D Conquer_Clip_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int width, int const * tabs) // RA2: at 0x497CE0
{
	wchar_t buffer[512];
	Point2D point = pt;

	if (text != NULL) {
		wcscpy(buffer, text);

		/*
		**	Set the font and spacing characteristics according to the flag
		**	value passed in.
		*/
		BitFont * font = BitFont::Instance;
		wchar_t * source = &buffer[0];
		unsigned offset = 0;
		int processing = true;
		while (processing && offset < (unsigned)width) {
			wchar_t * ptr = wcschr(source, L'\t');
			if (ptr != NULL) {
				*ptr = L'\0';
			}

			if (*source != L'\0') {
				int w = 0;
				wchar_t * bptr = source;
				do {
					w += font->Char_Width(*bptr++);
				} while (*bptr != L'\0' && offset + w < (unsigned)width);

				/*
				**	Scan forward until the end of the string is reached or the
				**	maximum width, whichever comes first.
				*/
				if (offset + w >= (unsigned)width) {
					bptr--;
					w -= font->Char_Width(*bptr);
					*bptr = L'\0';
					processing = 0;
				}

				/*
				**	Print this text block and advance the offset accordingly.
				*/
				point = Simple_Text_Print(source, surface, rect, Point2D(pt.X + offset, pt.Y), fore, back, flag, 1);
				offset += w;
			}

			/*
			**	If a <TAB> was the terminator for this text block, then advance
			**	to the next tabstop.
			*/
			if (ptr == NULL) {
				break;
			}
			if (tabs != NULL) {
				while (offset > (unsigned)*tabs) {
					tabs++;
				}
				offset = *tabs;
			} else {
				offset = ((offset + 1 / 50) + 1) * 50;
			}
			source = ptr + 1;
		}
	}
	return(point);
}

/***************************************************************************
 * Plain_Text_Print -- Prints text without using a color scheme            *
 *                                                                         *
 * INPUT:                                                                  *
 *      text      text to print                                            *
 *      x,y      coords to print at                                        *
 *      fore      desired foreground color                                 *
 *      back      desired background color                                 *
 *      flag      text print control flags                                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      Do not use the gradient control flag with this routine!  For       *
 *      a gradient appearance, use Fancy_Text_Print.                       *
 *      Despite this routine's name, it is actually faster to call         *
 *      Fancy_Text_Print than this routine.                                *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/05/1996 BRR : Created.                                             *
 *=========================================================================*/

/// <summary>
/// Prints wide text in a colour scheme's colour, honouring tabs and clipping it to the width given.
/// </summary>
Point2D Conquer_Clip_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, ColorScheme * fore, int back, TextPrintType flag, int width, int const * tabs) // RA2: at 0x497E80
{
	RGBClass rgb = fore->HSV;
	unsigned color = ((rgb.Get_Red() >> DSurface::RedLeft) << DSurface::RedRight) | ((rgb.Get_Green() >> DSurface::GreenLeft) << DSurface::GreenRight) | ((rgb.Get_Blue() >> DSurface::BlueLeft) << DSurface::BlueRight);
	return(Conquer_Clip_Text_Print(text, surface, rect, pt, color, back, flag, width, tabs));
}


/// <summary>
/// Fetches a font remap palette for a solid color.
/// This routine builds the small remap table that the font printing routines expect, filled
/// so that the text prints in the requested color over a transparent background.
/// </summary>
/// <param name="color">The color to print the text in.</param>
/// <remarks>The palette is a shared buffer, so it only lasts until the next call to this
/// routine.</remarks>
Point2D Plain_Text_Print(wchar_t const * text, Surface & surface, Rect const & rect, Point2D const & pt, unsigned fore, unsigned back, TextPrintType flag, int scheme, int unused) // RA2: at 0x497F40
{
	if (scheme == -1) {
		return(Simple_Text_Print(text, surface, rect, pt, fore, back, flag, unused));
	}
	return(Simple_Text_Print(text, surface, rect, pt, ColorSchemes[scheme], (int)back, flag, unused));
}


/// <summary>
/// As above, for callers holding pointers (the same code).
/// </summary>
Point2D Plain_Text_Print(wchar_t const * text, Surface * surface, Rect const * rect, Point2D const * pt, unsigned fore, unsigned back, TextPrintType flag, int scheme, int unused) // RA2: at 0x498050
{
	if (scheme == -1) {
		return(Simple_Text_Print(text, *surface, *rect, *pt, fore, back, flag, unused));
	}
	return(Simple_Text_Print(text, *surface, *rect, *pt, ColorSchemes[scheme], (int)back, flag, unused));
}
