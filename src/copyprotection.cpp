// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).

#define INCLUDE_COM
#include "always.h"
#include "globals.h"
#include "version.h"
#include "dbgprint.h"

#include <mmsystem.h>

namespace CopyProtection {

static char *LauncherMessage;
static const UINT LauncherMessageID = 0xBEEF;

bool __fastcall CheckVersion(void);

// CopyProtection::IsLauncherRunning
bool IsLauncherRunning(void)
{
	DebugString("COPYPROTECTION - Checking if launcher is running\n");
	HANDLE mutex = CreateMutex(NULL, FALSE, "48BC11BD-C4D7-466b-8A31-C6ABBAD47B3E");
	bool running = GetLastError() == ERROR_ALREADY_EXISTS;
	if (mutex) {
		CloseHandle(mutex);
	}
	return running;
}

// CopyProtection::NotifyLauncher
bool NotifyLauncher(void)
{
	MSG msg;
	DebugString("COPYPROTECTION - Notify launcher\n");
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	HANDLE event = OpenEvent(EVENT_MODIFY_STATE, TRUE, "D6E7FC97-64F9-4d28-B52C-754EDF721C6F");
	if (event) {
		SetEvent(event);
		CloseHandle(event);
		DebugString("Launcher notified.\n");
		return true;
	}
	DebugString("***** Failed to notify launcher!\n");
	return false;
}

// CopyProtection::DispatchLauncherMessage
void __fastcall DispatchLauncherMessage(MSG *msg)
{
	if (msg->message == LauncherMessageID) {
		DebugString("COPYPROTECTION - Received message from launcher.\n");
		LauncherMessage = (char *)MapViewOfFileEx((HANDLE)msg->lParam, FILE_MAP_ALL_ACCESS, 0, 0, 0, NULL);
		if (!LauncherMessage) {
			DebugString("***** MapViewOfFileEx() Failed!");
		} else {
			DebugString("The message says: %s\n", LauncherMessage);
		}
	}
}

// CopyProtection::CheckProtectedData
bool CheckProtectedData(void)
{
	DebugString("COPYPROTECTION - Validating\n");
	if (LauncherMessage) {
		return strcmp(LauncherMessage, "(c) 2000 Electronic Arts, Inc. All Rights Reserved") == 0;
	}
	return CheckVersion();
}

// CopyProtection::ShutDown
void ShutDown(void)
{
	DebugString("COPYPROTECTION - Shutdown\n");
	if (LauncherMessage) {
		UnmapViewOfFile(LauncherMessage);
		LauncherMessage = NULL;
	}
	HKEY key;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "WChat\\SysID", 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS) {
		RegDeleteValue(key, "ID");
		RegCloseKey(key);
	}
}

// CopyProtection::CheckVersion
bool __fastcall CheckVersion(void)
{
	HKEY key;
	DWORD size;
	DWORD type;
	DWORD value;

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "WChat\\SysID", 0, KEY_READ, &key) != ERROR_SUCCESS) {
		return(false);
	}
	size = 4;
	LONG status = RegQueryValueEx(key, "ID", NULL, &type, (BYTE *)&value, &size);
	RegCloseKey(key);
	if (status != ERROR_SUCCESS || type != REG_DWORD) {
		return(false);
	}
	return(value == VerNum.Version_Number());
}

}
