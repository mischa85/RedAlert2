/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "noinit.h"

class StorageClass
{
	public:
		StorageClass(void);
		StorageClass(NoInitClass const & x) {}
		~StorageClass(void) {};

		int Get_Total_Value(void) const;
		float Get_Total_Amount(void) const; // RA2: float
		float Get_Amount(int slot) const; // RA2: float
		float Increase_Amount(float amount, int slot); // RA2: float
		float Decrease_Amount(float amount, int slot); // RA2: float
		int First_Used_Slot(void) const;

		StorageClass operator+(StorageClass &that) const;
		StorageClass operator+=(StorageClass &that);
		StorageClass operator-(StorageClass &that) const;
		StorageClass operator-=(StorageClass &that);

	private:
		/*
		 * This is the amount held of each tiberium type, one slot per Tiberiums heap entry.
		 * Keeping them apart is what lets a refinery pay the right value for a mixed load.
		 */
		float Values[4]; // RA2: float
};
