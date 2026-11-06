/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 ra2-decomp contributors
 *
 * Renamed and ordered as in RA2 1.006 by RedAlert2 contributors, 2026.
 ******************************************************************************/

#include "always.h"

#include "bitfont.h"

#include "ccfile.h"
#include "dsurface.h"
#include "rgb.h"
#include "surface.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>


/*
**	The state of a font with nothing loaded.
*/
static inline void Reset_Defaults(BitFont * font)
{
	font->Data = 0;
	font->Color = 0x7FFF;
	font->ShadowColor = 0x3555;
	font->TabWidth = 0x40;
	font->IsBold = true;
	font->IsClipped = true;
	font->Stride = 1;
	font->LineHeight = 1;
	font->TabOrigin = 0;
	font->Buffer = 0;
	font->MissingChar = 0;
	SetRect((RECT *)&font->Bounds, 0, 0, 0, 0);
	font->Spacing = 1;
}


BitFont::BitFont(char const * filename)
{
	Reset_Defaults(this);
	Data = Load_Data(filename);
	if (Data) {
		Stride = Data->Stride;
		LineHeight = Data->Lines;
		Make_Missing_Char();
	} else {
		Reset_Defaults(this);
	}
}


BitFont::~BitFont(void)
{
	if (MissingChar) free(MissingChar);
	if (Data) Data->Destroy();
}


/*
**	The header of a "fonT" file: the symbol table follows it.
*/
struct FontHeaderType
{
	int Magic;
	int Unknown_4;
	int FontWidth;
	int Stride;
	int Unknown_10;
	int Lines;
	int Count;
};

/*
**	The header of a "FoNt" file: the symbol table is given as ranges of characters.
*/
struct FontHeader2Type
{
	int Magic;
	int FontWidth;
	int Stride;
	int FontHeight;
	int Lines;
	int RangeCount;
	int SymbolDataSize;
	int RangeOffset;
	int BitmapOffset;
};

/*
**	Consecutive characters whose glyphs follow each other from Index on.
*/
struct FontRangeType
{
	unsigned short Index;
	int First;
	int Last;
};


/*
**	Reads a font file; null if it cannot be read.
*/
BitFont::InternalData * BitFont::Load_Data(char const * filename)
{
	InternalData * data = NULL;
	FontRangeType * ranges;
	unsigned index;
	int size;
	CCFileClass * file = new CCFileClass(filename);

	if (file != NULL) {
		if (file->Is_Available() && file->Open(FileClass::READ)) {
			data = new InternalData;
			if (data != NULL) {
				FontHeaderType header;
				FontHeader2Type header2;
				memset(data, 0, sizeof(InternalData));
				data->SymbolTable = new unsigned short [0x10000];
				if (data->SymbolTable != NULL) {
					memset(data->SymbolTable, 0, 0x10000 * sizeof(unsigned short));
					if (file->Read(&header, sizeof(header)) == sizeof(header)) {
						if (header.Magic == 0x546E6F66) {
							data->FontHeight = header.Stride;
							data->Lines = header.Lines;
							data->Stride = header.Stride;
							data->FontWidth = header.FontWidth;
							data->SymbolDataSize = header2.Magic; // the dword after the 28 bytes read, as the executable reads it
							data->Count = header.Count;
							if (file->Read(data->SymbolTable, 0x10000 * sizeof(unsigned short)) != 0x10000 * sizeof(unsigned short)) goto failed;
						} else if (header.Magic == 0x744E6F46) {
							file->Seek(0, SEEK_SET);
							if (file->Read(&header2, sizeof(header2)) != sizeof(header2)) goto failed;
							data->Lines = header2.Lines;
							data->Stride = header2.Stride;
							data->SymbolDataSize = header2.SymbolDataSize;
							data->FontHeight = header2.FontHeight;
							data->FontWidth = header2.FontWidth;
							ranges = (FontRangeType *)malloc(header2.RangeCount * sizeof(FontRangeType));
							if (ranges == NULL) goto failed;
							file->Seek(header2.RangeOffset, SEEK_SET);
							if (file->Read(ranges, header2.RangeCount * sizeof(FontRangeType)) != (int)(header2.RangeCount * sizeof(FontRangeType))) goto failed;
							data->Count = 0;
							for (index = 0; index < (unsigned)header2.RangeCount; index++) {
								unsigned short symbol = ranges[index].Index + 1;
								for (int ch = ranges[index].First; ch <= ranges[index].Last; ch++) {
									data->SymbolTable[ch] = symbol++;
									data->Count++;
								}
							}
							free(ranges);
							file->Seek(header2.BitmapOffset, SEEK_SET);
						} else {
							goto failed;
						}
						size = data->Count * data->SymbolDataSize;
						data->ValidSymbolCount = 0;
						for (int i = 0; i < 0x10000; i++) {
							if (data->SymbolTable[i] != 0) data->ValidSymbolCount++;
						}
						data->Bitmaps = new char [size];
						if (data->Bitmaps != NULL && file->Read(data->Bitmaps, size) == size) goto loaded;
					}
				}
failed:
				if (data->SymbolTable != NULL) delete data->SymbolTable;
				if (data->Bitmaps != NULL) delete data->Bitmaps;
				delete data;
				data = NULL;
			}
		}
loaded:
		delete file;
	}
	return(data);
}


void BitFont::InternalData::Destroy(void)
{
	if (SymbolTable) delete SymbolTable;
	if (Bitmaps) delete Bitmaps;
	delete this;
}


bool BitFont::Load(char const * filename)
{
	Data = Load_Data(filename);
	if (Data) {
		Stride = Data->Stride;
		LineHeight = Data->Lines;
		Make_Missing_Char();
		return true;
	}
	return false;
}


unsigned short BitFont::Set_Color(unsigned short color)
{
	unsigned short old = Color;
	Color = color;
	return old;
}


unsigned short BitFont::Set_Shadow_Color(unsigned short color)
{
	ShadowColor = color;
	return Color;
}


bool BitFont::Set_Clip(bool clipped)
{
	bool old = IsClipped;
	IsClipped = clipped;
	return old;
}


void BitFont::Set_Bounds(LTRBStruct * bounds)
{
	if (bounds) {
		Bounds.Left = bounds->Left;
		Bounds.Top = bounds->Top;
		Bounds.Right = bounds->Right;
		Bounds.Bottom = bounds->Bottom;
	} else {
		Bounds.Bottom = 0;
		Bounds.Right = 0;
		Bounds.Top = 0;
		Bounds.Left = 0;
	}
}


bool BitFont::Set_Bold(bool bold)
{
	bool old = IsBold;
	IsBold = bold;
	return old;
}


/*
**	Measures text, wrapping lines at spaces where they would grow wider than maxwidth
**	(0: no wrapping). Either result pointer may be null.
*/
bool BitFont::Text_Dimensions(wchar_t const * text, int * width, int * height, int maxwidth)
{
	int break_width = 0;
	if (!Data || !text) {
		if (height) *height = 0;
		if (width) *width = 0;
		return false;
	}
	int max_line_width = 0;
	int line_width = 0;
	int line_height = LineHeight;
	int total_height = line_height;
	int count = 0;
	wchar_t const * break_text = 0;
	unsigned short previous = 0;
	unsigned short ch = *text++;
	while (ch) {
		switch (ch) {
		case 10:
		case 13:
			if (previous != 13) {
				line_width = 0;
				count = 0;
				total_height += line_height;
			}
			break;
		case 9:
			line_width += TabWidth;
			line_width -= line_width % TabWidth;
			break;
		case 32:
			break_text = text;
			break_width = max_line_width;
		default: {
			unsigned char * bitmap = Char_Data(ch);
			if (!bitmap) bitmap = (unsigned char *)MissingChar;
			if (bitmap) {
				int char_width = *bitmap;
				++count;
				line_width += char_width + Spacing;
				if (maxwidth && line_width > maxwidth) {
					if (break_text) {
						if (count > 1) {
							text = break_text;
							max_line_width = break_width;
						}
					} else {
						if (count > 1) {
							line_width -= char_width + Spacing;
							--text;
						}
						if (line_width > max_line_width) max_line_width = line_width;
					}
					line_width = 0;
					count = 0;
					break_text = 0;
					total_height += line_height;
				} else if (line_width > max_line_width) max_line_width = line_width;
			}
			break;
		}
		}
		previous = ch;
		ch = *text++;
	}
	if (width) *width = max_line_width;
	if (height) *height = total_height;
	return true;
}


int BitFont::String_Pixel_Width(wchar_t const * text, int maxwidth)
{
	Text_Dimensions(text, &maxwidth, 0, maxwidth);
	return maxwidth;
}


int BitFont::Bold_String_Pixel_Width(wchar_t const * text, int maxwidth)
{
	if (IsBold) {
		Spacing = 2;
		Text_Dimensions(text, &maxwidth, 0, maxwidth);
		Spacing = 1;
	} else {
		Text_Dimensions(text, &maxwidth, 0, maxwidth);
	}
	return maxwidth;
}


/*
**	The number of characters of text that fit in maxwidth, at most maxchars (0: no
**	limit). With wrap, a line break ends the count and a word that does not fit is left out.
*/
int BitFont::Chars_In_Width(wchar_t const * text, int maxwidth, int maxchars, int wrap)
{
	int width = 0;
	int count = 0;
	if (!Data || !text) return 0;
	{
		unsigned short ch = *text;
		while (ch) {
			switch (ch) {
				case 9:
					width += TabWidth;
					width -= width % TabWidth;
					break;
				case 10:
				case 13:
					break;
				default: {
					unsigned char * bitmap = Char_Data(ch);
					if (!bitmap) bitmap = (unsigned char *)MissingChar;
					if (bitmap) width += *bitmap + Spacing;
					break;
				}
			}
			++count;
			if (width > maxwidth) {
				--count;
				if (wrap && ch > 32) {
					--text;
					int split = count;
					while (split && *text > 32) {
						--split;
						--text;
					}
					if (split) count = split;
				}
				break;
			}
			if (maxchars && count == maxchars) break;
			if (wrap && (ch == 13 || ch == 10)) break;
			ch = *++text;
		}
	}
	return count;
}


bool BitFont::Bold_Text_Dimensions(wchar_t const * text, int * width, int * height, int maxwidth)
{
	if (IsBold) {
		Spacing = 2;
		bool r = Text_Dimensions(text, width, height, maxwidth);
		Spacing = 1;
		return r;
	}
	return Text_Dimensions(text, width, height, maxwidth);
}


int BitFont::Set_Tab_Width(int width)
{
	int old = TabWidth;
	TabWidth = width;
	return old;
}


void BitFont::Set_Tab_Origin(int x)
{
	TabOrigin = x;
}


#define BLIT_CLIPPED(mask) \
	if (cx >= clipL && cx <= clipR && (bits & (mask))) *d = color; \
	d++; cx++; w--;

#define BLIT_PLAIN(mask) \
	if (bits & (mask)) *d = color; \
	d++; w--;

/*
**	Draws one character at x, y in color (-1: the font's colour) and returns the x of the
**	next character. A tab moves to the next tab stop.
*/
int BitFont::Print_Char(unsigned short ch, int x, int y, int color_override)
{
	if (ch == 9) {
		x += TabWidth;
		return x - (x - TabOrigin) % TabWidth;
	}
	unsigned short color = Color;
	if (color_override != -1) color = color_override;
	unsigned char * glyph = Char_Data(ch);
	if (!glyph) {
		glyph = (unsigned char *)Missing_Char(&color);
		if (!glyph) return x;
	}
	if (!Buffer) return x;
	unsigned char * data = glyph + 1;
	int width = *glyph;
	int height = Data->FontHeight;
	if (IsClipped) {
		LTRBStruct rect;
		LTRBStruct clipped;
		SetRect((RECT *)&rect, x, y, x + width - 1, y + height - 1);
		if (!Clip_Rect(&clipped, &rect, &Bounds)) return Spacing + width + x;
		if (rect.Left < clipped.Left || rect.Right > clipped.Right || rect.Top < clipped.Top || rect.Bottom > clipped.Bottom) {
			int skip = clipped.Top - rect.Top;
			int rows = clipped.Bottom - clipped.Top + 1;
			y += skip;
			data += Stride * skip;
			short * line = Buffer + (Pitch * y + x);
			int clipL = clipped.Left;
			int clipR = clipped.Right;
			while (rows--) {
				int w = width;
				int cx = x;
				short * d = line;
				unsigned char * p = data;
				do {
					unsigned char bits = *p++;
					if (bits == 0) {
						d += 8;
						w -= 8;
						cx += 8;
						continue;
					}
					BLIT_CLIPPED(0x80) if (w <= 0) break;
					BLIT_CLIPPED(0x40) if (w <= 0) break;
					BLIT_CLIPPED(0x20) if (w <= 0) break;
					BLIT_CLIPPED(0x10) if (w <= 0) break;
					BLIT_CLIPPED(8) if (w <= 0) break;
					BLIT_CLIPPED(4) if (w <= 0) break;
					BLIT_CLIPPED(2) if (w <= 0) break;
					BLIT_CLIPPED(1)
				} while (w > 0);
				data += Stride;
				line += Pitch;
				y++;
			}
			return Spacing + width + x;
		}
	}
	short * line = Buffer + (Pitch * y + x);
	while (height--) {
		int w = width;
		unsigned char * p = data;
		short * d = line;
		do {
			unsigned char bits = *p++;
			if (bits == 0) {
				d += 8;
				w -= 8;
				continue;
			}
			BLIT_PLAIN(0x80) if (w <= 0) break;
			BLIT_PLAIN(0x40) if (w <= 0) break;
			BLIT_PLAIN(0x20) if (w <= 0) break;
			BLIT_PLAIN(0x10) if (w <= 0) break;
			BLIT_PLAIN(8) if (w <= 0) break;
			BLIT_PLAIN(4) if (w <= 0) break;
			BLIT_PLAIN(2) if (w <= 0) break;
			BLIT_PLAIN(1)
		} while (w > 0);
		data += Stride;
		line += Pitch;
	}
	return Spacing + width + x;
}


/*
**	Prints text, at most maxchars characters (0: all). With shown set, only that many
**	characters are drawn, the last eight brightened towards white, and the result is the x
**	the whole text would end at.
*/
int BitFont::Print_Typed(wchar_t const * text, int x, int y, int maxchars, int shown)
{
	int count = 0;
	unsigned short ch = *text++;
	unsigned short saved = Color;
	if (ch) {
		int t = shown - 1;
		int ratio = (9 - shown) * 31;
		while (ch) {
			if (shown) {
				if (count >= shown) {
					Color = saved;
					int width;
					Text_Dimensions(text, &width, 0, 0);
					return x + width;
				}
				if (t < 8) {
					unsigned char r = (saved >> DSurface::RedRight) << DSurface::RedLeft;
					unsigned char b = (saved >> DSurface::BlueRight) << DSurface::BlueLeft;
					unsigned char g = (saved >> DSurface::GreenRight) << DSurface::GreenLeft;
					RGBClass c = RGBClass(r, g, b);
					c.Adjust(ratio, WhiteColor);
					Color = (unsigned short)(((c.Get_Green() >> DSurface::GreenLeft) << DSurface::GreenRight)
						| ((c.Get_Blue() >> DSurface::BlueLeft) << DSurface::BlueRight)
						| ((c.Get_Red() >> DSurface::RedLeft) << DSurface::RedRight));
				}
				count++;
				ratio += 31;
				t--;
			}
			if (ch != 13 && ch != 10) {
				x = Print_Char(ch, x, y, -1);
			}
			if (maxchars && --maxchars == 0) break;
			ch = *text++;
		}
	}
	Color = saved;
	return x;
}


unsigned char * BitFont::Char_Data(wchar_t ch)
{
	unsigned char * result = 0;
	InternalData * data = Data;
	if (data) {
		int index = data->SymbolTable[ch];
		if (index) {
			result = (unsigned char *)(data->SymbolDataSize * (index - 1) + data->Bitmaps);
		}
	}
	return result;
}


/*
**	Builds the glyph drawn for characters the font lacks: an inverted 'X'.
*/
void BitFont::Make_Missing_Char(void)
{
	MissingChar = malloc(Data->FontHeight * Data->Stride + 2);
	if (Data) {
		unsigned char * src = Char_Data(0x58);
		if (src) {
			*(unsigned char *)MissingChar = *src++;
			unsigned char * dst = (unsigned char *)MissingChar + 1;
			for (unsigned y = 0; y < (unsigned)Data->FontHeight; y++) {
				for (unsigned x = 0; x < (unsigned)Data->Stride; x++) {
					*dst++ = ~*src++;
				}
			}
		}
	}
}


void * BitFont::Missing_Char(unsigned short * color)
{
	if (color) *color ^= 0x5555;
	return MissingChar;
}


void BitFont::Widen_Space(void)
{
	if (Data) {
		LineHeight++;
		unsigned char * p = 0;
		int index = Data->SymbolTable[0x20];
		if (index) p = (unsigned char *)(Data->SymbolDataSize * (index - 1) + Data->Bitmaps);
		(*p)++;
	}
}


/*
**	Gives the ideographic space (U+3000) twice the width of a space.
*/
void BitFont::Set_Wide_Space(void)
{
	if (Data) {
		LineHeight++;
		unsigned char * space = 0;
		int index = Data->SymbolTable[0x20];
		if (index) space = (unsigned char *)(Data->SymbolDataSize * (index - 1) + Data->Bitmaps);
		unsigned char * dest = 0;
		index = Data->SymbolTable[0x3000];
		if (index) dest = (unsigned char *)(Data->SymbolDataSize * (index - 1) + Data->Bitmaps);
		if (dest) {
			if (space) *dest = *space << 1;
			else *dest = (unsigned char)Data->FontWidth;
		}
	}
}


/*
**	The part of rect inside bounds; false, with an empty result, if they do not overlap.
*/
bool BitFont::Clip_Rect(LTRBStruct * out, LTRBStruct * rect, LTRBStruct * bounds)
{
	if (rect->Top > bounds->Bottom || bounds->Top > rect->Bottom || rect->Left > bounds->Right || bounds->Left > rect->Right) {
		out->Left = 0;
		out->Top = 0;
		out->Right = -1;
		out->Bottom = -1;
		return false;
	}
	*out = *rect;
	if (out->Top < bounds->Top) out->Top = bounds->Top;
	if (out->Left < bounds->Left) out->Left = bounds->Left;
	if (out->Bottom > bounds->Bottom) out->Bottom = bounds->Bottom;
	if (out->Right > bounds->Right) out->Right = bounds->Right;
	return true;
}


/*
**	Locks the surface for drawing; the bounds default to the whole surface and are
**	otherwise clipped to it.
*/
void BitFont::Lock(Surface * surface)
{
	Buffer = (short *)surface->Lock(Point2D(0, 0));
	Pitch = (unsigned)surface->Stride() >> 1;
	LTRBStruct area;
	area.Left = 0;
	area.Top = 0;
	area.Right = surface->Get_Width() - 1;
	area.Bottom = surface->Get_Height() - 1;
	if (!(Bounds.Right | Bounds.Bottom | Bounds.Top | Bounds.Left)) {
		Bounds.Left = area.Left;
		Bounds.Top = area.Top;
		Bounds.Right = area.Right;
		Bounds.Bottom = area.Bottom;
	} else {
		Clip_Rect(&Bounds, &Bounds, &area);
	}
}


void BitFont::Unlock(Surface * surface)
{
	surface->Unlock();
	Buffer = 0;
	Pitch = 0;
}


int BitFont::Char_Width(wchar_t ch)
{
	InternalData * data = Data;
	if (data) {
		int index = data->SymbolTable[ch];
		if (index) {
			unsigned char * bitmap = (unsigned char *)(data->SymbolDataSize * (index - 1) + data->Bitmaps);
			if (bitmap) return *bitmap;
		}
	}
	return 0;
}


void BitFont::Reset(void)
{
	Data = 0;
	TabOrigin = 0;
	Buffer = 0;
	MissingChar = 0;
	Color = 0x7FFF;
	ShadowColor = 0x3555;
	TabWidth = 0x40;
	IsBold = true;
	IsClipped = true;
	Stride = 1;
	LineHeight = 1;
	SetRect((RECT *)&Bounds, 0, 0, 0, 0);
	Spacing = 1;
}
