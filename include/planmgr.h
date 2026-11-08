/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 RedAlert2 contributors
 ******************************************************************************/

#pragma once

class Point2D;

/*
**	RA2: planning mode takes the mouse over the tactical view while it is active. Each
**	routine gets the mouse position relative to the tactical view; those that return a
**	value return true when planning mode handled the input.
*/
bool __stdcall Planning_Left_Press(int x, int y);
bool __stdcall Planning_Left_Release(Point2D point);
void __stdcall Planning_Right_Press(int x, int y);
void __stdcall Planning_Right_Release(int x, int y);
bool __stdcall Planning_Mouse_AI(int x, int y);
