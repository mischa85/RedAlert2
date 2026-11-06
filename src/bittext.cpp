/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 ra2-decomp contributors
 *
 * Renamed and ordered as in RA2 1.006 by RedAlert2 contributors, 2026.
 ******************************************************************************/

#include "always.h"

#include "bittext.h"

#include "bitfont.h"
#include "dsurface.h"
#include "rgb.h"

/*
**	The hicolor pixel of a colour.
*/
inline int Pixel_From_RGB(RGBClass const & rgb)
{
	return(DSurface::Build_Hicolor_Pixel(rgb.Get_Red(), rgb.Get_Green(), rgb.Get_Blue()));
}

/*
**	The x offset of a line of the given width in a box w wide: centred (flags 1) or
**	right-aligned (flags 2).
*/
inline int Align_Offset(int w, int width, int flags)
{
	int offset = 0;
	if (w > width) {
		if (flags & 1) {
			offset = (w - width) / 2;
		} else if (flags & 2) {
			offset = w - width;
		}
	}
	return offset;
}

/*
**	The glyph of a character, or of '?' if the font lacks it.
*/
inline unsigned char * Char_Bitmap(BitFont * font, wchar_t ch)
{
	unsigned char * bmp = font->Char_Data(ch);
	if (!bmp) {
		bmp = font->Char_Data('?');
	}
	return bmp;
}

/*
**	Draws the characters from start to end; false once shown characters have been drawn.
*/
inline bool Draw_Line(BitFont * font, wchar_t const * & start, wchar_t const * end, int x, int y, int w, int width, int flags, int & index, int & xpos, int color, int shown, int fade)
{
	xpos = x + Align_Offset(w, width, flags);
	while (start < end) {
		index++;
		int c = color;
		if (shown) {
			if (index >= shown) {
				c = -1;
			} else {
				int remain = shown - index - 1;
				if (remain < fade) {
					RGBClass rgb = DSurface::Deconstruct_Hicolor_Pixel((unsigned short)color);
					rgb.Adjust((255 / fade) * (fade - remain), WhiteColor);
					c = Pixel_From_RGB(rgb);
				}
			}
		}
		if (c == -1) {
			return false;
		}
		xpos = font->Print_Char(*start++, xpos, y, c);
	}
	return true;
}

BitText::BitText(void)
{
	Create_Font();
}

BitText::~BitText(void)
{
	Destroy_Font();
}

int BitText::Print_Char(BitFont * font, Surface * surface, unsigned short ch, int x, int y)
{
	int result = x;
	if (font) {
		font->Lock(surface);
		font->Set_Tab_Origin(x);
		result = font->Print_Char(ch, x, y, -1);
		font->Unlock(surface);
	}
	return result;
}

int BitText::Print(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int maxchars, int shown)
{
	int result = x;
	if (font) {
		font->Lock(surface);
		font->Set_Tab_Origin(x);
		result = font->Print_Typed(text, x, y, maxchars, shown);
		font->Unlock(surface);
	}
	return result;
}

int BitText::Print_Bold(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int maxchars)
{
	int result = 0;
	if (font) {
		if (font->IsBold) {
			font->Spacing = 2;
			font->Lock(surface);
			font->Set_Tab_Origin(x);
			font->Print_Typed(text, x, y, maxchars, 0);
			font->Unlock(surface);
			int x1 = x + 1;
			font->Lock(surface);
			font->Set_Tab_Origin(x1);
			result = font->Print_Typed(text, x1, y, maxchars, 0);
			font->Unlock(surface);
			font->Spacing = 1;
		} else {
			font->Lock(surface);
			font->Set_Tab_Origin(x);
			result = font->Print_Typed(text, x, y, maxchars, 0);
			font->Unlock(surface);
		}
	}
	return result;
}

/*
**	Draws text wrapped to w (0: no wrapping), stopping at h pixels (0: no limit). With shown
**	set, only that many characters are drawn, the last fade of them brightened towards white.
*/
bool BitText::Draw_Text(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade)
{
	int width = 0;
	if (!font || !text) {
		return false;
	}

	font->Lock(surface);
	font->Set_Tab_Origin(x);

	wchar_t const * start = text;
	int height = 0;
	int savedwidth = 0;
	wchar_t const * saved = 0;
	wchar_t prev = 0;
	int count = 0;
	int color = font->Color;
	int index = 0;
	int xpos;
	wchar_t ch = *text;

	while (ch) {
		switch (ch) {
			case '\r':
			case '\n':
				if (prev != '\r') {
					if (!Draw_Line(font, start, text, x, y, w, width, flags, index, xpos, color, shown, fade)) {
						font->Unlock(surface);
						return true;
					}
					width = 0;
					count = 0;
					height += font->LineHeight;
					if (h && height >= h) {
						font->Unlock(surface);
						return true;
					}
					y += font->LineHeight;
				}
				start++;
				break;

			case '\t':
				width += font->TabWidth;
				width -= width % font->TabWidth;
				break;

			case ' ':
				savedwidth = width;
				saved = text;

			default:
				{
					unsigned char * bmp = Char_Bitmap(font, ch);
					if (!bmp) {
						break;
					}
					count++;
					int cw = *bmp + font->Spacing;
					width += cw;
					if (w && width > w) {
						if (saved) {
							if (count > 1) {
								width = savedwidth;
								text = saved;
							}
						} else if (count > 1) {
							width -= cw;
							text--;
						}
						xpos = x + Align_Offset(w, width, flags);
						while (start < text) {
							index++;
							int c = color;
							if (shown) {
								if (index >= shown) {
									c = -1;
								} else {
									int remain = shown - index - 1;
									if (remain < fade) {
										RGBClass rgb = DSurface::Deconstruct_Hicolor_Pixel((unsigned short)color);
										rgb.Adjust((255 / fade) * (fade - remain), WhiteColor);
										c = Pixel_From_RGB(rgb);
									}
								}
							}
							if (c == -1) {
								font->Unlock(surface);
								return true;
							}
							xpos = font->Print_Char(*start++, xpos, y, c);
						}
						if (text == saved) {
							saved = 0;
							start++;
						}
						width = 0;
						count = 0;
						height += font->LineHeight;
						if (h && height >= h) {
							font->Unlock(surface);
							return true;
						}
						y += font->LineHeight;
					}
				}
				break;
		}
		prev = ch;
		text++;
		ch = *text;
	}

	xpos = x + Align_Offset(w, width, flags);
	while (start < text) {
		index++;
		int c = color;
		if (shown) {
			if (index >= shown) {
				c = -1;
			} else {
				int remain = shown - index - 1;
				if (remain < fade) {
					RGBClass rgb = DSurface::Deconstruct_Hicolor_Pixel((unsigned short)color);
					rgb.Adjust((255 / fade) * (fade - remain), WhiteColor);
					c = Pixel_From_RGB(rgb);
				}
			}
		}
		if (c == -1) {
			font->Unlock(surface);
			return true;
		}
		xpos = font->Print_Char(*start++, xpos, y, c);
	}
	font->Unlock(surface);
	return true;
}

bool BitText::Draw_Shadow(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade)
{
	if (!font) {
		return false;
	}
	font->Set_Tab_Origin(x);
	unsigned short c = font->ShadowColor;
	unsigned short old = font->Set_Color(c);
	Draw_Text(font, surface, text, x + 1, y + 1, w, h, flags, shown, fade);
	font->Set_Color(old);
	return Draw_Text(font, surface, text, x, y, w, h, flags, shown, fade);
}

bool BitText::Draw_Bold(BitFont * font, Surface * surface, wchar_t const * text, int x, int y, int w, int h, int flags, int shown, int fade)
{
	if (!font) {
		return false;
	}
	font->Set_Tab_Origin(x);
	bool result;
	if (font->IsBold) {
		font->Spacing = 2;
		Draw_Text(font, surface, text, x, y, w, h, flags, shown, fade);
		x++;
		result = Draw_Text(font, surface, text, x, y, w, h, flags, shown, fade);
		font->Spacing = 1;
	} else {
		result = Draw_Text(font, surface, text, x, y, w, h, flags, shown, fade);
	}
	return result;
}


void Create_Font(void)
{
	BitFont::Instance = new BitFont("GAME.FNT");
}


void Destroy_Font(void)
{
	Release_Font(BitFont::Instance);
}


void __stdcall Release_Font(BitFont * font)
{
	delete font;
	if (font == BitFont::Instance) {
		BitFont::Instance = 0;
	}
}
