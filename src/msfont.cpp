/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "msfont.h"

#include "_mixfile.h"
#include "_palette.h"
#include "_surface.h"
#include "ccfile.h"
#include "convert.h"
#include "data.h"
#include "dbgprint.h"
#include "draw.h"
#include "dsaudio.h"
#include "dsurface.h"
#include "globals.h"
#include "goptions.h"
#include "mixfile.h"
#include "palette.h"
#include "shapeset.h"
int __fastcall AudioDeviceAvailable(void); // RA2: whether the audio library has a device

#include <cstring>


static int InstanceCount = 0;

static struct {
	void * Sample;
	bool AllocLoaded;
} Sounds[3];


/// <summary>
/// Creates the standard menu font.
/// This routine loads the common font used throughout the menu screens, colored either to
/// suit the side being played or with the neutral palette.
/// </summary>
/// <param name="use_side_palette">Should the font take its colors from the side palette?</param>
MSFont::MSFont(bool use_side_palette) :
	Red(0),
	Green(0),
	Blue(0)
{
	Init(use_side_palette); // RA2
	InstanceCount++;
}


/// <summary>
/// Destroys the font.
/// The glyph data and the typing sound effects are shared by every font in play, so they
/// are only let go once the last font has been destroyed.
/// </summary>
MSFont::~MSFont(void)
{
	InstanceCount--;
	if (InstanceCount == 0) {
		for (int i = 0; i < ARRAY_SIZE(Sounds); i++) {
			if (Sounds[i].Sample != NULL) {
				Sounds[i].Sample = NULL;
				Sounds[i].AllocLoaded = false; // RA2
			}
		}

		if (Drawer != NULL) {
			delete Drawer;
		}

		if (FontFile != NULL) {
			if (AllocLoaded == true) {
				delete FontFile;
			}
		}

		FontFile = NULL;
		AllocLoaded = false;
	}
}


/// <summary>
/// Loads the font shapes and the palette that colors them.
/// This is the routine every constructor leans on. It fetches the glyph shapes from the
/// mixfiles or from disk, builds the drawer that the glyphs are remapped through, and sees
/// to it that the typing sound effects are on hand.
/// </summary>
/// <param name="file_name">The name of the font shape file to load.</param>
/// <param name="palette_name">The name of the palette to color the glyphs with.</param>
/// <returns>bool; Was the font made ready for use?</returns>
/// <remarks>The global CCPalette is overwritten with this font's palette.</remarks>
bool MSFont::Init(bool use_side_palette) // RA2
{
	CCFileClass file;

	if (InstanceCount == 0) { // RA2
		AllocLoaded = false;
		FontFile = (ShapeSet *)MFCD::Retrieve("FULLFNT3.SHP");

		if (FontFile == NULL) { // RA2
			file.Set_Name("FULLFNT3.SHP");
			FontFile = (ShapeSet *)Load_Alloc_Data(file);
			AllocLoaded = true;
			DebugString("MSFont: AllocLoaded FULLFNT3.SHP\n");
		}

		if (FontFile == NULL) {
			return(false);
		}

		if (use_side_palette == true) {
			file.Set_Name("SIDEFNT3.PAL");
		} else {
			file.Set_Name("FULLFNT3.PAL");
		}
		PaletteClass * palette = (PaletteClass *)Load_Alloc_Data(file);

		if (palette == NULL) {
			return(false);
		}

		for (int gindex = 0; gindex < 256; gindex++) {
			CCPalette[gindex] = RGBClass(
				((unsigned char *)*palette)[gindex*3]<<2,
				((unsigned char *)*palette)[gindex*3+1]<<2,
				((unsigned char *)*palette)[gindex*3+2]<<2);
		}

		unsigned char red = (*palette)[68].Get_Red()<<2;
		unsigned char blue = (*palette)[68].Get_Blue()<<2;
		unsigned char green = (*palette)[68].Get_Green()<<2;

		Red = red;
		Blue = blue;
		Green = green;

		Color = ((((*palette)[68].Get_Green()<<2) >> DSurface::GreenLeft) << DSurface::GreenRight)
			| ((((*palette)[68].Get_Blue()<<2) >> DSurface::BlueLeft) << DSurface::BlueRight)
			| ((((*palette)[68].Get_Red()<<2) >> DSurface::RedLeft) << DSurface::RedRight);

		delete palette;

		Drawer = new ConvertClass(CCPalette, CCPalette, *VisibleSurface);

		if (Drawer == NULL) {
			return(false);
		}

		FontWidth = FontFile->Get_Width();
		FontHeight = FontFile->Get_Height();
	}

	AudioDeviceAvailable(); // RA2: the font sounds are not loaded here
	return(true);
}


/// <summary>
/// Fetches the area a string will take up when printed.
/// Use this routine to center or otherwise place a block of text before drawing it.
/// Newlines are honored, so the rectangle spans every line of the string.
/// </summary>
/// <param name="rect">The rectangle to fill in. It is anchored at the origin, since it
/// describes the size of the text rather than where it will land.</param>
void MSFont::Get_String_Rect(wchar_t const * string, Rect & rect) // RA2
{
	int max_width = 0;

	if (string != NULL && wcslen(string) != 0) { // RA2

		int height = FontHeight;

		do {
			int width = 0;
			while (*string && *string != '\n') {
				width += Get_Character_Width(*string++);
			}

			if (width > max_width) {
				max_width = width;
			}

			if (*string == '\n') {
				string++;
				height += FontHeight;
			}

		} while (*string);

		rect.Set(0, 0, max_width + FontWidth, height);
		return;
	}

	rect.Set(0, 0, 0, 0);
}


/// <summary>
/// Fetches the printed width of a string.
/// Newlines are honored, so a multiple line string reports the width of its longest line.
/// </summary>
/// <returns>Returns with the width in pixels.</returns>
int MSFont::Get_String_Width(wchar_t const * string) // RA2
{
	int max_width = 0;

	if (string != NULL) {
		do {
			int width = 0;
			while (*string && *string != '\n') {
				width += Get_Character_Width(*string++);
			}

			if (*string == '\n') {
				string++;
			}

			if (width > max_width) {
				max_width = width;
			}

		} while (*string);
	}

	return(max_width);
}


/// <summary>
/// Fetches the printed width of a single character.
/// This is what the measuring and printing routines step along a string with, so the width
/// reported is an advance -- the gap that follows the glyph is included in it.
/// </summary>
/// <returns>Returns with the width in pixels. Characters below the space are worth
/// nothing.</returns>
int MSFont::Get_Character_Width(wchar_t character) // RA2
{
	unsigned char ch = (unsigned char)character; // RA2
	CharToOemBuff((LPCSTR)&ch, (LPSTR)&ch, 1);

	if (ch == ' ') { // RA2
		return(8);
	}

	if (ch > ' ') { // RA2
		int index = (0 >= ch - 33) ? 0 : ch - 33;
		index = (216 <= index) ? 216 : index;

		int shape_frame = index * 3;
		return(FontFile->Get_Rect(shape_frame + 2).Width + 1);
	}
	return(0);
}


/// <summary>
/// Draws a single character onto the surface.
/// This routine is used by the menu print animations, which walk a string one glyph at a
/// time so that the text appears to be typed out. Anything below the space character has
/// no glyph and is passed over.
/// </summary>
/// <param name="frame">The glyph frame to draw with. The print animations step through the
/// frames to fade a character in.</param>
/// <param name="do_sound">Should a typing sound accompany the character?</param>
void MSFont::Draw_Character(Surface * surface, wchar_t character, int x, int y, int frame, bool do_sound) // RA2
{
	char ch = (char)character; // RA2
	if (ch == 176) {
		DebugString("Denzil!\n");
	}

	CharToOemBuff((LPCSTR)&ch, (LPSTR)&ch, 1); // RA2

	if ((unsigned char)ch > ' ') { // RA2

		int index = (0 >= (unsigned char)ch - 33) ? 0 : (unsigned char)ch - 33; // RA2
		index = (216 <= index) ? 216 : index;

		int shape_frame = index * 3;

		Draw_Shape(*surface, *Drawer, FontFile, shape_frame + frame, Point2D(x - FontFile->Get_Rect(shape_frame + 2).X, y), surface->Get_Rect(), SHAPE_WIN_REL); // RA2: no typing sound
	}
}


/// <summary>
/// Draws a string onto the surface.
/// A newline in the string starts a fresh line back at the left edge it was handed. Unlike
/// the single character routine, this one prints silently.
/// </summary>
/// <param name="x">The left edge to begin each line of the string at.</param>
/// <param name="frame">The glyph frame to draw the text with.</param>
void MSFont::Draw_String(Surface * surface, wchar_t const * string, int x, int y, int frame) // RA2
{
	int current_x = x;

	while (*string) {
		if (*string == '\n') {
			current_x = x;
			y += FontHeight;
		} else {
			if (*string > ' ') {
				char source = (char)*string; // RA2
				char character;
				CharToOemBuff((LPCSTR)&source, (LPSTR)&character, 1);

				int index = (0 >= character - 33u) ? 0 : character - 33u; // RA2
				index = (216 <= index) ? 216 : index;

				int shape_frame = index * 3;

				Draw_Shape(*surface, *Drawer, FontFile, shape_frame + frame, Point2D(current_x - FontFile->Get_Rect(shape_frame + 2).X, y), surface->Get_Rect(), SHAPE_WIN_REL);
			}

			current_x += Get_Character_Width(*string);
		}

		string++;
	}
}
