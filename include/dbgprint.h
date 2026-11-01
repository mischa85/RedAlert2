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

#include "sun.h"

#ifdef _DEBUG
void __cdecl DebugString(const char *string, ...);
void __cdecl DebugStringNoPrefix(const char *string, ...);
#else
inline void __cdecl DebugString(const char *string, ...) {}
inline void __cdecl DebugStringNoPrefix(const char *string, ...) {}
#endif

#ifdef _DEBUG
char const * Last_Error_Text(void);
#else
inline char const * Last_Error_Text(void) { return(0); }
#endif
