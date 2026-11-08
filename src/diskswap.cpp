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

#include "always.h"

#include "diskswap.h"

#include "assert.h"
#include "cdfile.h"
#include "globals.h"
#include "playcd.h"
#include "stimer.h"
#include "timer.h"

/***********************************************************************************************
 * Get_CD_Index -- returns the volume type of the CD in the given drive                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    drive number                                                                      *
 *           timeout                                                                           *
 *                                                                                             *
 * OUTPUT:   0 = gdi                                                                           *
 *           1 = nod                                                                           *
 *           2 = covert or CS                                                                  *
 *           3 = Aftermath
 *           5 = DVD
 *          -1 = non C&C                                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/21/96 5:27PM ST : Created                                                              *
 *    01/20/97 V.Grippi added CS support                                                       *
 *=============================================================================================*/
DiskID DiskSwap::Get_Disk_ID(int cd_drive, int timeout)
{
	char		volname_label[32];
	char		volume_name[128];
	char		buffer[128];
	unsigned	filename_length;
	unsigned	misc_dword;

	CDTimerClass<SystemTimerClass> timer;

	timer = timeout;
	int cd = cd_drive + 'A';
	int index = -1;

	/*
	**	Get the volume label. If we get a 'not ready' error then retry for the timeout
	**	period.
	*/
	for (;;)
	{
		wsprintf(buffer,"%c:\\",cd);

		if (GetVolumeInformation ((char const *)buffer, &volume_name[0] ,
				(DWORD)sizeof(volume_name) - 1, (DWORD *)NULL ,
				(DWORD *)&filename_length, (DWORD *)&misc_dword,
				(char *)NULL, (DWORD)0))
		  {

			enum {
				NUM_VOLUMES = 3,
			};
			/*
			**	Match the volume label to the list of known C&C volume labels.
			*/
			for (int i = 0; i < NUM_VOLUMES; i++)
			{
				if (i == 2) {
					strcpy(volname_label, "Firestorm");
				} else {
					sprintf(volname_label, "TS%d", i + 1);
				}
				if (!stricmp(volname_label,volume_name))
				{
					index = i;
					break;
				}
			}
			break;
		}
		else
		{
			/*
			**	Failed to get the volume label on a known CD drive.
			**	If this is a CD changer it may require time to swap the disks so dont return
			**	immediately if the error is ROR_NOT_READY
			*/
			if (!timer)
			{
				return(DISK_ANY);
			}

			int val = GetLastError();

			if (val != ERROR_NOT_READY)
			{
				return(DISK_ANY);
			}
		}
	}

	if (index > -1) {
		int count = 0;
		for (;;) {
			/*
			 * Try opening '.dsk' to verify that the CD is really there and is what
			 * it says it is.
			 */
			wsprintf(buffer, "%c:\\TS%d.dsk", cd, index + 1);

			HANDLE handle = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			if (handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);

				return((DiskID)index);
			}
			else
			{
				count++;
				if (count >= 2) {
					 return(DISK_ANY);
				}
			}
		}
	}

	return(DISK_ANY);
}



/***********************************************************************************************
 * Force_CD_Available -- Ensures that specified CD is available.                               *
 *                                                                                             *
 *    Call this routine when you need to ensure that the specified CD is actually in the       *
 *    CD-ROM drive.                                                                            *
 *                                                                                             *
 * INPUT:   cd    -- The CD that must be available. This will either be "0" for the GDI CD, or *
 *                   "1" for the Nod CD. If either CD will qualify, then pass in "-1".         *
 *                0  = CD1
 *                1  = CD2
 *                2  = Counterstrike
 *                3  = Aftermath
 *                4  = Counterstrike or Aftermath
 *                5  = DVD
 *                -1 = Any CD
 *                -2 = Local Harddisk
 *                                                                                             *
 * OUTPUT:  Is the CD inserted and available? If false is returned, then this indicates that   *
 *          the player pressed <CANCEL>.                                                       *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/11/1995 JLB : Created.                                                                 *
 *   05/22/1996  ST : Handles multiple CD drives / CD changers                                 *
 *   01/20/1997 V.Grippi added expansion cd message
 *=============================================================================================*/
bool DiskSwap::Force_Available(DiskID disk)
{
	assert(disk < DISK_COUNT);

	static int _last = -1;
	int new_cd_drive = 0;

	/*
	** If the required CD is set to LOCAL then it means that the file is present
	** on the local hard drive and we shouldn't have to worry about it.
	*/
	if (disk == DISK_LOCAL) return(true);

	/*
	**	Find out if the CD in the current drive is the one we are looking for
	*/
	int current_drive = CDFileClass::Get_CD_Drive();
	int cd_current = DiskSwap::Get_Disk_ID(current_drive, 1*TIMER_SECOND);

	if (cd_current > DISK_ANY)
	{
		// If the current CD is requested or any CD will work
		if (disk == cd_current || disk == DISK_ANY)
		{
			/*
			**	The required CD is still in the CD drive we used last time
			*/
			new_cd_drive = current_drive;
		}
	}

	// Check the last drive
	if (!new_cd_drive)
	{
		/*
		**	Check the last CD drive we used if it's different from the current one
		*/
		int last_drive = CDFileClass::Get_Last_CD_Drive();

		/*
		**	Make sure the last drive is valid and it isn't the current drive
		*/
		if (last_drive && last_drive != CDFileClass::Get_CD_Drive())
		{
			/*
			**	Find out if there is a C&C cd in the last drive and if so is it the one we are looking for
			**	Give it a nice big timeout so the CD changer has time to swap the discs
			*/
			cd_current = DiskSwap::Get_Disk_ID(last_drive, 10*TIMER_SECOND);
			if (cd_current > DISK_ANY)
			{
				/*
				**	If the cd is present or any cd will work
				*/
				if (disk == cd_current || disk == DISK_ANY)
				{
					/*
					**	The required CD is in the CD drive we used last time
					*/
					new_cd_drive = last_drive;
				}
			}
		}
	}

	/*
	**	Lordy. No sign of that blimming CD anywhere. Search all the CD drives
	**	then if we still can't find it prompt the user to insert it.
	*/
	if (!new_cd_drive)
	{
		/*
		**	Small timeout for the first pass through the drives
		*/
		int drive_search_timeout = 2*TIMER_SECOND;

		for (;;)
		{
			/*
			**	Search all present CD drives for the required disc.
			*/
			for (int i = 0 ; i < CDList.Get_Number_Of_Drives(); i++)
			{
				int cd_drive = CDList.Get_Next_CD_Drive();
				int cd_current = DiskSwap::Get_Disk_ID(cd_drive, drive_search_timeout);

				if (cd_current > DISK_ANY)
				{
					/*
					**	We found a C&C cd - lets see if it was the one we were looking for
					*/
					if (disk == cd_current || disk == DISK_ANY)
					{
						/*
						**	Woohoo! The disk was in a different cd drive. Refresh the search path list
				 		*  and return.
						*/
						new_cd_drive = cd_drive;
						break;
					}
				}

			}

			/*
			**	A new disc has become available so break
			*/
			if (new_cd_drive > DISK_FIRST)
			{
				CDFileClass::Set_CD_Drive(new_cd_drive);
				CDFileClass::Refresh_Search_Drives();
				break;
			}

			/*
			**	Prompt to insert the CD into the drive.
			*/
			if (!Request_Disk(disk))
			{
				new_cd_drive = DISK_FIRST;
				break;
			}

			/*
			**	Increase the timeout for subsequent drive searches.
			*/
			drive_search_timeout = 5*TIMER_SECOND;

		}
	}


	if (!new_cd_drive)
	{
		return(false);
	}

	if (disk > DISK_ANY && _last != disk)
	{
		_last = disk;
		return(Swap(disk));
	}

	return(true);
}


/// <summary>
/// Handles the switch over to a newly inserted disc.
/// This is the placeholder implementation and does no more than put a system message box on
/// the screen. Derived classes override this routine to do the real work of rebinding the
/// game's mix files to the disc that has just been supplied.
/// </summary>
/// <param name="disk">The disc being switched to.</param>
/// <returns>bool; Was the switch accepted?</returns>
bool DiskSwap::Swap(DiskID disk)
{
	char str[256];

	sprintf(str, "Please insert disk %d", disk);
	if (MessageBox(NULL, str, "Hey!", MB_OKCANCEL) == IDOK) {
		return(true);
	}
	return(false);
}


/// <summary>
/// Asks the player to insert the disc that is needed.
/// This is the placeholder implementation and merely reports success. Derived classes
/// override this routine to put up the real disc request dialog and to let the player give
/// up on finding the disc.
/// </summary>
/// <param name="disk">The disc to ask the player for.</param>
/// <returns>bool; Should the search continue? A false return means the player gave
/// up.</returns>
bool DiskSwap::Request_Disk(DiskID disk)
{
	return(true);
}
