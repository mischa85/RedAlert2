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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Library/DDRAW.CPP                            $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 10/15/98 11:05a                                             $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                *
 *   Process_DD_Result -- Does a message box based on the result of a DD command               *
 *   Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                      *
 *   Get_Free_Video_Memory -- returns amount of free video memory                              *
 *   Get_Video_Hardware_Caps -- returns bitmask of direct draw video hardware support          *
 *   Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                 *
 *   Set_Palette -- set a direct draw palette                                                  *
 *   Check_Overlapped_Blit_Capability -- See if video driver supports blitting overlapped regions*
 *   Wait_Blit -- waits for the DirectDraw blitter to become idle                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "_surface.h"
#include "_timer.h"
#include "assert.h"
#include "dbgprint.h"
#include "dsurface.h"
#include "misc.h"
#include "stimer.h"
#include "timer.h"
#include "vector.h"

#include <cstdio>


LPDIRECTDRAW DirectDrawObject = NULL;       // Pointer to the direct draw object
LPDIRECTDRAW2 DirectDraw2Interface = NULL;  // Pointer to direct draw 2 interface

static PALETTEENTRY PaletteEntries[256];    // 256 windows palette entries
static LPDIRECTDRAWPALETTE	PalettePtr;     // Pointer to direct draw palette object
static bool FirstPaletteSet = false;        // Is this the first time 'Set_Palette' has been called?
LPDIRECTDRAWSURFACE	PaletteSurface = NULL;
bool SurfacesRestored = false;
static bool CanVblankSync = true;
bool SystemToVideoBlits = false;
bool VideoToSystemBlits = false;
bool SystemToSystemBlits = false;
//bool OverlappedVideoBlits = TRUE;

int VideoModeWidth = 0;
int VideoModeHeight = 0;
int VideoModeBitsPerPixel = 0;

unsigned char CurrentPalette[768];
bool WindowedMode;

int (*DirectDrawErrorHandler)(HRESULT error) = NULL;

/***********************************************************************************************
 * PaletteClass::Set -- Fade the display palette to this palette.                              *
 *                                                                                             *
 *    This routine will fade the display palette to match this palette over the time period    *
 *    specified. For smooth palette transitions, this is the routine to call.                  *
 *                                                                                             *
 * INPUT:   time  -- The time period (in system tick increments) to fade the display palette   *
 *                   to match this palette.                                                    *
 *                                                                                             *
 *          callback -- Optional pointer to callback function that, if non-null, will be       *
 *                      called as often as possible during the fading process.                 *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   This routine will not return until the palette is completely faded to the       *
 *             destination palette.                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/02/1995 JLB : Created.                                                                 *
 *   02/05/1996 JLB : Uses new timer system.                                                   *
 *=============================================================================================*/
void Set_Palette(PaletteClass const & pal, int time, void (* callback)(void))
{
	if (VisibleSurface->Bytes_Per_Pixel() != 1) return;

	CDTimerClass<SystemTimerClass> timer = time;
	PaletteClass original;
	memcpy(&original, CurrentPalette, sizeof(CurrentPalette));
	PaletteClass newpal = pal;

	while (timer) {

		/*
		**	Build an intermediate palette that is as close to the destination palette
		**	as the current time is proportional to the ending time.
		*/
		PaletteClass palette = original;
		int adjust = ((time - timer) * 256) / time;
		adjust = MIN(adjust, 255);
		palette.Adjust(adjust, newpal);

		/*
		**	Remember the current time so that multiple palette sets within the same game
		**	time tick won't occur. This is probably unnecessary since the palette setting
		**	code, at the time of this writing, delays at least one game tick in the process
		**	of setting the palette.
		*/
		int holdtime = timer;

		/*
		**	Set the palette to this intermediate palette and then loop back
		**	to calculate and set a new intermediate palette.
		*/
		Set_Palette((void*)&palette[0]);

		/*
		**	If the callback routine was specified, then call it once per palette
		**	setting loop.
		*/
		if (callback) {
			callback();
		}

		/*
		**	This loop ensures that the palette won't be set more than once per game tick. Setting
		**	the palette more than once per game tick will have no effect since the calculation will
		**	result in the same intermediate palette that was previously calculated.
		*/
		while (timer == holdtime && holdtime != 0) {
			if (callback) callback();
		}
	}

	/*
	**	Ensure that the final palette exactly matches the requested
	**	palette before exiting the fading routine.
	*/
	Set_Palette((void*)&newpal[0]);
}


/***********************************************************************************************
 * Process_DD_Result -- Does a message box based on the result of a DD command                 *
 *                                                                                             *
 * INPUT:      HRESULT result            - the result returned from the direct draw command    *
 *             int     display_ok_msg   - should a message be displayed if command ok          *
 *                                                                                             *
 * OUTPUT:      none                                                                           *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/27/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Process_DD_Result(HRESULT result, int display_ok_msg)
{
#ifdef _DEBUG
	static struct {
		HRESULT Error;
		char const * Message;
	} _errors[] = {
		{DDERR_ALREADYINITIALIZED, "This object is already initialized"},
		{DDERR_BLTFASTCANTCLIP, "Return if a clipper object is attached to the source surface passed into a BltFast call."},
		{DDERR_CANNOTATTACHSURFACE, "This surface can not be attached to the requested surface."},
		{DDERR_CANNOTDETACHSURFACE, "This surface can not be detached from the requested surface."},
		{DDERR_CANTCREATEDC, "Windows can not create any more DCs"},
		{DDERR_CANTDUPLICATE, "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created."},
		{DDERR_CANTLOCKSURFACE, "Unable to lock surface because no driver exists which can supply a pointer to the surface."},
		{DDERR_CLIPPERISUSINGHWND, "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd."},
		{DDERR_COLORKEYNOTSET, "No src color key specified for this operation."},
		{DDERR_CURRENTLYNOTAVAIL, "Support is currently not available."},
		{DDERR_DIRECTDRAWALREADYCREATED, "A DirectDraw object representing this driver has already been created for this process."},
		{DDERR_EXCEPTION, "An exception was encountered while performing the requested operation."},
		{DDERR_EXCLUSIVEMODEALREADYSET, "An attempt was made to set the cooperative level when it was already set to exclusive."},
		{DDERR_GENERIC, "Generic failure."},
		{DDERR_HEIGHTALIGN, "Height of rectangle provided is not a multiple of reqd alignment."},
		{DDERR_HWNDALREADYSET, "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created."},
		{DDERR_HWNDSUBCLASSED, "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state."},
		{DDERR_IMPLICITLYCREATED, "This surface can not be restored because it is an implicitly created surface."},
		{DDERR_INCOMPATIBLEPRIMARY, "Unable to match primary surface creation request with existing primary surface."},
		{DDERR_INVALIDCAPS, "One or more of the caps bits passed to the callback are incorrect."},
		{DDERR_INVALIDCLIPLIST, "DirectDraw does not support the provided cliplist."},
		{DDERR_INVALIDDIRECTDRAWGUID, "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier."},
		{DDERR_INVALIDMODE, "DirectDraw does not support the requested mode."},
		{DDERR_INVALIDOBJECT, "DirectDraw received a pointer that was an invalid DIRECTDRAW object."},
		{DDERR_INVALIDPARAMS, "One or more of the parameters passed to the function are incorrect."},
		{DDERR_INVALIDPIXELFORMAT, "The pixel format was invalid as specified."},
		{DDERR_INVALIDPOSITION, "Returned when the position of the overlay on the destination is no longer legal for that destination."},
		{DDERR_INVALIDRECT, "Rectangle provided was invalid."},
		{DDERR_INVALIDSURFACETYPE, "The requested action could not be performed because the surface was of the wrong type."},
		{DDERR_LOCKEDSURFACES, "Operation could not be carried out because one or more surfaces are locked."},
		{DDERR_NO3D, "There is no 3D present."},
		{DDERR_NOALPHAHW, "Operation could not be carried out because there is no alpha accleration hardware present or available."},
//		{DDERR_NOANTITEARHW, "Operation could not be carried out because there is no hardware support for synchronizing blts to avoid tearing.	"},
		{DDERR_NOBLTHW, "No blter hardware present."},
//		{DDERR_NOBLTQUEUEHW, "Operation could not be carried out because there is no hardware support for asynchronous blting."},
		{DDERR_NOCLIPLIST, "No cliplist available."},
		{DDERR_NOCLIPPERATTACHED, "No clipper object attached to surface object."},
		{DDERR_NOCOLORCONVHW, "Operation could not be carried out because there is no color conversion hardware present or available."},
		{DDERR_NOCOLORKEY, "Surface doesn't currently have a color key"},
		{DDERR_NOCOLORKEYHW, "Operation could not be carried out because there is no hardware support of the destination color key."},
		{DDERR_NOCOOPERATIVELEVELSET, "Create function called without DirectDraw object method SetCooperativeLevel being called."},
		{DDERR_NODC, "No DC was ever created for this surface."},
		{DDERR_NODDROPSHW, "No DirectDraw ROP hardware."},
		{DDERR_NODIRECTDRAWHW, "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware."},
		{DDERR_NODIRECTDRAWSUPPORT, "No DirectDraw support possible with current display driver."},
		{DDERR_NOEMULATION, "Software emulation not available."},
		{DDERR_NOEXCLUSIVEMODE, "Operation requires the application to have exclusive mode but the application does not have exclusive mode."},
		{DDERR_NOFLIPHW, "Flipping visible surfaces is not supported."},
		{DDERR_NOGDI, "There is no GDI present."},
		{DDERR_NOHWND, "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND."},
		{DDERR_NOMIRRORHW, "Operation could not be carried out because there is no hardware present or available."},
		{DDERR_NOOVERLAYDEST, "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination."},
		{DDERR_NOOVERLAYHW, "Operation could not be carried out because there is no overlay hardware present or available."},
		{DDERR_NOPALETTEATTACHED, "No palette object attached to this surface.	"},
		{DDERR_NOPALETTEHW, "No hardware support for 16 or 256 color palettes."},
		{DDERR_NORASTEROPHW, "Operation could not be carried out because there is no appropriate raster op hardware present or available."},
		{DDERR_NOROTATIONHW, "Operation could not be carried out because there is no rotation hardware present or available."},
		{DDERR_NOSTRETCHHW, "Operation could not be carried out because there is no hardware support for stretching."},
		{DDERR_NOT4BITCOLOR, "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette."},
		{DDERR_NOT4BITCOLORINDEX, "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette."},
		{DDERR_NOT8BITCOLOR, "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color."},
		{DDERR_NOTAOVERLAYSURFACE, "Returned when an overlay member is called for a non-overlay surface."},
		{DDERR_NOTEXTUREHW, "Operation could not be carried out because there is no texture mapping hardware present or available."},
		{DDERR_NOTFLIPPABLE, "An attempt has been made to flip a surface that is not flippable."},
		{DDERR_NOTFOUND, "Requested item was not found."},
		{DDERR_NOTLOCKED, "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted."},
		{DDERR_NOTPALETTIZED, "The surface being used is not a palette-based surface."},
		{DDERR_NOVSYNCHW, "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations."},
		{DDERR_NOZBUFFERHW, "Operation could not be carried out because there is no hardware support for zbuffer blting."},
		{DDERR_NOZOVERLAYHW, "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays."},
		{DDERR_OUTOFCAPS, "The hardware needed for the requested operation has already been allocated."},
		{DDERR_OUTOFMEMORY, "DirectDraw does not have enough memory to perform the operation."},
		{DDERR_OUTOFVIDEOMEMORY, "DirectDraw does not have enough memory to perform the operation."},
		{DDERR_OVERLAYCANTCLIP, "The hardware does not support clipped overlays."},
		{DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "Can only have ony color key active at one time for overlays."},
		{DDERR_OVERLAYNOTVISIBLE, "Returned when GetOverlayPosition is called on a hidden overlay."},
		{DDERR_PALETTEBUSY, "Access to this palette is being refused because the palette is already locked by another thread."},
		{DDERR_PRIMARYSURFACEALREADYEXISTS, "This process already has created a primary surface."},
		{DDERR_REGIONTOOSMALL, "Region passed to Clipper::GetClipList is too small."},
		{DDERR_SURFACEALREADYATTACHED, "This surface is already attached to the surface it is being attached to."},
		{DDERR_SURFACEALREADYDEPENDENT, "This surface is already a dependency of the surface it is being made a dependency of."},
		{DDERR_SURFACEBUSY, "Access to this surface is being refused because the surface is already locked by another thread."},
		{DDERR_SURFACEISOBSCURED, "Access to surface refused because the surface is obscured."},
		{DDERR_SURFACELOST, "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it."},
		{DDERR_SURFACENOTATTACHED, "The requested surface is not attached."},
		{DDERR_TOOBIGHEIGHT, "Height requested by DirectDraw is too large."},
		{DDERR_TOOBIGSIZE, "Size requested by DirectDraw is too large --	the individual height and width are OK."},
		{DDERR_TOOBIGWIDTH, "Width requested by DirectDraw is too large."},
		{DDERR_UNSUPPORTED, "Action not supported."},
		{DDERR_UNSUPPORTEDFORMAT, "FOURCC format requested is unsupported by DirectDraw."},
		{DDERR_UNSUPPORTEDMASK, "Bitmask in the pixel format requested is unsupported by DirectDraw."},
		{DDERR_VERTICALBLANKINPROGRESS, "Vertical blank is in progress."},
		{DDERR_WASSTILLDRAWING, "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete."},
		{DDERR_WRONGMODE, "This surface can not be restored because it was created in a different mode."},
		{DDERR_XALIGN, "Rectangle provided was not horizontally aligned on required boundary."}
	};
#endif
	/*
	**	If there iwas no error detected, then either bail out or display a message to
	**	this effect as indicated by the "display_ok_msg" parameter.
	*/
	if (result == DD_OK) {
		if (display_ok_msg) {
			MessageBox(MainWindow, "Direct Draw operation processed without error", "Note", MB_OK);
		}
		return;
	}

	if (DirectDrawErrorHandler) {
		DirectDrawErrorHandler(result);
		return;
	}
#ifdef _DEBUG
	/*
	**	Scan for a matching error code and display the appropriate message.
	*/
	for (int index = 0; index < ARRAY_SIZE(_errors); index++) {
		if (_errors[index].Error == result) {
			MessageBox(MainWindow, _errors[index].Message, "Westwood Library Direct Draw Error", MB_ICONEXCLAMATION|MB_OK);
			return;
		}
	}
#endif

	/*
	**	Since it fell out of the above loop, this must be an unrecognized error code.
	*/
	char str[80];
	sprintf(str, "DDRAW.DLL Error code = %08X", result);
	MessageBox(MainWindow, str, "Direct X", MB_ICONEXCLAMATION|MB_OK);
}


/***********************************************************************************************
 * Check_Overlapped_Blit_Capability -- See if video driver supports blitting overlapped regions*
 *                                                                                             *
 *  We will check for this by drawing something to a video page and blitting it over itself.   *
 * If we end up with the top line repeating then overlapped region blits dont work.            *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/7/96 5:06PM ST : Created                                                               *
 *=============================================================================================*/
void Check_Overlapped_Blit_Capability(void)
{
	DSurface test_buffer(64, 64, false);

	DebugString("Checking hardware region fill capability...");

	/*
	**	Assume we can until we find out otherwise
	*/
	DSurface::AllowHWFill = true;

	test_buffer.Fill(0);
	if (test_buffer.Get_Pixel(Point2D(5,5)) != 0) {
		DSurface::AllowHWFill = false;
	} else {
		test_buffer.Fill(1);
		if (test_buffer.Get_Pixel(Point2D(5,5)) != 1) {
			DSurface::AllowHWFill = false;
		}
	}
	DebugStringNoPrefix("%s\n", DSurface::AllowHWFill ? "OK" : "Failed!");

	DebugString("Checking overlapped blit capability...");

	/*
	**	Assume we can until we find out otherwise
	*/
	DSurface::OverlappedVideoBlits = true;

	test_buffer.Fill(0);

	/*
	**	Plot a pixel in the top left corner of the buffer.
	*/
	test_buffer.Put_Pixel(Point2D(0, 0), 255);

	/*
	 * Blit the buffer down by one line. If we end up with a vertical strip of pixel that's not 0's then
	 * overlapped blits dont work
	 */
	test_buffer.Blit_From(Rect(0, 1, 64, 64-1), test_buffer, Rect(0, 0, 64, 64-1), false, false);

	if (test_buffer.Get_Pixel(Point2D(0 ,5)) != 0) {
		DebugStringNoPrefix("Failed!\n");
		DSurface::OverlappedVideoBlits = false;
	} else {
		DebugStringNoPrefix("OK\n");
	}
}


/// <summary>
/// Creates the DirectDraw object if there is not one already.
/// This routine also declares how the game intends to share the display, asking for
/// exclusive full screen use unless windowed mode is in force. Use this routine before
/// anything that expects the DirectDraw object to exist.
/// </summary>
void Prep_Direct_Draw(void)
{
	DebugString("Prep direct draw.\n");

	//
	// If there is not currently a direct draw object then we need to define one.
	//
	if ( DirectDrawObject == NULL ) {
		//MessageBox(MainWindow, "In Set_Video_Mode. About to call DirectDrawCreate.","Note", MB_ICONEXCLAMATION|MB_OK);
		HRESULT result = DirectDrawCreate(NULL, &DirectDrawObject, NULL);
		Process_DD_Result(result, false);
		if (result == DD_OK) {
			if (WindowedMode) {
				//MessageBox(MainWindow, "In Set_Video_Mode. About to call SetCooperativeLevel.","Note", MB_ICONEXCLAMATION|MB_OK);
				result = DirectDrawObject->SetCooperativeLevel(MainWindow, DDSCL_NORMAL);
			} else {
				//MessageBox(MainWindow, "In Set_Video_Mode. About to call SetCooperativeLevel.","Note", MB_ICONEXCLAMATION|MB_OK);
				result = DirectDrawObject->SetCooperativeLevel(MainWindow, DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
			}
			Process_DD_Result(result, false);
		}else{
			DebugString("DirectDrawCreate failed!\n");
			//return (FALSE);
		}
	}
}


/// <summary>
/// Releases the DirectDraw object.
/// This routine is the counterpart to Prep_Direct_Draw. It is harmless to call when there
/// is no DirectDraw object about, so shutdown code need not check first.
/// </summary>
void Destroy_Direct_Draw(void)
{
	if (DirectDrawObject != NULL) {
		HRESULT result = DirectDrawObject->Release();
		Process_DD_Result(result, FALSE);
		DirectDrawObject = NULL;
	}
}


/// <summary>
/// Determines how much video memory the display card has.
/// This routine may be called before the video mode has been set. If there is no
/// DirectDraw object at the time, a temporary one is created and released again. The
/// memory that the current display mode already occupies is added back in, since that
/// memory becomes available once the game takes over the display.
/// </summary>
/// <returns>Returns with the size of video memory in bytes, rounded down to a whole number
/// of megabytes. Zero is returned if the card could not be questioned.</returns>
DWORD GetVideoMemory(void)
{
	HRESULT result;
	bool created = false;

	if (DirectDrawObject == NULL) {
		result = DirectDrawCreate(NULL, &DirectDrawObject, NULL);
		if (result != DD_OK) {
			Process_DD_Result(result, FALSE);
			DirectDrawObject = NULL;
		} else {
			created = true;
		}
	}

	DWORD video_memory = 0;
	if (DirectDrawObject) {
		DDCAPS video_capabilities;
		video_capabilities.dwSize = sizeof (video_capabilities);
		if (DD_OK == DirectDrawObject->GetCaps (&video_capabilities , NULL)){
			video_memory = video_capabilities.dwVidMemTotal;
			DebugString("DDraw returnded actual VRAM of %d bytes (%dMb)\n", video_memory, video_memory >> 20);

			DDSURFACEDESC desc;
			memset(&desc, 0, sizeof(desc));
			desc.dwSize = sizeof(desc);
			if (DirectDrawObject->GetDisplayMode(&desc) == 0) {
				DebugString("Current display size is %dx%dx%d. Pitch is %d\n", desc.dwWidth, desc.dwHeight, desc.ddpfPixelFormat.dwRGBBitCount, desc.lPitch);
				video_memory += desc.dwWidth * desc.dwHeight * (desc.ddpfPixelFormat.dwRGBBitCount >> 3);
			}

			video_memory += 512 * 256;
			video_memory &= 0xfff00000;
			DebugString("Adjusted VRAM size is %d bytes (%dMb)\n", video_memory, video_memory >> 20);
		}
	}

	if (created) {
		//Destroy_Direct_Draw();
		if (DirectDrawObject != NULL) {
			result = DirectDrawObject->Release();
			Process_DD_Result(result, FALSE);
			DirectDrawObject = NULL;
		}
	}

	return(video_memory);
}


/***********************************************************************************************
 * Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                  *
 *                                                                                             *
 * INPUT:        int width            - the width of the video mode in pixels                  *
 *               int height           - the height of the video mode in pixels                 *
 *               int bits_per_pixel   - the number of bits per pixel the video mode supports   *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
bool Set_Video_Mode(HWND , int w, int h, int bits_per_pixel)
{
	HRESULT result;

	Prep_Direct_Draw();

	//
	// Set the required display mode with 8 bits per pixel
	//
	DebugString("SetDisplayMode: %dx%dx%d\n", w, h, bits_per_pixel);
	//MessageBox(MainWindow, "In Set_Video_Mode. About to call call SetDisplayMode.","Note", MB_ICONEXCLAMATION|MB_OK);
	result = DirectDrawObject->SetDisplayMode (w, h, bits_per_pixel);
	if (result != DD_OK) {
//		Process_DD_Result(result, false);
//		DirectDrawObject->Release();
//		DirectDrawObject = NULL;
		DebugString("SetDisplayMode failed\n");
		return(false);
	}

	VideoModeWidth = w;
	VideoModeHeight = h;
	VideoModeBitsPerPixel = bits_per_pixel;
	if (bits_per_pixel == 8) {
		DebugString("CreatePalette\n");
		//
		// Create a direct draw palette object
		//
		//MessageBox(MainWindow, "In Set_Video_Mode. About to call CreatePalette.","Note", MB_ICONEXCLAMATION|MB_OK);
		result = DirectDrawObject->CreatePalette( DDPCAPS_8BIT | DDPCAPS_ALLOW256, &PaletteEntries[0] , &PalettePtr ,NULL);
		Process_DD_Result(result, false);
		if (result != DD_OK) {
			DebugString("CreatePalette failed\n");
			return(false);
		}
	}

	Check_Overlapped_Blit_Capability();
	DebugString("Display mode set\n");

	//MessageBox(MainWindow, "In Set_Video_Mode. About to return success.","Note", MB_ICONEXCLAMATION|MB_OK);
#if (0)
	/*
	**	Find out if DirectX 2 extensions are available
	*/
	result = DirectDrawObject->QueryInterface (IID_IDirectDraw2, (LPVOID*)&DirectDraw2Interface);
	SystemToVideoBlits = false;
	VideoToSystemBlits = false;
	SystemToSystemBlits= false;
	if (result != DD_OK) {
		DirectDraw2Interface = NULL;
	} else {
		DDCAPS capabilities;
		DDCAPS emulated_capabilities;

		memset ((char*)&capabilities, 0, sizeof(capabilities));
		memset ((char*)&emulated_capabilities, 0, sizeof(emulated_capabilities));
		capabilities.dwSize = sizeof (capabilities);
		emulated_capabilities.dwSize = sizeof (emulated_capabilities);

		DirectDrawObject->GetCaps (&capabilities, &emulated_capabilities);

		if (capabilities.dwCaps & DDCAPS_CANBLTSYSMEM) {
			SystemToVideoBlits = (capabilities.dwSVBCaps & DDCAPS_BLT) ? true : false;
			VideoToSystemBlits = (capabilities.dwVSBCaps & DDCAPS_BLT) ? true : false;
			SystemToSystemBlits = (capabilities.dwSSBCaps & DDCAPS_BLT) ? true : false;
		}
	}
#endif	//(0)

	//MessageBox(MainWindow, "In Set_Video_Mode. About to return success.","Note", MB_ICONEXCLAMATION|MB_OK);

	return(true);

}

/***********************************************************************************************
 * Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                        *
 *                                                                                             *
 * INPUT:      none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
	HRESULT result;

	//
	// If a direct draw object has been declared and a video mode has been set
	// then reset the video mode and release the direct draw object.
	//
	if ( DirectDrawObject ) {
		result = DirectDrawObject->RestoreDisplayMode();
		Process_DD_Result(result, false);
		VideoModeWidth = 0;
		VideoModeHeight = 0;
		VideoModeBitsPerPixel = 0;
		result = DirectDrawObject->Release();
		Process_DD_Result(result, false);

		DirectDrawObject = NULL;
	}
}


/***********************************************************************************************
 * Get_Free_Video_Memory -- returns amount of free video memory                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   bytes of available video RAM                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 12:52PM ST : Created                                                            *
 *=============================================================================================*/
unsigned int Get_Free_Video_Memory(void)
{
	DDCAPS	video_capabilities;

	if (DirectDrawObject) {

		video_capabilities.dwSize = sizeof (video_capabilities);

		//MessageBox(MainWindow, "In Get_Free_Video_Memory. About to call GetCaps","Note", MB_ICONEXCLAMATION|MB_OK);
		if (DD_OK == DirectDrawObject->GetCaps (&video_capabilities, NULL)) {
			char string [256];
			sprintf (string, "In Get_Free_Video_Memory. About to return %d bytes",video_capabilities.dwVidMemFree);
			//MessageBox(MainWindow, string,"Note", MB_ICONEXCLAMATION|MB_OK);
			return(video_capabilities.dwVidMemFree);
		}
	}

	//MessageBox(MainWindow, "In Get_Free_Video_Memory. About to return failure","Note", MB_ICONEXCLAMATION|MB_OK);
	return(0);
}


/***********************************************************************************************
 * Get_Video_Hardware_Caps -- returns bitmask of direct draw video hardware support            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   hardware flags                                                                    *
 *                                                                                             *
 * WARNINGS: Must call Set_Video_Mode 1st to create the direct draw object                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    1/12/96 9:14AM ST : Created                                                              *
 *=============================================================================================*/
unsigned Get_Video_Hardware_Capabilities(void)
{
	DDCAPS	video_capabilities;
	unsigned	video;

	/*
	**	Fail if the direct draw object has not been initialised
	*/
	if (!DirectDrawObject) return(0);

	/*
	**	Get the capabilities of the direct draw object
	*/
	memset(&video_capabilities, 0, sizeof(video_capabilities));
	video_capabilities.dwSize = sizeof(video_capabilities);
	//MessageBox(MainWindow, "In Get_Video_Hardware_Capabilities. About to call GetCaps","Note", MB_ICONEXCLAMATION|MB_OK);
	HRESULT result = DirectDrawObject->GetCaps (&video_capabilities, NULL);
	if (result != DD_OK) {
		Process_DD_Result(result, false);
		return(0);
	}

	/*
	**	Set flags to indicate the presence of the features we are interested in
	*/
	video = 0;

	/* Hardware blits supported? */
	if (video_capabilities.dwCaps & DDCAPS_BLT) 				video |= VIDEO_BLITTER;

	/* Hardware blits asyncronous? */
	if (video_capabilities.dwCaps & DDCAPS_BLTQUEUE) 		video |= VIDEO_BLITTER_ASYNC;

	/* Can palette changes be synced to vertical refresh? */
	if (video_capabilities.dwCaps & DDCAPS_PALETTEVSYNC) 	video |= VIDEO_SYNC_PALETTE;

	/* Is the video cards memory bank switched? */
	if (video_capabilities.dwCaps & DDCAPS_BANKSWITCHED) 	video |= VIDEO_BANK_SWITCHED;

	/* Can the blitter do filled rectangles? */
	if (video_capabilities.dwCaps & DDCAPS_BLTCOLORFILL)	video |= VIDEO_COLOR_FILL;

	/* Is there no hardware assistance avaailable at all? */
	if (video_capabilities.dwCaps & DDCAPS_NOHARDWARE) 	video |= VIDEO_NO_HARDWARE_ASSIST;

	/**/
	if (video_capabilities.dwCaps & DDCAPS_BLTSTRETCH) 	video |= VIDEO_BLIT_STRETCH;

	//MessageBox(MainWindow, "In Get_Video_Hardware_Capabilities. About to return success.","Note", MB_ICONEXCLAMATION|MB_OK);
	return(video);
}


/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
void Wait_Vert_Blank(void)
{
	if (CanVblankSync) {
		HRESULT result = DirectDrawObject->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
		if (result == E_NOTIMPL) {
			CanVblankSync = false;
			return;
		}
		Process_DD_Result(result, false);
	}
}


/***********************************************************************************************
 * Set_Palette -- set a direct draw palette                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to 768 rgb palette bytes                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/11/95 3:33PM ST : Created                                                             *
 *=============================================================================================*/
void Set_Palette(void const * palette)
{
	assert(palette != NULL);

	if (&CurrentPalette[0] != palette) {
		memmove(CurrentPalette, palette, sizeof(CurrentPalette));
	}

	if (DirectDrawObject != NULL && PaletteSurface != NULL) {
		unsigned char * palette_get = (unsigned char*)palette;
		for (int index = 0; index < 256; index++) {

			int red = *palette_get++;
			int green = *palette_get++;
			int blue = *palette_get++;

			PaletteEntries[index].peRed = (unsigned char)red;
			PaletteEntries[index].peGreen = (unsigned char)green;
			PaletteEntries[index].peBlue = (unsigned char)blue;
		}

		if (PalettePtr != NULL) {
			if (!FirstPaletteSet) {
				//MessageBox(MainWindow, "In Set_DD_Palette. About to call SetPalette","Note", MB_ICONEXCLAMATION|MB_OK);
				PaletteSurface->SetPalette(PalettePtr);
				FirstPaletteSet = true;
			}
			//MessageBox(MainWindow, "In Set_DD_Palette. About to call SetEntries","Note", MB_ICONEXCLAMATION|MB_OK);
			PalettePtr->SetEntries(0, 0, 256, &PaletteEntries[0]);
		}
	}
	//MessageBox(MainWindow, "Leaving Set_DD_Palette","Note", MB_ICONEXCLAMATION|MB_OK);
}


/***********************************************************************************************
 * Wait_Blit -- waits for the DirectDraw blitter to become idle                                *
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
 *   07-25-95 03:53pm ST : Created                                                             *
 *=============================================================================================*/
void Wait_Blit (void)
{
	HRESULT	return_code;

	do {
		return_code=PaletteSurface->GetBltStatus (DDGBS_ISBLTDONE);
	} while (return_code != DD_OK && return_code != DDERR_SURFACELOST);

}


/// <summary>
/// Handles one display mode reported by DirectDraw.
/// This routine is the callback that DirectDraw calls once per available mode. It keeps a
/// copy of the description so that the mode survives after the enumeration has finished.
/// </summary>
/// <param name="pddsd">Pointer to the surface description of the mode being reported.</param>
/// <param name="lpContext">Pointer to the vector that is collecting the mode descriptions.</param>
/// <returns>Returns with DDENUMRET_OK so that the enumeration carries on.</returns>
/// <remarks>Each description is copied onto the heap, so whoever started the enumeration
/// must delete the collected entries.</remarks>
HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd, LPVOID lpContext)
{
	DynamicVectorClass<DDSURFACEDESC *>* descs = (DynamicVectorClass<DDSURFACEDESC *>*)lpContext;
	DDSURFACEDESC *desc = new DDSURFACEDESC;
	*desc = *pddsd;

	descs->Add(desc);


	return(DDENUMRET_OK);
}


/// <summary>
/// Compares two display mode entries so they can be sorted.
/// This routine is the comparison callback handed to qsort when the display mode list is
/// put in order. Modes are ranked by width first and by height second.
/// </summary>
/// <param name="p1">Pointer to the first mode entry -- a width followed by a height.</param>
/// <param name="p2">Pointer to the second mode entry -- a width followed by a height.</param>
/// <returns>Returns with a negative value, zero or a positive value, qsort fashion.</returns>
int __cdecl CompareModes(const void *p1, const void *p2)
{
	int w1 = *((int *)p1);
	int h1 = *((int *)p1 + 1);
	int w2 = *((int *)p2);
	int h2 = *((int *)p2 + 1);

	if (w1 < w2) return(-1);
	if (w2 < w1) return(1);

	if (h1 < h2) return(-1);
	if (h2 < h1) return(1);

	return(0);
}


/// <summary>
/// Fetches the display modes that fall within the limits specified.
/// This routine is used to build the list of resolutions that the game is willing to
/// offer. Only modes of the requested color depth are considered, and the survivors
/// come back sorted by width and then by height.
/// </summary>
/// <param name="bitdepth">The color depth, in bits per pixel, that a mode must have.</param>
/// <returns>Returns with a pointer to a list of width and height pairs, terminated by a zero
/// entry. Otherwise, NULL is returned.</returns>
/// <remarks>The caller takes ownership of the list returned and must delete it.</remarks>
int * EnumDisplayModes(DWORD minw, DWORD minh, DWORD maxw, DWORD maxh, DWORD bitdepth)
{
	if (DirectDrawObject != NULL) {
		DynamicVectorClass<DDSURFACEDESC *> descs;
		HRESULT result = DirectDrawObject->EnumDisplayModes(0, NULL, &descs, EnumDisplayModesCallback);

		if (result == S_OK && descs.Count() != 0) {

			char *list = new char[2 * sizeof(int) * descs.Count() + sizeof(int)];
			int *l = (int *)list;
			memset(l, 0, 2 * sizeof(int) * descs.Count() + sizeof(int));
			int count = 0;

			for (int i = 0; i < descs.Count(); i++) {
				DDSURFACEDESC *dsc = descs[i];

				DWORD w = dsc->dwWidth;
				if (w <= maxw) {
					DWORD h = dsc->dwHeight;
					if (h <= maxh && w >= minw && h >= minh && dsc->ddpfPixelFormat.dwRGBBitCount == bitdepth) {
						/// the entries are written through an advancing pointer
						*l++ = dsc->dwWidth;
						*l++ = dsc->dwHeight;
						count++;
					}
				}
				delete descs[i];
			}

			/// the list is empty if that pointer never advanced past the start
			if ((char *)l == list) {
				delete list;
				return(NULL);
			}
			qsort(list, count, sizeof(int) + sizeof(int), CompareModes);

			return((int *)(list));
		}
	}
	return(NULL);
}
