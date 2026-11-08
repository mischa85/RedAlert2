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
 *                     $Archive:: /Renegade Setup/Autorun/CDCNTRL.CPP                         $*
 *                                                                                             *
 *                      $Author:: Maria_l                                                     $*
 *                                                                                             *
 *                     $Modtime:: 10/18/01 5:33p                                              $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *                                                                                             *
 *  CDControlClass::Auto_Eject_Volume -- Eject the removable media                             *
 *  CDControlClass::Auto_Eject_Volume_95 -- Eject the specified CD drive                       *
 *  CDControlClass::CDControlClass -- Class constructor                                        *
 *  CDControlClass::Close_Removable_Volume -- Close the handle to a removable volume           *
 *  CDControlClass::Close_VWin32 -- Closes the handle opened by Open_VWin32.                   *
 *  CDControlClass::Dismount_Volume -- Dismount the given volume                               *
 *  CDControlClass::Eject_CD -- Force the CD drive to eject                                    *
 *  CDControlClass::Eject_CD_Win95 -- Eject the cd in the given drive                          *
 *  CDControlClass::Force_CD_Eject -- Ppen the CD tray on the given drive                      *
 *  CDControlClass::Lock_CD_Drive -- Lock the CD tray and prevent ejection                     *
 *  CDControlClass::Lock_CD_Drive_95 -- Prevent the user from ejecting the cd in the given driv*
 *  CDControlClass::Lock_CD_Tray -- Prevent CD ejection on the specified drive                 *
 *  CDControlClass::Lock_Logical_Volume -- Take a lock on a logical volume                     *
 *  CDControlClass::Lock_Volume -- Prevent access by other threads to a given volume           *
 *  CDControlClass::Lock_Volume_95 -- Locks removable media so that it can't be ejected        *
 *  CDControlClass::Open_Removable_Volume -- Fetch a handle to a removable drive               *
 *  CDControlClass::Open_VWin32 -- Opens a handle to VWIN32 to issue low-level disk I/O        *
 *  CDControlClass::Prevent_Removal_Of_Volume -- Disable the eject button on the given drive   *
 *  CDControlClass::Unlock_CD_Drive -- Unlock the CD tray and allow ejection                   *
 *  CDControlClass::Unlock_CD_Drive_95 -- Allow the user to eject the cd in the given drive    *
 *  CDControlClass::Unlock_CD_Tray -- Allow CD ejection on the specified drive                 *
 *  CDControlClass::Unlock_Logical_Volume -- Unlocks a logical volume                          *
 *  CDControlClass::Unlock_Volume -- Allow access by other threads to a given volume           *
 *  CDControlClass::Unlock_Volume_95 -- Unlocks removable media so that it can be ejected      *
 *  CDControlClass::~CDControlClass -- Class destructor                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma warning(disable : 4201)
#include "always.h"

#include "cdcntrl.h"

#include "assert.h"
#include "dbgprint.h"
#include "winfix.h"
#include <cstdio>
#include <tchar.h>
#include <winioctl.h>


CDControlClass CDControl;


/***********************************************************************************************
 * CDControlClass::CDControlClass -- Class constructor                                         *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 2:03AM ST : Created                                                               *
 *=============================================================================================*/
CDControlClass::CDControlClass(void)
{
	for (int i = 0; i < MAX_CD_DRIVES; i++) {
		LockCount[i] = 0;
	}
}


/***********************************************************************************************
 * CDControlClass::~CDControlClass -- Class destructor                                         *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 2:03AM ST : Created                                                               *
 *=============================================================================================*/
CDControlClass::~CDControlClass(void)
{
}

/***********************************************************************************************
 * CDControlClass::Force_CD_Eject -- Ppen the CD tray on the given drive                       *
 *                                                                                             *
 * INPUT:    Drive number (0=a, 1=b etc)                                                       *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 1:17AM ST : Created                                                               *
 *=============================================================================================*/
void CDControlClass::Force_CD_Eject(int drive)
{
	if ( WinVersion.Is_Win9x() ) {
		Eject_CD_Win95(drive);
	}else{
		Eject_CD(drive);
	}
}

/***********************************************************************************************
 * CDControlClass::Lock_CD_Tray -- Prevent CD ejection on the specified drive                  *
 *                                                                                             *
 * INPUT:    drive number (0=a: etc)                                                           *
 *                                                                                             *
 * OUTPUT:   true if locked                                                                    *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *               2/17/99 1:56AM ST : Created                                                   *
 *=============================================================================================*/
bool CDControlClass::Lock_CD_Tray (int drive)
{
	bool locked = true;

	if ( WinVersion.Is_Win9x() ) {
		locked = (Lock_CD_Drive_95(drive));
	}else{
		locked = (Lock_CD_Drive(drive));
	}

	if (locked) {
		LockCount[drive]++;
	}

	DebugString("LockCount on drive %c is %d\n", drive + 'A', LockCount[drive]);
	return(locked);
}

/***********************************************************************************************
 * CDControlClass::Unlock_CD_Tray -- Allow CD ejection on the specified drive                  *
 *                                                                                             *
 * INPUT:    drive number (0=a: etc)                                                           *
 *                                                                                             *
 * OUTPUT:   true if unlocked OK                                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *         2/17/99 1:57AM ST : Created                                                         *
 *=============================================================================================*/
bool CDControlClass::Unlock_CD_Tray (int drive)
{
	bool unlocked = true;

	if (LockCount[drive]) {
		if ( WinVersion.Is_Win9x() ) {
			unlocked = (Unlock_CD_Drive_95(drive));
		}else{
			unlocked = (Unlock_CD_Drive(drive));
		}

		if (unlocked) {
			LockCount[drive]--;
		}
	}

	DebugString("LockCount on drive %c is %d\n", drive + 'A', LockCount[drive]);
	return(unlocked);
}


/// <summary>
/// Releases every CD tray lock this object still holds.
/// This routine is used when shutting down, so that the player is left able to eject their
/// CDs again regardless of how many locks were taken while the game was running.
/// </summary>
void CDControlClass::Unlock_All_CD_Trays (void)
{
	for (int i = 0; i < MAX_CD_DRIVES; i++) {
		while (LockCount[i] > 0) {
			Unlock_CD_Tray(i);
		}
	}
}

/***********************************************************************************************
 * CDControlClass::Open_Removable_Volume -- Fetch a handle to a removable drive                *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:25PM ST : Created                                                              *
 *=============================================================================================*/
HANDLE CDControlClass::Open_Removable_Volume( char drive )
{
	assert (WinVersion.Is_WinNT());

	HANDLE		volume;
	unsigned	drivetype;
	char		volume_name[8];
	char		rootname[5];
	DWORD	access_flags;

	/*----------------------------------------------------------------------------------------
	**	Get the drive type to ensure that this is a removable volume.
	*/
	_stprintf (rootname, _TEXT( "%c:\\" ), drive + 'A');
	drivetype = GetDriveType( rootname );

	switch ( drivetype ) {

		case DRIVE_REMOVABLE:
			access_flags = GENERIC_READ | GENERIC_WRITE;
			break;

		case DRIVE_CDROM:
			access_flags = GENERIC_READ;
			break;

		default:
			DebugString ("Attempt to open non-removable volume for locking or ejection\n");
			return( INVALID_HANDLE_VALUE );
	}

	/*----------------------------------------------------------------------------------------
	**	Get a handle to the volume.
	*/
	_stprintf( volume_name, _TEXT( "\\\\.\\%c:" ), drive + 'A' );
	volume = CreateFile( volume_name, access_flags, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

//	assert (volume != INVALID_HANDLE_VALUE);

	if ( volume == INVALID_HANDLE_VALUE ) {
		DebugString ("Unable to open drive %c: for ejection\n", drive + 'A');
	}
	return( volume );
}

/***********************************************************************************************
 * CDControlClass::Close_Removable_Volume -- Close the handle to a removable volume            *
 *                                                                                             *
 * INPUT:    HANDLE of volume to close                                                         *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:27PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Close_Removable_Volume(HANDLE volume)
{
	assert (WinVersion.Is_WinNT());
	return((CloseHandle(volume)) ? true : false);
}

/***********************************************************************************************
 * CDControlClass::Lock_Volume -- Prevent access by other threads to a given volume            *
 *                                                                                             *
 * INPUT:    HANDLE to volume                                                                  *
 *                                                                                             *
 * OUTPUT:   true if locked                                                                    *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:29PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Lock_Volume( HANDLE volume )
{
	assert( WinVersion.Is_WinNT( ));

	DWORD bytes_returned;
	DWORD sleep_amount		= LOCK_TIMEOUT / LOCK_RETRIES;

	/*
	**	Do this in a loop until a timeout period has expired
	*/
	for ( int trycount = 0; trycount < LOCK_RETRIES; trycount++ ) {

		if ( DeviceIoControl( volume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL )) {
			return( true );
		}
		DebugString("DeviceIoControl failed to lock volume. Error %d - %s\n", GetLastError(), Last_Error_Text());
//		Sleep( sleep_amount );
		Sleep( LOCK_TIMEOUT );
	}
	return( false );
}

/***********************************************************************************************
 * CDControlClass::Unlock_Volume -- Allow access by other threads to a given volume            *
 *                                                                                             *
 * INPUT:    HANDLE to volume                                                                  *
 *                                                                                             *
 * OUTPUT:   true if unlocked                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:29PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Unlock_Volume(HANDLE volume)
{
	assert( WinVersion.Is_WinNT());

	DWORD bytes_returned;
	DWORD sleep_amount	= LOCK_TIMEOUT / LOCK_RETRIES;

	/*
	**	Do this in a loop until a timeout period has expired
	*/
	for ( int trycount = 0; trycount < LOCK_RETRIES; trycount++ ) {

		if ( DeviceIoControl( volume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL )) return( true );
		DebugString ("DeviceIoControl failed to unlock volume. Error %d - %s\n", GetLastError(), Last_Error_Text());
//		Sleep( sleep_amount );
		Sleep( LOCK_TIMEOUT );
	}
	return( false );
}

/***********************************************************************************************
 * CDControlClass::Dismount_Volume -- Dismount the given volume                                *
 *                                                                                             *
 * INPUT:    HANDLE of volume to dismount                                                      *
 *                                                                                             *
 * OUTPUT:   true if volume dismounted OK                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:31PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Dismount_Volume(HANDLE volume)
{
	assert( WinVersion.Is_WinNT());

	DWORD bytes_returned;
	bool result = ((DeviceIoControl( volume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL)) ? true : false );

	if (result == false) {
		DebugString ("DeviceIoControl failed to dismount volume. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}
	return( result );
}

/***********************************************************************************************
 * CDControlClass::Prevent_Removal_Of_Volume -- Disable the eject button on the given drive    *
 *                                                                                             *
 * INPUT:    HANDLE of volume to enable/disable                                                *
 *           true to prevent removal. false to allow it.                                       *
 *                                                                                             *
 * OUTPUT:   true if status changed OK                                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:32PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Prevent_Removal_Of_Volume( HANDLE volume, bool prevent )
{
	assert( WinVersion.Is_WinNT());

	DWORD bytes_returned;
	PREVENT_MEDIA_REMOVAL pmrbuffer;

	pmrbuffer.PreventMediaRemoval = prevent;

	bool result = ((DeviceIoControl( volume, IOCTL_STORAGE_MEDIA_REMOVAL, &pmrbuffer, sizeof(PREVENT_MEDIA_REMOVAL), NULL, 0, &bytes_returned, NULL)) ? true : false);

	if (result == false) {
		DebugString ("DeviceIoControl failed to prevent media removal. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}
	return( result );
}

/***********************************************************************************************
 * CDControlClass::Auto_Eject_Volume -- Eject the removable media                              *
 *                                                                                             *
 * INPUT:    HANDLE of volume to eject                                                         *
 *                                                                                             *
 * OUTPUT:   true if ejection occured                                                          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:34PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Auto_Eject_Volume(HANDLE volume)
{
	assert (WinVersion.Is_WinNT());
	DWORD bytes_returned;
	bool result = ((DeviceIoControl( volume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytes_returned, NULL)) ? true : false);

	if (result == false) {
		DebugString ("DeviceIoControl failed to eject media. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}
	return(result);
}

/***********************************************************************************************
 * CDControlClass::Eject_CD -- Force the CD drive to eject                                     *
 *                                                                                             *
 * INPUT:    drive number                                                                      *
 *                                                                                             *
 * OUTPUT:   true if ejected                                                                   *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/16/99 11:35PM ST : Created                                                              *
 *=============================================================================================*/
bool CDControlClass::Eject_CD(char drive)
{
	assert (WinVersion.Is_WinNT());
	HANDLE volume;
	bool ejected = false;

	volume = Open_Removable_Volume(drive);

	if (volume == INVALID_HANDLE_VALUE) return(false);

	/*
	**	Lock and dismount the volume.
	*/
	if (Lock_Volume(volume) && Dismount_Volume(volume)) {

		/*
		**	Set prevent removal to false and eject the volume.
		*/
		if (Prevent_Removal_Of_Volume(volume, false) && Auto_Eject_Volume(volume)) {
			ejected = true;
		}
	}

	/*
	**	Close the volume so other processes can use the drive.
	*/
	Close_Removable_Volume(volume);

	return(ejected);
}

/***********************************************************************************************
 * CDControlClass::Lock_CD_Drive -- Lock the CD tray and prevent ejection                      *
 *                                                                                             *
 * INPUT:    drive number                                                                      *
 *                                                                                             *
 * OUTPUT:   true if locked                                                                    *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:11AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Lock_CD_Drive(char drive)
{
	assert (WinVersion.Is_WinNT());
	HANDLE volume;
	bool retval = false;

	volume = Open_Removable_Volume(drive);

	if (volume == INVALID_HANDLE_VALUE) return(false);

	/*
	**	Lock the volume.
	*/
	if (Lock_Volume(volume)) {

		/*
		**	Set prevent removal to false
		*/
		if (Prevent_Removal_Of_Volume(volume, true)) {
			retval = true;
		}
	}

	/*
	**	Close the volume so other processes can use the drive.
	*/
	Unlock_Volume(volume);
	Close_Removable_Volume(volume);

	return(true);
}

/***********************************************************************************************
 * CDControlClass::Unlock_CD_Drive -- Unlock the CD tray and allow ejection                    *
 *                                                                                             *
 * INPUT:    drive number                                                                      *
 *                                                                                             *
 * OUTPUT:   true if unlocked                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:11AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Unlock_CD_Drive(char drive)
{
	assert (WinVersion.Is_WinNT());
	HANDLE volume;
	bool retval = false;

	volume = Open_Removable_Volume(drive);

	if (volume == INVALID_HANDLE_VALUE) return(false);

	/*
	**	Lock the volume.
	*/
	if (Lock_Volume(volume)) {

		/*
		**	Set prevent removal to false
		*/
		if (Prevent_Removal_Of_Volume(volume, false)) {
			retval = true;
		}
	}

	/*
	**	Close the volume so other processes can use the drive.
	*/
	Unlock_Volume(volume);
	Close_Removable_Volume(volume);

	return(true);
}

/***********************************************************************************************
 * CDControlClass::Eject_CD_Win95 -- Eject the cd in the given drive                           *
 *                                                                                             *
 * INPUT:    drive. 0=a etc.                                                                   *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 1:04AM ST : Created                                                               *
 *=============================================================================================*/
void CDControlClass::Eject_CD_Win95 (char drive)
{
	assert (WinVersion.Is_Win9x());
	HANDLE vwin32 = INVALID_HANDLE_VALUE;

	drive++;
	vwin32 = Open_VWin32 ();
	assert (vwin32 != INVALID_HANDLE_VALUE);

	/*
	**	Make sure no other applications are using the drive.
	*/
	bool drive_locked = Lock_Logical_Volume (vwin32, drive, 0, 0);
//	assert (drive_locked);
	if (!drive_locked) {
		DebugString("Unable to lock volume %c:\n", 'A' + drive - 1);
		return;
	}

	/*
	**	Make sure there is no software lock keeping the media in the drive.
	*/
	if (!Unlock_Volume_95 (vwin32, drive)) {
		DebugString("Could not unlock media from drive %c:\n", 'A' + drive - 1);
		Unlock_Logical_Volume (vwin32, drive);
		return;
	}

	/*
	**	Eject!
	*/
	if (!Auto_Eject_Volume_95 (vwin32, drive)) {
		DebugString("Could not eject media from drive %c:\n", 'A' + drive - 1);
	}

	Unlock_Logical_Volume (vwin32, drive);
	Close_VWin32 (vwin32);
}

/***********************************************************************************************
 * CDControlClass::Lock_CD_Drive_95 -- Prevent the user from ejecting the cd in the given drive*
 *                                                                                             *
 * INPUT:    drive. 0=a etc.                                                                   *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 1:04AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Lock_CD_Drive_95 (char drive)
{
	DebugString ("CDControlClass::Lock_CD_Drive_95\n");

	bool retval = true;
	assert (WinVersion.Is_Win9x());
	HANDLE vwin32 = INVALID_HANDLE_VALUE;

	DebugString ("Preventing ejection on CD drive %c\n", drive + 'A');

	drive++;
	vwin32 = Open_VWin32();
	assert (vwin32 != INVALID_HANDLE_VALUE);

	/*
	**	Make sure no other applications are using the drive.
	*/
	DebugString ("About to lock logical volume to enable exclusive access\n");

	bool drive_locked = Lock_Logical_Volume (vwin32, drive, 0, 0);
//	assert (drive_locked);
	if (!drive_locked) {
		DebugString("Unable to lock volume %c:\n", 'A' + drive - 1);
		return(false);
	}
	DebugString ("Volume locked OK\n");

	/*
	**	Lock the tray in the closed position.
	*/
	DebugString ("About to prevent CD tray ejection\n");

	if (!Lock_Volume_95 (vwin32, drive)) {
		DebugString("Could not lock CD tray in drive %c:\n", 'A' + drive - 1);
		retval = false;
	}else{
		DebugString ("CD tray ejection disabled OK\n");
	}

	Unlock_Logical_Volume (vwin32, drive);
	Close_VWin32 (vwin32);

	DebugString ("CDControlClass::Lock_CD_Drive_95 returning %s\n", retval ? "true" : "false");

	return(retval);
}

/***********************************************************************************************
 * CDControlClass::Unlock_CD_Drive_95 -- Allow the user to eject the cd in the given drive     *
 *                                                                                             *
 * INPUT:    drive. 0=a etc.                                                                   *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 1:04AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Unlock_CD_Drive_95 (char drive)
{
	DebugString ("CDControlClass::Unlock_CD_Drive_95\n");

	bool retval = true;
	assert (WinVersion.Is_Win9x());
	HANDLE vwin32 = INVALID_HANDLE_VALUE;

	DebugString ("Allowing ejection on CD drive %c\n", drive + 'A');

	drive++;
	vwin32 = Open_VWin32();
	assert (vwin32 != INVALID_HANDLE_VALUE);

	/*
	**	Make sure no other applications are using the drive.
	*/
	DebugString ("About to lock logical volume to enable exclusive access\n");

	bool drive_locked = Lock_Logical_Volume (vwin32, drive, 0, 0);
//	assert (drive_locked);
	if (!drive_locked) {
		DebugString("Unable to lock volume %c:\n", 'A' + drive - 1);
		return(false);
	}
	DebugString ("Volume locked OK\n");

	/*
	**	Unlock the tray to allow ejection.
	*/
	DebugString ("About to allow CD tray ejection\n");

	if (!Unlock_Volume_95 (vwin32, drive)) {
		DebugString("Could not unlock CD tray in drive %c:\n", 'A' + drive - 1);
		retval = false;
	}else{
		DebugString ("CD tray ejection enabled OK\n");
	}

	Unlock_Logical_Volume (vwin32, drive);
	Close_VWin32 (vwin32);

	DebugString ("CDControlClass::Unlock_CD_Drive_95 returning %s\n", retval ? "true" : "false");
	return(retval);
}

/***********************************************************************************************
 * CDControlClass::Unlock_Volume_95 -- Unlocks removable media so that it can be ejected       *
 *                                                                                             *
 * INPUT:    Handle to VWIN32                                                                  *
 *           drive to unlock (DOS format)                                                      *
 *                                                                                             *
 * OUTPUT:   true if unlocked                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:19AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Unlock_Volume_95 (HANDLE vwin32, char drive)
{
	assert (WinVersion.Is_Win9x());
	DIOC_REGISTERS regs = {0};
	PARAMBLOCK     unlock_params = {0};
	bool  result;
	DWORD cb;

	/*
	**	First, check the lock status. This way, we'll know the number of pending locks we must unlock.
	*/
	unlock_params.bOperation = 2;   // return lock/unlock status

	regs.reg_EAX = 0x440D;
	regs.reg_EBX = drive;
	regs.reg_ECX = MAKEWORD(0x48, 0x08);
	regs.reg_EDX = (DWORD)&unlock_params;

	result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;

	if (result) {

		/*
		**	DeviceIoControl succeeded. Now see if the unlock succeeded. It
		**	succeeded if the carry flag is not set, or if the carry flag is
		**	set but EAX is 0x01 or 0xB0.
		**
		**	It failed if the carry flag is set and EAX is not 0x01 or 0xB0.
		**
		**	If the carry flag is clear, then unlock succeeded. However, you
		**	don't need to set fResult because it is already TRUE when you get
		**	in here.
		*/
		if (regs.reg_Flags & CARRY_FLAG) {
			result = (regs.reg_EAX == 0xB0) || (regs.reg_EAX == 0x01);
		}

	} else {
		DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}

	if (!result) return(false);

	DebugString("Apparent number of locks to undo = %d\n", unlock_params.bNumLocks);

	if (unlock_params.bNumLocks == 0) {
		unlock_params.bNumLocks = 1;
	}

	/*
	**	Now we have to unlock the media for every time it was locked. This gets us a lock count of
	**	0 and totally unlocked media.
	*/
	for (int i = 0; i < unlock_params.bNumLocks; ++i) {
		unlock_params.bOperation = 1;   // unlock the media
		regs.reg_EAX = 0x440D;
		regs.reg_EBX = drive;
		regs.reg_ECX = MAKEWORD(0x48, 0x08);  // LOCK/UNLOCK
		regs.reg_EDX = (DWORD)&unlock_params;

		result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;
		if (result == false) {
			DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
		}

		/*
		**	See if DeviceIoControl and the lock succeeded
		*/
		result = result && !(regs.reg_Flags & CARRY_FLAG);
		if (!result) break;
	}
	return(result);
}

/***********************************************************************************************
 * CDControlClass::Lock_Volume_95 -- Locks removable media so that it can't be ejected         *
 *                                                                                             *
 * INPUT:    Handle to VWIN32                                                                  *
 *           drive to lock (DOS format)                                                        *
 *                                                                                             *
 * OUTPUT:   true if unlocked                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:19AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Lock_Volume_95 (HANDLE vwin32, char drive)
{
	assert (WinVersion.Is_Win9x());
	DIOC_REGISTERS regs = {0};
	PARAMBLOCK     unlock_params = {0};
	bool  result;
	DWORD cb;

	/*
	**	Bring the lock count down to 0.
	*/
	Unlock_Volume_95(vwin32, drive);

	/*
	**	Increment the lock count.
	*/
	unlock_params.bOperation = 0;   // lock the media
	regs.reg_EAX = 0x440D;
	regs.reg_EBX = drive;
	regs.reg_ECX = MAKEWORD(0x48, 0x08);  // LOCK/UNLOCK
	regs.reg_EDX = (DWORD)&unlock_params;

	result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;
	if (result == false) {
		DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}

	/*
	**	See if DeviceIoControl and the lock succeeded
	*/
	result = result && !(regs.reg_Flags & CARRY_FLAG);
	return(result);
}

/***********************************************************************************************
 * CDControlClass::Auto_Eject_Volume_95 -- Eject the specified CD drive                        *
 *                                                                                             *
 * INPUT:    Handle to VWIN32                                                                  *
 *           Drive number (DOS format)                                                         *
 *                                                                                             *
 * OUTPUT:   True if ejected                                                                   *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:24AM ST : Created                                                               *
 *=============================================================================================*/
bool CDControlClass::Auto_Eject_Volume_95 (HANDLE vwin32, char drive)
{
	assert (WinVersion.Is_Win9x());
	DIOC_REGISTERS regs = {0};
	bool  result;
	DWORD cb;

	regs.reg_EAX = 0x440D;
	regs.reg_EBX = drive;
	regs.reg_ECX = MAKEWORD(0x49, 0x08);		//EJECT

	result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;
	if (result == false) {
		DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
	}

	/*
	**	See if we ejected OK
	*/
	result = result && !(regs.reg_Flags & CARRY_FLAG);

	return(result);
}

/***********************************************************************************************
 * CDControlClass::Open_VWin32 -- Opens a handle to VWIN32 to issue low-level disk I/O         *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   HANDLE to VWin32                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:26AM ST : Created                                                               *
 *=============================================================================================*/
HANDLE WINAPI CDControlClass::Open_VWin32 (void)
{
	assert (WinVersion.Is_Win9x());
	HANDLE result = CreateFile ( TEXT("\\\\.\\vwin32"), 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
	assert (result != INVALID_HANDLE_VALUE);
	return(result);
}

/***********************************************************************************************
 * CDControlClass::Close_VWin32 -- Closes the handle opened by Open_VWin32.                    *
 *                                                                                             *
 * INPUT:    Handle to VWin32                                                                  *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:26AM ST : Created                                                               *
 *=============================================================================================*/
bool WINAPI CDControlClass::Close_VWin32 (HANDLE vwin32)
{
	assert (WinVersion.Is_Win9x());
	return((CloseHandle (vwin32)) ? true : false);
}

/***********************************************************************************************
 * CDControlClass::Lock_Logical_Volume -- Take a lock on a logical volume                      *
 *                                                                                             *
 * INPUT:    Handle to VWin32                                                                  *
 *           drive number (DOS format)                                                         *
 *           lock level                                                                        *
 *           permissions                                                                       *
 *                                                                                             *
 * OUTPUT:   true if locked                                                                    *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:33AM ST : Created                                                               *
 *=============================================================================================*/
bool WINAPI CDControlClass::Lock_Logical_Volume (HANDLE vwin32, char drive, char lock_level, short permissions)
{
	assert (WinVersion.Is_Win9x());
	bool           result;
	DIOC_REGISTERS regs = {0};
	char           device_cat;  // can be either 0x48 or 0x08
	DWORD  cb;

	/*
	**	lock_level
	**    Can be 0, 1, 2, or 3. Level 0 is an exclusive lock that can only
	**    be taken when there are no open files on the specified drive.
	**    Levels 1 through 3 form a hierarchy where 1 must be taken before
	**    2, which must be taken before 3.
	**
	**	permissions
	**    Specifies how the lock will affect file operations when lock levels
	**    1 through 3 are taken. Also specifies whether a formatting lock
	**    should be taken after a level 0 lock.
	*/

	int retry_count = 5;
	while (retry_count > 0) {
		/*
		**	Try first with device category 0x48 for FAT32 volumes. If it
		**	doesn't work, try again with device category 0x08. If that
		**	doesn't work, then the lock failed.
		*/
		device_cat = 0;

		do {
			if (device_cat == 0) {
				device_cat = 0x48;
			}else{
				device_cat = 0x08;
			}

			/*
			**	Set up the parameters for the call.
			*/
			regs.reg_EAX = 0x440D;
			regs.reg_EBX = MAKEWORD(drive, lock_level);
			regs.reg_ECX = MAKEWORD(0x4A, device_cat);
			regs.reg_EDX = permissions;

			result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;
			if (result == false) {
				DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
			} else {
				result = result && !(regs.reg_Flags & CARRY_FLAG);
				if (result != false) {
					return(result);
				}
			}

		} while (result == false && device_cat != 0x08);

		retry_count--;

		switch (regs.reg_EAX) {
			case 1:
				DebugString("LOCK return eax error 'Function not supported'\n");
				break;
			case 0xB0:
				DebugString("LOCK return eax error 'Volume not locked in drive'\n");
				break;
			case 0xB2:
				DebugString("LOCK return eax error 'Volume not removable'\n");
				break;
			case 0xB4:
				DebugString("LOCK return eax error 'Lock count exceeded'\n");
				break;
			default:
				DebugString("LOCK return eax error %d\n", regs.reg_EAX);
				break;
		}

		Sleep(LOCK_TIMEOUT * 2);

	}

	return(result);
}

/***********************************************************************************************
 * CDControlClass::Unlock_Logical_Volume -- Unlocks a logical volume                           *
 *                                                                                             *
 * INPUT:    Handle to VWin32                                                                  *
 *           drive number (DOS format)                                                         *
 *                                                                                             *
 * OUTPUT:   true if unlocked                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 *  Must be called the same number of times as LockLogicalVolume() to                          *
 *  completely unlock a volume.                                                                *
 *                                                                                             *
 *  Only the lock owner can unlock a volume.                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/17/99 0:39AM ST : Created                                                               *
 *=============================================================================================*/
bool WINAPI CDControlClass::Unlock_Logical_Volume (HANDLE vwin32, char drive)
{
	assert (WinVersion.Is_Win9x());
	bool           result;
	DIOC_REGISTERS regs = {0};
	char           device_cat;  // can be either 0x48 or 0x08
	DWORD  cb;

	/*
	**	Try first with device category 0x48 for FAT32 volumes. If it
	**	doesn't work, try again with device category 0x08. If that
	**	doesn't work, then the unlock failed.
	*/
	device_cat = 0;

	do {
		if (device_cat == 0) {
			device_cat = 0x48;
		}else{
			device_cat = 0x08;
		}

		/*
		**	Set up the parameters for the call.
		*/
		regs.reg_EAX = 0x440D;
		regs.reg_EBX = drive;
		regs.reg_ECX = MAKEWORD(0x6A, device_cat);

		result = (DeviceIoControl (vwin32, VWIN32_DIOC_DOS_IOCTL, &regs, sizeof(regs), &regs, sizeof(regs), &cb, 0)) ? true : false;
		if (result == false) {
			DebugString ("DeviceIoControl failed to perform DOS IO control function. Error %d - %s\n", GetLastError(), Last_Error_Text());
		}
		result = result && !(regs.reg_Flags & CARRY_FLAG);

	} while (result == false && device_cat != 0x08);

	return(result);
}
