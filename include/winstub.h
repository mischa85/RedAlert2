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

#pragma once

#include "win.h"

class Surface;
class PaletteClass;

void Create_Main_Window ( HINSTANCE instance , int command_show , int width , int height);

void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette);

void Build_Info(void);
void Build_Date_String(char * buffer, int buflen);
char *Build_Number_String(char * buffer, int buflen);
unsigned int Build_Number(void);
char *Build_By_String(char * buffer, int buflen);
char *Build_By_Initials(char * buffer, int buflen);
char *Build_Version_String(char * buffer, int buflen);
