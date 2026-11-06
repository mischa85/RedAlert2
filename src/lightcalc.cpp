/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "lightcalc.h"

#include "globals.h"
#include "goptions.h"
#include "sun.h"


/// <summary>
/// Normalizes a tile lighting tint to its strongest channel.
/// This routine is used by the cell lighting recalculation so that the tint handed to a
/// tile drawer always peaks at full strength, with the brightness taken out of the tint
/// folded into the light level instead. The caller uses the reported intensity to bring
/// any other brightness values it is tracking for the cell into line.
/// </summary>
/// <param name="intensity">Filled in with the scale divided out of the tint, expressed in
/// 16.16 fixed point.</param>
/// <param name="light">The light level for the tile. It is scaled to suit the normalized
/// tint and limited to full brightness.</param>
/// <param name="red">The red tint level, where 1000 is full strength.</param>
/// <param name="green">The green tint level, where 1000 is full strength.</param>
/// <param name="blue">The blue tint level, where 1000 is full strength.</param>
void Adjust_Tile_RGB(int & intensity, int & light, int & red, int & green, int & blue)
{
	double temp;

	red = max(red, 0);
	green = max(green, 0);
	blue = max(blue, 0);

	red = min(red, 2000); // RA2: limited to twice full strength
	green = min(green, 2000); // RA2: limited to twice full strength
	blue = min(blue, 2000); // RA2: limited to twice full strength

	intensity = 65536;

	if (red != NORMAL_LIGHT || green != red || blue != red) {
		if (green <= red && red >= blue) {
			temp = ((double)red * 65536) * 0.001; // RA2: multiplies by a thousandth
			intensity = temp;

			if (intensity < 66) {
				intensity = 65536;
				red = NORMAL_LIGHT;
				green = NORMAL_LIGHT;
				blue = NORMAL_LIGHT;
				light = 0;
			} else {
				red = NORMAL_LIGHT;

				green = (((double)green * 65536) / (double)temp);
				blue = (((double)blue * 65536) / (double)temp);

				light *= intensity;
				light >>= 16;
			}
		} else if (green >= red && green >= blue) {
			temp = ((double)green * 65536) * 0.001; // RA2: multiplies by a thousandth
			intensity = temp;

			if (intensity < 66) {
				intensity = 65536;
				red = NORMAL_LIGHT;
				green = NORMAL_LIGHT;
				blue = NORMAL_LIGHT;
				light = 0;
			} else {
				green = NORMAL_LIGHT;

				red = (((double)red * 65536) / (double)intensity);
				blue = (((double)blue * 65536) / (double)intensity);

				light *= intensity;
				light >>= 16;
			}
		} else {
			temp = ((double)blue * 65536) * 0.001; // RA2: multiplies by a thousandth
			intensity = temp;

			if (intensity < 66) {
				intensity = 65536;
				red = NORMAL_LIGHT;
				green = NORMAL_LIGHT;
				blue = NORMAL_LIGHT;
				light = 0;
			} else {
				blue = NORMAL_LIGHT;

				red = (((double)red * 65536) / (double)intensity);
				green = (((double)green * 65536) / (double)intensity);

				light *= intensity;
				light >>= 16;
			}
		}
	}

	light = light >= 2000 ? 2000 : light;
}


/// <summary>
/// Clamps and quantizes a tile lighting tint.
/// This routine is used before a tint is looked up in a tile's drawer cache. The lower the
/// detail level, the more coarsely the tint is rounded off, so that neighboring shades
/// share one drawer rather than each building a conversion table of its own.
/// </summary>
/// <param name="red">The red tint level, where 1000 is full strength.</param>
/// <param name="green">The green tint level, where 1000 is full strength.</param>
/// <param name="blue">The blue tint level, where 1000 is full strength.</param>
void Clamp_Tile_RGB(int & red, int & green, int & blue)
{
	red = min(red, NORMAL_LIGHT);
	green = min(green, NORMAL_LIGHT);
	blue = min(blue, NORMAL_LIGHT);

	red = max(red, 0);
	green = max(green, 0);
	blue = max(blue, 0);

	/// Apply quantization based on DetailLevel
	switch (Options.DetailLevel) {

		case 0:
			red &= ~((1 << 7) - 1); // RA2: one bit coarser, without the rounding add

			green &= ~((1 << 7) - 1); // RA2: one bit coarser, without the rounding add

			blue &= ~((1 << 7) - 1); // RA2: one bit coarser, without the rounding add
			break;

		case 1:
			red &= ~((1 << 6) - 1); // RA2: one bit coarser, without the rounding add

			green &= ~((1 << 6) - 1); // RA2: one bit coarser, without the rounding add

			blue &= ~((1 << 6) - 1); // RA2: one bit coarser, without the rounding add
			break;

		case 2:
			red &= ~((1 << 5) - 1); // RA2: one bit coarser, without the rounding add

			green &= ~((1 << 5) - 1); // RA2: one bit coarser, without the rounding add

			blue &= ~((1 << 5) - 1); // RA2: one bit coarser, without the rounding add
			break;

		default:
			/*
			 * High detail (3 or more): no quantization applied
			 */
			break;
	}
}
