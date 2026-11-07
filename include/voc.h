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

#include "coord.h"

#include "voc.hh"

class CCINIClass;
class VocClass;

void Init_Vocs(CCINIClass const &ini);
void Free_Vocs(void);

int Sound_Effect(VocType voc, float volume=1, int=0);
int Voice_Sound_Effect(VocType voc, float volume=1);
int Sound_Effect(VocType voc, Coord const & coord);
VocClass * VocClass_From_Name(char const * name);
char const * Voc_Name(VocType voc);

/***************************************************************************
**	Controls what special effects may occur on the sound effect.
*/
enum ContextType {
	IN_NOVAR,			// No variation or alterations allowed.
	IN_VAR				// Infantry variance response modification.
};

class VocClass
{
	public:
		VocClass(const char *filename);
		~VocClass(void);

		bool Fill_In(CCINIClass const &ini);

		bool Can_Play(void);
		int Play(float vol, int var);
		int Play(float vol);

		VocType Voc_Type(void);

		static VocType From_Name(char const * name);
		friend VocClass *VocClass_From_Name(char const * name);
		friend char const * Voc_Name(VocType voc);

	private:
		char 				Name[256];			// Digitized voice file name.
		int	 				Priority;			// Playback priority of this sample.
		float 				Volume;
		const void *		FilePtr;
};
