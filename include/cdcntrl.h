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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Renegade Setup/Autorun/CDCNTRL.H                           $*
 *                                                                                             *
 *                      $Author:: Maria_l                                                     $*
 *                                                                                             *
 *                     $Modtime:: 4/20/01 2:07p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once


#undef STRICT
#define  STRICT
#include <cctype>
#include <windows.h>
#include <windowsx.h>


#define MAX_CD_DRIVES 26

class CDControlClass {

	public:
		CDControlClass(void);
		~CDControlClass(void);

		/*
		**	Force the CD tray to open on the specified drive.
		*/
		void Force_CD_Eject(int drive);

		/*
		**	Prevent the user from ejecting the CD in the specified drive.
		*/
		bool Lock_CD_Tray(int drive);

		/*
		**	Allow the user to eject the CD in the specified drive.
		*/
		bool Unlock_CD_Tray(int drive);

		void Unlock_All_CD_Trays(void);


	private:
		/*
		**	NT functions.
		*/
		bool Eject_CD(char drive);
		HANDLE Open_Removable_Volume(char drive);
		bool Lock_Volume(HANDLE volume);
		bool Unlock_Volume(HANDLE volume);
		bool Dismount_Volume(HANDLE volume);
		bool Prevent_Removal_Of_Volume(HANDLE volume, bool prevent);
		bool Auto_Eject_Volume(HANDLE volume);
		bool Close_Removable_Volume(HANDLE volume);
		bool Lock_CD_Drive(char drive);
		bool Unlock_CD_Drive(char drive);

		/*
		**	Win9x functions.
		*/
		HANDLE WINAPI Open_VWin32 (void);
		bool WINAPI Close_VWin32 (HANDLE vwin32);
		bool WINAPI Unlock_Logical_Volume (HANDLE vwin32, char drive);
		bool WINAPI Lock_Logical_Volume (HANDLE vwin32, char drive, char lock_level, short permissions);
		bool Lock_Volume_95 (HANDLE vwin32, char drive);
		bool Unlock_Volume_95 (HANDLE vwin32, char drive);
		bool Auto_Eject_Volume_95 (HANDLE vwin32, char drive);
		void Eject_CD_Win95 (char drive);
		bool Lock_CD_Drive_95(char drive);
		bool Unlock_CD_Drive_95(char drive);

		/*
		 * This is the number of tray locks outstanding on each drive, indexed by drive
		 * number (0 = A:). Locks nest, so the count is what allows every one of them to be
		 * released on shutdown and the player left able to eject their CDs again.
		 */
		int LockCount[MAX_CD_DRIVES];
};

extern CDControlClass CDControl;


#define LOCK_TIMEOUT        500			/// Half a Second
#define LOCK_RETRIES        20			/// 20 times

/*
**	Low level structures for Win9x.
**
**	DeviceIoControl infrastructure
*/
#if !defined (VWIN32_DIOC_DOS_IOCTL)
#define VWIN32_DIOC_DOS_IOCTL      1

typedef struct _DIOC_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS, *PDIOC_REGISTERS;

#endif

/*
**	Intel x86 processor status flags
*/
#define CARRY_FLAG             0x0001

/*
**	DOS IOCTL function support
*/

#pragma pack(1)

/*
**	Parameters for locking/unlocking removable media
*/
typedef struct _PARAMBLOCK {
   BYTE bOperation;
   BYTE bNumLocks;
} PARAMBLOCK, *PPARAMBLOCK;

#pragma pack()
