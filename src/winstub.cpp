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

/* $Header: /CounterStrike/WINSTUB.CPP 3     3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WINSTUB.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 10/04/95                                                     *
 *                                                                                             *
 *                  Last Update : October 4th 1995 [ST]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *   This file contains stubs for undefined externals when linked under Watcom for Win 95      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   Assert_Failure -- display the line and source file where a failed assert occurred         *
 *   Check_For_Focus_Loss -- check for the end of the focus loss                               *
 *   Create_Main_Window -- opens the MainWindow for C&C                                        *
 *   Focus_Loss -- this function is called when a library function detects focus loss          *
 *   Memory_Error_Handler -- Handle a possibly fatal failure to allocate memory                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "winstub.h"

#include "_keyboar.h"
#include "_map.h"
#include "_rect.h"
#include "_surface.h"
#include "_tooltip.h"
#include "ccfile.h"
#include "cctooltip.h"
#include "cdcntrl.h"
#include "convert.h"
#include "dbgprint.h"
#include "draw.h"
#include "dsaudio.h"
#include "dsurface.h"
#include "globals.h"
#include "init.h"
#include "misc.h"
#include "movie.h"
#include "movies.h"
#include "pcx.h"
#include "resource.h"
#include "theme.h"
#include "win.h"
#include "windlg.h"
#include "winfix.h"
#include "wsproto.h"
#include "wwmouse.h"
#include "mainopt.h"
#include "conquer.h"

#include <commctrl.h>

HANDLE	MainThread;
int		ShowCommand;
HWND	MainWindow;
HWND	UnusedWindow;

HINSTANCE	ProgramInstance;
bool _MouseCaptured;
bool _MouseWheel;


//void output(short,short)
//{}

/*
 * Taken from later Windows SDK after what is shipped in VS6
 */

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  /// message that will be supported
#endif

#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif
///////////////////////////////////////////////////////////

//unsigned long CCFocusMessage = WM_USER+50;	//Private message for receiving application focus
extern	void VQA_PauseAudio(void);
extern	void VQA_ResumeAudio(void);

ThemeType OldTheme = THEME_NONE;


/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus loss            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 2:10PM ST : Created                                                               *
 *=============================================================================================*/

void Focus_Loss(void)
{
	DebugString("Focus_Loss()\n");
	Pause_Ingame_Movie(true);
	OldTheme = Theme.What_Is_Playing();
	Theme.Suspend();
	if (Audio_Available()) Audio.Stop_Primary_Sound_Buffer();
	if (MouseCursor) {
		_MouseCaptured = MouseCursor->Is_Captured();
		DebugString("Focus_Loss(): _MouseCaptured = %s\n", _MouseCaptured ? "true" : "false");
		MouseCursor->Release_Mouse();
	}
}


/// <summary>
/// Restores the game when it regains the input focus.
/// This routine is the counterpart to Focus_Loss. It starts the sound and the music
/// back up, recaptures the mouse if it was captured when focus was lost, checks the
/// display surfaces for loss, and flags the whole screen for redraw.
/// </summary>
void Focus_Restore(void)
{
	DebugString("Focus_Restore()\n");
	if (Audio_Available()) Audio.Start_Primary_Sound_Buffer(TRUE);
	DebugString("Focus_Restore(): _MouseCaptured = %s\n", _MouseCaptured ? "true" : "false");
	if (MouseCursor && _MouseCaptured == true && !Debug_Map) {
		MouseCursor->Capture_Mouse();
	}
	if (!WindowedMode) {
		if (AlternateSurface != NULL) {
			((DSurface *)AlternateSurface)->Restore_Check();
		}
		if (HiddenSurface != NULL) {
			((DSurface *)HiddenSurface)->Restore_Check();
		}
		if (CompositeSurface != NULL && ((DSurface *)CompositeSurface)->Restore_Check()) {
			if (!ScenarioActive || Frame > 16) {
				CompositeSurface->Fill(0);
			}
		}
		if (TileSurface != NULL && ((DSurface *)TileSurface)->Restore_Check()) {
			if (!ScenarioActive || Frame > 16) {
				TileSurface->Fill(0);
			}
		}
		if (SidebarSurface != NULL && ((DSurface *)SidebarSurface)->Restore_Check()) {
			if (!ScenarioActive || Frame > 16) {
				SidebarSurface->Fill(0);
			}
		}
		if (VisibleSurface != NULL && ((DSurface *)VisibleSurface)->Restore_Check()) {
			if (!ScenarioActive || Frame > 16) {
				VisibleSurface->Fill_Rect(VisibleRect, 0);
			}
		}
	}
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
	InvalidateRect(MainWindow, 0, 0);
	Theme.Play_Song(OldTheme);
	Pause_Ingame_Movie(false);
	if (WS_Top_Window()) {
		SetActiveWindow(WS_Top_Window());
		SetFocus(WS_Top_Window());
	}
}


extern bool InMovie;

/// <summary>
/// Handles the Windows messages sent to the main game window.
/// This is the window procedure registered for the main window. It offers each
/// message to the network transport, the map and the keyboard handlers, deals with
/// the messages the game must react to itself -- focus changes, painting, tray
/// locking and shutdown -- and passes everything else back to Windows.
/// </summary>
/// <returns>Returns with the result Windows expects for the message handled.</returns>
LRESULT CALLBACK /*_export*/ Windows_Procedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{

	int	low_param = LOWORD(wParam);

	/*
	**	Pass on any messages intended for the winsock message handler.
	*/
	if ( PacketTransport ) {
		if ( message == (UINT) PacketTransport->Protocol_Event_Message() ) {
			if ( PacketTransport->Message_Handler (hwnd, message, wParam, lParam) ){
				return( DefWindowProc (hwnd, message, wParam, lParam) );
			}else{
				return(0);
			}
		}
	}

	Map.Message_Handler(hwnd, message, wParam, lParam);

	if (MainWindow) {
		GetMenu(MainWindow);
	}

	switch ( message ) {
//		case WM_SYSKEYDOWN:
//			Mono_Printf("wparam=%08X lparam=%08X\n", (long)wParam, (long)lParam);
			// fall through

//		case WM_MOUSEMOVE:
//		case WM_KEYDOWN:
//		case WM_SYSKEYUP:
//		case WM_KEYUP:
//		case WM_LBUTTONDOWN:
//		case WM_LBUTTONUP:
//		case WM_LBUTTONDBLCLK:
//		case WM_MBUTTONDOWN:
//		case WM_MBUTTONUP:
//		case WM_MBUTTONDBLCLK:
//		case WM_RBUTTONDOWN:
//		case WM_RBUTTONUP:
//		case WM_RBUTTONDBLCLK:
//	 		Keyboard->Message_Handler(hwnd, message, wParam, lParam);
//			return(0);

		case WM_SHOWWINDOW:
			return(0);

		case WM_PAINT:
			if (GameInFocus == true || WindowedMode == true) {
				if (MouseCursor != NULL && VisibleSurface != NULL && HiddenSurface != NULL && CompositeSurface != NULL) {
					if (ScenarioActive == true) {
						Update_Visible_Surface(MouseCursor->Is_Captured(), CompositeSurface);
						Map.Blit_Sidebar(true);
					} else if (Movie_Is_Playing() == true) {
						Movie_Update_Visible_Surface();
					} else {
						Update_Visible_Surface(MouseCursor->Is_Captured(), HiddenSurface);
					}
				}
			}
			ValidateRect(hwnd, NULL);
			break;

		case WM_ERASEBKGND:
			return(1);

		case WM_CLOSE:
			CDControl.Unlock_All_CD_Trays();
			break;

		case WM_CREATE:
			ToolTips = new CCToolTip(hwnd);
			if (ToolTips) {
				ToolTips->Set_Timer_Delay(500);
			}
			break;

		case WM_MOVE:
			if (WindowedMode == true && MouseCursor != NULL) {
				((WWMouseClass *)MouseCursor)->Calc_Confining_Rect();
			}
			break;

			/*
			**	Windoze message says we have to shut down. Try and do it cleanly.
			*/
		case WM_DESTROY:
			if (ToolTips != NULL) {
				delete ToolTips;
				ToolTips = NULL;
			}
			CDControl.Unlock_All_CD_Trays();
			MainWindow = 0;

			/*
			**	If we are shutting down gracefully than flag that the message loop has finished.
			**	If this is a forced shutdown (ReadyToQuit == 0) then try and close down everything
			**	before we exit.
			*/
			switch (ReadyToQuit) {
				default:
				case 1:
					ReadyToQuit = 2;
					break;

				case 0:
					break;

			}
			return(0);

		case WM_ACTIVATEAPP:
			if (hwnd == MainWindow && GameInFocus != (wParam != 0)) {
				GameInFocus = (wParam != 0);
				if (!GameInFocus) {
					Focus_Loss();
					DebugString("Focus lost\n");
				} else {
					Focus_Restore();
					DebugString("Focus gained\n");
				}
				SurfacesRestored = true;
			}
			return(0);

		case WM_RBUTTONUP:
			Map.Set_Scroll_Coasting_Allowed(false);
			break;

		case WM_MOVING:
			return(On_WM_MOVING(hwnd, wParam, lParam));

		case WM_MOUSEWHEEL:
			if (!_MouseWheel) {
				_MouseWheel = true;
				if (GET_WHEEL_DELTA_WPARAM(wParam) < 0) {
					Execute_Command("SidebarDown");
				} else {
					Execute_Command("SidebarUp");
				}
				_MouseWheel = false;
			}
			break;

		case WM_SYSCOMMAND:
			switch ( wParam ) {

				case SC_CLOSE:
					CDControl.Unlock_All_CD_Trays();
					/*
					**	Windows sent us a close message. Probably in response to Alt-F4. Ignore it by
					**	pretending to handle the message and returning true;
					*/
					return(0);

				case SC_SCREENSAVE:
					/*
					**	Windoze is about to start the screen saver. If we just return without passing
					**	this message to DefWindowProc then the screen saver will not be allowed to start.
					*/
					return(0);
			}
			break;

	}

	/*
	**	Pass this message through to the keyboard handler. If the message
	**	was processed and requires no further action, then return with
	**	this information.
	*/
	if (Keyboard->Message_Handler(hwnd, message, wParam, lParam)) {
		return(0);
	}

	return(DefWindowProc (hwnd, message, wParam, lParam));
}


/// <summary>
/// Reports the build information for this executable.
/// This routine is an empty hook. Callers that actually need the build stamp fetch
/// it through the Build_Date_String, Build_Number and Build_By_String accessors.
/// </summary>
void Build_Info(void)
{
}


/// <summary>
/// Fetches the date and time this executable was built.
/// This routine is used by the crash reporter and the debug overlay to identify the
/// build, and formats the stamped time as month, day, year and clock time.
/// </summary>
/// <remarks>The buffer is left as it was found if the stamped time cannot be
/// converted.</remarks>
void Build_Date_String(char * buffer, int buflen)
{
	SYSTEMTIME systime;

	if (FileTimeToSystemTime ((LPFILETIME)(&BuildDate[28]), &systime) ) {
		sprintf(buffer, "%02d/%02d/%04d - %02d:%02d:%02d", systime.wMonth, systime.wDay, systime.wYear, systime.wHour, systime.wMinute, systime.wSecond);
	}
}


/// <summary>
/// Fetches the build number as a printable string.
/// </summary>
/// <returns>Returns with a pointer to the buffer supplied.</returns>
char *Build_Number_String(char * buffer, int buflen)
{
	sprintf (buffer, "%d", *(unsigned int*)(&BuildNumber[28]));
	return(buffer);
}


/// <summary>
/// Fetches the build number of this executable.
/// This routine is used by the network and modem code to check that every machine
/// joining a game is running the same build.
/// </summary>
/// <returns>Returns with the build number stamped into the executable.</returns>
unsigned int Build_Number(void)
{
	return(*(unsigned int*)(&BuildNumber[28]));
}


/// <summary>
/// Fetches the name of whoever built this executable.
/// This routine is used by the crash reporter and the debug overlay so that a bad
/// build can be traced back to the machine that made it.
/// </summary>
/// <returns>Returns with a pointer to the buffer supplied.</returns>
char *Build_By_String(char * buffer, int buflen)
{
	strncpy(buffer, &BuildNumber[32], 32);
	return(buffer);
}


/// <summary>
/// Fetches the initials of whoever built this executable.
/// The build stamp carries the builder's name in "First_Last" form, and this routine
/// reduces it to the two initials.
/// </summary>
/// <returns>Returns with a pointer to the buffer supplied.</returns>
char *Build_By_Initials(char * buffer, int buflen)
{
	buffer[0] = BuildNumber[32];
	buffer[1] = 0;
	buffer[2] = 0;

	char *lastname = strchr(&BuildNumber[32], '_');
	if (lastname) {
		buffer[1] = lastname[1];
	}
	return(buffer);
}


/// <summary>
/// Builds the short version string for this executable.
/// This routine is used where the build must be identified in very little space --
/// the builder's initials and the build number, joined by a dash.
/// </summary>
/// <returns>Returns with a pointer to the buffer supplied.</returns>
char *Build_Version_String(char * buffer, int buflen)
{
	char _buffer1[128];
	char _buffer2[128];

	sprintf(buffer, "%s-%s", Build_By_Initials(_buffer2, sizeof(_buffer2)), Build_Number_String(_buffer1, sizeof(_buffer1)));
	return(buffer);
}


/***********************************************************************************************
 * Create_Main_Window -- opens the MainWindow for C&C                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    instance -- handle to program instance                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/10/95 4:08PM ST : Created                                                             *
 *=============================================================================================*/

#define CC_ICON		IDI_SUN
#define CC_CURSOR	IDC_CURSOR1

#define WINDOW_NAME		"Tiberian Sun"


void Create_Main_Window ( HINSTANCE instance , int command_show , int width , int height )
{
	InitCommonControls();

	WNDCLASS    	wndclass ;
	//
	// Register the window class
	//

	wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
	wndclass.lpfnWndProc   = Windows_Procedure ;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = 0 ;
	wndclass.hInstance     = instance ;
	wndclass.hIcon         = LoadIcon (instance, MAKEINTRESOURCE(CC_ICON)) ;
	wndclass.hCursor       = LoadCursor(ProgramInstance, MAKEINTRESOURCE(CC_CURSOR));
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName  = NULL;	///WINDOW_NAME
	wndclass.lpszClassName = WINDOW_NAME;

	RegisterClass (&wndclass) ;


	//
	// Create our main window
	//
	if (WindowedMode) {
		MainWindow = CreateWindowEx(
								0,
								WINDOW_NAME,
								WINDOW_NAME,
								WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX|WS_CLIPCHILDREN,
								0,
								0,
								0,
								0,
								NULL,
								NULL,
								instance,
								NULL );

		RECT rect;
		SetRect(&rect, 0, 0, width, height);
		AdjustWindowRectEx(&rect, GetWindowLong(MainWindow, GWL_STYLE), GetMenu(MainWindow) != 0, GetWindowLong(MainWindow, GWL_EXSTYLE));
		MoveWindow(MainWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 1);

	} else {
		MainWindow = CreateWindowEx(
								WS_EX_TOPMOST,
								WINDOW_NAME,
								WINDOW_NAME,
								WS_POPUP|WS_CLIPCHILDREN,
								0,
								0,
								// Denzil 5/18/98 - Making window fullscreen prevents other apps
								// from getting WM_PAINT messages
								GetSystemMetrics(SM_CXSCREEN), //width,
								GetSystemMetrics(SM_CYSCREEN), //height,
								// End Denzil
								NULL,
								NULL,
								instance,
								NULL );
	}
// Denzil
width = width; height = height;
// End

	ShowWindow (MainWindow, SW_NORMAL);
	ShowCommand = command_show;
	UpdateWindow (MainWindow);
	SetFocus (MainWindow);

	RegisterHotKey(MainWindow, 1, MOD_ALT|MOD_CONTROL|MOD_SHIFT, VK_M);

	SetCursor(LoadCursor(ProgramInstance, MAKEINTRESOURCE(CC_CURSOR)));
	Audio.Audio_Focus_Loss_Function = Focus_Loss;

	//Misc_Focus_Loss_Function = &Focus_Loss;
	//Misc_Focus_Restore_Function = &Focus_Restore;
	//Gbuffer_Focus_Loss_Function = &Focus_Loss;
}


/// <summary>
/// Loads a title screen picture and centers it on the surface.
/// This routine is used by the startup and scenario loading sequences to put some
/// artwork on the screen while the game gets itself ready. A paletted picture is
/// drawn through a converter built from the palette supplied.
/// </summary>
/// <param name="name">The name of the picture file to load.</param>
/// <param name="surface">The surface to draw the title screen upon.</param>
/// <param name="palette">The palette to load the picture's colors into.</param>
void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette)
{
	Surface *load_buffer;
	CCFileClass file(name);
	load_buffer = Read_PCX_File (file, palette);

	if (load_buffer) {
		Point2D point;
		int x = (surface->Get_Width() - load_buffer->Get_Width()) / 2;
		int y = (surface->Get_Height() - load_buffer->Get_Height()) / 2;
		if (palette && load_buffer->Bytes_Per_Pixel() == 1) {
			ConvertClass *drawer = new ConvertClass(*palette, *palette, *surface);
			Blit_Block(*surface, *drawer, *load_buffer, load_buffer->Get_Rect(), Point2D(x, y), surface->Get_Rect());
			delete drawer;
		} else {

			surface->Blit_From(surface->Get_Rect(), Rect(x, y, load_buffer->Get_Width(), load_buffer->Get_Height()), *load_buffer, load_buffer->Get_Rect(), load_buffer->Get_Rect());
		}
		delete load_buffer;
	}
}
