/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

class AbstractClass;

/*
**	RA2 keeps the bombs that Crazy Ivan plants in one list.
*/
class BombListClass
{
	public:
		void Detach(AbstractClass * target);
};

extern BombListClass BombList;
