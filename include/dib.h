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
 *                     $Archive:: /Commando/Code/Library/dib.h                                $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/26/97 11:33a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "bsurface.h"
#include "palette.h"
#include "win.h"


class DIB8Class
{
public:

	DIB8Class(HWND hwnd,int width, int height, PaletteClass & pal);
	~DIB8Class(void);

	HBITMAP		Get_Handle(void)		{ return(Handle); }
	int			Get_Width(void)		{ return(Width); }
	int			Get_Height(void)	{ return(Height); }
	Surface &	Get_Surface(void)	{ return(*Surface); }

	void		Clear(unsigned char color);

private:

	bool				IsZombie;	// object constructor failed, its a living-dead object!
	BITMAPINFO *		Info;		// info used in creating the dib + the palette.
	HBITMAP				Handle;		// handle to the actual dib
	unsigned char *		Pixels;		// address of memory containing the pixel data
	int					Width;		// width of the dib
	int					Height;		// height of the dib
	unsigned char *		PixelBase;	// address of upper left pixel (this and DIBPitch abstract up/down DIBS)
	int					Pitch;		// offset from DIBPixelBase to next row (can be negative for bottom-up DIBS)

	BSurface *			Surface;	// Bsurface wrapped around the pixel buffer.
};
