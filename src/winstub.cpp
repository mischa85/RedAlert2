// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "winstub.h"
#include "geoconst1.h"
#include "_keyboar.h"
#include "_map.h"
#include "_rect.h"
#include "_surface.h"
#include "_tooltip.h"
#include "ccfile.h"
#include "cctooltip.h"
#include "convert.h"
#include "dbgprint.h"
#include "draw.h"
#include "dsurface.h"
#include "globals.h"
#include "init.h"
#include "misc.h"
#include "movie.h"
#include "movies.h"
#include "pcx.h"
#include "win.h"
#include "windlg.h"
#include "winfix.h"
#include "wsproto.h"
#include "wwmouse.h"
#include "bsurface.h"
#include "geoconst2.h"
#include <commctrl.h>
#include <math.h>
#define WINIMMAPI
#include <imm.h>

HANDLE	MainThread;
int		ShowCommand;
HWND	MainWindow;
HWND	UnusedWindow;

HINSTANCE	ProgramInstance;
bool _MouseCaptured;
bool _MouseWheel;

/*
 * Taken from later Windows SDK after what is shipped in VS6
 */

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  /// message that will be supported
#endif

#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

void Audio_Focus_Lose(void);	// RA2: 0x00406F50
void Audio_Focus_Regain(void);	// RA2: 0x00406F70

wchar_t IMEResultString[256];	// RA2: 0x00B26 ; characters received through WM_IME_CHAR
int IMEResult;	// RA2: the value Windows_Procedure returns for a message the IME handled
int IMEComposing;
int IMECompositionStringLength;
int IMECompositionCursorPos;
wchar_t IMECompositionString[256];
HIMC IMEContext;
wchar_t IMECompositionTerminator;

int __fastcall IME_Service_Message(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);	// RA2: 0x00737FC0

void Focus_Loss(void)
{
	DebugString("Focus_Loss()\n");
	Pause_Ingame_Movie(true);
	Audio_Focus_Lose();	// RA2
	if (MouseCursor) {
		_MouseCaptured = MouseCursor->Is_Captured();
		DebugString("Focus_Loss(): _MouseCaptured = %s\n", _MouseCaptured ? "true" : "false");
		MouseCursor->Release_Mouse();
	}
}

void Focus_Restore(void)
{
	DebugString("Focus_Restore()\n");
	Audio_Focus_Regain();	// RA2
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
			Map.IsForceCompleteRedraw = true;	// RA2
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
	Pause_Ingame_Movie(false);
	if (WS_Top_Window()) {
		SetActiveWindow(WS_Top_Window());
		SetFocus(WS_Top_Window());
	}
}

LRESULT CALLBACK Windows_Procedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	if (PacketTransport) {
		if (message == (UINT) PacketTransport->Protocol_Event_Message(INVALID_SOCKET) || message == 0x475) {	// RA2: 0x475 too
			if (PacketTransport->Message_Handler(hwnd, message, wParam, lParam)) {
				return(DefWindowProc(hwnd, message, wParam, lParam));
			} else {
				return(0);
			}
		}
	}

	Map.Message_Handler(hwnd, message, wParam, lParam);

	if (MainWindow) {
		GetMenu(MainWindow);
	}

	if (IME_Service_Message(hwnd, message, wParam, lParam)) {	// RA2
		DebugString("Default\n");
		return(IMEResult);
	}

	switch (message) {
		case WM_SHOWWINDOW:
			return(0);

		case WM_PAINT:
			if (SpecialDialog == 0) {	// RA2
				if (GameInFocus == true || WindowedMode == true) {
					if (MouseCursor != NULL && VisibleSurface != NULL && HiddenSurface != NULL && CompositeSurface != NULL) {
						if (ScenarioActive == true) {
							Update_Visible_Surface(MouseCursor->Is_Captured(), CompositeSurface);
							Map.Blit_Sidebar(true);
							ValidateRect(hwnd, NULL);
							break;
						} else if (Movie_Is_Playing()) {
							Movie_Update_Visible_Surface();
							ValidateRect(hwnd, NULL);
							break;
						} else {
							Update_Visible_Surface(MouseCursor->Is_Captured(), HiddenSurface);
						}
					}
				}
			}
			ValidateRect(hwnd, NULL);
			break;

		case WM_ERASEBKGND:
			return(1);

		case WM_CREATE:
			ToolTips = new CCToolTip(hwnd);
			break;

		case WM_MOVE:
			if (WindowedMode == true && MouseCursor != NULL) {
				((WWMouseClass *)MouseCursor)->Calc_Confining_Rect();
			}
			break;

		case WM_DESTROY:
			if (ToolTips != NULL) {
				delete ToolTips;
				ToolTips = NULL;
			}
			MainWindow = 0;

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
			switch (wParam) {
				case SC_CLOSE:
					return(0);

				case SC_SCREENSAVE:
					return(0);
			}
			break;

	}

	if (Keyboard->Message_Handler(hwnd, message, wParam, lParam)) {
		return(0);
	}

	if (message == WM_IME_CHAR) {	// RA2
		int length = wcslen(IMEResultString);
		char mb[4];
		int wc;
		if (wParam & 0xFF00) {
			mb[0] = (char)(wParam >> 8);
			mb[1] = (char)wParam;
			mb[2] = 0;
		} else {
			mb[0] = (char)wParam;
			mb[1] = 0;
		}
		int converted = MultiByteToWideChar(0, 0, mb, strlen(mb), (LPWSTR)&wc, 1);
		IMEResultString[length] = (converted == 1) ? wc : 0;
		IMEResultString[length + 1] = 0;
		return(0);
	}

	return(DefWindowProc(hwnd, message, wParam, lParam));
}

void Build_Info(void)
{
}

void Build_Date_String(char * buffer, int buflen)
{
	SYSTEMTIME systime;

	if (FileTimeToSystemTime ((LPFILETIME)(&BuildDate[28]), &systime) ) {
		sprintf(buffer, "%02d/%02d/%04d - %02d:%02d:%02d", systime.wMonth, systime.wDay, systime.wYear, systime.wHour, systime.wMinute, systime.wSecond);
	}
}

char *Build_Number_String(char * buffer, int buflen)
{
	sprintf (buffer, "%d", *(unsigned int*)(&BuildNumber[28]));
	return(buffer);
}

unsigned int Build_Number(void)
{
	return(*(unsigned int*)(&BuildNumber[28]));
}

char *Build_By_String(char * buffer, int buflen)
{
	strncpy(buffer, &BuildNumber[32], 32);
	return(buffer);
}

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

char *Build_Version_String(char * buffer, int buflen)
{
	char _buffer1[128];
	char _buffer2[128];

	sprintf(buffer, "%s-%s", Build_By_Initials(_buffer2, sizeof(_buffer2)), Build_Number_String(_buffer1, sizeof(_buffer1)));
	return(buffer);
}

HICON ProgramIcon;
HCURSOR ProgramCursor;

extern "C" unsigned int __cdecl _mbsnccnt(const unsigned char * string, unsigned int count);

#define CC_ICON		93	// RA2
#define CC_CURSOR	104
#define WINDOW_NAME		"Red Alert 2"	// RA2

void Create_Main_Window(HINSTANCE instance, int command_show, int width, int height)
{
	InitCommonControls();

	ProgramIcon = LoadIcon(instance, MAKEINTRESOURCE(CC_ICON));
	ProgramCursor = LoadCursor(ProgramInstance, MAKEINTRESOURCE(CC_CURSOR));

	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = Windows_Procedure;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = instance;
	wndclass.hIcon = ProgramIcon;
	wndclass.hCursor = ProgramCursor;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = WINDOW_NAME;

	RegisterClass(&wndclass);

	if (WindowedMode) {
		MainWindow = CreateWindowEx(0, WINDOW_NAME, WINDOW_NAME, WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX|WS_CLIPCHILDREN, 0, 0, 0, 0, NULL, NULL, instance, NULL);

		RECT rect;
		SetRect(&rect, 0, 0, width, height);
		AdjustWindowRectEx(&rect, GetWindowLong(MainWindow, GWL_STYLE), GetMenu(MainWindow) != 0, GetWindowLong(MainWindow, GWL_EXSTYLE));
		MoveWindow(MainWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 1);
	} else {
		MainWindow = CreateWindowEx(WS_EX_TOPMOST, WINDOW_NAME, WINDOW_NAME, WS_POPUP|WS_CLIPCHILDREN, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, instance, NULL);
	}

	IMEContext = ImmGetContext(MainWindow);	// RA2

	ShowWindow(MainWindow, SW_NORMAL);
	ShowCommand = command_show;
	UpdateWindow(MainWindow);
	SetFocus(MainWindow);

	RegisterHotKey(MainWindow, 1, MOD_ALT|MOD_CONTROL|MOD_SHIFT, 'M');

	SetCursor(LoadCursor(ProgramInstance, MAKEINTRESOURCE(CC_CURSOR)));
}

void IME_Cancel_Composition(void)
{
	IMECompositionString[0] = 0;
	IMECompositionStringLength = 0;
	IMECompositionCursorPos = 0;
	IMEComposing = 0;
	IMEResultString[0] = 0;
	if (IMEContext) {
		ImmNotifyIME(IMEContext, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
	}
}

struct IMECandidateList {
	unsigned long Size;
	unsigned long Style;
	unsigned long Count;
	unsigned long Selection;
	unsigned long PageStart;
	unsigned long PageSize;
	unsigned long Offset[1];
};

void __fastcall IME_Get_Candidate_Text(HIMC context, unsigned int index, char * buffer, int length)
{
	if (context) {
		unsigned int size = ImmGetCandidateListA(context, index, NULL, 0);
		if (size) {
			IMECandidateList * list = (IMECandidateList *)malloc(size);
			if (list) {
				if (list->Count && list->Selection < list->Count) {
					strncat(buffer, (char const *)list->Offset[list->Selection], length);
					buffer[length - 1] = 0;
				}
				free(list);
			}
		}
	}
}

void IME_Update_Composition(void)
{
	IMECompositionString[0] = 0;
	IMECompositionStringLength = 0;
	IMECompositionCursorPos = 0;
	if (IMEContext) {
		char buffer[513];
		int size = ImmGetCompositionStringA(IMEContext, GCS_COMPSTR, buffer, 512);
		if (size > 0) {
			buffer[size] = 0;
			int wide = MultiByteToWideChar(0, 0, buffer, -1, IMECompositionString, 256);
			IMECompositionTerminator = 0;
			int length;
			if (wide < 0) {
				length = 0;
			} else {
				IMECompositionCursorPos = ImmGetCompositionStringA(IMEContext, GCS_CURSORPOS, NULL, 0) & 0xFFFF;
				length = wcslen(IMECompositionString);
			}
			int cursor = _mbsnccnt((unsigned char *)buffer, IMECompositionCursorPos);
			IMECompositionCursorPos = cursor;
			IMECompositionString[length] = 0;
			IMECompositionStringLength = length;
			if (cursor > length) {
				IMECompositionCursorPos = length;
			} else if (cursor < 0) {
				IMECompositionCursorPos = 0;
			}
		}
	}
}

void __fastcall IME_Send_Result(HWND window)
{
	if (IMEContext) {
		char buffer[513];
		int size = ImmGetCompositionStringA(IMEContext, GCS_RESULTSTR, buffer, 512);
		if (size > 0) {
			buffer[size] = 0;
			wchar_t wide[257];
			int count = MultiByteToWideChar(0, 0, buffer, strlen(buffer), wide, 256);
			if (count < 0) {
				count = 0;
			}
			wide[count] = 0;
			for (int i = 0; i < count; i++) {
				SendMessage(window, 0x4D9, wide[i], wide[i]);
			}
		}
	}
}

int __fastcall IME_Service_Message(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case 0x10F:
			IME_Update_Composition();
			IMEResult = DefWindowProc(hwnd, 0x10F, wParam, lParam);
			return(1);

		case 0x10E:
			IME_Update_Composition();
			IMEComposing = 0;
			IMEResult = DefWindowProc(hwnd, 0x10E, wParam, lParam);
			return(1);

		case 0x10D:
			IME_Update_Composition();
			IMEResultString[0] = 0;
			IMEComposing = 1;
			IMEResult = DefWindowProc(hwnd, 0x10D, wParam, lParam);
			return(1);

		case 0x281:
			IMEResult = DefWindowProc(hwnd, 0x281, wParam, lParam);
			return(1);

		case 0x282:
		case 0x285:
		case 0x290:
		case 0x291:
			IMEResult = DefWindowProc(hwnd, message, wParam, lParam);
			return(1);
	}
	return(0);
}

extern int WideScreenThreshold;	// RA2: 0x007ADD70
extern bool __fastcall Fill_Surface_Rect(Surface * surface, Rect const & rect, int color);	// RA2: 0x006F3A20
extern Rect __fastcall Get_Title_Screen_Area(void);	// RA2: 0x006F1F30

int IME_Get_Result(void)
{
	return(IMEResult);
}

void IME_Close(void)
{
	if (IMEContext) {
		ImmSetOpenStatus(IMEContext, FALSE);
	}
}

void IME_Close_Alt(void)	// RA2: same body as IME_Close
{
	if (IMEContext) {
		ImmSetOpenStatus(IMEContext, FALSE);
	}
}

void IME_Stub1(void)	// RA2: empty
{
}

void IME_Stub2(void)	// RA2: empty
{
}

void IME_Stub3(void)	// RA2: empty
{
}

wchar_t IME_Char_To_Wide(unsigned int wparam, long lparam)
{
	char mb[4];
	int wc;
	if (wparam & 0xFF00) {
		mb[0] = (char)(wparam >> 8);
		mb[1] = (char)wparam;
		mb[2] = 0;
	} else {
		mb[0] = (char)wparam;
		mb[1] = 0;
	}
	int converted = MultiByteToWideChar(0, 0, mb, strlen(mb), (LPWSTR)&wc, 1);
	return (converted == 1) ? wc : 0;
}

void __fastcall Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette)
{
	CCFileClass file(name);
	Surface * load_buffer = Read_PCX_File(file, palette);

	if (load_buffer) {
		int surface_width = surface->Get_Width();
		int surface_height = surface->Get_Height();
		Rect screen(0, 0, surface_width, surface_height);
		BSurface * black = new BSurface(surface_width, surface_height, 2);
		Fill_Surface_Rect(black, screen, 0);
		surface->Blit_From(*black, false, true);
		delete black;

		int x = 0;
		int y = 0;
		int width = load_buffer->Get_Width();
		int height = load_buffer->Get_Height();
		Point2D origin;
		if (surface->Get_Width() > WideScreenThreshold) {
			Rect area = Get_Title_Screen_Area();
			origin = Point2D(area.X, area.Y);
			x = (area.Width - width) / 2;
			y = (area.Height - height) / 2;
		}
		if (palette && load_buffer->Bytes_Per_Pixel() == 1) {
			ConvertClass * drawer = new ConvertClass(*palette, *palette, *surface);
			Blit_Block(*surface, *drawer, *load_buffer, load_buffer->Get_Rect(), Point2D(x, y), surface->Get_Rect());
			delete drawer;
		} else {
			surface->Blit_From(surface->Get_Rect(), Rect(x, y, load_buffer->Get_Width(), load_buffer->Get_Height()), *load_buffer, load_buffer->Get_Rect(), load_buffer->Get_Rect());
		}
		delete load_buffer;
	}
}
