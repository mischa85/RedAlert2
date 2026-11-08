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

#ifndef VQADEBUG_H
#define VQADEBUG_H

// If this is a debug build, extern to the debug function defined in the project linking to it, otherwise just emit empty inlines.
#ifdef _DEBUG
extern void __cdecl DebugString(const char *string, ...);
extern void __cdecl DebugStringNoPrefix(const char *string, ...);
#else
inline void __cdecl DebugString(const char *string, ...) {}
inline void __cdecl DebugStringNoPrefix(const char *string, ...) {}
#endif

#endif /* VQADEBUG_H */
