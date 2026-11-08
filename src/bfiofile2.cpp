// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "rect.h"
#include <windows.h>
#include <string.h>
#include "movies.h"
#include "surface.h"
#include "dsurface.h"
#include "_surface.h"
#include "keyboard.h"
#include "globals.h"
#include "gscreen.h"
#include "rawfile.h"
#include "mixfile.h"
#include "goptions.h"
#include "win.h"
#include "msgloop.h"
#include "dbgprint.h"
#include "_keyboar.h"

extern "C" {
__declspec(dllimport) BinkInfo * __stdcall BinkOpen(void const *, unsigned int);
__declspec(dllimport) int __stdcall BinkSetSoundSystem(void *, unsigned int);
__declspec(dllimport) void * __stdcall BinkOpenDirectSound(unsigned int);
__declspec(dllimport) int __stdcall BinkSetVolume(BinkInfo *, int);
__declspec(dllimport) int __stdcall BinkDDSurfaceType(void *);
__declspec(dllimport) int __stdcall BinkDoFrame(BinkInfo *);
__declspec(dllimport) void __stdcall BinkNextFrame(BinkInfo *);
__declspec(dllimport) void __stdcall BinkClose(BinkInfo *);
__declspec(dllimport) void __stdcall BinkGoto(BinkInfo *, unsigned int, int);
__declspec(dllimport) int __stdcall BinkPause(BinkInfo *, int);
__declspec(dllimport) int __stdcall BinkWait(BinkInfo *);
__declspec(dllimport) char * __stdcall BinkGetError(void);
__declspec(dllimport) int __stdcall BinkCopyToBuffer(BinkInfo *, void *, int, unsigned int, unsigned int, unsigned int, unsigned int);
}


int __fastcall AudioDeviceAvailable(void); // RA2: whether the audio library has a device
void * GetDirectSoundObject(void); // RA2: the DirectSound object of the audio library
/*
**	RA2: the voice volume last handed to Bink; a change in the options is passed on as the movie plays.
*/
float BinkVolume;



/// <summary>
/// Opens the movie specified, to be drawn to the screen.
/// </summary>
BinkHandle::BinkHandle(char const * name)
{
	Flag = false;
	Bink = NULL;
	DrawSurface = NULL;
	Subtitles = NULL;
	Finished = false;
	FileHandle = INVALID_HANDLE_VALUE;
	Playing = true;
	Open(name);
}

/// <summary>
/// Opens the movie specified, to be drawn to the surface specified.
/// </summary>
BinkHandle::BinkHandle(char const * name, Surface * surface)
{
	Flag = false;
	DrawSurface = surface;
	Bink = NULL;
	Subtitles = NULL;
	FileHandle = INVALID_HANDLE_VALUE;
	Playing = true;
	Finished = false;
	Open(name);
}

/// <summary>
/// Closes the movie and its subtitles.
/// </summary>
BinkHandle::~BinkHandle(void)
{
	if (Subtitles != NULL) {
		delete Subtitles;
		Subtitles = NULL;
	}
	if (Bink != NULL) {
		BinkClose(Bink);
		Bink = NULL;
	}
	if (FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(FileHandle);
		FileHandle = INVALID_HANDLE_VALUE;
	}
}

/// <summary>
/// Opens a movie from a loose file or from inside a mixfile, centring it on its surface.
/// </summary>
bool BinkHandle::Open(char const * name)
{
	Close();
	if (AudioDeviceAvailable()) {
		BinkSetSoundSystem(BinkOpenDirectSound, (unsigned int)GetDirectSoundObject());
	}
	bool exists = (RawFileClass(name).Is_Available(0) == 1);
	if (exists) {
		Bink = BinkOpen(name, 0);
	} else {
		int offset = 0;
		MixFileClass * mix = NULL;
		if (MixFileClass::Offset(name, NULL, &mix, &offset, NULL) == true) {
			FileHandle = CreateFileA(mix->Filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
			if (FileHandle != INVALID_HANDLE_VALUE) {
				SetFilePointer(FileHandle, offset, NULL, FILE_BEGIN);
				Bink = BinkOpen(FileHandle, 0x800000);
			}
		}
	}
	if (Bink == NULL) {
		DebugString("Bink Error: %s\n", BinkGetError());
		return false;
	}
	memcpy(&BinkVolume, &Options.VoiceVolume, sizeof(BinkVolume));
	BinkSetVolume(Bink, (int)(BinkVolume * 32768.0f));
	FrameDelay = 60 / (Bink->FrameRate / Bink->FrameRateDiv);
	Subtitles = Subtitles_Open(name);
	Rect draw(0, 0, Bink->Width, Bink->Height);
	if (DrawSurface == NULL) {
		if (Subtitles != NULL && Subtitles->mSubTitles != NULL) {
			DrawSurface = HiddenSurface;
			draw.X = (DrawSurface->Get_Width() - draw.Width) / 2;
			draw.Y = (DrawSurface->Get_Height() - draw.Height) / 2;
		} else {
			DrawSurface = VisibleSurface;
			RECT rc;
			GetClientRect(MainWindow, &rc);
			draw.X = (rc.right - rc.left - draw.Width) / 2;
			draw.Y = (rc.bottom - rc.top - draw.Height) / 2;
		}
	}
	DestRect = Intersect(DrawSurface->Get_Rect(), draw);
	CopyFlags = BinkDDSurfaceType(((DSurface *)VisibleSurface)->Get_DD_Surface());
	return true;
}

/// <summary>
/// Closes the movie, its subtitles and the file it was read from.
/// </summary>
void BinkHandle::Close(void)
{
	if (Subtitles != NULL) {
		delete Subtitles;
		Subtitles = NULL;
	}
	if (Bink != NULL) {
		BinkClose(Bink);
		Bink = NULL;
	}
	if (FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(FileHandle);
		FileHandle = INVALID_HANDLE_VALUE;
	}
}

/// <summary>
/// Places the movie with its top left corner at the position specified.
/// </summary>
void BinkHandle::Set_Position(int x, int y)
{
	DestRect = Intersect(DrawSurface->Get_Rect(), Rect(x, y, Bink->Width, Bink->Height));
}

/// <summary>
/// Moves the movie to the frame specified.
/// </summary>
void BinkHandle::Goto_Frame(int frame)
{
	BinkGoto(Bink, frame, 1);
}

/// <summary>
/// Fetches how long each frame of the movie is shown, in milliseconds.
/// </summary>
int BinkHandle::Frame_Time(void)
{
	return 1000 / (Bink->FrameRate / Bink->FrameRateDiv);
}

/// <summary>
/// Sets the routine the subtitles call as each one is shown.
/// </summary>
void BinkHandle::Set_Subtitle_Callback(void (__fastcall * callback)(void *))
{
	if (Subtitles != NULL) {
		Subtitles->SetCallback(callback);
	}
}

/// <summary>
/// Pauses or resumes the movie.
/// </summary>
void BinkHandle::Pause(bool pause)
{
	BinkPause(Bink, pause);
}

/// <summary>
/// Have all the frames of the movie been shown?
/// </summary>
bool BinkHandle::Is_Finished(void) const
{
	return(Bink->FrameNum >= Bink->Frames); // RA2: no start frame
}

/// <summary>
/// Plays the whole movie, returning once it ends or the player presses escape.
/// </summary>
void BinkHandle::Play(void)
{
	if (Bink == NULL) {
		DebugString("Bink Error: %s\n", BinkGetError());
		return;
	}
	Keyboard->Clear();
	Playing = true;
	Finished = false;
	for (;;) {
		if (Is_Finished()) {
			break;
		}
		MessageLoopCount++;
		MSG msg;
		if (Peek_Windows_Message(&msg, 0, 0, 0, 0)) {
			if (!Get_Windows_Message(&msg, 0, 0, 0)) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		if (!Playing) {
			Sleep(33);
		}
		if (DrawSurface == VisibleSurface) {
			RECT rc;
			GetClientRect(MainWindow, &rc);
			ClientToScreen(MainWindow, (POINT *)&rc);
			int px = DestRect.X + rc.left;
			int py = DestRect.Y + rc.top;
			Draw_Frame(DrawSurface, px, py);
		} else if (Draw_Frame(DrawSurface, DestRect.X, DestRect.Y)) {
			RECT rc;
			GetClientRect(MainWindow, &rc);
			ClientToScreen(MainWindow, (POINT *)&rc);
			Rect dest = DestRect;
			dest.X += rc.left;
			dest.Y += rc.top;
			VisibleSurface->Blit_From(dest, *DrawSurface, DestRect, false, true);
		}
		if (Keyboard->Check() != KN_NONE && Flag == false) {
			if (Keyboard->Get() == (KN_ESC | KN_RLSE_BIT)) {
				break;
			}
		}
	}
	Keyboard->Clear();
}

/// <summary>
/// Draws any frames that are due to the surface specified; true if a frame was drawn.
/// </summary>
bool BinkHandle::Draw_Frame(Surface * surface, int x, int y)
{
	if (BinkVolume != Options.VoiceVolume) {
		memcpy(&BinkVolume, &Options.VoiceVolume, sizeof(BinkVolume));
		BinkSetVolume(Bink, (int)(BinkVolume * 32768));
	}
	if (!GameInFocus) {
		if (Playing == true) {
			DebugString("Pause bink movie\n");
			Playing = false;
			BinkPause(Bink, 1);
		}
	} else if (!Playing) {
		DebugString("Resume bink movie\n");
		Playing = true;
		BinkPause(Bink, 0);
		if (DrawSurface == VisibleSurface) {
			RECT rc;
			GetClientRect(MainWindow, &rc);
			ClientToScreen(MainWindow, (POINT *)&rc);
			int px = DestRect.X + rc.left;
			int py = DestRect.Y + rc.top;
			Copy_Frame(DrawSurface, px, py, true);
		} else {
			Copy_Frame(DrawSurface, DestRect.X, DestRect.Y, true);
		}
	}
	bool ready = Finished;
	if (!ready) {
		ready = BinkWait(Bink) == 0;
	}
	if (!ready) {
		return false;
	}
	do {
		BinkDoFrame(Bink);
		Finished = false;
		bool subtitle = false;
		if (Subtitles != NULL) {
			subtitle = Subtitles->Process(Bink->FrameNum * FrameDelay);
			if (subtitle) {
				Erase_Subtitle(surface);
			}
		}
		void * buffer = surface->Lock(Point2D(0, 0));
		if (buffer != NULL) {
			int pitch = surface->Stride();
			int height = surface->Get_Height();
			unsigned int flags = CopyFlags;
			if (subtitle) {
				flags |= 0x80000000;
			}
			BinkCopyToBuffer(Bink, buffer, pitch, height, x, y, flags);
			surface->Unlock();
		}
		Draw_Subtitle(surface);
		BinkNextFrame(Bink);
	} while (BinkWait(Bink) == 0);
	return true;
}

/// <summary>
/// Notes whether the next frame is due without drawing it.
/// </summary>
void BinkHandle::Check_Frame_Ready(void)
{
	if (BinkWait(Bink) == 0) {
		Finished = true;
	}
}

/// <summary>
/// Draws any frames that are due to the movie's own surface; true if a frame was drawn.
/// </summary>
bool BinkHandle::Render_Frame(void)
{
	return Draw_Frame(DrawSurface, DestRect.X, DestRect.Y);
}

/// <summary>
/// Draws the current frame again at the movie's own position.
/// </summary>
void BinkHandle::Redraw(void)
{
	if (DrawSurface == VisibleSurface) {
		RECT rc;
		GetClientRect(MainWindow, &rc);
		ClientToScreen(MainWindow, (POINT *)&rc);
		int x = DestRect.X + rc.left;
		int y = DestRect.Y + rc.top;
		Redraw_Frame(DrawSurface, x, y);
	} else {
		Redraw_Frame(DrawSurface, DestRect.X, DestRect.Y);
	}
}

/// <summary>
/// Draws the current frame and its subtitle again at the position specified.
/// </summary>
void BinkHandle::Redraw_Frame(Surface * surface, int x, int y)
{
	Erase_Subtitle(surface);
	void * buffer = surface->Lock(Point2D(0, 0));
	if (buffer != NULL) {
		int pitch = surface->Stride();
		int height = surface->Get_Height();
		BinkCopyToBuffer(Bink, buffer, pitch, height, x, y, CopyFlags | 0x80000000);
		surface->Unlock();
	}
	Draw_Subtitle(surface);
}

/// <summary>
/// Copies the current frame to the surface specified, optionally with its subtitle.
/// </summary>
void BinkHandle::Copy_Frame(Surface * surface, int x, int y, bool subtitle)
{
	if (subtitle) {
		Erase_Subtitle(surface);
	}
	void * buffer = surface->Lock(Point2D(0, 0));
	if (buffer != NULL) {
		int pitch = surface->Stride();
		int height = surface->Get_Height();
		unsigned int flags = CopyFlags;
		if (subtitle) {
			flags |= 0x80000000;
		}
		BinkInfo * bink = Bink;
		BinkCopyToBuffer(bink, buffer, pitch, height, x, y, flags);
		surface->Unlock();
	}
	Draw_Subtitle(surface);
}

/// <summary>
/// Draws the current subtitle below the movie.
/// </summary>
void BinkHandle::Draw_Subtitle(Surface * surface)
{
	MovieSubtitles * subtitles = Subtitles;
	if (subtitles != NULL && subtitles->mSubTitles != NULL) {
		Rect * caption = &subtitles->CaptionRect;
		if (caption->Width > 0 && caption->Height > 0) {
			Surface * image = subtitles->SubSurface;
			Rect size = image->Get_Rect();
			Rect dest = *caption;
			int dx = (DestRect.Width - size.Width) / 2;
			int dy = DestRect.Height - size.Height;
			dest.X += dx;
			dest.Y += dy;
			dest.X += DestRect.X;
			dest.Y += DestRect.Y;
			surface->Blit_From(dest, *image, *caption, true, true);
		}
	}
}

/// <summary>
/// Clears the area the previous subtitle occupied.
/// </summary>
void BinkHandle::Erase_Subtitle(Surface * surface)
{
	if (Subtitles != NULL && Subtitles->mSubTitles != NULL) {
		Rect rect = Subtitles->PreviousRect;
		if (rect.Width > 0 && rect.Height > 0) {
			Rect size = Subtitles->SubSurface->Get_Rect();
			int dx = (DestRect.Width - size.Width) / 2;
			int dy = DestRect.Height - size.Height;
			rect.X += dx;
			rect.Y += dy;
			rect.X += DestRect.X;
			rect.Y += DestRect.Y;
			surface->Fill_Rect(rect, 0);
		}
	}
}

/// <summary>
/// Fetches how far into the movie play has got, in milliseconds.
/// </summary>
int BinkHandle::Current_Time(void)
{
	return Bink->FrameNum * FrameDelay;
}

/// <summary>
/// Pauses the movie while the game is not in focus, and resumes it when it is again.
/// </summary>
void BinkHandle::Update_Focus(void)
{
	if (!GameInFocus) {
		if (Playing == true) {
			DebugString("Pause bink movie\n");
			Playing = false;
			BinkPause(Bink, 1);
		}
	} else if (!Playing) {
		DebugString("Resume bink movie\n");
		Playing = true;
		BinkPause(Bink, 0);
		Redraw();
	}
}

struct BitCursor {
	unsigned char * Ptr;
	unsigned char Bit;
};

/// <summary>
/// Starts reading bits from the buffer specified, returning the first one.
/// </summary>
unsigned char __fastcall Bits_Open_Read(BitCursor * cursor, unsigned char * ptr)
{
	cursor->Ptr = ptr;
	cursor->Bit = 0;
	unsigned char first = *ptr;
	first &= 1;
	return first;
}

/// <summary>
/// Reads the next bit.
/// </summary>
bool __fastcall Bits_Read(BitCursor * cursor)
{
	cursor->Bit++;
	if (cursor->Bit == 8) {
		cursor->Bit = 0;
		cursor->Ptr++;
	}
	int mask = 1 << cursor->Bit;
	int v = *cursor->Ptr;
	unsigned char r = ((*cursor->Ptr & (1 << cursor->Bit)) > 0);
	if ((*cursor->Ptr & (1 << cursor->Bit)) > 0) return 1;
	return 0;
}

/// <summary>
/// Starts writing bits to the buffer specified, with the first byte given.
/// </summary>
void __fastcall Bits_Open_Write(BitCursor * cursor, unsigned char * ptr, unsigned char value)
{
	cursor->Ptr = ptr;
	cursor->Bit = 0;
	*ptr = value;
}

/// <summary>
/// Writes the next bit.
/// </summary>
void __fastcall Bits_Write(BitCursor * cursor, unsigned char bit)
{
	cursor->Bit++;
	if (cursor->Bit == 8) {
		cursor->Bit = 0;
		cursor->Ptr++;
		*cursor->Ptr = 0;
	}
	*cursor->Ptr |= (unsigned char)(bit << cursor->Bit);
}

/// <summary>
/// Copies the bits specified, inserting a set bit after every seven so that no byte is zero; returns the bits added.
/// </summary>
int __fastcall Bit_Stuff(unsigned char * source, unsigned char * dest, int count)
{
	BitCursor in;
	BitCursor out;
	int stuffed = 1;
	unsigned int index = 0;
	Bits_Open_Write(&out, dest, Bits_Open_Read(&in, source));
	while (--count != 0) {
		index++;
		if (index % 7 == 0) {
			Bits_Write(&out, 1);
			stuffed++;
		}
		Bits_Write(&out, Bits_Read(&in));
	}
	return stuffed;
}

/// <summary>
/// Copies stuffed bits back, dropping every eighth; returns the bytes written.
/// </summary>
unsigned int __fastcall Bit_Unstuff(unsigned char * source, unsigned char * dest, unsigned int count)
{
	BitCursor in;
	BitCursor out;
	unsigned int bytes = (count + 7) >> 3;
	Bits_Open_Write(&out, dest, Bits_Open_Read(&in, source));
	unsigned int index = 0;
	while (--count != 0) {
		index++;
		if (index % 7 == 0) {
			Bits_Read(&in);
		}
		Bits_Write(&out, Bits_Read(&in));
	}
	return bytes;
}

/// <summary>
/// Unstuffs a wide string of up to the characters specified; returns the characters written.
/// </summary>
unsigned int __fastcall Bit_Unstuff_String(unsigned char * source, wchar_t * dest, unsigned int count)
{
	BitCursor in;
	BitCursor out;
	Bits_Open_Write(&out, (unsigned char *)dest, Bits_Open_Read(&in, source));
	unsigned int index = 0;
	int bits = 0;
	unsigned int words = 0;
	while (words < count) {
		++index;
		if (index % 7 == 0) Bits_Read(&in);
		Bits_Write(&out, Bits_Read(&in));
		if (++bits % 16 == 0) {
			++words;
			if (*(wchar_t *)(out.Ptr - 2) == 0) return words;
		}
	}
	dest[count - 1] = 0;
	return words;
}
