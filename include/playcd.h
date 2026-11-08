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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : WWLIB                                    *
 *                                                                         *
 *                    File Name : PLAYCD.H                                 *
 *                                                                         *
 *                   Programmer : STEVE WETHERILL                          *
 *                                                                         *
 *                   Start Date : 5/13/94                                  *
 *                                                                         *
 *                  Last Update : June 4, 1994   [SW]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*/
#pragma once


/***************************************************************************
 * GetCDClass -- object which will return logical CD drive                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/04/1994 SW : Created.                                              *
 *=========================================================================*/

#define MAX_CD_DRIVES 26
#define NO_CD_DRIVE -1

class	GetCDClass {

protected:

	int	CDDrives[MAX_CD_DRIVES];    //Array containing CD drive letters
	int	CDCount;                    //Number of available CD drives
	int	CDIndex;

public:


	GetCDClass(void);   // This is the default constructor
	~GetCDClass(void);  // This is the destructor

	inline	int	Get_First_CD_Drive(void);
	inline	int	Get_Next_CD_Drive(void);
	inline	int	Get_Number_Of_Drives(void) {return(CDCount);};

};



/***********************************************************************************************
 * GCDC::Get_Next_CD_Drive -- return the logical drive number of the next CD drive             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Logical drive number of a cd drive or -1 if none                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/21/96 3:50PM ST : Created                                                              *
 *=============================================================================================*/
inline int GetCDClass::Get_Next_CD_Drive(void)
{
	if (CDCount){
		if (CDIndex == CDCount) CDIndex = 0;
		return(CDDrives[CDIndex++]);
	}else{
		return(-1);
	}
}



/***************************************************************************
 * GCDC::Get_First_CD_Drive -- return the number of the first CD drive     *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *         none                                                            *
 * OUTPUT:                                                                 *
 *         logical drive number                                            *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/26/1994 SW : Created.                                              *
 *   12/4/95    ST : fixed for Win95                                       *
 *=========================================================================*/
inline int GetCDClass::Get_First_CD_Drive(void)
{
	CDIndex = 0;
	return(Get_Next_CD_Drive());
}


/***************************** End of Playcd.h ****************************/
