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

/// The support code for this lives in dbgprint.cpp rather than a module of its own.

/*
** For WIN32, replace the assert macro so we get an error on the debugger screen
** where we can see it.
**
*/
#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
#define assert(__ignore) ((void)0)
#else
void Assert_Failure(char * expression, int line, char * file);
#define assert(expr)   ((expr)?(void)0:Assert_Failure(#expr,__LINE__,__FILE__))
#endif
