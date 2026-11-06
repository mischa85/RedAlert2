/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "volinfo.h"

#include "cd.h"
#include "dbgprint.h"
#include "win.h"

#include <cstdio>


/// <summary>
/// Determines if a game CD is present in the machine.
/// This routine is used by the CD handling code to confirm that one of the game discs
/// is mounted before it goes looking for data on it.
/// </summary>
/// <returns>bool; Was a game CD found?</returns>
/// <remarks>The volume scan is disabled at present, so this check always succeeds.</remarks>
bool VolumeCheck(void)
{
	return(true);

#ifdef _DEMO
	return(true);
#endif

/// This is the volume scan from TS 1.13, kept here for reference rather than compiled.
#if 0
	char buffer[260];
	VolInfo info;

	if (!GetLogicalDriveStrings(sizeof(buffer), buffer)) {
		return (false);
	}

	//
	//	Loop over all the drives
	//
	const char * drive_root_name = buffer;
	while (strlen(drive_root_name) != 0) {

		//
		//	Only check CD drives
		//
		if (GetDriveType(drive_root_name) == DRIVE_CDROM) {

			if (!GetVolInfo(drive_root_name, &info)) {
				DebugString("VolumeCheck() - GetVolInfo failed\n");
				drive_root_name += strlen(drive_root_name) + 1;
				continue;
			} else {
				/// Is this the game CD?
				char name[32];
				for (int i = 0; i < 8; i++) {
					sprintf(name, "TS%d", i + 1);
					if (!stricmp(name, info.VolumeName)) {
						return (true);
					}
				}
			}
		}

		//
		//	Advance to the next drive
		//
		drive_root_name += strlen(drive_root_name) + 1;
	}

	if (CD().Force_Available()) {
		return (true);
	}
	return (false);
#endif
}


/// <summary>
/// Fetches the volume information for a drive.
/// This routine is used by the CD scan to identify a disc. It records the drive type,
/// the free and total space, and the volume and file system names.
/// </summary>
/// <param name="drive_root_name">The root path of the drive to examine.</param>
/// <param name="info">The record to fill in with the volume information.</param>
/// <returns>bool; Was the information fetched? A drive that still reports free space
/// is rejected along with any drive that fails to answer.</returns>
inline bool GetVolInfo(const char * drive_root_name, VolInfo * info)
{
	memset(info, 0, sizeof(VolInfo));
	info->DriveType = GetDriveType(drive_root_name);

	DWORD sectorspercluster;
	DWORD bytespersector;
	DWORD freeclustercount;
	DWORD totalclustercount;

	if (GetDiskFreeSpace(drive_root_name, &sectorspercluster, &bytespersector, &freeclustercount, &totalclustercount) == 0) {
		DebugString("GetDiskFreeSpace() failed: %s\n", Last_Error_Text());
		return(false);
	}

	info->FreeSpace = freeclustercount * bytespersector * sectorspercluster;

	if (freeclustercount != 0) {
		return(false);
	}

	info->TotalSpace = totalclustercount * bytespersector * sectorspercluster;

	DWORD FileSystemFlags;
	DWORD MaximumComponentLength;

	//
	// Get the name of this volume
	//
	if (GetVolumeInformation(drive_root_name, info->VolumeName, sizeof(info->VolumeName), (DWORD *)&info->VolumeSerialNumber, &MaximumComponentLength, &FileSystemFlags, info->FileSystemName, sizeof(info->FileSystemName)) == 0) {
		DebugString("GetVolumeInformation() - failed: %s\n", Last_Error_Text());
		return(false);
	}

	return(true);
}


/// <summary>
/// Counts the files and directories beneath a path.
/// This routine walks the whole directory tree under the search path and adds what it
/// finds to the supplied totals.
/// </summary>
/// <param name="path">The wildcard search path to scan, as handed to FindFirstFile.</param>
/// <remarks>The totals are added to rather than assigned, so clear them before the
/// first call.</remarks>
inline void CountFiles(char const * path, unsigned int & dircount, unsigned int & filecount, unsigned int & size)
{
	HANDLE handle;
	char filename[260];
	WIN32_FIND_DATA finddata;

	handle = FindFirstFile(path, &finddata);
	if (handle != INVALID_HANDLE_VALUE) {
		do {

			if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

				/// FindFirstFile will always return "." and ".." as first two dirs
				if (strcmpi(finddata.cFileName, ".") == 0) continue;
				if (strcmpi(finddata.cFileName, "..") == 0) continue;

				dircount++;
				strcpy(filename, path);

				filename[strlen(filename) - 1] = '\0';

				strcat(filename, finddata.cFileName);
				strcat(filename, "\\*");

				CountFiles(filename, dircount, filecount, size);

			} else {

				filecount++;
				size += finddata.nFileSizeLow;
			}
		}

		while (FindNextFile(handle, &finddata));

		FindClose(handle);
	}
}
