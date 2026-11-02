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
 *                 Project Name : Command & Conquer/ WW Library                                *
 *                                                                                             *
 *                    File Name : MODEMREG.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 10/18/96                                                     *
 *                                                                                             *
 *                  Last Update : October 18th 1996 [ST]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *                                                                                             *
 *   Functions for obtaining modem infommation from the Win95 registry                         *
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *                                                                                             *
 * Search_Registry_Key -- Search a registry key and all its subkeys for a given value          *
 * MREC::ModemRegistryEntryClass -- Constructor for ModemRegistryEntryClass                    *
 * MREC::~ModemRegistryEntryClass -- Destructor.Free all the memory we allocated for modem info*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "modemreg.h"

#include "_winfix.h"
#include "dbgprint.h"

#include <cstdio>


extern HKEY Get_Registry_Sub_Key (HKEY base_key, char const *search_key, bool close);


/***********************************************************************************************
 * Search_Registry_Key -- Search a registry key and all its subkeys for a given value          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    handle to key to search in                                                        *
 *           name of key to search for                                                         *
 *           value expected in key                                                             *
 *                                                                                             *
 * OUTPUT:   Handle to key containing value. Null if not found.                                *
 *                                                                                             *
 * WARNINGS: This function reenters itself.                                                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/18/96 4:01AM ST : Created                                                             *
 *=============================================================================================*/
HKEY Search_Registry_Key (HKEY key_in, char const *value_name, char const *search_string)
{

	int 	top_key_index = 0;  // Index of topmost key
	int 	retval;             // Result of registry api calls
	HKEY	next_key;           // handle of next key examine
	HKEY	next_search;        // handle of next key to search


	char				*subkey_name = new char [256];      // Area to contain result of key enumeration
	DWORD	subkey_name_length = 256;               // Length of enumeration result area
	FILETIME			filetime;                           // Time key was last touched. Not used.
	DWORD	value_type;                             // Type of data that is contained in a key.
	unsigned char	*key_value = new unsigned char [256];   // Area to return key values into
	DWORD	key_value_length = 256;                 // Length of key value area

	/*
	**	Scan through and enumerate all subkeys of this key. Exit the loop when there are
	**	no more sub keys to enumerate.
	*/
	do {
		subkey_name_length = 256;			// Has to be set each time through the loop

		/*
		**	Get the next key
		*/
		retval = RegEnumKeyEx (key_in, top_key_index++, subkey_name, &subkey_name_length, NULL, NULL, NULL, &filetime);

		if ( retval == ERROR_SUCCESS ){

			/*
			**	Get a handle to this key so we can search it.
			*/
			next_key = Get_Registry_Sub_Key (key_in, subkey_name, FALSE);

			if (next_key){

				key_value_length = 256;		// Has to be set each time through the loop

				if ( RegQueryValueEx (next_key, value_name, NULL, &value_type, key_value, &key_value_length) == ERROR_SUCCESS){

					/*
					**	If this value is type string then do a compare with the value we are looking for
					*/
					if (value_type == REG_SZ && !strcmp ((char*)key_value, search_string)){
						/*
						**	This is our man. Delete our workspace and return the key handle
						*/
						delete [] subkey_name;
						delete [] key_value;
						return(next_key);
					}
				}

				/*
				**	We didnt find our search value so search this key for more sub keys by reentering
				**	this function with the handle of the subkey.
				*/
				next_search = Search_Registry_Key (next_key, value_name, search_string);
				RegCloseKey (next_key);

				/*
				**	If the value was found in a subkey then just return with the key handle.
				*/
				if (next_search){
					delete [] subkey_name;
					delete [] key_value;
					return(next_search);
				}
			}

		}
	} while (retval == ERROR_SUCCESS);

	/*
	**	Clean up and exit.
	*/
	delete [] subkey_name;
	delete [] key_value;

	return(0);
}


/***********************************************************************************************
 * MREC::ModemRegistryEntryClass -- Constructor for ModemRegistryEntryClass                    *
 *                                                                                             *
 *  This function does all the work in the class. All the registry searching is done here      *
 *                                                                                             *
 * INPUT:    Modem number                                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/18/96 4:12AM ST : Created                                                             *
 *=============================================================================================*/
ModemRegistryEntryClass::ModemRegistryEntryClass (int modem_number)
{
	HKEY			key;
	unsigned char	return_buf[256];
	DWORD			retbuf_size = sizeof(return_buf);

	int				pnp = 0;					//Not a plug n pray modem

	/*
	**	Initialise all the info we expect from the registry to NULL.
	**	Any entries we cant find will just stay NULL.
	*/
	ModemName = NULL;
	ModemDeviceName = NULL;
	ErrorCorrectionEnable = NULL;
	ErrorCorrectionDisable = NULL;
	CompressionEnable = NULL;
	CompressionDisable = NULL;
	HardwareFlowControl = NULL;


	/*
	**	Modem info is stored under
	**	HKEY_LOCAL_MACHINE / System / CurrentControlSet / Services / Class / Modem / nnnn
	**	where nnnn is a four digit modem number.
	*/

	char which_modem[5];
	wsprintf (which_modem, "%04d", modem_number);

	/*
	**	Get a handle to the modem key if it exists. Then extract the info we need.
	*/
	if (WinVersion.Is_Win9x()) {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\Modem", 0, KEY_READ, &key) != ERROR_SUCCESS) {
			return;
		}
		if (RegOpenKeyEx(key, which_modem, REG_OPTION_NON_VOLATILE, KEY_READ, &key) != ERROR_SUCCESS) {
			DebugString("Unable to find System\\CurrentControlSet\\Services\\Class\\Modem in registry\n");
			return;
		}
	} else {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class", 0, KEY_READ, &key) != ERROR_SUCCESS) {
			DebugString("Unable to find System\\CurrentControlSet\\Control\\Class in registry\n");
			return;
		}

		char registry_path[256];
		char key_name_buf[256];
		char key_class_buf[256];

		int key_index = 0;
		int key_name_buf_size = sizeof (key_name_buf);
		int key_class_buf_size = sizeof (key_class_buf);

		HKEY modem_key;
		FILETIME key_time;
		bool found = false;

		while (RegEnumKeyEx(key, key_index, key_name_buf, (LPDWORD)&key_name_buf_size, 0, key_class_buf, (LPDWORD)&key_class_buf_size, &key_time) == ERROR_SUCCESS) {
			strcpy(registry_path, "System\\CurrentControlSet\\Control\\Class\\");
			strcat(registry_path, key_name_buf);
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0, KEY_READ, &modem_key) == ERROR_SUCCESS) {
				retbuf_size = sizeof (return_buf);
				if (RegQueryValueEx(modem_key, "Class", 0, 0, return_buf, &retbuf_size) == ERROR_SUCCESS && stricmp((char *)return_buf, "Modem") == 0) {
					found = true;
					break;
				}
			}
			key_index++;
			key_name_buf_size = sizeof (key_name_buf);
			key_class_buf_size = sizeof (key_class_buf);
		}

		if (!found) return;

		strcpy(registry_path, "System\\CurrentControlSet\\Control\\Class\\");
		strcat(registry_path, key_name_buf);
		strcat(registry_path, "\\");
		strcat(registry_path, which_modem);
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registry_path, 0, KEY_READ, &key) != ERROR_SUCCESS) {
			return;
		}
	}

	/*
	**	Get the name of the modem. This is what will be displayed in the modem list presented
	**	to the user.
	*/
	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "Model", NULL, NULL, return_buf, &retbuf_size) != ERROR_SUCCESS){
		RegCloseKey (key);
		return;
	}
	ModemName = new char [retbuf_size+1];
	memcpy (ModemName, return_buf, retbuf_size);
	DebugString("Modem name is %s\n", ModemName);

	/*
	**	Find out what COM port the modem is attached to. If this info isnt here, then its a
	**	Plug n Pray modem. Set the flag so we know to do the pnp search later.
	*/
	retbuf_size = sizeof (return_buf);
	bool attached_pnp = false;
	if (RegQueryValueEx(key, "AttachedTo", NULL, NULL, return_buf, &retbuf_size) != ERROR_SUCCESS) {
		retbuf_size = sizeof (return_buf);
		if (WinVersion.Is_Win9x() || RegQueryValueEx(key, "PnPAttachedTo", NULL, NULL, return_buf, &retbuf_size) != ERROR_SUCCESS) {
			attached_pnp = true;
		}
	}
	if (attached_pnp) {
		/*
		**	Must be a plug n pray modem. Set the flag. We will look for the port later.
		*/
		pnp = 1;
		ModemDeviceName = new char [strlen (ModemName)+1];
		strcpy (ModemDeviceName, ModemName);
	} else {
		ModemDeviceName = new char [retbuf_size+1];
		memcpy (ModemDeviceName, return_buf, retbuf_size);
	}
	DebugString("Modem attached to %s\n", ModemDeviceName);


	/*
	**	The list of modem 'AT' commands is stored in the 'Settings'  key.
	*/
	key = Get_Registry_Sub_Key (key, "Settings", TRUE);
	if (!key) return;


	/*
	**	Extract the control strings for error control.
	*/
	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "ErrorControl_On", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){
		ErrorCorrectionEnable = new char [retbuf_size+1];
		memcpy (ErrorCorrectionEnable, return_buf, retbuf_size);
		DebugString("Modem error control enable string: %s\n", ErrorCorrectionEnable);
	}

	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "ErrorControl_Off", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){
		ErrorCorrectionDisable = new char [retbuf_size+1];
		memcpy (ErrorCorrectionDisable, return_buf, retbuf_size);
		DebugString("Modem error control disable string: %s\n", ErrorCorrectionDisable);
	}

	/*
	**	Extract the control strings for data compression.
	*/
	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "Compression_On", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){
		CompressionEnable = new char [retbuf_size+1];
		memcpy (CompressionEnable, return_buf, retbuf_size);
		DebugString("Modem compression enable string: %s\n", CompressionEnable);
	}

	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "Compression_Off", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){
		CompressionDisable = new char [retbuf_size+1];
		memcpy (CompressionDisable, return_buf, retbuf_size);
		DebugString("Modem compression disable string: %s\n", CompressionDisable);
	}

	/*
	**	Extract the control strings for hardware flow control.
	*/
	retbuf_size = sizeof (return_buf);
	if (RegQueryValueEx(key, "FlowControl_Hard", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){
		HardwareFlowControl = new char [retbuf_size+1];
		memcpy (HardwareFlowControl, return_buf, retbuf_size);
		DebugString("Modem hardware flow control enable string: %s\n", HardwareFlowControl);
	}

	RegCloseKey (key);

	/*
	**	If this is a plug n pray modem then we need to search for the COM port it is
	**	attached to.
	*/
	if (pnp){

		/*
		**	The driver name in the HKEY_LOCAL_MACHINE / Enum section will be Modem\nnnn where nnnn
		**	is a four digit modem number.
		*/
		char search_string [256] = {"Modem\\"};
		strcat (search_string, which_modem);

		/*
		**	Search through all the registry entries under HKEY_LOCAL_MACHINE / Enum
		*/
		key = Get_Registry_Sub_Key (HKEY_LOCAL_MACHINE, "Enum", FALSE);
		if (!key) return;

		HKEY newkey = Search_Registry_Key ( key, "Driver", search_string );

		if (newkey){
			retbuf_size = sizeof (return_buf);

			/*
			**	Extract the PORTNAME value. This is the name of the port to use to communicate
			**	with the modem.
			*/
			retbuf_size = sizeof (return_buf);
			if (RegQueryValueEx(newkey, "PORTNAME", NULL, NULL, return_buf, &retbuf_size) == ERROR_SUCCESS){

				if (ModemDeviceName) delete [] ModemDeviceName;

				ModemDeviceName = new char [retbuf_size+1];
				memcpy (ModemDeviceName, return_buf, retbuf_size);
				DebugString("PNP Modem attached to %s\n", ModemName);
			}
		} else {
			if (ModemDeviceName != NULL && ModemName != NULL) {
				if (!strcmp (ModemName, ModemDeviceName)) {
					DebugString("Failed to find port for device %s\n", ModemName);
					delete [] ModemName;
					ModemName = NULL;
					delete [] ModemDeviceName;
					ModemDeviceName = NULL;
				}
			}
		}
		RegCloseKey (key);
	}
}


/***********************************************************************************************
 * MREC::~ModemRegistryEntryClass -- Destructor.Free all the memory we allocated for modem info*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/18/96 11:39AM ST : Created                                                            *
 *=============================================================================================*/
ModemRegistryEntryClass::~ModemRegistryEntryClass (void)
{
	Clear();
}


/// <summary>
/// Frees the modem description strings held by this entry.
/// This routine releases every string that was fetched out of the registry for the modem and
/// leaves the entry empty. The destructor uses this routine to do its cleaning up.
/// </summary>
void ModemRegistryEntryClass::Clear (void)
{
	if (ModemName) {
		delete [] ModemName;
		ModemName = NULL;
	}
	if (ModemDeviceName) {
		delete [] ModemDeviceName;
		ModemDeviceName = NULL;
	}

	if (ErrorCorrectionEnable) {
		delete [] ErrorCorrectionEnable;
		ErrorCorrectionEnable = NULL;
	}
	if (ErrorCorrectionDisable) {
		delete [] ErrorCorrectionDisable;
		ErrorCorrectionDisable = NULL;
	}

	if (CompressionEnable) {
		delete [] CompressionEnable;
		CompressionEnable = NULL;
	}
	if (CompressionDisable) {
		delete [] CompressionDisable;
		CompressionDisable = NULL;
	}

	if (HardwareFlowControl) {
		delete [] HardwareFlowControl;
		HardwareFlowControl = NULL;
	}
}
