// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#include "always.h"
#include "geoconst1.h"
#include "movies.h"
#include "multiplayer.h"
#include "_map.h"
#include "_surface.h"
#include "dsaudio.h"
#include "dsurface.h"
#include "surface.h"
#include "theme.h"
#include "vqa.h"
#include "dbgprint.h"
#include "vqoption.h"
#include "win.h"
#include "ccfile.h"
#include "goptions.h"
#include "globals.h"
#include "geoconst2.h"
#include <string.h>
#include <cmath>

int MovieUseLoops = 0; // RA2: 0x00A710B4
MovieHandle * ActiveMovieHandle = NULL; // RA2: 0x00A710B0
namespace Game { bool __fastcall IsMoviePlaying(void); void __fastcall BlitMovie(void); }

int __fastcall AudioDeviceAvailable(void); // RA2: 0x00406F30

DynamicVectorClass<char const *> Movies;

void * Movie_Lock_Surface(void)
{
	Surface *surf = CurrentVQ->DrawSurface;
	void *buffptr = surf->Lock();
	VQAClass * vqa = CurrentVQ->VQA;

	vqa->Set_Draw_Buffer(0,
		surf->Stride() / surf->Bytes_Per_Pixel(),
		surf->Get_Height(),
		CurrentVQ->InitialRect.X,
		CurrentVQ->InitialRect.Y
	);

	return(buffptr);
}

bool Movie_Unlock_Surface(void)
{
	return(CurrentVQ->DrawSurface->Unlock());
}

void Movie_Blit_To_Screen(void)
{
	RECT rect;

	if (!Get_Option(OPTION_NO_BUFFER)) {
		GetClientRect(MainWindow, &rect);
		ClientToScreen(MainWindow, (LPPOINT)&rect);

		Rect area;
		area.Set(rect.left + CurrentVQ->StretchRect.X, rect.top + CurrentVQ->StretchRect.Y,
			CurrentVQ->StretchRect.Width, CurrentVQ->StretchRect.Height);

		VisibleSurface->Blit_From(
			area,
			*CurrentVQ->DrawSurface,
			CurrentVQ->InitialRect
		);
	}
}

void Movie_Blit_Subtitled(void)
{
	RECT rect;

	if (!Get_Option(OPTION_NO_BUFFER)) {
		HiddenSurface->Blit_From(CurrentVQ->InitialRect, *CurrentVQ->DrawSurface, CurrentVQ->InitialRect);

		if (CurrentVQ->Subtitles != NULL && CurrentVQ->Subtitles->mSubTitles != NULL) {
			Surface *sub = CurrentVQ->Subtitles->SubSurface;
			Rect src = sub->Get_Rect();
			Rect dest;
			dest.X = (HiddenSurface->Get_Width() - src.Width) / 2;
			dest.Y = HiddenSurface->Get_Height() - src.Height;
			dest.Width = src.Width;
			dest.Height = src.Height;
			HiddenSurface->Blit_From(dest, *sub, src, true, true);
		}

		GetClientRect(MainWindow, &rect);
		ClientToScreen(MainWindow, (LPPOINT)&rect);

		Rect area;
		area.Set(rect.left + CurrentVQ->StretchRect.X, rect.top + CurrentVQ->StretchRect.Y,
			CurrentVQ->StretchRect.Width, CurrentVQ->StretchRect.Height);

		VisibleSurface->Blit_From(
			area,
			*HiddenSurface,
			CurrentVQ->InitialRect
		);
	}
}

VQHandle * Movie_Create(char const * name, Surface * surface, Rect rect1, Rect rect2, int volume, bool fullscreen)
{
	VQA_SURF_DRAW_CALLBACK callback = NULL;
	VQHandle *handle = new VQHandle;
	if (handle != NULL) {
		if (!CCFileClass(name).Is_Available()) {
			return(NULL);
		}
		int flags = 0;

		if (!AudioDeviceAvailable()) {
			Set_Option(OPTION_NO_AUDIO);
		}

		if (Get_Option(OPTION_PLAY_FROM_MIXFILE)) {
			flags |= VQACF_PLAY_FROM_MIXFILE;
		}

		if (MovieUseLoops == 1) {
			flags |= VQACF_2;
		}

		if (fullscreen == true) {
			callback = Movie_Blit_Subtitled;
		}

		handle->VQA = new VQAClass(name, flags, Movie_Lock_Surface, Movie_Unlock_Surface, callback);
		if (handle->VQA == NULL) {
			delete handle;
			return(NULL);
		}

		handle->VQA->Set_VQA_Volume(volume);

		if (!handle->VQA->Open_And_Load_Buffers()) {
			delete handle;
			return(NULL);
		}

		int width = handle->VQA->Get_VQA_Width();
		int height = handle->VQA->Get_VQA_Height();
		int cmode = handle->VQA->Get_Desired_Color_Mode();

		if (cmode == 4) {
			cmode = 1;
		}

		if (rect1 == Rect(0,0,0,0)) {
			rect1 = Rect(Point2D(0, 0), width, height);
		}

		if (rect2 == Rect(0,0,0,0)) {
			rect2 = Rect((surface->Get_Width() - width) / 2, (surface->Get_Height() - height) / 2, width, height);
		}

		Rect irect1(0, 0, width, height);
		Rect irect2(0, 0, surface->Get_Width(), surface->Get_Height());

		handle->InitialRect = Intersect(rect1, irect1);
		handle->StretchRect = Intersect(rect2, irect2);

		if (cmode == 1 || cmode == 4) {
			handle->VQA->Set_Primary_Color_Mode(DSurface::Get_Primary_Color_Mode());
		}

		handle->DrawSurface = surface;
		handle->VQA->Set_Draw_Buffer(NULL, surface->Stride() / surface->Bytes_Per_Pixel(), surface->Get_Height());
		handle->Subtitles = Subtitles_Open(name);
		if (handle->Subtitles != NULL) {
			handle->VQA->Subtitles = handle->Subtitles;
		}
		handle->IsInitialized = true;
		return(handle);
	}
	return(NULL);
}

void Movie_Destroy(VQHandle * handle)
{
	if (handle != NULL) {
		if (handle->VQA != NULL) {
			handle->VQA->Close_And_Free_VQA();
			delete handle->VQA;
			handle->VQA = NULL;
		}
		if (handle->Subtitles != NULL) {
			delete handle->Subtitles;
			handle->Subtitles = NULL;
		}
		handle->IsInitialized = false;
	}
}

void Movie_Play(VQHandle *movie, bool hide_mouse, ThemeType theme, bool user_break_not_allowed)
{
	if (CurrentVQ == NULL) {
		movie->IsInitialized = true;
		CurrentVQ = movie;
		int start_frame = 0;
		int end_frame = -1;
		int iterations = 0;
		if (Get_Option(OPTION_USE_LOOPS)) {
			start_frame = movie->StartFrame;
			if (movie->EndFrame >= 0) {
				end_frame = movie->EndFrame;
			}
			iterations = movie->Iterations;
			if (movie->PlayMode == 1) {
				if (movie->LoopMode == 0) {
					movie->VQA->Set_Loop(movie->LoopID, movie->Iterations);
				} else {
					movie->VQA->Set_Loop(start_frame, end_frame, movie->Iterations);
				}
			}
		}

		if (hide_mouse) {
			Hide_Mouse();
		}

		if (theme != THEME_NONE) {
			Theme.Queue_Song(theme);
		}

		while (true) {
			if (movie->PlayMode == 2 || start_frame != 0) {
				movie->VQA->Seek_To_Frame(start_frame);
			}
			int ret = movie->VQA->Play_VQA(end_frame, user_break_not_allowed);
			if (movie->PlayMode == 1) {
				iterations = 0;
			}
			movie->VQA->Reset_VQA();
			if (ret == 1) {
				break;
			}
			if (iterations > 0) {
				iterations--;
			}
			if (iterations == 0) {
				break;
			}
		}

		if (hide_mouse) {
			Show_Mouse();
		}

		CurrentVQ = NULL;
	}
}

bool Movie_Advance_Frame(VQHandle * handle, bool &finished)
{
	if (CurrentVQ != NULL) {
		return(false);
	}
	CurrentVQ = handle;
	bool res = handle->VQA->Advance_Frame(finished);
	CurrentVQ = NULL;
	return(res);
}

bool Movie_Is_Playing(void)
{
	return(CurrentVQ != NULL);
}

void Movie_Pause(VQHandle * handle)
{
	if (handle != NULL) {
		if (handle->VQA != NULL) {
			handle->VQA->Pause_VQA();
		}
	}
}

void Movie_Resume(VQHandle * handle)
{
	if (handle != NULL) {
		if (handle->VQA != NULL) {
			handle->VQA->Resume_VQA();
		}
	}
}

void Movie_Update_Visible_Surface(void)
{
	if (CurrentVQ != NULL) {
		AlternateSurface->Fill(0);
		Update_Visible_Surface(false, AlternateSurface);
		Movie_Blit_To_Screen();
	}
}

VQHandle::VQHandle(void) :
	VQA(NULL),
	field_4(-1),
	DrawSurface(NULL),
	PlayMode(0),
	StartFrame(0),
	EndFrame(0),
	LoopMode(0),
	Iterations(1),
	LoopID(0),
	InitialRect(0,0,0,0),
	StretchRect(0,0,0,0),
	field_44(false),
	IsInitialized(false),
	Subtitles(NULL)
{
}

VQHandle::~VQHandle(void)
{
	if (VQA != NULL) {
		VQA->Close_And_Free_VQA();
		VQAClass * vqa = VQA;
		if (vqa != NULL) {
			delete vqa;
		}
	}
	MovieSubtitles * subs = Subtitles;
	if (subs != NULL) {
		delete subs;
		Subtitles = NULL;
	}
}

VQMovieHandle::VQMovieHandle(VQHandle * player) :
	Finished(false)
{
	Player = player;
	Width = player->VQA->Get_VQA_Width();
	Height = Player->VQA->Get_VQA_Height();
}

void VQMovieHandle::Pause(bool pause)
{
	if (pause) {
		if (Player != NULL && Player->VQA != NULL) {
			Player->VQA->Pause_VQA();
		}
	} else {
		if (Player != NULL && Player->VQA != NULL) {
			Player->VQA->Resume_VQA();
		}
	}
}

void VQMovieHandle::Stop(void)
{
	ActiveMovieHandle = this;
	Movie_Play(Player, true, THEME_NONE, false);
	ActiveMovieHandle = NULL;
}

bool VQMovieHandle::FramesLeft(void)
{
	return(Finished);
}

bool VQMovieHandle::AdvanceFrame(void)
{
	VQHandle * p = Player;
	if (!p->VQA->Is_Paused() && p->IsInitialized && CurrentVQ == NULL) {
		CurrentVQ = p;
		bool res = p->VQA->Advance_Frame(Finished);
		CurrentVQ = NULL;
		return(res);
	}
	return(false);
}

void VQMovieHandle::Waiting(void)
{
}

void VQMovieHandle::SetPosition(int x, int y)
{
	Player->InitialRect.X = x;
	Player->InitialRect.Y = y;
	Player->StretchRect = Player->InitialRect;
}

void VQMovieHandle::InitSubtitles(void (__fastcall * callback)(void *))
{
	Player->Subtitles->SetCallback(callback);
}

void VQMovieHandle::SeekToFrame(int frame)
{
	Player->VQA->Reset_VQA();
	Player->VQA->Seek_To_Frame(frame);
}

int VQMovieHandle::Timing(void)
{
	return(33);
}

void VQMovieHandle::Blit(void)
{
	Movie_Update_Visible_Surface();
}

VQMovieHandle::~VQMovieHandle(void)
{
	Movie_Destroy(Player);
}

BinkMovieHandle::BinkMovieHandle(BinkHandle * player)
{
	Player = (VQHandle *)player;
	Width = player->Bink->Width; // RA2: no null check
	Height = player->Bink->Height;
}

void BinkMovieHandle::Pause(bool pause)
{
	((BinkHandle *)Player)->Pause(pause);
}

void BinkMovieHandle::Stop(void)
{
	ActiveMovieHandle = this;
	((BinkHandle *)Player)->Play();
	ActiveMovieHandle = NULL;
}

bool BinkMovieHandle::FramesLeft(void)
{
	return(((BinkHandle *)Player)->Is_Finished());
}

bool BinkMovieHandle::AdvanceFrame(void)
{
	return(((BinkHandle *)Player)->Render_Frame());
}

void BinkMovieHandle::Waiting(void)
{
	((BinkHandle *)Player)->Check_Frame_Ready();
}

void BinkMovieHandle::SetPosition(int x, int y)
{
	((BinkHandle *)Player)->Set_Position(x, y);
}

void BinkMovieHandle::InitSubtitles(void (__fastcall * callback)(void *))
{
	((BinkHandle *)Player)->Set_Subtitle_Callback(callback);
}

void BinkMovieHandle::SeekToFrame(int frame)
{
	((BinkHandle *)Player)->Goto_Frame(frame);
}

int BinkMovieHandle::Timing(void)
{
	return(((BinkHandle *)Player)->Frame_Time());
}

void BinkMovieHandle::Blit(void)
{
	((BinkHandle *)Player)->Redraw();
}

BinkMovieHandle::~BinkMovieHandle(void)
{
	if (Player != NULL) {
		delete (BinkHandle *)Player;
	}
}

MovieHandle::~MovieHandle(void)
{
}

bool __fastcall Find_Movie_File(char const * name, char * out, bool unused)
{
	char buffer[200];
	strcpy(buffer, name);
	char * ext = buffer;
	while (*ext != '\0' && *ext != '.') {
		ext++;
	}
	*ext = '\0';
	Options.Unlock_Movie(buffer);
	strcpy(ext, ".BIK");
	if (CCFileClass(buffer).Is_Available()) {
		if (out != NULL) {
			strcpy(out, buffer);
		}
		return(true);
	}
	strcpy(ext, ".VQA");
	if (CCFileClass(buffer).Is_Available()) {
		if (out != NULL) {
			strcpy(out, buffer);
		}
		return(true);
	}
	return(false);
}


MovieHandle * __fastcall OwnerDraw_InitMovieHandle(char const * name, Surface * surface, Rect * bounds)
{
	char buffer[200];
	if (surface == NULL || !Find_Movie_File(name, buffer, true)) {
		return(NULL);
	}
	Rect r;
	if (bounds != NULL) {
		r = *bounds;
	} else {
		r = surface->Get_Rect();
	}
	char * ext = strrchr(buffer, '.');
	if (ext != NULL && strcmpi(ext, ".bik") == 0) {
		DebugString("Play_Movie() as Bink!\n");
		BinkHandle * bink = new BinkHandle(buffer, surface);
		return(new BinkMovieHandle(bink));
	}
	VQHandle * handle = Movie_Create(buffer, surface, Rect(0, 0, 0, 0), r, 255, false);
	if (handle == NULL) {
		return(NULL);
	}
	VQMovieHandle * movie = new VQMovieHandle(handle);
	return(movie);
}


bool __fastcall Game::IsMoviePlaying(void)
{
	return(CurrentVQ != NULL || ActiveMovieHandle != NULL);
}

void __fastcall Game::BlitMovie(void)
{
	if (ActiveMovieHandle != NULL) {
		ActiveMovieHandle->Blit();
	}
}


template class DynamicVectorClass<MovieHandle *>;

template class VectorClass<MovieHandle *>;

template class VectorClass<char const *>;

template class DynamicVectorClass<char const *>;

