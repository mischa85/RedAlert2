/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 ra2-decomp contributors
 *
 * Reduced to its data members for RA2 1.006 by RedAlert2 contributors, 2026.
 ******************************************************************************/

#pragma once

class AudioEvent;
class VocClass;

/*
**	A handle on a sound that is playing, so that it can be stopped or replaced later.
*/
struct AudioController
{
	AudioEvent * Event;
	unsigned Stamp;
	VocClass * EventType;
	void ** AudioIndex;
	unsigned Unused;

	void Init(void); // RA2: 0x00405BF0
	void Release(void); // RA2: 0x00405C10
	void EndLooping(void); // RA2
	void __fastcall SetEvent(AudioEvent * event, VocClass * type); // RA2
};
