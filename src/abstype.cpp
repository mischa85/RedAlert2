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

#include "abstype.h"
#include "stringtable.h" // RA2

#include "ccini.h"
#include "crc.h"
#include "globals.h"
#include "vector.h"

#include <cstdio>


/***********************************************************************************************
 * AbstractTypeClass::AbstractTypeClass -- Constructor for abstract type objects.              *
 *                                                                                             *
 *    This is the constructor for AbstractTypeClass objects. It initializes the INI name and   *
 *    the text name for this object type.                                                      *
 *                                                                                             *
 * INPUT:   name  -- Text number for the full name of the object.                              *
 *                                                                                             *
 *          ini   -- The ini name for this object type.                                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/22/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
AbstractTypeClass::AbstractTypeClass(char const * ininame) :
	BASECLASS(),
	IniName(),
	GivenName()
{
	if (ininame == NULL) {
		char pstr[16];
		sprintf(pstr, "%08X", (unsigned int)this);
		IniName = TStringID<24>(pstr);
	} else {
		IniName = TStringID<24>(ininame);
	}

	GivenName = TStringID<48>(IniName);
	UIName = L""; // RA2: the display name, empty until Read_INI
	AbstractTypes.Add(this);
}


/***********************************************************************************************
 * AbstractTypeClass::AbstractTypeClass -- Constructor for abstract type objects.              *
 *                                                                                             *
 *    This is the constructor for AbstractTypeClass objects. It initializes the INI name and   *
 *    the text name for this object type.                                                      *
 *                                                                                             *
 * INPUT:   name  -- Text number for the full name of the object.                              *
 *                                                                                             *
 *          ini   -- The ini name for this object type.                                        *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/22/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
AbstractTypeClass::AbstractTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	IniName(x),
	GivenName(x)
{
}


/// <summary>
/// Removes this object type from the master type list.
/// Every abstract type adds itself to the AbstractTypes list when it is created, so it
/// must take itself back off again here or the list would be left holding a dead pointer.
/// </summary>
AbstractTypeClass::~AbstractTypeClass(void)
{
	AbstractTypes.Delete(this);
}


/// <summary>
/// RA2
/// </summary>
bool AbstractTypeClass::operator == (char const * string) const
{
	return(stricmp(string, IniName) == 0);
}


/***********************************************************************************************
 * AbstractTypeClass::Read_INI -- Reads the techno type data from the INI database.            *
 *                                                                                             *
 *    Use this routine to fill in the data for this techno type class object from the          *
 *    database specified. Typical use of this is for the rules parsing.                        *
 *                                                                                             *
 * INPUT:   ini   -- Reference to the INI database that the information will be lifted from.   *
 *                                                                                             *
 * OUTPUT:  bool; Was the database used to extract information? A failure (false) response     *
 *                would mean that the database didn't contain a section that applies to this   *
 *                techno class object.                                                         *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/19/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
bool AbstractTypeClass::Read_INI(CCINIClass const & ini)
{
	char buffer[1024]; // RA2: 1024

	if (ini.Section_Present(IniName)) {

		ini.Get_String(IniName, "Name", GivenName, buffer, GivenName.Size());
		GivenName = TStringID<48>(buffer);
		ini.Get_String(IniName, "UIName", UINameLabel, buffer, UINameLabel.Size()); // RA2: the display name's string-table label
		UINameLabel = TStringID<31>(buffer); // RA2: as above
		if (UINameLabel[0] != '\0') { // RA2: as above
			UIName = StringTable::LoadString(UINameLabel, NULL, "D:\\Ra2\\AbsType.cpp", 205); // RA2: as above
		} else { // RA2: as above
			UIName = L""; // RA2: as above
		}
		return(true);
	}
	return(false);
}


/// <summary>
/// Writes this object type's data out to the INI database.
/// This routine is the counterpart to Read_INI. The type's own section is cleared before
/// anything is written, so no stale entries survive from an earlier save.
/// </summary>
bool AbstractTypeClass::Write_INI(CCINIClass & ini) const // RA2: Write_INI at 0x410910 matches only so
{
	ini.Clear(IniName);
	ini.Put_String(IniName, "Name", GivenName);
	if (UINameLabel[0]) { // RA2: as above
		ini.Put_String(IniName, "UIName", UINameLabel); // RA2: as above
	}
	return(true);
}


/// <summary>
/// Adds this object type's identity to a CRC calculation.
/// This routine is called while the game checksum is being built so that both the INI
/// name and the displayed name of the type contribute to the result.
/// </summary>
void AbstractTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc((const char *)IniName);
	crc((const char *)GivenName);
	crc((const char *)UINameLabel); // RA2: Compute_CRC at 0x410960 matches only so
}




