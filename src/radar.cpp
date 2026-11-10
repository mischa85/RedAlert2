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

/* $Header: /CounterStrike/RADAR.CPP 3     3/12/97 2:35p Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : RADAR.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 12/15/94                                                     *
 *                                                                                             *
 *                  Last Update : September 16, 1996 [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Get_Multi_Color -- Get the multi color offset number                                      *
 *   RadarClass::AI -- Processes radar input (non-tactical).                                   *
 *   RadarClass::Cell_On_Radar -- Determines if a cell is currently visible on radar.          *
 *   RadarClass::Click_Cell_Calc -- Determines what cell the pixel coordinate is over.         *
 *   RadarClass::Click_In_Radar -- Check to see if a click is in radar map                     *
 *   RadarClass::Click_In_Radar -- Converts a radar click into cell X and Y coordinate.        *
 *   RadarClass::Draw_It -- Displays the radar map of the terrain.                             *
 *   RadarClass::Draw_Names -- draws players' names on the radar map                           *
 *   RadarClass::Get_Jammed -- Fetch the current radar jammed state for the player.            *
 *   RadarClass::Init_Clear -- Sets the radar map to a known state                             *
 *   RadarClass::Is_Radar_Active -- Determines if the radar map is currently being displayed.  *
 *   RadarClass::Is_Radar_Existing -- Queries to see if radar map is available.                *
 *   RadarClass::Is_Zoomable -- Determines if the map can be zoomed.                           *
 *   RadarClass::Map_Cell -- Updates radar map when a cell becomes mapped.                     *
 *   RadarClass::One_Time -- Handles one time processing for the radar map.                    *
 *   RadarClass::Player_Names -- toggles the Player-Names mode of the radar map                *
 *   RadarClass::Plot_Radar_Pixel -- Updates the radar map with a terrain pixel.               *
 *   RadarClass::RTacticalClass::Action -- I/O function for the radar map.                     *
 *   RadarClass::RadarClass -- Default constructor for RadarClass object.                      *
 *   RadarClass::Radar_Activate -- Controls radar activation.                                  *
 *   RadarClass::Radar_Anim -- Renders current frame of radar animation                        *
 *   RadarClass::Radar_Cursor -- Adjust the position of the radar map cursor.                  *
 *   RadarClass::Radar_Pixel -- Mark a cell to be rerendered on the radar map.                 *
 *   RadarClass::Radar_Position -- Returns with the current position of the radar map.         *
 *   RadarClass::Refresh_Cells -- Intercepts refresh request and updates radar if needed       *
 *   RadarClass::Render_Infantry -- Displays objects on the radar map.                         *
 *   RadarClass::Render_Overlay -- Renders an icon for given overlay                           *
 *   RadarClass::Render_Terrain -- Render the terrain over the given cell                      *
 *   RadarClass::Set_Map_Dimensions -- Sets the tactical map dimensions.                       *
 *   RadarClass::Set_Radar_Position -- Sets the radar position to center around specified cell.*
 *   RadarClass::Set_Tactical_Position -- Called when setting the tactical display position.   *
 *   RadarClass::Set_Tactical_Position -- Called when setting the tactical display position.   *
 *   RadarClass::Set_Tactical_Position -- Sets the map's tactical position and adjusts radar to*
 *   RadarClass::Zoom_Mode(void) -- Handles toggling zoom on the map                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "radar.h"

#include "_convert.h"
#include "_keyboar.h"
#include "_map.h"
#include "_mixfile.h"
#include "_rect.h"
#include "_rules.h"
#include "_surface.h"
#include "_tactica.h"
#include "bsurface.h"
#include "builtype.h"
#include "cell.h"
#include "convert.h"
#include "dbgprint.h"
#include "dialog.h"
#include "draw.h"
#include "dsaudio.h"
#include "dsurface.h"
#include "goptions.h"
#include "house.h"
#include "houstype.h"
#include "incdec.h"
#include "infantry.h"
#include "infatype.h"
#include "init.h"
#include "language\language.h"
#include "lightcon.h"
#include "mixfile.h"
#include "movies.h"
#include "revent.h"
#include "rules.h"
#include "scheme.h"
#include "tactical.h"
#include "voc.h"
#include "vox.h"
#include "vqa.h"
#include "audplay.h" // RA2
#include "beacon.h" // RA2
#include "cctooltip.h" // RA2
#include "data.h" // RA2
#include "multiplayer.h" // RA2
#include "scenario.h" // RA2
#include "session.h" // RA2
#include "stringtable.h" // RA2
#include "tooltip.h" // RA2
#include "_tooltip.h" // RA2
#include "_zbuffer.h" // RA2
#include "zbuffer.h" // RA2

#define RA2_SOURCE "D:\\Ra2\\Radar.CPP"

extern RGBClass TooltipColor; // RA2: the outline colour of a tooltip
extern int MuteSWLaunches; // RA2: 0x00A3D548
extern int SidebarWidth; // RA2: 0x007ADD80

/*
**	RA2: the tooltip colour is read as one dword and packed in the order of its bytes.
*/
struct TextColor {unsigned char R, G, B, A;};
inline int Pack_Color(TextColor c) {return(DSurface::Build_Hicolor_Pixel(c.R, c.G, c.B));}

/*
**	RA2: the speech and audio helpers of the radar movie. Each is declared with its address
**	because the modules that define them are not matched yet.
*/
void Add_Speech_Hold(void); // RA2: 0x00716940
void Remove_Speech_Hold(void); // RA2: 0x00716950
void Flush_Speech(void); // RA2: 0x007157B0
void __fastcall Queue_Speech(int voice, int position, int priority); // RA2: 0x007158C0
bool __fastcall Is_Speech_Finished(int voice); // RA2: 0x00716690
void Pause_Game_Audio(void); // RA2: 0x005A0B10
void Resume_Game_Audio(void); // RA2: 0x005A0AC0
void __fastcall Set_Frame_Rate_Limit(int value); // RA2: 0x0053D820
void __fastcall Hold_Scenario(int seconds); // RA2: 0x0065C1F0
void Release_Scenario(void); // RA2: 0x0065C310


/// <summary>
/// Should this tracked object go at the head of its hash bucket?
/// The radar draws only the first object it finds on any given pixel, so the local player's
/// own units are placed at the front of the bucket and win the blip.
/// </summary>
/// <returns>bool; Does the tracked object belong to the local player?</returns>
inline bool RadarTrackingStruct::Use_Head(void) const { return(Object->House == PlayerPtr); }


RadarClass::RTacticalClass RadarClass::RadarButton;

void const * RadarClass::RadarAnim  = NULL;

/*
**	RA2: the briefing (diplomacy in multiplayer) and options buttons beside the radar, with the
**	artwork they were loaded from. The shape names are pointers that the game fills in.
*/
extern void * BriefingButtonShape; // RA2: 0x00AB6350
extern void * OptionsButtonShape; // RA2: 0x00AB6318
extern bool IsBriefingButtonShapeLoaded; // RA2: 0x00AB6369
extern bool IsOptionsButtonShapeLoaded; // RA2: 0x00AB6368
extern char const * BriefingButtonShapeName; // RA2: 0x007EE064
extern char const * OptionsButtonShapeName; // RA2: 0x007EE068
extern Rect BriefingButtonRect; // RA2: 0x00AB6468
extern Rect OptionsButtonRect; // RA2: 0x00AB6338
extern ShapeButtonClass BriefingButton; // RA2: 0x00AB63E8
extern ShapeButtonClass OptionsButton; // RA2: 0x00AB6380
extern void const * RadarFrameShape; // RA2: 0x00AC0FF0
extern void const * RadarAnimShape; // RA2: 0x00AC1070


/***********************************************************************************************
 * RadarClass::RadarClass -- Default constructor for RadarClass object.                        *
 *                                                                                             *
 *    This default constructor merely sets the radar specific values to default settings. The  *
 *    radar must be deliberately activated in order for it to be displayed.                    *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/16/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
RadarClass::RadarClass(void) :
	RadY(0),
	RadWidth(0),
	RadHeight(0),
	RadOffX(0),
	RadOffY(0),
	RadIWidth(0),
	RadIHeight(0),
	RadPWidth(0),
	LastDrawRect(RECT_NONE),
	RadarSurface(0),
	BackgroundSurface(0),
	BackgroundColors(NULL),
	RadarCellWidth(0),
	RadarCellHeight(0),
	CellRedrawRect(0,0,0,0),
	RadarTrackingTable(0),
	PixelFlags(0),
	ZoomFactor(0),
	RadarScale(1),
	RadarX(0),
	field_149C(0),
	RadarY(0),
	RadarRect(RECT_NONE),
	RadarState(0),
	RadarMode(0),
	SuspendedRadarMode(0),
	IsScenarioHeld(false),
	IsMoviePlaying(false),
	TransmissionVoice(-1),
	DoesRadarExist(false),
	IsToRedraw(0),
	FullRedraw(0),
	RadarViewRect(RECT_NONE),
	RadarAnimFrame(0)
{
	Init_Radar();
	PixelStack.Set_Growth_Step(500);
	MovieSound.Init();
}


/// <summary>
/// Destroys the radar map, releasing its surfaces and object tracking table.
/// </summary>
RadarClass::~RadarClass(void)
{
	MovieSound.Release(); // RA2
	Clear_Radar();
}


/// <summary>
/// Starts the next queued radar movie.
/// </summary>
void RadarClass::Queue_Next_Movie(void)
{
	RadarState = RSTATE_NEXT_MOVIE;
	RadarAnimFrame = RADAR_ACTIVATED_FRAME;
}


/***********************************************************************************************
 * RadarClass::One_Time -- Handles one time processing for the radar map.                      *
 *                                                                                             *
 *    This routine handles any one time processing required in order for the radar map to      *
 *    function. This actually only requires an allocation of the radar staging buffer. This    *
 *    buffer is needed for those cases where the radar area of the page is being destroyed     *
 *    and it needs to be destroyed.                                                            *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Be sure to call this routine only ONCE.                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/22/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::One_Time(void)
{
	DebugString("RadarClass::One_Time()\n");
	RadX = 0;
	RadY = 16; // RA2
	RadWidth = 48; // RA2
	RadIHeight = 140; // RA2
	RadPWidth = 108; // RA2
	RadOffY = 140; // RA2
	RadIWidth = 108; // RA2
	RadOffX = 49; // RA2

	BASECLASS::One_Time();

	RadarButton.Y = RadOffX; // RA2
	RadarButton.Width = RadIHeight;
	RadarButton.Height = RadPWidth;
	RadarButton.Set_Flags(GadgetClass::FlagEnum(GadgetClass::FlagEnum::LEFTPRESS |
														GadgetClass::FlagEnum::LEFTHELD |
														GadgetClass::FlagEnum::LEFTRELEASE |
														GadgetClass::FlagEnum::LEFTUP |
														GadgetClass::FlagEnum::RIGHTPRESS |
														GadgetClass::FlagEnum::RIGHTRELEASE |
														GadgetClass::FlagEnum::RIGHTUP));
	TransmissionVoice = Vox_From_Name("EVA_IncomingTransmission"); // RA2
}


/// <summary>
/// Frees the artwork of the two buttons beside the radar.
/// </summary>
void RadarClass::DisposeOfArt(void) // RA2
{
	if (IsBriefingButtonShapeLoaded) {
		delete BriefingButtonShape;
		IsBriefingButtonShapeLoaded = false;
	}
	BriefingButtonShape = NULL;
	if (IsOptionsButtonShapeLoaded) {
		delete OptionsButtonShape;
		IsOptionsButtonShapeLoaded = false;
	}
	OptionsButtonShape = NULL;
}


/***********************************************************************************************
 * RadarClass::Init_Clear -- Sets the radar map to a known state.                              *
 *                                                                                             *
 *    This routine is used to initialize the radar map at the start of the scenario. It        *
 *    sets the radar map position and starts it in the disabled state.                         *
 *                                                                                             *
 * INPUT:   theater  -- The theater that the scenario is starting (unused by this routine).    *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/22/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Init_Clear(void)
{
	BASECLASS::Init_Clear();
	DebugString("RadarClass::Init_Clear\n");

	RadarState 				= RSTATE_INACTIVE;
	RadarMode 				= RMODE_UNAVAILABLE;
	IsMoviePlaying 			= false; // RA2
	IsScenarioHeld 			= false; // RA2
	RadarAnimFrame 			= 0;
	DoesRadarExist 			= false;
	IsToRedraw 				= true;
	BackgroundStack.Clear();
	PixelStack.Clear();

	DebugString("RadarClass::Init_Clear done\n");
}


/// <summary>
/// Adds or removes the two buttons beside the radar.
/// </summary>
void RadarClass::Set_Buttons(bool on) // RA2
{
	if (on) {
		Add_A_Button(BriefingButton);
		Add_A_Button(OptionsButton);
	} else {
		Remove_A_Button(BriefingButton);
		Remove_A_Button(OptionsButton);
	}
}


/// <summary>
/// Handles the radar initialization required for the player's house.
/// This routine loads the artwork of the two buttons beside the radar and places them.
/// </summary>
void RadarClass::Init_For_House(void)
{
	DebugString("RadarClass::Init_For_House()\n");
	DisposeOfArt();

	if (Scen->IsGDI) { // RA2
		RadHeight = (SidebarWidth - 144) / 2 + 4;
		BriefingButtonRect.X = RadX + 11;
		BriefingButtonRect.Y = RadY + 4;
		OptionsButtonRect.X = RadX + 83;
	} else {
		RadHeight = (SidebarWidth - 145) / 2 + 5;
		BriefingButtonRect.X = RadX + 14;
		BriefingButtonRect.Y = RadY + 5;
		OptionsButtonRect.X = RadX + 86;
	}
	OptionsButtonRect.Y = BriefingButtonRect.Y;
	RadarButton.X = RadHeight;

	BriefingButtonShape = Load_Alloc_Data(BriefingButtonShapeName, IsBriefingButtonShapeLoaded);
	OptionsButtonShape = Load_Alloc_Data(OptionsButtonShapeName, IsOptionsButtonShapeLoaded);
	BriefingButtonRect.Width = ((short *)BriefingButtonShape)[1];
	BriefingButtonRect.Height = ((short *)BriefingButtonShape)[2];
	OptionsButtonRect.Width = ((short *)OptionsButtonShape)[1];
	OptionsButtonRect.Height = ((short *)OptionsButtonShape)[2];
	RadarAnim = RadarAnimShape;

	RadPHeight = Pack_Color(*(TextColor *)&TooltipColor);
}


/// <summary>
/// Sets up the two buttons beside the radar.
/// </summary>
void RadarClass::Init_IO(void) // RA2
{
	BASECLASS::Init_IO();

	if (!Debug_Map) {
		BriefingButton.ID = 242;
		BriefingButton.DrawOnSidebar = true;
		BriefingButton.ShapeDrawer = SidebarDrawer;
		BriefingButton.IsOn = false;
		BriefingButton.IsToggleType = 0;
		BriefingButton.Set_Flags(5);
		BriefingButton.Set_Position(BriefingButtonRect.X, BriefingButtonRect.Y);
		BriefingButton.Set_Shape((ShapeSet const *)BriefingButtonShape, 0, 0);
		BriefingButton.Flag_To_Redraw();

		OptionsButton.ID = 243;
		OptionsButton.DrawOnSidebar = true;
		OptionsButton.ShapeDrawer = SidebarDrawer;
		OptionsButton.IsOn = false;
		OptionsButton.IsToggleType = 0;
		OptionsButton.Set_Flags(5);
		OptionsButton.Set_Position(OptionsButtonRect.X, OptionsButtonRect.Y);
		OptionsButton.Set_Shape((ShapeSet const *)OptionsButtonShape, 0, 0);
		OptionsButton.Flag_To_Redraw();
	}
}


/***********************************************************************************************
 * RadarClass::Draw_It -- Displays the radar map of the terrain.                               *
 *                                                                                             *
 *    This is used to display the radar map that appears in the lower                          *
 *    right corner. The main changes to this map are the vehicles and                          *
 *    structure pixels.                                                                        *
 *                                                                                             *
 * INPUT:      none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/24/1991 JLB : Created.                                                                 *
 *   05/08/1994 JLB : Converted to member function.                                            *
 *=============================================================================================*/
void RadarClass::Draw_It(bool forced)
{
	bool timer_fired = false;

	if (Debug_Map) {
		FullRedraw = false;
		IsToRedraw = false;
		return;
	}

	FullRedraw = FullRedraw == true || forced == true;
	IsToRedraw = IsToRedraw == true || FullRedraw == true;

	if (IsToRedraw) {
		ShapeSet const * frame_shape = (ShapeSet const *)RadarFrameShape;
		Draw_Shape(*SidebarSurface, *SidebarDrawer, frame_shape, 0, Point2D(RadX, RadY), SidebarSurface->Get_Rect());
		BriefingButton.Draw_Me(true);
		OptionsButton.Draw_Me(true);
	}

	if (RadarState != RSTATE_ACTIVE) {
		if (RadarState != RSTATE_INACTIVE) {
			if (!RadarAnimTimer) {
				RadarAnimTimer = 4;
				timer_fired = true;

				if (RadarState == RSTATE_DEACTIVATING) {
					if (--RadarAnimFrame <= 0) {
						RadarState = RSTATE_INACTIVE;
						MovieSound.EndLooping();
						RadarAnimFrame = 0;
					}
				} else {
					if (++RadarAnimFrame >= 32) {
						MovieSound.EndLooping();
						RadarState = RSTATE_ACTIVE;
						FullRedraw = true;
						RadarAnimFrame = 32;
					}
				}
			}
		}

		if (RadarState != RSTATE_ACTIVE) {
			if (IsToRedraw == true || timer_fired) {
				if (RadarAnimFrame >= 0 && RadarAnimFrame < ((short *)RadarAnim)[3]) {
					Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, RadarAnimFrame, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect());
					LastDrawRect = Rect(RadX, RadWidth, SidebarWidth, 110);
				}
			}
		}
	}

	Process_Radar_Events();
	Render_Radar();

	if (RadarState == RSTATE_ACTIVE) {
		switch (RadarMode) {
			case RMODE_TACTICAL:
				break;

			case RMODE_PLAYER_NAMES:
				if (IsToRedraw) {
					bool full = FullRedraw;
					IsToRedraw = false;
					if (full) {
						Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, 32, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect());
						Draw_Names();
						LastDrawRect = Rect(RadX, RadWidth, SidebarWidth, 110);
					} else {
						Draw_Names();
						LastDrawRect = Rect(RadHeight, RadOffX, RadIHeight, RadPWidth);
					}
				}
				break;

			case RMODE_MOVIE:
				Play_Movie();
				break;

			case RMODE_4:
				break;

			default:
				if (IsToRedraw) {
					IsToRedraw = false;
					Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, 0, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect());
					LastDrawRect = Rect(RadX, RadWidth, SidebarWidth, 110);
				}
				break;
		}
	}

	if (FullRedraw) {
		FullRedraw = false;
		Map.Upgrade.Draw_Me(true);
		Map.Repair.Draw_Me(true);
	}

	if (BriefingButton.IsDrawn) {
		LastDrawRect = Union(LastDrawRect, BriefingButtonRect);
		Map.IsToBlitSidebar = true;
	}

	if (OptionsButton.IsDrawn) {
		LastDrawRect = Union(LastDrawRect, OptionsButtonRect);
		Map.IsToBlitSidebar = true;
	}
}


void RadarClass::Noop(int, int)
{

}


/***********************************************************************************************
 * RadarClass::Click_Cell_Calc -- Determines what cell the pixel coordinate is over.           *
 *                                                                                             *
 *    This routine will examine the pixel coordinate provided and determine what cell it       *
 *    represents. If the radar map is not active or the coordinates are not positioned over    *
 *    the radar map, then it will fall into the base class corresponding routine.              *
 *                                                                                             *
 * INPUT:   x,y   -- The pixel coordinate to convert into a cell number.                       *
 *                                                                                             *
 * OUTPUT:  Returns with the cell number that the coordinate is over or -1 if not over any     *
 *          cell.                                                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/22/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
Cell RadarClass::Click_Cell_Calc(Point2D const & point) const
{
	Point2D pixel = point;
	Coord coord = TacticalMap->Pixel_To_Coord(pixel);

	if (coord != COORD_NONE) {
		if (Debug_Map) {
			return(TacticalMap->Pixel_To_Cell(pixel));
		} else {
			return(TacticalMap->Pixel_To_Cell(pixel));
		}
	}

	return(CELL_NONE);
}


/***********************************************************************************************
 * RadarClass::Map_Cell -- Updates radar map when a cell becomes mapped.                       *
 *                                                                                             *
 *    This routine will update the radar map if a cell becomes mapped.                         *
 *                                                                                             *
 * INPUT:   cell  -- The cell that is being mapped.                                            *
 *                                                                                             *
 *          house -- The house that is doing the mapping.                                      *
 *                                                                                             *
 * OUTPUT:  bool; Was the cell mapped (for the first time) by this routine?                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/22/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RadarClass::Map_Cell(Cell const & cell, HouseClass * house)
{
	if (BASECLASS::Map_Cell(cell, house)) {
		return(true);
	}
	return(false);
}


/// <summary>
/// Hands the cell to the base class unchanged.
/// </summary>
bool RadarClass::RevealFogShroud(Cell const & cell, HouseClass * house, bool increase) // RA2
{
	if (BASECLASS::RevealFogShroud(cell, house, increase)) {
		return(true);
	}
	return(false);
}


/***********************************************************************************************
 * RadarClass::AI -- Processes radar input (non-tactical).                                     *
 *                                                                                             *
 *    This routine intercepts any player input that concerns the radar map, but not those      *
 *    areas that represent the tactical map. These are handled by the tactical map AI          *
 *    processor. Primarily, this routine handles the little buttons that border the radar      *
 *    map.                                                                                     *
 *                                                                                             *
 * INPUT:   input -- The player input code.                                                    *
 *                                                                                             *
 *          x,y   -- Mouse coordinate parameters to use.                                       *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/23/1994 JLB : Created.                                                                 *
 *   12/26/1994 JLB : Moves tactical map with click or drag.                                   *
 *   12/31/1994 JLB : Uses mouse coordinate parameters.                                        *
 *=============================================================================================*/
void RadarClass::AI(KeyNumType & input, Point2D const & xy)
{
	BASECLASS::AI(input, xy);

	int offset = Options.IsSidebarOnRight ? TacticalRect.Width : 0; // RA2

	Rect r1 = BriefingButtonRect;
	r1.X += offset;
	if (r1.Is_Point_Within(xy)) {
		Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
	}

	Rect r2 = OptionsButtonRect;
	r2.X += offset;
	if (r2.Is_Point_Within(xy)) {
		Map.Override_Mouse_Shape(MOUSE_NORMAL, false);
	}

	if (input == 0x80F2) {
		if (Session.Type != GAME_NORMAL && !((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) && Session.MPGameMode && Session.MPGameMode->vt_entry_08() && Session.MPGameMode->vt_entry_04())) {
			SpecialDialog = (SpecialDialogType)8;
		} else {
			SpecialDialog = (SpecialDialogType)9;
		}
	} else if (input == 0x80F3) {
		SpecialDialog = (SpecialDialogType)1;
	}

	if (IngameVQ.Count() > 0 && RadarMode != RMODE_MOVIE && RadarMode != RMODE_4) {
		SuspendedRadarMode = RadarMode;
		Radar_Activate(RMODE_4, false);
		TransmissionState = 0;
	}

	if (RadarMode == RMODE_4) {
		Update_Transmission();
	} else {
		Remove_Speech_Hold();
	}
}


/***********************************************************************************************
 * RadarClass::RTacticalClass::Action -- I/O function for the radar map.                       *
 *                                                                                             *
 *    This is the main action function for handling player I/O on the radar map. It processes  *
 *    mouse clicks as well as mouse moves.                                                     *
 *                                                                                             *
 * INPUT:   flags -- The event flags that trigger this function call.                          *
 *                                                                                             *
 *          key   -- Reference the keyboard event that applies to the trigger event.           *
 *                                                                                             *
 * OUTPUT:  Should further processing of the input list be aborted?                            *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/08/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int RadarClass::RTacticalClass::Action(unsigned flags, KeyNumType & key)
{
	if (flags & (RIGHTHELD|LEFTHELD)) {
		return(false);
	}

	if (Map.Is_Playing_Movie()) {
		return(false);
	}

	if (!Map.Is_Radar_Tactical()) {
		return(false);
	}

	int			x,y;							// Sub cell pixel coordinates.

	/*
	**	Set some working variables that depend on the mouse position. For the press
	**	or release event, special mouse queuing storage variables are used. Other
	**	events must use the current mouse position globals.
	*/
	if (flags & (LEFTPRESS|LEFTRELEASE|RIGHTPRESS|RIGHTRELEASE)) {
		x = Keyboard->MouseQX;
		y = Keyboard->MouseQY;
	} else {
		x = Get_Mouse_X();
		y = Get_Mouse_Y();
	}


	x -= Options.IsSidebarOnRight ? TacticalRect.Width : 0;

	/*
	**	See if the mouse is over the radar general area, but not yet
	**	over the active region of the radar map. In such a case, the
	**	mouse is overridden to be the normal cursor and no other
	**	action is performed.
	*/
	if (x < Map.RadarRect.X || x >= Map.RadarRect.X+Map.RadarRect.Width || y < Map.RadarRect.Y || y >= Map.RadarRect.Y+Map.RadarRect.Height) {
		Map.Override_Mouse_Shape(MOUSE_NORMAL);
		return(false);
	}

	Point2D click(x,y);

	Cell			cell(0,0);                  // cell num click happened over
	bool			shadow;                     // is the cell in shadow or not
	ObjectClass * object = NULL;                // what object is in the cell

	Map.Resolve_Radar_Point(click, cell, object);
	if (cell != CELL_NONE) {
		Coord coord = cell.As_Coord();
		coord.Z = Map.Get_Height_GL(coord);
		shadow	= (Map.Is_Shrouded(coord) && MainWindow);

		/*
		**	If a super weapon is being aimed or there is a currently selected object, then the
		**	action to perform if the left mouse button were clicked must be determined.
		*/
		ActionType action = ACTION_NONE; // RA2
		bool multi_missile = Map.IsTargettingMode == SUPER_MULTI_MISSILE; // RA2
		bool iron_curtain = Map.IsTargettingMode == SUPER_IRON_CURTAIN;
		bool lightning_storm = Map.IsTargettingMode == SUPER_LIGHTNING_STORM;
		bool para_drop = Map.IsTargettingMode == SUPER_PARA_DROP;
		bool amer_para_drop = Map.IsTargettingMode == SUPER_AMER_PARA_DROP;
		bool right = (flags & (RIGHTPRESS|RIGHTRELEASE)) != 0; // RA2
		if (!right && (CurrentObject.Count() || multi_missile || iron_curtain || lightning_storm || para_drop || amer_para_drop)) {
			if (multi_missile) {
				action = ACTION_NUKE_BOMB;
			} else if (iron_curtain) {
				action = ACTION_IRON_CURTAIN;
			} else if (lightning_storm) {
				action = ACTION_LIGHTNING_STORM;
			} else if (para_drop) {
				action = ACTION_PARA_DROP;
			} else if (amer_para_drop) {
				action = (ActionType)65; // RA2: the American paradrop
			} else if (object) {
				action = Best_Selected_Object(NULL, object)->What_Action(object);
			} else {
				action = Best_Selected_Object(&cell, NULL)->What_Action(cell);
			}

			/*
			**	If this is not a valid radar map action then we are not going to do
			**	anything.
			*/
			switch (action) {
				case ACTION_MOVE:
				case ACTION_NOMOVE:
				case ACTION_ENTER:
				case ACTION_ATTACK:
				case ACTION_HARVEST:
				case ACTION_CAPTURE:
				case ACTION_SABOTAGE:
				case ACTION_NUKE_BOMB:
				case ACTION_GUARD_AREA:
				case ACTION_IRON_CURTAIN:
				case ACTION_LIGHTNING_STORM:
				case ACTION_PARA_DROP:
				case 65:
					break;

				default:
					action = ACTION_NONE;
					object = NULL;
					break;
			}
		}

		if (action != ACTION_NONE) {

			/*
			**	When the mouse buttons aren't pressed, only the mouse cursor shape is processed.
			**	The shape changes depending on what object the mouse is currently over and what
			**	object is currently selected.
			*/
			if (flags & LEFTUP) {
				Map.Mouse_Left_Up(cell, shadow, object, action, true);
			}

			/*
			**	Normal actions occur when the mouse button is released. The press event is
			**	intercepted and possible rubber-band mode is flagged.
			*/
			if (flags & LEFTRELEASE && !drag_select_aborted) {
				Map.Mouse_Left_Release(Coord(cell), cell, object, action, true);
			}
		} else {
			if (!right) {
				Map.Set_Default_Mouse(MOUSE_NORMAL, true);
			}

			if ((flags & LEFTPRESS) || (flags & RIGHTPRESS)) {

				if (cell != Cell(0, 0)) {

					/*
					 * The tactical map is isometric, so the click is kept on the playable
					 * area by clamping the diagonal coordinates rather than X and Y directly:
					 * (cell.X - cell.Y) is the horizontal screen axis (side_edge) and
					 * (cell.X + cell.Y) is the vertical axis (top_edge/bottom_edge). Each
					 * limit is the play area pulled in by half the tactical view, so the view
					 * stays on the map.
					 */
					int side_edge = Map.PlayRect.Width - (TacticalRect.Width / ISO_TILE_PIXEL_W + 2) / 2 - 1;
					int half_view_height = TacticalRect.Height / (2 * ISO_TILE_PIXEL_H);
					int top_edge = half_view_height + Map.PlayRect.Width + 1;
					int bottom_edge = 2 * Map.PlayRect.Height - half_view_height + Map.PlayRect.Width - 1;

					int adjust;

					if ((cell.Y - cell.X) > side_edge) {
						adjust = (cell.Y - cell.X) - side_edge;
						cell.Y -= adjust;
						cell.X += adjust;
					}

					if ((cell.X - cell.Y) > side_edge - 1) {
						adjust = (cell.X - cell.Y) - side_edge + 1;
						cell.Y += adjust;
						cell.X -= adjust;
					}

					if ((cell.X + cell.Y) < top_edge) {
						adjust = top_edge - cell.Y - cell.X;
						cell.X += adjust;
						cell.Y += adjust;
					}

					if ((cell.X + cell.Y) > bottom_edge) {
						adjust = (cell.X + cell.Y) - bottom_edge;
						cell.X -= adjust;
						cell.Y -= adjust;
					}

					Coord coord = Map[cell].Cell_Coord();
					TacticalMap->Set_Tactical_Position(coord); // RA2
					Map.Flag_To_Redraw(GS_REDRAW_TACTICAL);
					if (DepthBuffer) {
						DepthBuffer->Set_Scroll(ZBUFFER_MAX); // RA2
					}
				}
			}
		}
	}
	GadgetClass::Action(0, key);
	return(true);
}


/***********************************************************************************************
 * RadarClass::Set_Map_Dimensions -- Sets the tactical map dimensions.                         *
 *                                                                                             *
 *    This routine is called when the tactical map changes its dimensions. This occurs when    *
 *    the tactical map moves and when the sidebar pops on or off.                              *
 *                                                                                             *
 * INPUT:   x,y   -- The cell coordinate of the upper left corner of the tactical map.         *
 *                                                                                             *
 *          w,y   -- The cell width and height of the tactical map.                            *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/08/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Set_Map_Dimensions(Rect const & size, bool reset_cells, int cell_height, bool refresh_map)
{
	BASECLASS::Set_Map_Dimensions(size, reset_cells, cell_height, refresh_map);
}


/***********************************************************************************************
 * RadarClass::Set_Tactical_Position -- Sets the map's tactical position and adjusts radar to  *
 *                                                                                             *
 *    This routine is called when the tactical map is to change position. The radar map might  *
 *    be adjusted as well by this routine.                                                     *
 *                                                                                             *
 * INPUT:   coord -- The new coordinate to use for the upper left corner of the tactical       *
 *                   map.                                                                      *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/17/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Set_Tactical_Position(Coord const & coord)
{
	TacticalMap->Set_Tactical_Position(coord);
}


/***********************************************************************************************
 * RadarClass::Cell_On_Radar -- Determines if a cell is currently visible on radar.            *
 *                                                                                             *
 *    This routine will examine the specified cell number and return whether it is visible     *
 *    on the radar map. This depends on the radar map position.                                *
 *                                                                                             *
 * INPUT:   cell  -- The cell number to check.                                                 *
 *                                                                                             *
 * OUTPUT:  Is the specified cell visible on the radar map currently?                          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/03/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RadarClass::Cell_On_Radar(Cell const & cell)
{
	return(In_Local_Radar(cell));
}


/***********************************************************************************************
 * Draw_Names -- draws players' names on the radar map                                         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/07/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Draw_Names(void)
{
	int c_idx;
	int house;
	HouseClass * ptr;
	int y;
	wchar_t txt[40];
	int h;
	int kills;
	ColorScheme * color;
	int style;

	/*
	**	Do nothing if the sidebar isn't there
	*/
	if (!Map.IsSidebarActive) {
		return;
	}

	Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, 32, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect());

	y = RadOffX + 2;

	Fancy_Text_Print(StringTable::LoadString("TXT_NAME_COLON", NULL, RA2_SOURCE, 1217), *SidebarSurface, SidebarSurface->Get_Rect(), Point2D(RadHeight, y), Fetch_Scheme_By_Name("Grey", 1), 0, TextPrintType(0x19));
	Fancy_Text_Print(StringTable::LoadString("TXT_KILLS_COLON", NULL, RA2_SOURCE, 1219), *SidebarSurface, SidebarSurface->Get_Rect(), Point2D(RadOffY + RadHeight - 2, y), Fetch_Scheme_By_Name("Grey", 1), 0, TextPrintType(0x219));
	y += 7;

	SidebarSurface->Draw_Line(Point2D(RadHeight, y), Point2D(RadOffY + RadHeight - 1, y), SidebarDrawer->Convert_Pixel(14));
	y += 4;

	for (house = 0; house < Houses.Count(); house++) {
		ptr = Houses[house];

		if (!ptr || !ptr->Class->IsMultiplay) continue;

		/*
		**	Decode this house's color
		*/
		c_idx = ptr->Scheme;

		if (ptr->IsDefeated) {
			color = Fetch_Scheme_By_Name("Grey", 1);
			style = 0x19;
		} else {
			color = ColorSchemes[c_idx];
			style = 0x4019;
		}

		/*
		**	Initialize our message
		*/
		txt[0] = 0;
		swprintf(txt, L"%s", ptr->UIName);

		if (wcslen(txt) == 0) {
			wcscpy(txt, L"________");
		}

		/*
		**	Print the player name, and the # of kills
		*/
		if (wcslen(txt) > 18) {
			txt[18] = '.';
			txt[19] = 0;
		}
		Fancy_Text_Print(txt, *SidebarSurface, SidebarSurface->Get_Rect(), Point2D(RadHeight, y), color, 0, TextPrintType(style));

		kills = 0;
		for (h = 0; h < 20; h++) {
			kills += ptr->UnitsKilled[h];
			kills += ptr->BuildingsKilled[h];
		}
		swprintf(txt, L"%2d", kills);
		Fancy_Text_Print(txt, *SidebarSurface, SidebarSurface->Get_Rect(), Point2D(RadOffY + RadHeight - 2, y), color, 0, TextPrintType(style | 0x200));

		y += 9;
	}
}


/// <summary>
/// Handles the sidebar moving to the other side of the screen.
/// This routine drags the radar's click region along with the sidebar and forces the radar to
/// redraw itself in its new home.
/// </summary>
void RadarClass::Reposition_Sidebar(void)
{
	BASECLASS::Reposition_Sidebar();

	int offset = Options.IsSidebarOnRight ? TacticalRect.Width : 0; // RA2

	BriefingButton.DrawOffsetX = -offset;
	BriefingButton.Set_Position(BriefingButtonRect.X + offset, BriefingButtonRect.Y);
	BriefingButton.Flag_To_Redraw();

	OptionsButton.DrawOffsetX = -offset;
	OptionsButton.Set_Position(OptionsButtonRect.X + offset, OptionsButtonRect.Y);
	OptionsButton.Flag_To_Redraw();

	RadarButton.Set_Position(RadHeight + offset, RadOffX);
	RadarButton.Flag_To_Redraw();

	ToolTip tip;
	tip.ID = 242;
	tip.Text = (int)"Tip:BriefingButton";
	if (Session.Type != GAME_NORMAL) {
		tip.Text = (int)"Tip:DiplomacyButton";
	}
	tip.Region.X = BriefingButton.X;
	tip.Region.Y = BriefingButton.Y;
	tip.Region.Width = BriefingButton.Width;
	tip.Region.Height = BriefingButton.Height;
	ToolTips->Remove(242);
	ToolTips->Add(&tip);

	tip.ID = 243;
	tip.Text = (int)"Tip:OptionsButton";
	tip.Region.X = OptionsButton.X;
	tip.Region.Y = OptionsButton.Y;
	tip.Region.Width = OptionsButton.Width;
	tip.Region.Height = OptionsButton.Height;
	ToolTips->Remove(243);
	ToolTips->Add(&tip);

	FullRedraw = true;
}


/// <summary>
/// Sets the local map dimensions and refits the radar to them.
/// The radar shows the local map rather than the whole playfield, so this routine works out
/// where the visible portion lands in radar cell space and how large a rectangle it needs.
/// </summary>
/// <param name="size">The new local map rectangle.</param>
void RadarClass::Set_Local_Dimensions(Rect const & size)
{
	BASECLASS::Set_Local_Dimensions(size);

	RadarRect.Y = 10000;
	RadarRect.X = 10000;
	RadarRect.Height = 0;
	RadarRect.Width = 0;

	Cell corner(LocalRect.Y + LocalRect.X + 1, LocalRect.Y + PlayRect.Width - LocalRect.X);
	Cell corner2(corner);
	int x = LocalRect.Width - 1;
	int y = 1 - LocalRect.Width;
	Cell opposite = corner + Cell(x, y);

	RadarX = corner2.Y - corner.X;
	field_149C = opposite.X - opposite.Y;
	RadarY = -1;

	Map.Reset_Iterator();
	CellClass * iter = Iterate();

	while (iter != NULL) {
		Cell cell = iter->CellID;

		if (In_Local_Radar(cell)) {

			if (RadarY == -1) {
				RadarY = iter->CellID.X + iter->CellID.Y;
			}

			cell = iter->CellID;
			Rect r = Cell_Radar_Rect(cell);

			if (r.X < RadarRect.X) {
				RadarRect.X = r.X;
			} else if (r.X + r.Width > RadarRect.X + RadarRect.Width) {
				RadarRect.Width = r.X + r.Width - RadarRect.X;
			}

			if (r.Y < RadarRect.Y) {
				RadarRect.Y = r.Y;
			} else if (r.Y + r.Height > RadarRect.Y + RadarRect.Height) {
				RadarRect.Height = r.Y + r.Height - RadarRect.Y;
			}
		}
		iter = Iterate();
	}
}


/// <summary>
/// Builds the radar image for the current map.
/// This routine discards any radar surfaces already in hand and creates fresh ones sized to
/// fit the map into the radar pane, centering the picture when the map is too small to fill
/// it. The building footprints are recomputed to suit the new zoom.
/// </summary>
void RadarClass::Compute_Radar_Image(void)
{
	if (!RadarRect.Is_Valid()) {
		return;
	}

	RadarRect.Y = 0;
	RadarRect.X = 0;

	if (BackgroundSurface != NULL) {
		delete BackgroundSurface;
		BackgroundSurface = NULL;
	}
	if (BackgroundColors != NULL) {
		delete BackgroundColors;
		BackgroundColors = NULL;
	}
	if (PixelFlags != NULL) {
		delete [] PixelFlags;
		PixelFlags = NULL;
	}

	Compute_Background(RadarRect, RadarRect, true);

	RadarRect.Width = BackgroundSurface->Get_Width();
	RadarRect.Height = BackgroundSurface->Get_Height();

	RadarRect.X = RadHeight;
	if (RadarRect.Width < 140) {
		RadarRect.X += (140 - RadarRect.Width) / 2;
	}

	RadarRect.Y = RadOffX;
	if (RadarRect.Height < 108) {
		RadarRect.Y += (108 - RadarRect.Height) / 2;
	}

	if (RadarSurface != NULL) {
		delete RadarSurface;
		RadarSurface = NULL;
	}

	RadarSurface = new DSurface(BackgroundSurface->Get_Width(), BackgroundSurface->Get_Height(), true);
	RadarSurface->Fill(TBLACK);

	Compute_Foundations();
}


/// <summary>
/// Builds the scaled down radar picture from the map's cell colors.
/// On its first pass this routine allocates the radar background surface and settles on a
/// zoom factor that fits the whole map into the radar pane. It then resamples the requested
/// region of the background color array onto that surface, averaging together the cells that
/// have to share a radar pixel.
/// </summary>
/// <param name="cell_rect">The extent of the radar background color array.</param>
/// <param name="update_rect">The region to resample. When filling in, an invalid rectangle is
/// widened to cover the entire map.</param>
/// <param name="fill_in">Should the background colors be gathered from the map first?</param>
/// <returns>Returns with the region of the radar surface that was rewritten.</returns>
static inline void Radar_Pixel_Bounds(Rect const &rect, float zoom, int width, int height,
                                     int &left, int &right, int &top, int &bottom)
{
	left = (int)(rect.X * zoom);
	right = MIN((int)((rect.X + rect.Width) * zoom) + 1, width);
	top = (int)(rect.Y * zoom);
	bottom = MIN((int)((rect.Y + rect.Height) * zoom) + 1, height);
}

Rect RadarClass::Compute_Background(Rect const & cell_rect, Rect & update_rect, bool fill_in)
{
	if (fill_in && !update_rect.Is_Valid()) {
		update_rect = cell_rect;
	}
	if (BackgroundColors == NULL) {
		BackgroundColors = new RGBClass[cell_rect.Height * cell_rect.Width];
		memset(BackgroundColors, 0, cell_rect.Height * cell_rect.Width * sizeof(RGBClass));
		RadarCellWidth = cell_rect.Width;
		RadarCellHeight = cell_rect.Height;
	}
	if (fill_in) {
		Fill_In_Background(cell_rect, update_rect);
	}

	int cell_width = cell_rect.Width;
	float zoom_scale = ZoomFactor;
	int surface_width;
	int surface_height;
	if (BackgroundSurface == NULL) {
		zoom_scale = 140 / (float)cell_width;
		float scaled_height = cell_rect.Height * zoom_scale;
		if (scaled_height < 108) {
			surface_width = 140;
			surface_height = scaled_height;
		} else {
			zoom_scale = 108 / (float)cell_rect.Height;
			float scaled_width = cell_width * zoom_scale;
			surface_width = scaled_width;
			surface_height = 108;
		}
		ZoomFactor = zoom_scale;
		BackgroundSurface = new BSurface(surface_width, surface_height, 2);
		BackgroundSurface->Fill(TBLACK);
	} else {
		surface_width = BackgroundSurface->Get_Width();
		surface_height = BackgroundSurface->Get_Height();
	}

	float pixels_per_cell_x = (float)cell_rect.Width / (float)surface_width;
	float pixels_per_cell_y = (float)cell_rect.Height / (float)surface_height;
	float inv_area = 1.0 / (pixels_per_cell_x * pixels_per_cell_y);
	unsigned short *data = (unsigned short *)BackgroundSurface->Lock();
	int surf_left, surf_right, surf_top, surf_bottom;
	Radar_Pixel_Bounds(update_rect, zoom_scale, surface_width, surface_height, surf_left, surf_right, surf_top, surf_bottom);
	float src_x = surf_left / zoom_scale;
	float src_y = surf_top / zoom_scale;
	if (surf_top < surf_bottom) {
		for (int surfy = surf_top; surfy < surf_bottom; ++surfy) {
			int src_y_min = MIN(RadarCellHeight, (int)src_y);
			float src_y_end = src_y + pixels_per_cell_y;
			int src_y_max = MIN(RadarCellHeight, (int)src_y_end + 1);
			if (surf_left < surf_right) {
				unsigned short *pixel_data = data + surf_left + surface_width * surfy;
				for (int surfx = surf_left; surfx < surf_right; ++surfx) {
					float accum_r = 0.0f;
					float accum_g = 0.0f;
					float accum_b = 0.0f;
					int src_x_min = MIN(RadarCellWidth, (int)src_x);
					float src_x_end = src_x + pixels_per_cell_x;
					int src_x_max = MIN(RadarCellWidth, (int)src_x_end + 1);
					for (int srcy = src_y_min; srcy < src_y_max; ++srcy) {
						float y_weight;
						if (src_y_max - src_y_min <= 1) {
							y_weight = pixels_per_cell_y;
						} else if (srcy == src_y_min) {
							y_weight = (float)(srcy + 1) - src_y;
						} else if (srcy == src_y_max - 1) {
							y_weight = src_y_end - (float)srcy;
						} else {
							y_weight = 1.0f;
						}
						int idx = src_x_min + srcy * cell_width;
						for (int srcx = src_x_min; srcx < src_x_max; ++srcx, ++idx) {
							float x_weight;
							if (src_x_max - src_x_min <= 1) {
								x_weight = pixels_per_cell_x;
							} else if (srcx == src_x_min) {
								x_weight = (float)(srcx + 1) - src_x;
							} else if (srcx == src_x_max - 1) {
								x_weight = src_x_end - (float)srcx;
							} else {
								x_weight = 1.0f;
							}
							float weight = x_weight * y_weight * inv_area;
							if (idx < RadarCellHeight * RadarCellWidth) {
								RGBClass &color = BackgroundColors[idx];
								accum_r += (float)color.Get_Red() * weight;
								accum_g += (float)color.Get_Green() * weight;
								accum_b += (float)color.Get_Blue() * weight;
							}
						}
					}
					*pixel_data++ = DSurface::Build_Hicolor_Pixel(
						MIN(255, (int)(accum_r + 0.5)),
						MIN(255, (int)(accum_g + 0.5)),
						MIN(255, (int)(accum_b + 0.5)));
					src_x = src_x_end;
				}
			}
			src_x = surf_left / zoom_scale;
			src_y = src_y_end;
		}
	}

	BackgroundSurface->Unlock();
	int pixel_count = (surface_width * surface_height) / 8 + 1;
	if (PixelFlags == NULL) {
		PixelFlags = new unsigned char[pixel_count];
		memset(PixelFlags, 0, pixel_count);
	}
	Rect result(surf_left, surf_top, surf_right - surf_left, surf_bottom - surf_top);
	return result;
}


/// <summary>
/// Fills in the radar background colors for a region of the map.
/// This routine asks every visible cell for its low and high terrain colors and stores them
/// into the radar background color array. That array is the source data the radar image is
/// scaled down from.
/// </summary>
/// <param name="cell_rect">The extent of the radar background color array.</param>
/// <param name="update_rect">The region of that array to fill in.</param>
void RadarClass::Fill_In_Background(Rect const & cell_rect, Rect const & update_rect)
{
	RGBClass low(0, 0, 0);
	RGBClass high(0, 0, 0);

	Reset_Iterator();
	CellClass * iter = Map.Iterate();

	int a = 2 * Map.LocalRect.X - Map.PlayRect.Width + 1;

	while (iter != NULL) {

		int x = iter->CellID.X;
		int y = iter->CellID.Y;

		if (x + y >= RadarY) {
			int origx = x - y - a;
			int origy = x + y - RadarY;
			Rect mrect(origx, origy, 2, 1);
			mrect = Intersect(mrect, update_rect);
			if (mrect.Is_Valid()) {
				int idx = mrect.X + mrect.Y * cell_rect.Width;
				iter->Cell_Color(low, high);
				if (origx == update_rect.X - 1) {
					BackgroundColors[idx] = high;
				} else if (origx == update_rect.Width + update_rect.X - 1) {
					BackgroundColors[idx] = low;
				} else {
					BackgroundColors[idx] = low;
					BackgroundColors[idx+1] = high;
				}
			}
		}
		iter = Map.Iterate();
	}
}


/// <summary>
/// Converts a map cell into its place in radar cell space.
/// The radar flattens the isometric map into a rectangular grid of background colors, two
/// entries wide for every cell. This routine is what maps a cell onto that grid, narrowing
/// the half cells that hang off the left and right edges.
/// </summary>
/// <returns>Returns with the rectangle the cell occupies in the background color
/// array.</returns>
Rect RadarClass::Cell_Radar_Rect(Cell const & cell)
{
	int cellx = cell.X;
	int celly = cell.Y;
	int x = cellx - celly + RadarX;
	int y = cellx + celly - RadarY;
	int width = 2;
	if (x == -1) {
		x = 0;
		width = 1;
	} else if (x == width * LocalRect.Width - 1) {
		width = 1;
	}
	return(Rect(x, y, width, 1));
}


/// <summary>
/// Converts a map cell into its position on the radar.
/// </summary>
/// <returns>Returns with the single pixel rectangle that the cell occupies on the
/// radar.</returns>
Rect RadarClass::Cell_To_Radar_Pixel(Cell const & cell)
{
	int cellx = cell.X;
	int celly = cell.Y;
	int x = (cellx - celly + RadarX) * ZoomFactor + RadarRect.X;
	int y = (cellx + celly - RadarY) * ZoomFactor + RadarRect.Y;
	if (x == RadarRect.X - 1) x++;
	return(Rect(x, y, 1, 1));
}


/// <summary>
/// Converts a radar pixel position back into a map cell.
/// </summary>
/// <returns>Returns with the map cell that the radar pixel stands for.</returns>
Cell RadarClass::Radar_Pixel_To_Cell(Point2D const& point)
{
	float x = (point.X / ZoomFactor) - RadarX;
	float y = (point.Y / ZoomFactor) + RadarY;

	Cell cell;
	cell.X = int(((x + y) / 2) + 0.5);
	cell.Y = int(((y - x) / 2) + 0.5);

	return(cell);
}


/// <summary>
/// Flags a cell's radar background color to be recomputed.
/// The cell is only queued here -- Plot_Radar_Background does the recoloring when the radar
/// next redraws. Use this routine whenever the terrain of a cell changes appearance.
/// </summary>
void RadarClass::Radar_Background(Cell const & cell)
{
	for (int i = BackgroundStack.Count() - 1; i >= 0; i--) {
		if (BackgroundStack[i] == cell) {
			return;
		}
	}
	BackgroundStack.Add(cell);
	IsToRedraw = true;
}


/// <summary>
/// Flushes the pending radar background updates.
/// This routine recolors the radar background for every cell that Radar_Background queued up
/// and remembers the region of the radar image that will need blitting as a result.
/// </summary>
void RadarClass::Plot_Radar_Background(void)
{
	Rect r(0, 0, 0, 0);
	for (int i = BackgroundStack.Count() - 1; i >= 0; i--) {

		Cell cell = BackgroundStack[i];
		if (In_Local_Radar(cell)) {
			Rect update_rect = Cell_Radar_Rect(cell);

			int x = update_rect.X;
			Rect bounds(0, 0, RadarCellWidth, RadarCellHeight);
			update_rect = Intersect(update_rect, bounds);

			if (update_rect.Is_Valid()) {
				RGBClass low(0, 0, 0);
				RGBClass high(0, 0, 0);

				CellClass * cptr = &Map[cell];
				int idx = update_rect.X + RadarCellWidth * update_rect.Y;
				cptr->Cell_Color(low, high);

				if (x == 0 && update_rect.Width == 1) {
					BackgroundColors[idx+1] = high;
				} else if (update_rect.Width == 1) {
					BackgroundColors[idx] = low;
				} else {
					BackgroundColors[idx] = low;
					BackgroundColors[idx+1] = high;
				}

				Rect radarcellrect(0, 0, RadarCellWidth, RadarCellHeight);
				Rect rr = Compute_Background(radarcellrect, update_rect, false);
				r = Union(r, rr);
			}
		}
	}

	BackgroundStack.Clear();
	CellRedrawRect = r;
}


/// <summary>
/// Adds an object to the radar tracking table.
/// This routine registers the object at a radar pixel so that it will be drawn as a blip on
/// the next radar render. A unit that lands outside the radar surface is pulled back to the
/// nearest edge pixel; a building in that position is simply not tracked.
/// </summary>
/// <param name="point">The radar pixel to track the object at.</param>
void RadarClass::Radar_Track(TechnoClass * techno, Point2D point)
{
	Rect surfacerect = RadarSurface->Get_Rect();

	if (!surfacerect.Is_Point_Within(point)) {
		if (techno->RTTI == RTTI_BUILDING) return;

		if (point.X < surfacerect.X) {
			point.X = surfacerect.X;
		}
		if (point.X >= surfacerect.X + surfacerect.Width) {
			point.X = surfacerect.X + surfacerect.Width - 1;
		}
		if (point.Y < surfacerect.Y) {
			point.Y = surfacerect.Y;
		}
		if (point.Y >= surfacerect.Y + surfacerect.Height) {
			point.Y = surfacerect.Y + surfacerect.Height - 1;
		}
		techno->RadarPos = point;
	}

	RADAR_HASH_TABLE::ObjectType track;
	track.Key.Position = point;
	track.Key.Object = techno;
	track.Value = techno;

	if (RadarTrackingTable->Add_Object(track, true)) {
		Radar_Pixel(point);
		IsToRedraw = true;
	}
}


/// <summary>
/// Removes an object from the radar tracking table.
/// This routine is called when a tracked object moves off a radar pixel or leaves the game.
/// The vacated pixel is flagged for redraw so that whatever lies beneath it shows through
/// again.
/// </summary>
/// <param name="point">The radar pixel the object was being tracked at.</param>
void RadarClass::Radar_Untrack(TechnoClass * techno, Point2D point)
{
	RADAR_HASH_TABLE::BucketType * bucket = &RadarTrackingTable->Buckets[RadarTrackingStruct(techno, point.X, point.Y).Hash()];
	RADAR_HASH_TABLE::ObjectType track;
	track.Key.Position = point;
	track.Key.Object = techno;
	track.Value = techno;
	if (bucket->Delete(track)) {
		Radar_Pixel(point);
		IsToRedraw = true;
	}
}


/// <summary>
/// Converts a map coordinate into a position on the radar.
/// </summary>
/// <param name="clip">Should the result be pulled back inside the radar rectangle?</param>
/// <returns>Returns with the radar pixel that the coordinate falls on.</returns>
Point2D RadarClass::Coord_To_Radar_Pixel(Coord const & coord, bool clip)
{
	Point2D pt = coord;
	int x = (pt.X - pt.Y + (RadarX * CELL_LEPTON_W)) * ZoomFactor / CELL_LEPTON_W;
	int y = (pt.X + pt.Y - (RadarY * CELL_LEPTON_H)) * ZoomFactor / CELL_LEPTON_H;
	if (clip) {
		if (x < 0) {
			x = 0;
		}
		if (x >= RadarRect.Width) {
			x = RadarRect.Width - 1;
		}
		if (y < 0) {
			y = 0;
		}
		if (y >= RadarRect.Height) {
			y = RadarRect.Height - 1;
		}
	}
	return(Point2D(x, y));
}


int RadarTrackingStruct::Hash_Old(RadarTrackingStruct const & s)
{
	return(((unsigned int)s.Object) + 251 * s.Position.X);
}

int RadarTrackingStruct::Hash2(RadarTrackingStruct const & s)
{
	return(s.Position.X + 251 * s.Position.Y);
}


/// <summary>
/// Creates the radar's object tracking table.
/// </summary>
void RadarClass::Init_Radar(void)
{
	RadarTrackingTable = new RADAR_HASH_TABLE(10, 256, RadarTrackingStruct::Hash2);
}


/// <summary>
/// Resets the radar map back to a blank slate.
/// This routine throws the object tracking table away and rebuilds the radar image from the
/// current local map bounds. Every object is marked as untracked so that it registers itself
/// again on its next logic pass.
/// </summary>
void RadarClass::Reset_Radar(void)
{
	if (RadarTrackingTable != NULL) {
		delete RadarTrackingTable;
	}

	Init_Radar();
	Map.Set_Local_Dimensions(Map.LocalRect);
	Compute_Radar_Image();

	for (int i = 0; i < Technos.Count(); i++) {
		Technos[i]->IsRadarTracked = false;
	}
}


/// <summary>
/// Frees everything the radar map has allocated.
/// </summary>
void RadarClass::Clear_Radar(void)
{
	if (RadarSurface != NULL) {
		delete RadarSurface;
		RadarSurface = NULL;
	}
	if (BackgroundSurface != NULL) {
		delete BackgroundSurface;
		BackgroundSurface = NULL;
	}
	if (BackgroundColors != NULL) {
		delete BackgroundColors;
		BackgroundColors = NULL;
	}
	if (RadarTrackingTable != NULL) {
		delete RadarTrackingTable;
		RadarTrackingTable = NULL;
	}
	if (PixelFlags != NULL) {
		delete [] PixelFlags;
		PixelFlags = NULL;
	}
}


/// <summary>
/// Handles the radar repairs needed after a save game is loaded.
/// The radar's surfaces and object tracking table are never saved, so this routine discards
/// the stale pointers and builds fresh ones from the loaded map. Every object is marked as
/// untracked so that it registers itself again as the game resumes.
/// </summary>
void RadarClass::Post_Load_Radar_Fixup(void)
{
	RadarSurface = NULL;
	BackgroundSurface = NULL;
	BackgroundColors = NULL;
	RadarTrackingTable = NULL;
	PixelFlags = NULL;

	Init_Radar();

	Map.Set_Local_Dimensions(Map.LocalRect);
	Compute_Radar_Image();

	for (int i = 0; i < Technos.Count(); i++) {
		Technos[i]->IsRadarTracked = false;
	}
	if (RadarMode == RMODE_MOVIE) {
		RadarState = RSTATE_MOVIE_DONE;
		Radar_Activate(SuspendedRadarMode, false);
		Set_Frame_Rate_Limit(Rule->DetailMinFrameRateNormal);
	}
}


/***********************************************************************************************
 * RadarClass::Plot_Radar_Pixel -- Updates the radar map with a terrain pixel.                 *
 *                                                                                             *
 *    This will update the radar map with a pixel. It is used to display                       *
 *    vehicle positions on the radar map.                                                      *
 *                                                                                             *
 * INPUT:   unit  -- Pointer to unit to render at the given position. If                       *
 *                   NULL is passed in, then the underlying terrain is                         *
 *                   displayed instead.                                                        *
 *                                                                                             *
 *          pos   -- Position on the map to update.                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:   This routine does NOT hide the mouse. It is up to you to                        *
 *             do so.                                                                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/04/1991 JLB : Created.                                                                 *
 *   06/21/1991 JLB : Large blips for units & buildings.                                       *
 *   02/14/1994 JLB : Revamped.                                                                *
 *   04/17/1995 PWG : Created.                                                                 *
 *   04/18/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Plot_Radar_Pixel(Point2D const & point)
{
	if (PlayerPtr == NULL || !RadarSurface->Get_Rect().Is_Point_Within(point)) {
		return;
	}

	Coord coord = Radar_Pixel_To_Cell(point);
	coord.Z = Map.Get_Height_GL(coord);
	bool shadow = MainWindow && Map.Is_Shrouded(coord);
	bool fog = MainWindow && Map.Is_Under_Fog(coord); // RA2

	int hash = (point.X - 5 * point.Y) & 0xFF;
	RADAR_HASH_TABLE::BucketType & bucket = RadarTrackingTable->Buckets[hash];
	int count = bucket.Count();
	RADAR_HASH_TABLE::ObjectType * entries = &bucket[0];
	TechnoClass * techno = NULL;
	for (int index = 0; index < count; ++index) {
		RadarTrackingStruct & track = entries[index].Key;
		TechnoClass * candidate = track.Object;
		if (track.Position != point) continue;
		if (shadow && !candidate->House->Is_Player_Control()) continue;
		if (candidate->Techno_Type_Class()->IsStealthy && !PlayerPtr->Is_Ally(candidate)) continue;

		techno = candidate;
		break;
	}

	if (techno != NULL) {
		HouseClass * house = techno->House;
		if (techno->IsDisguised()) {
			house = techno->GetDisguiseHouse(false);
		}
		int color = house ? DSurface::Build_Hicolor_Pixel(house->RemapColorRGB) : Fetch_Scheme_By_Name("LightGrey", 1)->Bright;
		int flash = techno->RadarFlashTimer;
		if (flash > 0 && (flash - 1) / Rule->FlashFrameTime % 2 == 1 && techno->House == PlayerPtr) {
			RadarSurface->Put_Pixel(point, ~color);
		} else {
			RadarSurface->Put_Pixel(point, color);
		}
	} else if (fog) {
		RGBClass rgb = DSurface::Deconstruct_Hicolor_Pixel(BackgroundSurface->Get_Pixel(point));
		RadarSurface->Put_Pixel(point, DSurface::Build_Hicolor_Pixel(rgb.Get_Red() / 2, rgb.Get_Green() / 2, rgb.Get_Blue() / 2));
	} else if (!shadow) {
		RadarSurface->Put_Pixel(point, BackgroundSurface->Get_Pixel(point));
	} else {
		RadarSurface->Put_Pixel(point, 0);
	}

	LastDrawRect = Union(LastDrawRect, Rect(point, 1, 1) + RadarRect.Top_Left());
}


/// <summary>
/// Draws every tracked object onto the radar surface.
/// This routine plots one blip per tracked object in the bright color of the owning house.
/// Disguised infantry are drawn in the local player's colors so that the disguise holds up on
/// the radar as well. Where several objects claim the same radar pixel, only the first one
/// found is drawn.
/// </summary>
void RadarClass::Render_Tracked_Objects(void)
{
	memset(PixelFlags, 0, RadarSurface->Get_Width() * RadarSurface->Get_Height() / 8 + 1);

	for (int b = 0; b < 256; b++) {

		RADAR_HASH_TABLE::BucketType &bucket = RadarTrackingTable->Buckets[b];

		int count = bucket.Count();

		RADAR_HASH_TABLE::ObjectType *ptr = &bucket[0];

		for (int t = 0; t < count; t++) {
			RadarTrackingStruct *track = &ptr[t].Key;
			TechnoClass *tech = track->Object;

			int id = track->Position.X + track->Position.Y * RadarSurface->Get_Width();
			int i = id >> 3;
			int bit = 1 << (id & 7);

			if ((PixelFlags[i] & bit) == 0) {
				PixelFlags[i] |= bit;

				ColorScheme *scheme = ColorSchemes[tech->House->Scheme];

				if (tech->IsDisguised()) { // RA2
					if (tech->GetDisguiseHouse(false) != NULL) {
						scheme = ColorSchemes[tech->GetDisguiseHouse(false)->Scheme];
					} else {
						scheme = Fetch_Scheme_By_Name("LightGrey", 1);
					}
				}

				int color = scheme->Bright;
				ConvertClass *drawer = scheme->Converter;
				if (drawer->Bytes_Per_Pixel() == 1) {
					unsigned char *translator = (unsigned char *)drawer->Get_Translate_Table();
					color = translator[color];
				} else {
					unsigned short *translator = (unsigned short *)drawer->Get_Translate_Table();
					color = translator[color];
				}

				RadarSurface->Put_Pixel(track->Position, color);
			}
		}
	}
}


/***********************************************************************************************
 * RadarClass::Radar_Pixel -- Mark a point to be rerendered on the radar map.                  *
 *                                                                                             *
 *    This routine is used to inform the system that a pixel needs to be                       *
 *    rerendered on the radar map. The pixel(s) will be rendered the                           *
 *    next time the map is refreshed.                                                          *
 *                                                                                             *
 * INPUT:   point  -- The map point to be rerendered.                                          *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/12/1992 JLB : Created.                                                                 *
 *   05/08/1994 JLB : Converted to member function.                                            *
 *=============================================================================================*/
void RadarClass::Radar_Pixel(Point2D const & point)
{
	if (point.X >= 0 && point.X < RadarSurface->Get_Width() && point.Y >= 0 && point.Y < RadarSurface->Get_Height()) {
		int id = point.X + point.Y * RadarSurface->Get_Width();
		int i = id >> 3;
		int bit = 1 << (id & 7);
		if ((PixelFlags[i] & bit) == 0) {
			PixelFlags[i] |= bit;
			PixelStack.Add(point);
			IsToRedraw = true;
		}
	}
}


/// <summary>
/// Computes the radar footprint of every building size.
/// This routine works out the diamond of radar pixels that a building of each size covers, so
/// that the radar can stamp a building down without recomputing the shape every time.
/// </summary>
/// <remarks>The footprints are measured in radar pixels, so this routine must be run again
/// whenever the radar zoom changes.</remarks>
void RadarClass::Compute_Foundations(void)
{
	int i;
	for (i = 0; i < BSIZE_COUNT; i++) {
		Foundation[i].Clear();

		int bwidth = BuildingTypeClass::SizeWidth[i];
		int bheight = BuildingTypeClass::SizeHeight[i];

		int zw;
		if (bwidth == 1) {
			zw = max(1.0, ZoomFactor + 0.5);
		} else {
			zw = max(2.0, bwidth * ZoomFactor + 0.5);
		}

		int zh;
		if (bheight == 1) {
			zh = max(1.0, ZoomFactor + 0.5);
		} else {
			zh = max(2.0, bheight * ZoomFactor + 0.5);
		}

		int fy = 0;
		int zs = zh + zw - 1;

		if (zs > 0) {
			int left_edge = 0;
			while (true) {
				int xbegin;
				if (fy < zh) {
					xbegin = left_edge;
				} else {
					xbegin = fy - 2 * zh + 2;
				}
				int xend;
				if (fy < zw) {
					xend = fy;
				} else {
					xend = 2 * zw - fy - 2;
				}
				int fx = xbegin;

				while ( fx <= xend )
				{
					Foundation[i].Add(Point2D(fx, fy));
					++fx;
				}
				++fy;
				--left_edge;
				if (fy >= zs) {
					break;
				}
			}
		}

	}
}


/// <summary>
/// Fetches the radar footprint for a building type.
/// </summary>
/// <returns>Returns with the list of radar pixels a building of that type's size
/// covers.</returns>
FOUNDATION_LIST const& RadarClass::Get_Foundation(BuildingTypeClass * btype)
{
	return(Foundation[btype->Size]);
}


/***********************************************************************************************
 * RadarClass::Radar_Pixel -- Mark a cell to be rerendered on the radar map.                   *
 *                                                                                             *
 *    This routine is used to inform the system that a cell needs to be                        *
 *    rerendered on the radar map. The pixel(s) will be rendered the                           *
 *    next time the map is refreshed.                                                          *
 *                                                                                             *
 * INPUT:   cell  -- The map cell to be rerendered.                                            *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/12/1992 JLB : Created.                                                                 *
 *   05/08/1994 JLB : Converted to member function.                                            *
 *=============================================================================================*/
void RadarClass::Radar_Cell(Cell const & cell)
{
	if (RadarSurface != NULL) {
		Rect r1 = Cell_Radar_Rect(cell);
		r1 = Intersect(r1, Rect(0, 0, RadarCellWidth, RadarCellHeight));
		int swidth = RadarSurface->Get_Width();
		int sheight = RadarSurface->Get_Height();

		if (r1.Is_Valid()) {

			int zx = (int)(r1.X * ZoomFactor);
			int startx = (zx - 1) & ((zx - 1 < 0) - 1);
			int width = min((int)((double)(r1.X + r1.Width) * ZoomFactor) + 1, swidth);

			int zy = (int)(r1.Y * ZoomFactor);
			int starty = (zy - 1) & ((zy - 1 < 0) - 1);
			int height = min((int)((double)(r1.Y + r1.Height) * ZoomFactor) + 1, sheight);

			for (int y = starty; y < height; y++) {
				for (int x = startx; x < width; x++) {
					Radar_Pixel(Point2D(x, y));
				}
			}
		}
	}
}


/// <summary>
/// Determines what lies underneath a point on the radar map.
/// This routine is used to turn a click on the radar into something actionable -- the tracked
/// object sitting on that radar pixel if there is one, otherwise the map cell that the pixel
/// stands for.
/// </summary>
/// <param name="point">The point on the radar, in screen coordinates.</param>
/// <param name="cell">Set to the map cell that the point refers to.</param>
/// <param name="object">Set to the tracked object under the point, or NULL if the point is
/// bare terrain.</param>
void RadarClass::Resolve_Radar_Point(Point2D const & point, Cell & cell, ObjectClass *& object)
{
	Point2D pt = point - RadarRect.TopLeft;
	RadarTrackingStruct track;
	track.Position = pt;

	RADAR_HASH_TABLE::BucketType & bucket = RadarTrackingTable->Buckets[track.Hash()];
	int count = bucket.Count();
	object = NULL;
	for (int index = count - 1; index >= 0; --index) {
		RadarTrackingStruct entry = bucket[index].Key;
		if (entry == track) {
			object = entry.Object;
			break;
		}
	}

	if (object != NULL) {
		cell = object->Destination_Coord();
	} else {
		cell = Radar_Pixel_To_Cell(pt);
	}
}


/// <summary>
/// Loads the radar map from the save game stream.
/// The pending update lists are constructed in place as they are read back. Everything else
/// the radar owns is rebuilt afterwards by Post_Load_Radar_Fixup.
/// </summary>
/// <returns>Returns with S_OK if the radar was read back, otherwise the failing stream
/// result.</returns>
HRESULT RadarClass::Load(IStream * stream)
{
	int i;

	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		int count;
		result = stream->Read(&count, sizeof(count), NULL);
		if (FAILED(result)) {
			return(result);
		}

		MovieSound.Init();

		new (&PixelStack) FOUNDATION_LIST;

		for (i = 0; i < count; i++) {
			Point2D point;
			result = stream->Read(&point, sizeof(point), NULL);
			if (FAILED(result)) {
				return(result);
			}
			PixelStack.Add(point);
		}

		result = stream->Read(&count, sizeof(count), NULL);
		if (FAILED(result)) {
			return(result);
		}

		new (&BackgroundStack) DynamicVectorClass<Cell>;

		for (i = 0; i < count; i++) {
			Cell cell;
			result = stream->Read(&cell, sizeof(cell), NULL);
			if (FAILED(result)) {
				return(result);
			}
			BackgroundStack.Add(cell);
		}

		for (i = 0; i < BSIZE_COUNT; i++) {
			new (&Foundation[i]) FOUNDATION_LIST;
		}

		result = stream->Read(&RadarMode, sizeof(RadarMode), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Read(&RadarState, sizeof(RadarState), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Read(&SuspendedRadarMode, sizeof(SuspendedRadarMode), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Read(&TransmissionState, sizeof(TransmissionState), NULL);
		if (FAILED(result)) {
			return(result);
		}

		MovieSound.Init();
		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Saves the radar map to the save game stream.
/// Only the pending update lists are written out. The radar surfaces and the object tracking
/// table are rebuilt from the map when the game is loaded again.
/// </summary>
/// <returns>Returns with S_OK if the radar was written, otherwise the failing stream
/// result.</returns>
HRESULT RadarClass::Save(IStream * stream)
{
	int i;

	HRESULT result = BASECLASS::Save(stream);
	if (SUCCEEDED(result)) {
		int count = PixelStack.Count();
		result = stream->Write(&count, sizeof(count), NULL);
		if (FAILED(result)) {
			return(result);
		}

		for (i = 0; i < count; i++) {
			result = stream->Write(&PixelStack[i], sizeof(PixelStack[i]), NULL);
			if (FAILED(result)) {
				return(result);
			}
		}

		count = BackgroundStack.Count();
		result = stream->Write(&count, sizeof(count), NULL);
		if (FAILED(result)) {
			return(result);
		}

		for (i = 0; i < count; i++) {
			result = stream->Write(&BackgroundStack[i], sizeof(BackgroundStack[i]), NULL);
			if (FAILED(result)) {
				return(result);
			}
		}

		result = stream->Write(&RadarMode, sizeof(RadarMode), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Write(&RadarState, sizeof(RadarState), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Write(&SuspendedRadarMode, sizeof(SuspendedRadarMode), NULL);
		if (FAILED(result)) {
			return(result);
		}
		result = stream->Write(&TransmissionState, sizeof(TransmissionState), NULL);
		return(SUCCEEDED(result) ? S_OK : result);
	}
	return(result);
}


/// <summary>
/// Sets the radar display into its activating or deactivating state.
/// This routine starts the radar opening or closing and plays the matching sound effect. The
/// display does not change over instantly -- the radar state machine carries it the rest of
/// the way.
/// </summary>
/// <param name="on">Should the radar display be brought up?</param>
void RadarClass::Set_Radar_State(bool on, bool sound)
{
	if (on == true) {
		if (RadarState == RSTATE_INACTIVE || RadarState == RSTATE_DEACTIVATING) {
			RadarState = RSTATE_ACTIVATING;
			MovieSound.EndLooping();
			if (sound) {
				VocClass::PlayGlobal(Rule->RadarOn, 1.0f, 0x2000);
			}
			DebugString("Radar: ACTIVATING\n");
		}
	} else {
		MovieSound.EndLooping();
		RadarState = RSTATE_DEACTIVATING;
		if (sound) {
			VocClass::PlayGlobal(Rule->RadarOff, 1.0f, 0x2000);
		}
		DebugString("Radar: DEACTIVING\n");
	}
	RadarAnimFrame = RadarAnimFrame;
}


/***********************************************************************************************
 * RadarClass::Is_Radar_Active -- Determines if the radar map is currently being displayed.    *
 *                                                                                             *
 *    Determines if the radar map is currently being displayed.                                *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the radar map currently being displayed as active?                        *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/12/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RadarClass::Is_Radar_Active(void)
{
	return(RadarState == RSTATE_ACTIVE);
}


/***********************************************************************************************
 * RadarClass::Radar_Activate -- Controls radar activation.                                    *
 *                                                                                             *
 *    Use this routine to turn the radar map on or off.                                        *
 *                                                                                             *
 * INPUT:   control  -- What to do with the radar map:                                         *
 *                      0 = Turn radar off.                                                    *
 *                      1 = Turn radar on.                                                     *
 *                      2 = Remove Radar Gadgets                                               *
 *                      3 = Add Radar Gadgets                                                  *
 *                      4 = Remove radar.                                                      *
 *                      -1= Toggle radar on or off.                                            *
 *                                                                                             *
 * OUTPUT:  bool; Was the radar map already on?                                                *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/11/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void RadarClass::Radar_Activate(int control, bool sound)
{
	if (Session.Type != GAME_NORMAL && PlayerPtr->IsDefeated && MuteSWLaunches) {
		control = 1;
	}
	if (RadarMode == control) {
		return;
	}
	if (RadarMode == RMODE_MOVIE || (RadarMode == RMODE_4 && control != RMODE_MOVIE)) {
		if (RadarState != RSTATE_MOVIE_DONE) {
			SuspendedRadarMode = control;
			return;
		}
	}
	switch (control) {
		case 0:
			Set_Radar_State(false, RadarMode == RMODE_TACTICAL);
			break;
		case 1:
			if ((RadarState == RSTATE_ACTIVE || RadarState == RSTATE_MOVIE_DONE) && Is_Radar_Existing() == true) {
				Queue_Next_Movie();
				break;
			}
			Set_Radar_State(Is_Radar_Existing(), sound);
			break;
		case 4:
			break;
		default:
			if (RadarState == RSTATE_ACTIVE || RadarState == RSTATE_MOVIE_DONE) {
				Queue_Next_Movie();
				break;
			}
			Set_Radar_State(true, false);
			break;
	}
	RadarMode = control;
}


/// <summary>
/// Determines if the radar pane is showing the tactical map.
/// </summary>
/// <returns>bool; Is the radar map itself currently displayed?</returns>
bool RadarClass::Is_Radar_Tactical(void)
{
	return(RadarMode == RMODE_TACTICAL && RadarState == RSTATE_ACTIVE);
}


/***********************************************************************************************
 * RadarClass::Is_Radar_Existing -- Queries to see if radar map is available.                  *
 *                                                                                             *
 *    This will determine if the radar map is available. If available, the radar will show     *
 *    representations of terrain, units, and buildings.                                        *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the radar map available to be displayed?                                  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/12/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool RadarClass::Is_Radar_Existing(void)
{
	return(DoesRadarExist);
}


/// <summary>
/// Sets whether the player has radar coverage at all.
/// This routine is used when a radar facility is gained or lost. Losing coverage shuts the
/// radar display down; regaining it brings the display back up.
/// </summary>
/// <param name="on">Does the player have a working radar facility?</param>
void RadarClass::Toggle_Radar(bool on)
{
	if (DoesRadarExist != on) {
		DoesRadarExist = on;
		char const * txt = on ? "on" : "off";
		DebugString("Radar: TacticalMap availability is %s\n", txt);
		if (RadarMode == RMODE_TACTICAL) {
			Set_Radar_State(on, true);
		} else {
			Radar_Activate(1, true);
		}
	}
}


/// <summary>
/// Determines if the radar pane is showing the player name list.
/// </summary>
/// <returns>bool; Is the player name list currently displayed?</returns>
bool RadarClass::Is_Player_Names(void)
{
	return(RadarMode == RMODE_PLAYER_NAMES && RadarState == RSTATE_ACTIVE);
}


/// <summary>
/// Determines if the radar pane is showing a movie.
/// </summary>
/// <returns>bool; Is a movie currently playing in the radar pane?</returns>
bool RadarClass::Is_Playing_Movie(void)
{
	return(RadarMode == RMODE_MOVIE && RadarState == RSTATE_ACTIVE);
}


/// <summary>
/// Flags the radar map to be redrawn.
/// </summary>
/// <param name="complete">Should the entire radar face be redrawn rather than just the
/// pending updates?</param>
void RadarClass::Redraw_Radar(bool complete)
{
	IsToRedraw = true;
	FullRedraw = FullRedraw || complete;
}


/// <summary>
/// Draws the radar map into the sidebar.
/// This routine tracks the tactical view rectangle across the radar face, flushes any pending
/// background and pixel updates, lets the radar events draw themselves, and then blits the
/// dirty portion of the radar surface onto the sidebar.
/// </summary>
void RadarClass::Render_Radar(void)
{
	int i;
	Rect prev_rect = LastDrawRect;
	bool is_tactical = Is_Radar_Tactical();

	Cell center = Map[TacticalMap->Pixel_To_Cell(Point2D(TacticalRect.X + TacticalRect.Width / 2, TacticalRect.Y + TacticalRect.Height / 2))].Fetch_CellID();

	RadarViewRect = Cell_To_Radar_Pixel(center);
	RadarViewRect.Width = ((2 * TacticalRect.Width) / (ISO_TILE_PIXEL_W / ZoomFactor) + 1.0f);
	RadarViewRect.Height = ((2 * TacticalRect.Height) / (ISO_TILE_PIXEL_H / ZoomFactor));
	RadarViewRect -= Point2D(TacticalRect.Width / (ISO_TILE_PIXEL_W / ZoomFactor), (2 * TacticalRect.Height) / (ISO_TILE_PIXEL_H / ZoomFactor) * 0.5f);

	if (RadarViewRect.X < RadarRect.X) {
		RadarViewRect.X = RadarRect.X;
	} else {
		if (RadarViewRect.X + RadarViewRect.Width >= RadarRect.X + RadarRect.Width) {
			RadarViewRect.X = RadarRect.X - (RadarViewRect.Width - RadarRect.Width) - 1;
		}
	}

	if (RadarViewRect.Y < RadarRect.Y) {
		RadarViewRect.Y = RadarRect.Y;
	} else {
		if (RadarViewRect.Y + RadarViewRect.Height >= RadarRect.Y + RadarRect.Height) {
			RadarViewRect.Y = RadarRect.Y - (RadarViewRect.Height - RadarRect.Height) - 1;
		}
	}

	if (FullRedraw) {
		OldRadarViewRect = RadarViewRect;
	}

	if (IsToRedraw || OldRadarViewRect != RadarViewRect || !No_Radar_Events_Submitted() || PixelStack.Count() > 0 || BackgroundStack.Count() > 0 || BeaconManager.Is_Flashing()) {
		if (is_tactical) {
			IsToRedraw = false;
		}

		Plot_Radar_Background();

		bool moved = false;
		if (OldRadarViewRect != RadarViewRect) {
			Point2D pt;
			for (i = 0; i < OldRadarViewRect.Height; i++) {
				pt = Point2D(OldRadarViewRect.X, i + OldRadarViewRect.Y) - Point2D(RadarRect.X, RadarRect.Y);
				Radar_Pixel(pt);
				pt = Point2D(OldRadarViewRect.X + OldRadarViewRect.Width - 1, i + OldRadarViewRect.Y) - Point2D(RadarRect.X, RadarRect.Y);
				Radar_Pixel(pt);
			}
			for (i = 0; i < OldRadarViewRect.Width; i++) {
				pt = Point2D(i + OldRadarViewRect.X, OldRadarViewRect.Y) - Point2D(RadarRect.X, RadarRect.Y);
				Radar_Pixel(pt);
				pt = Point2D(i + OldRadarViewRect.X, OldRadarViewRect.Height + OldRadarViewRect.Y - 1) - Point2D(RadarRect.X, RadarRect.Y);
				Radar_Pixel(pt);
			}
			moved = true;
		}

		if (CellRedrawRect.Is_Valid()) {
			RadarSurface->Blit_From(CellRedrawRect, *BackgroundSurface, CellRedrawRect);
			LastDrawRect = Union(LastDrawRect, CellRedrawRect + RadarRect.TopLeft);

			for (int x = CellRedrawRect.X; x < CellRedrawRect.X + CellRedrawRect.Width; x++) {
				for (int y = CellRedrawRect.Y; y < CellRedrawRect.Y + CellRedrawRect.Height; y++) {
					Plot_Radar_Pixel(Point2D(x, y));
				}
			}
			CellRedrawRect = Rect(0, 0, 0, 0);
		}

		/*
		**	Render all pixels in the "to redraw" stack.
		*/
		for (i = PixelStack.Count() - 1; i >= 0; i--) {
			Plot_Radar_Pixel(PixelStack[i]);
		}
		PixelStack.Clear();

		RadarEventClass::Draw_Events();
		BeaconManager.Draw_On_Radar(RadarSurface, RadarSurface->Get_Rect()); // RA2

		if (is_tactical) {
			if (FullRedraw) {
				LastDrawRect = RadarRect;
				Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, 32, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect()); // RA2
			}

			if (LastDrawRect.Is_Valid()) {
				SidebarSurface->Blit_From(LastDrawRect, *RadarSurface, LastDrawRect - Point2D(RadarRect.X, RadarRect.Y));
			}

			SidebarSurface->Draw_Rect(RadarViewRect, RadPHeight); // RA2: the tooltip colour, packed by Init_For_House
			Rect area = RadarRect;
			Rect rr(area.X - 1, area.Y - 1, area.Width + 2, area.Height + 2);
			SidebarSurface->Draw_Rect(rr, RadPHeight);
		}

		if (moved) {
			Rect const & inner = Union(OldRadarViewRect, LastDrawRect);
			LastDrawRect = Union(RadarViewRect, inner);
		}

		OldRadarViewRect = RadarViewRect;
		RadarEventClass::Remove_Finished();
	}

	if (PixelFlags != NULL) {
		memset(PixelFlags, 0, RadarSurface->Get_Height() * RadarSurface->Get_Width() / 8 + 1);
	}

	if (!is_tactical) {
		LastDrawRect = prev_rect;
	}
}


/// <summary>
/// Steps the incoming transmission that introduces a radar movie.
/// The EVA announcement plays first. Once it is done, and the other sound has faded, the
/// radar switches over to movie mode.
/// </summary>
void RadarClass::Update_Transmission(void) // RA2
{
	switch (TransmissionState) {
		case 0:
			Flush_Speech();
			Queue_Speech(TransmissionVoice, 0, 3);
			Add_Speech_Hold();
			TransmissionState = 1;
			break;

		case 1:
			if (Is_Speech_Finished(TransmissionVoice)) {
				if (IngameVQ.Count()) {
					AudioStreamerFadeOutAllStreams();
				}
				TransmissionState = 2;
			}
			break;

		case 2:
			if (IngameVQ.Count() == 0 || AudioStreamerAllFaded()) {
				if (IngameVQ.Count()) {
					Remove_Speech_Hold();
					Pause_Game_Audio();
				}
				Radar_Activate(RMODE_MOVIE, false);
				MovieSound.EndLooping();
				MovieSound.SetEvent((AudioEvent *)VocClass::PlayGlobal(Rule->MovieOn, 1.0f, 0x2000), NULL);
				Set_Frame_Rate_Limit(Rule->DetailMinFrameRateMovie);
			}
			break;
	}
}


/// <summary>
/// Handles the movie playing in the radar pane.
/// This routine ducks the game volume for the duration, advances the current in-game VQA by
/// one frame, and moves on to the next queued movie. Once the queue drains, the volume is
/// restored and the radar is handed back to whatever mode the movie interrupted.
/// </summary>
/// <remarks>Call this routine once per frame for as long as the radar is in movie
/// mode.</remarks>
void RadarClass::Play_Movie(void)
{
	MovieHandle * movie = NULL;
	bool hold = false;
	bool has = IngameVQ.Count() > 0;
	bool finished;

	if (!IsMoviePlaying && has) {
		Is_Speaking();
	}

	if (FullRedraw == true && !IsMoviePlaying) {
		Draw_Shape(*SidebarSurface, *SidebarDrawer, (ShapeSet const *)RadarAnim, 32, Point2D(RadX, RadWidth), SidebarSurface->Get_Rect());
		LastDrawRect = Rect(RadX, RadWidth, SidebarWidth, 110);
		DebugString("Radar: Movie full redrawn\n");
	}

	if (has) {
		if (IngameVQ.Count() > 0 && (movie = IngameVQ[0]) != NULL) {
			if (movie->AdvanceFrame() == true) {
				LastDrawRect = Rect(RadX, RadWidth, SidebarWidth, 110);
			}
			finished = movie->FramesLeft();
			hold = movie->State;
		} else {
			finished = true;
		}
	} else {
		finished = false;
	}

	if (finished == true) {
		if (movie != NULL) {
			IngameVQ.Delete(movie);
			delete movie;
		}
		has = IngameVQ.Count() != 0;
		if (has) {
			DebugString("Radar: Next movie.\n");
			MovieSound.EndLooping();
			MovieSound.SetEvent((AudioEvent *)VocClass::PlayGlobal(Rule->MovieOn, 1.0f, 0x2000), NULL);
			RadarState = RSTATE_NEXT_MOVIE;
			RadarAnimFrame = RADAR_ACTIVATED_FRAME;
		}
	}

	IsToRedraw = false;

	if (!has) {
		DebugString("Radar: Movie done.\n");
		RadarState = RSTATE_MOVIE_DONE;
		Radar_Activate(SuspendedRadarMode, false);
		Resume_Game_Audio();
		hold = has;
		MovieSound.EndLooping();
		MovieSound.SetEvent((AudioEvent *)VocClass::PlayGlobal(Rule->MovieOff, 1.0f, 0x2000), NULL);
		Set_Frame_Rate_Limit(Rule->DetailMinFrameRateNormal);
		IsToRedraw = true;
	}

	IsMoviePlaying = !finished;
	if (IsScenarioHeld != hold) {
		if (hold) {
			Hold_Scenario(0);
			IsScenarioHeld = true;
		} else {
			Release_Scenario();
			IsScenarioHeld = false;
		}
	}
}


/// <summary>
/// Redraws the whole radar map from the background image.
/// Use this routine when the radar contents can no longer be patched up piecemeal and
/// must be rebuilt in their entirety.
/// </summary>
void RadarClass::Complete_Radar_Refresh(void)
{
	if (MainWindow == NULL) {
		RadarSurface->Blit_From(RadarSurface->Get_Rect(), *BackgroundSurface, BackgroundSurface->Get_Rect());
		Render_Tracked_Objects();
	} else {
		int width = RadarSurface->Get_Width();
		int height = RadarSurface->Get_Height();
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				Plot_Radar_Pixel(Point2D(x, y));
			}
		}
	}
}
