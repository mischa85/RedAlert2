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

/* $Header: /CounterStrike/CDFILE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood LIbrary                                             *
 *                                                                                             *
 *                    File Name : CDFILE.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 18, 1994                                             *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "bfiofile.h"

/*
**	This class is derived from the BufferIOFileClass. This class adds the functionality of searching
**	across multiple directories or drives. It is designed for the typical case of a CD-ROM game
**	were some data exists in the current directory (hard drive) and the rest exists on the CD-ROM.
**	Searching for the file occurs by first examining the current directory. If the file does not
**	exist there, then all the paths available are examined in turn until the file can be found.
**	For opening files to write, only the current directory is examined. The directory search order
**	is controlled by the path list as submitted to Set_Search_Drives(). The format of the path
**	string is the same as the DOS path string.
*/
class CDFileClass : public BufferIOFileClass
{
		typedef BufferIOFileClass BASECLASS;

	public:
		CDFileClass(char const *filename);
		CDFileClass(void);
		virtual ~CDFileClass(void) override {};

		virtual char const * Set_Name(char const *filename) override;
		virtual int Open(char const *filename, int rights=READ) override;
		virtual int Open(int rights=READ) override;

		void Searching(int on) {IsDisabled = !on;};

		static bool Is_There_Search_Drives(void) {return(First != NULL);};
		static int Set_Search_Drives(char * pathlist);
		static void Add_Search_Drive(char *path);
		static void Clear_Search_Drives(void);
		static void Refresh_Search_Drives(void);
		static void Set_CD_Drive(int drive);
		static int  Get_CD_Drive(void) {return(CurrentCDDrive);};
		static int  Get_Last_CD_Drive(void) {return(LastCDDrive);};

		static bool Find_First_File(char *buffer);
		static bool Find_Next_File(char *buffer);
		static void Find_Close(void);

	private:

		/*
		**	Is multi-drive searching disabled for this file object?
		*/
		bool IsDisabled;

		/*
		**	This is the control record for each of the drives specified in the search
		**	path. There can be many such search paths available.
		*/
		struct SearchDriveType {
			void * Next;        // Pointer to next search record.
			char const * Path;  // Pointer to path string.
		};

		/*
		**	This points to the first path record.
		*/
		static SearchDriveType * First;

		/*
		**	This is a copy of the unparsed search path list
		*/
		static char RawPath[512];

		/*
		**	The drive letter of the current cd drive
		*/
		static int CurrentCDDrive;

		/*
		**	The drive letter of the last used CD drive
		*/
		static int LastCDDrive;
};
