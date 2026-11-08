/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

/*
**	windows.h maps LoadString to LoadStringA.
*/
#ifdef LoadString
#undef LoadString
#endif

/*
**	RA2 fetches its text from a string table file by label. Each lookup also passes the
**	path and line of the source that asks for it, which the game reports when a label is
**	missing; the original path of each source file is therefore part of its code.
*/
class StringTable
{
	public:
		static wchar_t const * LoadString(char const * label, char ** extra, char const * file, int line);
};
