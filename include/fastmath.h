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

#include <cmath>

#define M_PI  3.14159265358979323846
#define M_FPI 3.141592654f

/*
**	Macros to convert between degrees and radians
*/
#ifndef RAD_TO_DEG
#define RAD_TO_DEG(x)	(((double)x)*180.0/M_PI)
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD(x)	(((double)x)*M_PI/180.0)
#endif

#ifndef RAD_TO_DEGF
#define RAD_TO_DEGF(x)	(((float)x)*180.0f/M_PI)
#endif

#ifndef DEG_TO_RADF
#define DEG_TO_RADF(x)	(((float)x)*M_PI/180.0f)
#endif

namespace fastmath
{
	double __cdecl sqrt(double x);

	double __cdecl sin(double x);
	double __cdecl cos(double x);

	double __cdecl tan(double x);

	double __cdecl asin(double x);
	double __cdecl acos(double x);

	double __cdecl atan(double x);
	double __cdecl atan2(double y, double x);

	float sqrtf(float x);

	float sinf(float x);
	float cosf(float x);

	float asinf(float x);
	float acosf(float x);

	float tanf(float);

	float atanf(float x);
	float atan2f(float y, float x);
};
