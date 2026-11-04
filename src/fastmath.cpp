/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "fastmath.h"

#include "dbgprint.h"
#include "win.h"

using namespace fastmath;

/// truncation from 'const double' to 'float'
#pragma warning(disable:4305)

/// Note: we assume 32 bit integers throughout.

/// fastmath square root -- provenance
/// This "fast square root by table lookup" is a port of the gem:
/// "IEEE Fast Square Root" by Steve Hill, Graphics Gems III
/// (Academic Press, 1992), p. 48; code gemsiii/sqrt.c, pp. 446-447.
/// That gem is itself described in its own source header as "adapted from
/// the code of Paul Lalonde and Robert Dawson in Graphics Gems I" -- their
/// "A High Speed, Low Precision Square Root" (Graphics Gems I, 1990). The
/// macro names below (MOST_SIG_OFFSET, SQRT_TAB_SIZE, MANT_SHIFTS, EXP_BIAS,
/// EXP_SHIFTS, EXP_LSB, MANT_MASK), the init_sqrt_tab() two-half build loop,
/// and the sqrtf() comments are taken verbatim from the Gems III source.
/// The Gems III original operates on a double; Westwood retargeted it to a
/// float. That retargeting is the only substantive change and is why the
/// constants below differ from the gem: EXP_BIAS 1023->127, EXP_SHIFTS
/// 20->23, EXP_LSB 0x00100000->0x00800000, MANT_MASK 0x000FFFFF->0x007FFFFF,
/// and MOST_SIG_OFFSET 1->0. A float is a single int, so there is no
/// high/low word to select -- the "double" wording and the SPARC/architecture
/// notes inherited in the comments below do not apply to this float port
/// (MOST_SIG_OFFSET is always 0 here).

/*
 * MOST_SIG_OFFSET gives the (int *) offset from the address of the double
 * to the part of the number containing the sign and exponent.
 * You will need to find the relevant offset for your architecture.
 */
#define MOST_SIG_OFFSET 0

/*
 * Number of bits of mantissa used to index the sqrt table. The SQRT_BITS-driven
 * form here matches the FLOAT variant Jonathan Shekter posted to
 * comp.graphics.algorithms (1995), who used SQRT_BITS 11; this port uses 13
 * (table = 1<<14 = 16384, MANT_SHIFTS = 23-13 = 10) for more precision. The 23 is
 * the float mantissa width; the Gems III original was a double (EXP_SHIFTS 20,
 * MANT_SHIFTS 7). The table is two halves of SQRT_TAB_SIZE/2 entries (even/odd
 * exponent, selected by EXP_LSB); the top SQRT_BITS mantissa bits index each half.
 */
#define SQRT_BITS 13

/*
 * SQRT_TAB_SIZE - the size of the lookup table - must be a power of four.
 */
#define SQRT_TAB_SIZE (1<<(SQRT_BITS+1))

/*
 * MANT_SHIFTS is the number of shifts to move mantissa into position.
 * If you quadruple the table size subtract two from this constant,
 * if you quarter the table size then add two.
 */

#define EXP_BIAS   127				/// Exponents are always positive
#define EXP_SHIFTS 23				/// Shifts exponent to least sig. bits
#define EXP_LSB	(1<<EXP_SHIFTS)		/// The LSB of the exponent 1 << EXP_SHIFTS
#define MANT_MASK  (EXP_LSB-1)		/// Mask to extract mantissa

/*
 * Note: the "(16384, 7) ..." pairs above are the Graphics Gems III
 * DOUBLE values (EXP_SHIFTS 20). For this FLOAT port (EXP_SHIFTS 23) the table is
 * split into two halves of SQRT_TAB_SIZE/2 entries selected by EXP_LSB, so the top
 * log2(SQRT_TAB_SIZE/2) mantissa bits index each half: MANT_SHIFTS = 23 - that.
 * Float-correct pairs are: (16384, 10) (4096, 12) (1024, 14) (256, 16) -- which is
 * why MANT_SHIFTS is 10 here, not the gem's 7.
 */
#define MANT_SHIFTS (23-SQRT_BITS)

#define EXP_BIAS   127               /// Exponents are always positive
#define EXP_SHIFTS 23                /// Shifts exponent to least sig. bits
#define EXP_LSB	(1<<EXP_SHIFTS)      /// 0x00800000 // 1 << EXP_SHIFTS
#define MANT_MASK  (EXP_LSB-1)       /// 0x007FFFFF // Mask to extract mantissa

#define SIN_TABLE_SIZE   8192                                  /// sin/cos: samples per 2*pi
#define SIN_TABLE_EXTENT (SIN_TABLE_SIZE + SIN_TABLE_SIZE / 4) /// 10240: base period + cos quarter-shift headroom
#define TAN_TABLE_SIZE   4096                                  /// tan: samples per 2*pi
#define ASIN_TABLE_SIZE  4096                                  /// asin: entries over input domain [-1, +1]
#define ATAN_TABLE_SIZE  4096                                  /// atan: ratio-range subdivisions
#define ATAN_MAX_INDEX   1017                                  /// tan index just below pi/2 (1024-7); max ratio ~93

/*
 * Tables, forward declarations of the tables that live in fastmath.asm
 */
extern "C" {
	extern float sin_tab[];
	extern float asin_tab[];
	extern float tan_tab[];
	extern float atan_tab[];
	extern int sqrt_tab[];
}


/// <summary>
/// Fetches the square root of the value supplied.
/// This is the double precision face of sqrtf, so that callers can reach for
/// fastmath in place of the runtime library without changing their types. It
/// carries no more precision than the float table behind it.
/// </summary>
/// <returns>Returns with the square root. A negative value is treated as positive.</returns>
double __cdecl fastmath::sqrt(double f)
{
	return((float)(fastmath::sqrtf(f)));
}


/// <summary>
/// Fetches the sine of the angle supplied.
/// This is the double precision face of sinf, and carries no more precision than
/// the float table behind it.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the sine of the angle.</returns>
double __cdecl fastmath::sin(double f)
{
	return((float)(fastmath::sinf(f)));
}


/// <summary>
/// Fetches the cosine of the angle supplied.
/// This is the double precision face of cosf, and carries no more precision than
/// the float table behind it.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the cosine of the angle.</returns>
double __cdecl fastmath::cos(double f)
{
	return((float)(fastmath::cosf(f)));
}


/// <summary>
/// Fetches the tangent of the angle supplied.
/// This is the double precision face of tanf, and carries no more precision than
/// the float table behind it.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the tangent of the angle.</returns>
double __cdecl fastmath::tan(double f)
{
	return((float)(fastmath::tanf(f)));
}


/// <summary>
/// Fetches the arc sine of the value supplied.
/// This is the double precision face of asinf, and carries no more precision than
/// the float table behind it.
/// </summary>
/// <returns>Returns with the angle in radians, in the range -pi/2 to +pi/2.</returns>
/// <remarks>The value must lie between -1 and +1; the table index is not clamped.</remarks>
double __cdecl fastmath::asin(double f)
{
	return((float)(fastmath::asinf(f)));
}


/// <summary>
/// Fetches the arc cosine of the value supplied.
/// This is the double precision face of acosf, and carries no more precision than
/// the float table behind it.
/// </summary>
/// <returns>Returns with the angle in radians, in the range 0 to pi.</returns>
double __cdecl fastmath::acos(double f)
{
	return((float)(fastmath::acosf(f)));
}


/// <summary>
/// Fetches the arc tangent of the value supplied.
/// This is the double precision face of the single argument arc tangent, and it
/// hands the value on as a ratio against one.
/// </summary>
/// <returns>Returns with the angle in radians, in the range -pi/2 to +pi/2.</returns>
double __cdecl fastmath::atan(double f)
{
	return((float)(fastmath::atan2(f, 1)));
}


/// <summary>
/// Fetches the arc tangent of a vector, resolved into the correct quadrant.
/// This is the double precision face of atan2f, so that callers can reach for
/// fastmath in place of the runtime library without changing their types.
/// </summary>
/// <param name="y">The vertical component of the vector.</param>
/// <param name="x">The horizontal component of the vector.</param>
/// <returns>Returns with the angle in radians, in the range -pi to +pi.</returns>
double __cdecl fastmath::atan2(double y, double x)
{
	return((float)(fastmath::atan2f(y, x)));
}


void fastmath_blank_1(void)
{
}

void fastmath_blank_3(void)
{
}

void fastmath_blank_2(void)
{
}


/// Builds sqrt_tab -- the Graphics Gems III init_sqrt_tab() (Gems I "build_table"),
/// retargeted to float. Each of the two halves holds the square-root mantissas for
/// an even (EXP_BIAS) and an odd (EXP_BIAS + 1) exponent; sqrtf() picks the half via
/// the odd-exponent bit. NOTE: sqrt_tab is baked as static data in fastmathtab.asm
/// (taken from the game), so fastmath::sqrtf() reads it directly. This generator
/// reproduces those same values but has no caller: the game builds the table during
/// startup, and that startup path is not part of this tree. The same is true of
/// init_sin_tab / init_asin_tab / init_tan_tab / init_atan_tab below.

/// <summary>
/// Builds the square root lookup table used by sqrtf.
/// </summary>
void init_sqrt_tab(void)
{
	short		 i;
	float		 value;
	unsigned int  *fi = (unsigned int *) &value + MOST_SIG_OFFSET;

	for (i = 0; i != SQRT_TAB_SIZE/2; i++) {

		value = 0; /// Clears least sig part
		*fi = (i << MANT_SHIFTS) | (EXP_BIAS << EXP_SHIFTS);
		value = fastmath::sqrt(value);
		sqrt_tab[i] = *fi & MANT_MASK;

		value = 0; /// Clears least sig part
		*fi = (i << MANT_SHIFTS) | ((EXP_BIAS + 1) << EXP_SHIFTS);
		value = fastmath::sqrt(value);
		sqrt_tab[i + SQRT_TAB_SIZE/2] = *fi & MANT_MASK;
	}
}


/// <summary>
/// Fetches the square root of the value supplied.
/// This is the table driven replacement for the runtime library sqrt. The exponent
/// is halved outright and only the mantissa's root comes from the table, so the
/// answer is a close approximation rather than a correctly rounded result.
/// </summary>
/// <param name="f">The value to take the square root of.</param>
/// <returns>Returns with the square root. A negative value is treated as positive.</returns>
float fastmath::sqrtf(float f)
{
	short expo; /// the exponent
	float x = f;

	unsigned int  *fi = (unsigned int *) &x + MOST_SIG_OFFSET;

	/*
	 * check for square root of 0
	 */
	if (f == 0) {
		return(0);
	}

	x = (x <= 0) ? x * -1.0 : x;

	/*
	 * get the exponent - on a SPARC the exponent is stored with EXP_BIAS added
	 */
	expo = (*fi >> EXP_SHIFTS) - EXP_BIAS;

	/// leave only the mantissa
	*fi &= MANT_MASK;

	if (expo & 0x01) {
		/*
		 * the exponent is odd so we have to look it up in the second half of
		 * the lookup table, so we set the high bit
		 */
		*fi |= EXP_LSB;
	}

	/*
	 * divide the exponent by two
	 * note that in C the shift operators are sign preserving for signed operands
	 */
	expo >>= 1;

	/// Do the table lookup, based on the quaternary mantissa,
	/// then reconstruct the result back into a float

	*fi = (sqrt_tab[*fi >> MANT_SHIFTS]) + ((expo + EXP_BIAS) << EXP_SHIFTS);

	return(x);
}


/// NOTE: the trig routines below (sin/cos/tan/asin/acos/atan/atan2) are NOT from
/// Graphics Gems -- only the square root above is. These are plain quarter-wave /
/// quadrant table lookups (Westwood in-house). Key points:
/// - sin and cos share sin_tab; cos indexes it with a +2048 quarter-period offset
/// (2048 = a quarter of the 8192-sample half-period), i.e. cos(x) = sin(x + pi/2).
/// - the "odd" bit is a 1-sample linear nudge toward the next table entry.
/// - acosf uses the identity acos(x) = pi/2 - asin(x) (DEG_TO_RAD(90) == pi/2).
/// - atan2f uses the generic quadrant reconstruction (x<0 -> pi-r, y<0 -> -r).
/// - atan_tab[4097] is a padding slot -- the tables are over-allocated by one
/// entry -- repurposed to hold the atan input step size; init_atan_tab()
/// computes it once and atan2f() reuses it as the divisor when indexing
/// atan_tab.

/// <summary>
/// Builds the sine lookup table used by both sinf and cosf.
/// </summary>
void init_sin_tab(void)
{
	double half = SIN_TABLE_SIZE/2;

	for (int i = 0; i != SIN_TABLE_EXTENT + 1; i++) {
		float angle = i * (M_PI / half);
		sin_tab[i] = fastmath::sinf(angle);
	}
}


/// <summary>
/// Fetches the sine of the angle supplied.
/// This is the table driven replacement for the runtime library sin, and it is the
/// routine the rest of the fastmath trig family is built around.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the sine of the angle.</returns>
float fastmath::sinf(float f)
{
	static float _s = SIN_TABLE_SIZE / M_PI;

	int scaled = f * _s;
	bool odd = scaled & 1;
	int index = (scaled / 2) % SIN_TABLE_SIZE;

	if (index < 0) {
		index += SIN_TABLE_SIZE;
	}

	//assert(index >= 0 && index < SIN_TABLE_SIZE);

	if (odd && index < (SIN_TABLE_SIZE - 1)) {
		index++;
	}
	return(sin_tab[index]);
}


/// <summary>
/// Fetches the cosine of the angle supplied.
/// This is the table driven replacement for the runtime library cos. It shares the
/// sine table, reading it a quarter of a period further along.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the cosine of the angle.</returns>
float fastmath::cosf(float f)
{
	static float _s = SIN_TABLE_SIZE / M_PI;

	int scaled = f * _s;
	bool odd = scaled & 1;
	int index = (scaled / 2) % SIN_TABLE_SIZE;

	if (index < 0) {
		index += SIN_TABLE_EXTENT;
	} else {
		index += SIN_TABLE_SIZE / 4;
	}

	//assert(index >= 0 && index < 10241);

	if (odd && index < (SIN_TABLE_EXTENT - 1)) {
		index++;
	}

	return(sin_tab[index]);
}


/// <summary>
/// Builds the arc sine lookup table used by asinf.
/// </summary>
void init_asin_tab(void)
{
	double half = ASIN_TABLE_SIZE/2;

	for (int i = 0; i != ASIN_TABLE_SIZE + 1; i++) {
		float value = i / half - 1;
		asin_tab[i] = fastmath::asinf(value);
	}
}


/// <summary>
/// Fetches the arc sine of the value supplied.
/// This is the table driven replacement for the runtime library asin.
/// </summary>
/// <param name="f">The sine value to be turned back into an angle.</param>
/// <returns>Returns with the angle in radians, in the range -pi/2 to +pi/2.</returns>
/// <remarks>The value must lie between -1 and +1; the table index is not clamped.</remarks>
float fastmath::asinf(float f)
{
	int i = (int)((f + 1) * (ASIN_TABLE_SIZE / 2));

	return(asin_tab[i]);
}


/// <summary>
/// Fetches the arc cosine of the value supplied.
/// This routine reaches its answer through asinf by way of the identity
/// acos(x) = pi/2 - asin(x), so it shares that routine's table and its accuracy.
/// </summary>
/// <returns>Returns with the angle in radians, in the range 0 to pi.</returns>
float fastmath::acosf(float f)
{
	return(DEG_TO_RAD(90) - fastmath::asinf(f));
}


/// <summary>
/// Builds the tangent lookup table used by tanf.
/// </summary>
void init_tan_tab(void)
{
	double half = TAN_TABLE_SIZE/2;

	for (int i = 0; i != TAN_TABLE_SIZE + 1; i++) {
		float angle = i * (M_PI/half);
		tan_tab[i] = fastmath::tanf(angle);
	}
}


/// <summary>
/// Fetches the tangent of the angle supplied.
/// This is the table driven replacement for the runtime library tan.
/// </summary>
/// <param name="f">The angle expressed in radians.</param>
/// <returns>Returns with the tangent of the angle.</returns>
float fastmath::tanf(float f)
{
	int i = int(f * (TAN_TABLE_SIZE / 2 / M_PI)) % TAN_TABLE_SIZE;
	if (i < 0) {
		i += TAN_TABLE_SIZE;
	}
	//assert(((i) >= 0) && i < TAN_TABLE_SIZE);
	return(tan_tab[i]);
}


/// <summary>
/// Builds the arc tangent lookup table used by atan2f.
/// This routine also records the input step size that atan2f divides by when it
/// indexes the table.
/// </summary>
void init_atan_tab(void)
{
	float size = ATAN_TABLE_SIZE;

	float pi = M_PI;
	atan_tab[ATAN_TABLE_SIZE + 1] = pi/size/pi * tan_tab[ATAN_MAX_INDEX];
	float value = 0;
	for (int i = 0; i != ATAN_TABLE_SIZE + 1; i++) {
		atan_tab[i] = fastmath::atan(value);
		value += atan_tab[ATAN_TABLE_SIZE + 1];
	}
}


/// <summary>
/// Fetches the arc tangent of a vector, resolved into the correct quadrant.
/// This is the table driven replacement for the runtime library atan2. The angle is
/// looked up from the ratio of the two components, then reflected into the quadrant
/// that their signs call for.
/// </summary>
/// <param name="y">The vertical component of the vector.</param>
/// <param name="x">The horizontal component of the vector.</param>
/// <returns>Returns with the angle in radians, in the range -pi to +pi.</returns>
float fastmath::atan2f(float y, float x)
{
	if (x == 0) {
		if (y == 0) {
			return(0);
		}

		if (y > 0) {
			return(M_PI/2);
		}
		return(-M_PI/2);
	}

	float result;
	float z = y / x;
	int i = abs((int)(z / atan_tab[ATAN_TABLE_SIZE + 1]));

	if (i < ATAN_TABLE_SIZE + 1) {
		result = atan_tab[i];
	} else  {
		result = M_PI/2;
	}

	if (x < 0) {
		result = M_PI - result;
	}

	if (y < 0) {
		result = -result;
	}

	return(result);
}


/// <summary>
/// Fetches the arc tangent of the value supplied.
/// This routine is the single argument form, and simply hands the value to atan2f
/// as a ratio against one.
/// </summary>
/// <returns>Returns with the angle in radians, in the range -pi/2 to +pi/2.</returns>
float fastmath::atanf(float f)
{
	return((float)(fastmath::atan2f(f, 1)));
}


/// This looks like the "punishment" half of an anti-tamper scheme -- a check elsewhere would
/// call it to quietly wreck a hot function so the game fails later, far from the check. That
/// is an inference: no caller exists in this codebase and none was found.

/// <summary>
/// Sabotages a routine so that its next call will fault.
/// This routine makes the page holding the target writable, overwrites the first
/// dword of its prologue with zeros, restores execute protection, and flushes the
/// instruction cache so the processor sees the damage. The target does not fault
/// here -- it faults the next time somebody calls it.
/// </summary>
/// <param name="funcptr">Pointer to the routine that is to be sabotaged.</param>
static void Sabotage_Routine(LPVOID funcptr)
{
	MEMORY_BASIC_INFORMATION info;
	memset(&info, 0, sizeof(info));

	DWORD * numptr = (DWORD*)funcptr;
	if (VirtualQuery(numptr, &info, sizeof(info))) {
		PVOID addr = info.BaseAddress;
		SIZE_T regsize = info.RegionSize;
		DWORD flOldProtect = 0;
		if (!VirtualProtect(addr, regsize, PAGE_WRITECOPY, &flOldProtect)) {
			DebugString("VirtualProtect failed with error code %d - %s\n", GetLastError(), Last_Error_Text());
			return;
		}

		*numptr = 0;
		if (!VirtualProtect(addr, regsize, PAGE_EXECUTE, &flOldProtect)) {
			DebugString("VirtualProtect failed with error code %d - %s\n", GetLastError(), Last_Error_Text());
		}
		FlushInstructionCache(GetCurrentProcess(), 0, 0);
	}
}


/// <summary>
/// Corrupts the entry point of fastmath::sin.
/// This is the punishment half of an anti-tamper scheme. Sine is a deliberate
/// choice of victim: it is hot enough (facing, ballistics, rendering) that the
/// crash arrives within a frame or two, but at a call site nowhere near whatever
/// detection routine decided to fire this.
/// </summary>
void Sabotage_Sine(void)
{
	Sabotage_Routine(fastmath::sin);
}
