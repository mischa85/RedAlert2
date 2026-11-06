/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once


struct VolInfo {
	/*
	 * This is the Win32 drive type of the volume -- DRIVE_CDROM, DRIVE_FIXED, and so on.
	 */
	int DriveType;

	/*
	 * This is the volume label. The disc scan compares it against "TS1" through "TS8" to
	 * recognize a game CD.
	 */
	char VolumeName[64];

	/*
	 * This is the serial number the volume reports.
	 */
	unsigned int VolumeSerialNumber;

	/*
	 * This is the name of the file system the volume is formatted with, such as "CDFS".
	 */
	char FileSystemName[64];

	/*
	 * This is the number of free bytes on the volume. A drive with any free space at all
	 * cannot be a pressed disc, so the scan rejects it out of hand.
	 */
	unsigned int FreeSpace;

	/*
	 * This is the capacity of the volume, expressed in bytes.
	 */
	unsigned int TotalSpace;
};


extern bool VolumeCheck(void);
extern bool GetVolInfo(const char * path, VolInfo * info);
extern void CountFiles(char const * path, unsigned int & dircount, unsigned int & filecount, unsigned int & size);
