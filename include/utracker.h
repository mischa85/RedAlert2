/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : UTRACKER.H                               *
 *                                                                         *
 *                   Programmer : Steve Tall                               *
 *                                                                         *
 *                   Start Date : June 3rd, 1996                           *
 *                                                                         *
 *                  Last Update : June 7th, 1996 [ST]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *  The UnitTracker class exists to track the various statistics           *
 *   required for internet games.                                          *
 *                                                                         *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once


/*
**	UnitTracker Class
*/

class UnitTrackerClass
{
	public:
		UnitTrackerClass(void); // RA2: no count; the totals are held inline
		~UnitTrackerClass(void);

		void Increment_Unit_Total(int unit_type);
		void Decrement_Unit_Total(int unit_type);
		void Clear_Unit_Total(void);
		void Populate_Unit_Count(int count); // RA2

		int Get_Unit_Total(void); // RA2: the sum of all totals
		int * Get_All_Totals(void);
		int Get_Unit_Count(void) { return(UnitCount); };

		void To_Network_Format(void);
		void To_PC_Format(void);

	private:
		int UnitTotals[512]; // RA2: held inline rather than allocated; the constructor sets UnitCount to 512
		int UnitCount;
		int InNetworkFormat;
};
