/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

class AbstractClass;

/*
**	RA2 tracks the aircraft that fly into their target.
*/
class KamikazeClass
{
	public:
		void Remove(AbstractClass * target);
};

extern KamikazeClass Kamikazes;
