// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "movie.h"
#include "geoconst1.h"
#include "_keyboar.h"
#include "_map.h"
#include "_rect.h"
#include "_surface.h"
#include "audattr.h"
#include "audlevel.h"
#include "audplay.h"
#include "ccfile.h"
#include "dbgprint.h"
#include "dsurface.h"
#include "globals.h"
#include "goptions.h"
#include "gscreen.h"
#include "movies.h"
#include "session.h"
#include "vector.h"
#include "vqa.h"
#include "vq.hh"
#include "_xmouse.h"
#include "msglist.h"
#include "geoconst2.h"
#include <math.h>

extern AudioAttribs * AudioGroupAttribs1; // RA2: 0x00831468
extern AudioAttribs * AudioGroupAttribs2; // RA2: 0x00831460
extern int SidebarMovieX; // RA2: 0x008336CC
extern int SidebarMovieY; // RA2: 0x008336D0
extern bool MovieAudioSuspended; // RA2: set while the movie has the audio suspended

void Audio_Pause_Channels(void); // RA2: 0x00406DE0
void Audio_Resume_Channels(void); // RA2: 0x00406E00
void __fastcall Set_Primary_Audio_Format(int rate, int bits, int channels); // RA2: 0x0040A6E0
void Restore_Primary_Audio_Format(void); // RA2: 0x0040A770
void Theme_Suspend(void); // RA2: 0x00716980
void Theme_Resume(void); // RA2: 0x007169F0

void Movie_Audio_Resume(void);
void Movie_Audio_Suspend(void);
void __fastcall Ingame_Movie_Subtitle(void * data);

bool __fastcall Find_Movie_File(char const * name, char * out, bool unused); // RA2: 0x005A1BB0
MovieHandle * __fastcall OwnerDraw_InitMovieHandle(char const * name, Surface * surface, Rect * bounds);
void Play_Movie(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before, bool fullscreen);

void Rebuild_Interpolated_Palette(unsigned char * interpal)
{
	for (int y=0; y<SIZE_OF_PALETTE - 1; y++) {
		for (int x=y+1; x<SIZE_OF_PALETTE; x++) {
			*(interpal + (y*SIZE_OF_PALETTE+x)) = *(interpal + (x*SIZE_OF_PALETTE+y));
		}
	}
}

void Play_Movie(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before)
{
	Play_Movie(name, theme, clrscrn_after, stretch, clrscrn_before, true);
}

void Play_Movie(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before, bool fullscreen)
{
	char buffer[256];
	if (!Find_Movie_File(name, buffer, true)) {
		return;
	}
	if (Session.Type != GAME_NORMAL) {
		return;
	}
	Keyboard->Clear();
	char * ext = strrchr(buffer, '.');
	if (ext != NULL && stricmp(ext, ".bik") == 0) {
		DebugString("Play_Movie() as Bink!\n");
		MouseCursor->Hide_Mouse();
		if (clrscrn_before) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(true, HiddenSurface);
		}
		BinkHandle bink(buffer);
		if (fullscreen == true) {
			bink.Flag = true;
		}
		Movie_Audio_Suspend();
		Audio_Pause_Channels();
		Set_Primary_Audio_Format(0xAC44, 0x10, 2);
		bink.Play();
		Restore_Primary_Audio_Format();
		Audio_Resume_Channels();
		Movie_Audio_Resume();
		if (clrscrn_after == true) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(true, HiddenSurface);
		}
		Keyboard->Clear();
		MouseCursor->Show_Mouse();
		Map.Flag_To_Redraw(GS_REDRAW_ALL);
		return;
	}

	Keyboard->Clear();
	VQHandle * vqa = Movie_Create(buffer, AlternateSurface, Rect(0,0,0,0), Rect(0,0,0,0), int(Options.VoiceVolume * 255.0), 1);
	if (vqa != NULL) {
		if (vqa->VQA->Get_VQA_Width() < 320 && vqa->VQA->Get_VQA_Height() < 200) {
			Movie_Destroy(vqa);
			delete vqa;
			return;
		}
		bool dostretch = (stretch == true && Options.StretchMovies == true);
		if (DSurface::AllowStretchBlits == true && dostretch == true) {
			if (vqa->InitialRect.Width < VisibleRect.Width && vqa->InitialRect.Height < VisibleRect.Height) {
				float ratio = ((float)VisibleRect.Width / (float)vqa->InitialRect.Width);
				vqa->StretchRect.Width = VisibleRect.Width;
				vqa->StretchRect.Height = (int)(vqa->InitialRect.Height * ratio);
				vqa->StretchRect.X = 0;
				vqa->StretchRect.Y = (VisibleRect.Height - vqa->StretchRect.Height) / 2;
				DebugString("Stretching movie %dx%d -> %dx%d\n", vqa->InitialRect.Width, vqa->InitialRect.Height, vqa->StretchRect.Width, vqa->StretchRect.Height);
			}
		}
		MouseCursor->Hide_Mouse();
		if (clrscrn_before) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(true, HiddenSurface);
		}
		Movie_Audio_Suspend();
		Audio_Pause_Channels();
		Set_Primary_Audio_Format(0xAC44, 0x10, 2);
		Movie_Play(vqa, true, theme, false);
		Restore_Primary_Audio_Format();
		Audio_Resume_Channels();
		Movie_Audio_Resume();
		Movie_Destroy(vqa);
		delete vqa;
		if (clrscrn_after == true) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(true, HiddenSurface);
		}
		MouseCursor->Show_Mouse();
		Map.Flag_To_Redraw(GS_REDRAW_ALL);
		Keyboard->Clear();
	}
}

void __fastcall Play_Movie_Simple(char const * name)
{
	MovieHandle * movie = OwnerDraw_InitMovieHandle(name, HiddenSurface, NULL);
	if (movie != NULL) {
		HiddenSurface->Fill(0);
		Update_Visible_Surface(true, HiddenSurface);
		Keyboard->Clear();
		Movie_Audio_Suspend();
		movie->Stop();
		Movie_Audio_Resume();
		Keyboard->Clear();
		delete movie;
	}
}

void Play_Movie(VQType vq, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before)
{
	if (vq >= 0 && vq < Movies.Count()) {
		bool active = TacticalActive;
		TacticalActive = false;
		Play_Movie(Movies[vq], THEME_NONE, clrscrn_after, stretch, clrscrn_before, false);
		TacticalActive = active;
	}
}

void __fastcall Play_Ingame_Movie(char const * name, bool loop)
{
	if (Session.Type == GAME_NORMAL) {
		MovieHandle * movie = OwnerDraw_InitMovieHandle(name, SidebarSurface, NULL);
		if (movie != NULL) {
			movie->SetPosition(SidebarMovieX - 3, SidebarMovieY - 1);
			movie->InitSubtitles(Ingame_Movie_Subtitle);
			movie->State = loop;
			IngameVQ.Add(movie);
		}
	}
}

void __fastcall Ingame_Movie_Subtitle(void * data)
{
	if (data != NULL) {
		Session.Messages.Add_Message(NULL, 0, ((wchar_t **)data)[5], 0, (TextPrintType)0x4046, ((int *)data)[1], false);
	}
}

void __fastcall Play_Ingame_Movie(VQType vq, bool loop)
{
	if (vq >= 0 && vq < Movies.Count()) {
		char const * name = Movies[vq];
		if (Session.Type == GAME_NORMAL) {
			MovieHandle * movie = OwnerDraw_InitMovieHandle(name, SidebarSurface, NULL);
			if (movie != NULL) {
				movie->SetPosition(SidebarMovieX - 3, SidebarMovieY - 1);
				movie->InitSubtitles(Ingame_Movie_Subtitle);
				movie->State = loop;
				IngameVQ.Add(movie);
			}
		}
	}
}

void Stop_Ingame_Movie(void)
{
	for (int i = 0; i < IngameVQ.Count(); i++) {
		if (IngameVQ[i] != NULL) {
			delete IngameVQ[i];
		}
	}
	IngameVQ.Clear();
	Movie_Audio_Resume();
}

void __fastcall Pause_Ingame_Movie(bool pause)
{
	if (IngameVQ.Count() > 0) {
		IngameVQ[0]->Pause(pause);
	}
}

bool Has_Ingame_Movies(void)
{
	return(IngameVQ.Count() > 0);
}

void Movie_Audio_Resume(void)
{
	if (MovieAudioSuspended) {
		AudioStreamerUnlockAllStreams();
		AudioStreamerResumeAllStreams();
		AudioStreamerFadeInAllStreams();
		AudioLevelAdjust(&AudioGroupAttribs1->VolumeLevel, 0x4000);
		AudioLevelAdjust(&AudioGroupAttribs2->VolumeLevel, 0x4000);
		Theme_Resume();
		MovieAudioSuspended = false;
	}
}

void Movie_Audio_Suspend(void)
{
	if (!MovieAudioSuspended) {
		Theme_Suspend();
		AudioStreamerLockAllStreams();
		AudioStreamerPauseAllStreams();
		AudioLevelAdjust(&AudioGroupAttribs1->VolumeLevel, 0x2000);
		AudioLevelAdjust(&AudioGroupAttribs2->VolumeLevel, 0x2000);
		MovieAudioSuspended = true;
	}
}
