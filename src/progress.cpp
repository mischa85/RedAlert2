/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "progress.h"
#include "bitfont.h"

#include "_convert.h"
#include "_mixfile.h"
#include "_surface.h"
#include "convert.h"
#include "data.h"
#include "dialog.h"
#include "draw.h"
#include "gscreen.h"
#include "init.h"
#include "language\language.h"
#include "lightcon.h"
#include "mixfile.h"
#include "ownrdraw.h"
#include "scheme.h"
#include "session.h"
#include "shapeset.h"
#include "surface.h"
#include "voc.h"
#include "windlg.h"
#include "srfcache.h" // RA2
#include "dsurface.h" // RA2

Surface * __fastcall Country_Flag(int country); // RA2: 0x4D1120, the flag image of a country (-2 random, -3 observer)
void __fastcall Inflate_Rect(Rect & rect, int amount); // RA2: 0x6F1B00, moves each edge in by amount
void __fastcall Shade_Rect(Rect const & rect, Surface & surface, int color, int alpha); // RA2: 0x5FEB60, blends a colour over the rectangle
Rect __fastcall Get_Title_Screen_Area(void); // RA2: 0x6F1F30
wchar_t * __fastcall Narrow_To_Wide(char const * narrow); // RA2: 0x6FA180, static buffer


ProgressScreenClass Progress;


/// <summary>
/// Constructs the progress screen object.
/// The screen starts out idle and with no artwork attached. Initialize and
/// Set_Graphic_Data supply both before anything can be displayed.
/// </summary>
ProgressScreenClass::ProgressScreenClass(void) : // RA2: ProgressScreenClass at 0x61ED00 matches only so
	MainProgress(0), // RA2: the RA2 fields too, as an initializer list
	String(NULL), // RA2: as above
	Shape(NULL), // RA2: as above
	BufferOwned(false), // RA2: as above
	Background(NULL), // RA2: as above
	IsActive(false), // RA2: as above
	PlayerCount(0), // RA2: as above
	Unused_62(false), // RA2: as above
	Dialog(NULL), // RA2: as above
	Pos(0, 0), // RA2: as above
	ShowFlags(false), // RA2: as above
	RowWidth(-1), // RA2: as above
	Percentage(0), // RA2: as above
	Side(-1) // RA2: as above
{
	for (int i = 0; i < MAX_PLAYERS; i++) {
		PlayerProgress[i] = 0;
	}
}


/// <summary>
/// Destroys the progress screen object.
/// </summary>
ProgressScreenClass::~ProgressScreenClass(void)
{
	if (BufferOwned) { // RA2: the shape is freed when it was loaded here
		delete [] (char *)Shape; // RA2: as above
		BufferOwned = false; // RA2: as above
	} // RA2: as above
	Shape = NULL; // RA2: as above
}

/// <summary>
/// RA2: prepares the progress screen for a job of the given size for a number of players. Without a dialog
/// the hidden surface is cleared for the full screen display.
/// </summary>
void ProgressScreenClass::Initialize(double progress, char count, HWND dialog)
{
	MainProgress = progress;
	PlayerCount = count;
	for (int i = 0; i < PlayerCount; i++) {
		PlayerProgress[i] = 0;
	}
	IsActive = true;
	if (dialog != NULL) {
		Dialog = dialog;
	} else {
		HiddenSurface->Fill(0);
	}
	String = NULL;
	Percentage = -1;
}


/// <summary>
/// RA2: ends the job; later progress requests are ignored.
/// </summary>
void ProgressScreenClass::End(void)
{
	IsActive = false;
} // RA2: End at 0x61EDF0 has no MainProgress


/// <summary>
/// RA2: releases the progress bar shape.
/// </summary>
void ProgressScreenClass::Free_Shape(void)
{
	if (BufferOwned) {
		delete [] (char *)Shape;
		BufferOwned = false;
	}
	Shape = NULL;
}


/// <summary>
/// RA2: sets the side whose colours the single player bar is drawn in (0 Allied, else Soviet).
/// </summary>
void ProgressScreenClass::Set_Side(int side)
{
	Side = side;
}


/// <summary>
/// RA2: the side set with Set_Side.
/// </summary>
int ProgressScreenClass::Get_Side(void)
{
	return(Side);
}


/// <summary>
/// RA2: the colour scheme of the side.
/// </summary>
ColorScheme * ProgressScreenClass::Side_Scheme(void)
{
	if (Side == 0) {
		return(Fetch_Scheme_By_Name("AlliedLoad"));
	}
	return(Fetch_Scheme_By_Name("SovietLoad"));
}


/// <summary>
/// RA2: the colour scheme of the loading text: grey for an observer in a network game, else the side's.
/// </summary>
ColorScheme * ProgressScreenClass::Text_Scheme(void)
{
	if ((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) && Session.Players[0]->Slot.SpectatorFlag == -1) {
		return(Fetch_Scheme_By_Name("LightGrey"));
	}
	if (Side == 0) {
		return(Fetch_Scheme_By_Name("AlliedLoad"));
	}
	return(Fetch_Scheme_By_Name("SovietLoad"));
}


/// <summary>
/// RA2: the colour scheme of a player's row: the observer colour for an observer in a network game, else the
/// player's colour.
/// </summary>
ColorScheme * ProgressScreenClass::Player_Scheme(int index)
{
	ColorScheme * scheme = NULL;
	if ((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) && Session.Players[index]->Slot.SpectatorFlag == -1) {
		scheme = ColorSchemes[Session.Color_Index_To_Scheme(-2)];
		return(scheme);
	}
	if (index >= 0 && index < MAX_PLAYERS) {
		scheme = ColorSchemes[Session.Color_Index_To_Scheme(Session.Players[index]->Get_Color())];
	}
	return(scheme);
}


/// <summary>
/// RA2: loads the progress bar shape and, when given, the title page to show behind the bars; the drawer
/// replaces the bars' colour scheme drawer when not NULL.
/// </summary>
void ProgressScreenClass::Set_Graphic_Data(char const * progbar, char const * background, ConvertClass * drawer)
{
	if (progbar != NULL) {
		if (Shape != NULL) {
			if (BufferOwned) {
				delete [] (char *)Shape;
				BufferOwned = false;
			}
			Shape = NULL;
		}
		Shape = (ShapeSet *)Load_Alloc_Data(progbar, BufferOwned);
		if (Shape != NULL) {
			if (background != NULL) {
				Background = background;
				Load_Title_Page(background, false);
			}
			Drawer = drawer;
		}
	}
}


/// <summary>
/// RA2: places the rows (centred on the title screen for -1,-1), and sets the single player's name, whether the
/// rows show the country flags and whether the bars are outlined.
/// </summary>
void ProgressScreenClass::Set_Layout(Point2D pt, wchar_t const * string, bool flags, bool outline)
{
	if (pt.X == -1 && pt.Y == -1) {
		int width = 0;
		int height = 0;
		Box_Size(&width, &height, PlayerCount, flags);
		Rect rect = Get_Title_Screen_Area();
		Pos = Point2D((rect.Width - width) / 2, (rect.Height - height) / 2);
	} else {
		Pos = pt;
	}
	String = string;
	ShowFlags = flags;
	IsFilled = outline;
}


/// <summary>
/// RA2: sets the width of the name column (-1 for the box's).
/// </summary>
void ProgressScreenClass::Set_Row_Width(int width)
{
	RowWidth = width;
}


/// <summary>
/// RA2: the size of the bar: the shape's first frame and a 3-pixel border.
/// </summary>
void ProgressScreenClass::Bar_Size(int * width, int * height)
{
	Rect rect = Shape->Get_Rect(0);
	*width = rect.Width + 6;
	*height = rect.Height + 6;
}


/// <summary>
/// RA2: the size of a country flag (the first country's; zero without one).
/// </summary>
void ProgressScreenClass::Flag_Size(int * width, int * height)
{
	Surface * flag = Country_Flag(0);
	if (flag == NULL) {
		*width = 0;
		*height = 0;
		return;
	}
	*width = flag->Get_Width();
	*height = flag->Get_Height();
}


/// <summary>
/// RA2: the size of a name column: fifteen "W"s in the progress font.
/// </summary>
void ProgressScreenClass::Text_Size(int * width, int * height)
{
	BitFont * font = Font_From_TPF(TextPrintType(0x46));
	wchar_t text[16];
	for (int i = 0; i < 15; i++) {
		text[i] = *Narrow_To_Wide("W");
	}
	text[15] = 0;
	*width = font->String_Pixel_Width(text, 0);
	*height = font->LineHeight;
}


/// <summary>
/// RA2: the size of the progress box: bar, flag and name side by side with margins, a row per player.
/// </summary>
void ProgressScreenClass::Box_Size(int * width, int * height, int rows, bool flags)
{
	int barwidth = 0;
	int barheight = 0;
	Bar_Size(&barwidth, &barheight);
	int flagwidth = 0;
	int flagheight = 0;
	if (flags) {
		Flag_Size(&flagwidth, &flagheight);
	}
	int textwidth = 0;
	int textheight = 0;
	Text_Size(&textwidth, &textheight);
	*width = textwidth + flagwidth + barwidth + 20;
	*height = barheight > flagheight ? barheight : flagheight;
	*height = *height > textheight ? *height : textheight;
	*width += 10;
	*height += 4;
	if (rows > 1) {
		*height *= rows;
	}
}


/// <summary>
/// RA2: the top of a row.
/// </summary>
int ProgressScreenClass::Row_Y(int y, int row, bool flags)
{
	int width = 0;
	int height = 0;
	Box_Size(&width, &height, 1, flags);
	return(y + height * row);
}


/// <summary>
/// RA2: the bar's offset from the top of its row (centred).
/// </summary>
int ProgressScreenClass::Bar_Offset(bool flags)
{
	int width = 0;
	int height = 0;
	Box_Size(&width, &height, 1, flags);
	Rect rect = Shape->Get_Rect(0);
	return((height - 6 - rect.Height) / 2);
}


/// <summary>
/// RA2: the flag's offset from the top of its row (centred).
/// </summary>
int ProgressScreenClass::Flag_Offset(bool flags)
{
	int width = 0;
	int height = 0;
	Box_Size(&width, &height, 1, flags);
	int flagwidth = 0;
	int flagheight = 0;
	Flag_Size(&flagwidth, &flagheight);
	return((height - flagheight) / 2);
}


/// <summary>
/// RA2: the name's offset from the top of its row (centred).
/// </summary>
int ProgressScreenClass::Text_Offset(bool flags)
{
	int width = 0;
	int height = 0;
	Box_Size(&width, &height, 1, flags);
	int textwidth = 0;
	int textheight = 0;
	Text_Size(&textwidth, &textheight);
	return((height - textheight) / 2);
}


/// <summary>
/// RA2: the players' mean progress as a fraction of the job.
/// </summary>
double ProgressScreenClass::Get_Current_Progress(void) const
{
	double progress = 0; // RA2: Get_Current_Progress at 0x61F8C0 matches only so
	int count = PlayerCount; // RA2: as above
	for (int i = 0; i < count; i++) { // RA2: as above
		progress += PlayerProgress[i]; // RA2: as above
	}
	return(progress / count / MainProgress); // RA2: as above
}


/// <summary>
/// RA2: draws a bar filled to the fraction, in the scheme's colours (DarkRed without one, Drawer when set),
/// outlined in the scheme's colour when IsFilled is set.
/// </summary>
void ProgressScreenClass::Draw_Bar(Point2D pt, double fraction, Surface * surface, ColorScheme * scheme)
{
	Rect rect = Shape->Get_Rect(0);
	rect.X = pt.X + 3;
	rect.Y = pt.Y + 3;
	Rect outline = rect;
	Inflate_Rect(outline, -3);
	ConvertClass * drawer;
	if (scheme == NULL) {
		scheme = Fetch_Scheme_By_Name("DarkRed");
		drawer = NormalDrawer;
	} else {
		drawer = scheme->Converter;
	}
	if (Drawer != NULL) {
		drawer = Drawer;
	}
	if (IsFilled) {
		RGBClass color = scheme->HSV;
		surface->Draw_Rect(outline, DSurface::Build_Hicolor_Pixel(color));
	}
	Rect bar = rect;
	bar.Width = rect.Width * fraction;
	Draw_Shape(*surface, *drawer, Shape, 0, Point2D(0, 0), bar, SHAPE_WIN_REL);
}


/// <summary>
/// RA2: copies a country flag to the hidden surface, magenta transparent.
/// </summary>
void ProgressScreenClass::Draw_Flag(Point2D pt, Surface * flag)
{
	int width = 0;
	int height = 0;
	Flag_Size(&width, &height);
	Rect rect(pt.X, pt.Y, width, height);
	short trans = (short)DSurface::Build_Hicolor_Pixel(255, 0, 255);
	if (HiddenSurface != NULL && !((DSurface *)HiddenSurface)->Is_Lost()) {
		SurfaceCache.DrawTrans(rect, *HiddenSurface, *flag, trans);
	}
}


/// <summary>
/// RA2: prints a name in the progress font, clipped to the name column (-1: the default column).
/// </summary>
void ProgressScreenClass::Draw_Name(Point2D pt, wchar_t const * name, ColorScheme * scheme, int width)
{
	int textwidth;
	int textheight;
	Text_Size(&textwidth, &textheight);
	if (width != -1) {
		textwidth = width;
	}
	Rect clip(0, 0, pt.X + textwidth, pt.Y + textheight);
	Fancy_Text_Print(name, *HiddenSurface, clip, pt, scheme, 0, TextPrintType(0x19));
}


/// <summary>
/// RA2: draws a player's row: the bar, the country flag (when shown; the observer's in a network game) and the
/// name.
/// </summary>
void ProgressScreenClass::Draw_Player(ColorScheme * scheme, wchar_t const * name, Point2D & pt, int player, bool flags)
{
	double fraction = Get_Current_Progress(player);
	int y = Row_Y(pt.Y, player, flags);
	int width = 0;
	int height = 0;
	Box_Size(&width, &height, 1, flags);
	if (RowWidth != -1) {
		width = RowWidth;
	}
	if (IsFilled) {
		Rect shade(pt.X, y, width, height);
		Shade_Rect(shade, *HiddenSurface, 0, 0x9F);
	}
	int barwidth = 0;
	int barheight = 0;
	Bar_Size(&barwidth, &barheight);
	Point2D barpt = pt;
	barpt.X += 5;
	barpt.Y = y + Bar_Offset(flags);
	Point2D itempt = pt;
	itempt.X += barwidth + 15;
	Point2D flagpt = itempt;
	if (flags) {
		flagpt.Y = y + Flag_Offset(flags);
	}
	Point2D namept = itempt;
	int textoffset = Text_Offset(flags);
	if (flags) {
		int flagwidth = 0;
		int flagheight = 0;
		Flag_Size(&flagwidth, &flagheight);
		namept.X += flagwidth + 10;
	}
	namept.Y = y + textoffset;
	Draw_Bar(barpt, fraction, HiddenSurface, scheme);
	if (flags) {
		int country = Session.Players[player]->Get_Country();
		if ((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) && Session.Players[player]->Slot.SpectatorFlag == -1) {
			country = -3;
		}
		Surface * image = Country_Flag(country);
		if (image != NULL) {
			Draw_Flag(flagpt, image);
		}
	}
	Draw_Name(namept, name, scheme, pt.X - namept.X - 3 + width);
}


/// <summary>
/// RA2: draws the progress: into the dialog's bar frame when there is a dialog, else a row per player from the
/// point (the layout position for -1,-1) - a single player shows its name in the side's colours in a single
/// player game - and shows the hidden surface.
/// </summary>
void ProgressScreenClass::Display_Progress(Point2D xpt)
{
	if (IsActive && Shape != NULL) { // RA2: Display_Progress rewritten as RA2's 0x620050 (a row per player; TibSun's form cannot be placed: with Draw_Player beside it the two must fill RA2's room)
		Point2D pt; // RA2: as above
		if (Dialog != NULL) { // RA2: as above
			RECT crect; // RA2: as above
			HWND control = GetDlgItem(Dialog, IDC_PROGRESS_BAR_FRAME); // RA2: as above
			Get_Display_Rect(control, &crect); // RA2: as above
			pt = Point2D(crect.left, crect.top); // RA2: as above
			Draw_Bar(pt, Get_Current_Progress(0), AlternateSurface, NULL); // RA2: as above
			return; // RA2: as above
		}
		pt = xpt; // RA2: as above
		if (pt.X == -1 && pt.Y == -1) { // RA2: as above
			pt = Pos; // RA2: as above
		}
		wchar_t const * name = String; // RA2: as above
		if (PlayerCount == 1) { // RA2: as above
			ColorScheme * scheme; // RA2: as above
			if (Session.Type == GAME_NORMAL) { // RA2: as above
				if (name == NULL) { // RA2: as above
					name = L""; // RA2: as above
				}
				scheme = Side_Scheme(); // RA2: as above
			} else {
				if (name == NULL) { // RA2: as above
					name = Session.Players[0]->Name; // RA2: as above
				}
				scheme = Player_Scheme(0); // RA2: as above
			}
			Draw_Player(scheme, name, pt, 0, ShowFlags); // RA2: as above
		} else {
			for (int i = 0; i < PlayerCount; i++) { // RA2: as above
				Draw_Player(Player_Scheme(i), Session.Players[i]->Name, pt, i, ShowFlags); // RA2: as above
			}
		}
		Update_Visible_Surface(); // RA2: as above (TibSun's loading messages and per-player drawing are gone)
	}
}


/// <summary>
/// RA2: sets a player's progress to a percent of the job and redraws when it changed; the callback (while
/// the screen is active) hears of the mean percent before and after.
/// </summary>
void ProgressScreenClass::Set_Progress_Percent(int index, double value, Point2D pt)
{
	int before = Get_Current_Progress() * 100.0; // RA2: Set_Progress_Percent at 0x6201C0 matches only so
	double old = PlayerProgress[index]; // RA2: as above
	PlayerProgress[index] = (MainProgress * 0.01) * value; // RA2: as above
	if (PlayerProgress[index] > MainProgress) { // RA2: as above
		PlayerProgress[index] = MainProgress; // RA2: as above
	}
	if (old != PlayerProgress[index]) { // RA2: as above
		if (Dialog != NULL) {
			SendMessage(Dialog, WM_PAINT, 0, 0);
		} else {
			Display_Progress(pt);
		}
	}
	int after = Get_Current_Progress() * 100.0; // RA2: as above
	int percent[2] = {before, after}; // RA2: as above
	if (Callback != NULL && IsActive) { // RA2: as above
		Callback->Unknown_Virtual_0(0x11AE, (int)percent); // RA2: as above
	}
}


/// <summary>
/// RA2: adds a percent of the job to a player's progress, as Set_Progress_Percent.
/// </summary>
void ProgressScreenClass::Add_Progress_Percent(int index, double value, Point2D pt)
{
	int before = Get_Current_Progress() * 100.0; // RA2: Add_Progress_Percent at 0x6202C0 matches only so
	double old = PlayerProgress[index]; // RA2: as above
	PlayerProgress[index] += (MainProgress * 0.01) * value; // RA2: as above
	if (PlayerProgress[index] > MainProgress) { // RA2: as above
		PlayerProgress[index] = MainProgress; // RA2: as above
	}
	if (old != PlayerProgress[index]) { // RA2: as above
		if (Dialog != NULL) {
			SendMessage(Dialog, WM_PAINT, 0, 0);
		} else {
			Display_Progress(pt);
		}
	}
	int after = Get_Current_Progress() * 100.0; // RA2: as above
	int percent[2] = {before, after}; // RA2: as above
	if (Callback != NULL) { // RA2: as above
		Callback->Unknown_Virtual_0(0x11AE, (int)percent); // RA2: as above
	}
}


/// <summary>
/// RA2: whether the progress is shown in a dialog.
/// </summary>
bool ProgressScreenClass::Has_Dialog(void)
{
	return(Dialog != NULL);
}


/// <summary>
/// RA2: forgets the dialog.
/// </summary>
void ProgressScreenClass::Clear_Dialog(void)
{
	Dialog = NULL;
}



