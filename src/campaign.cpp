/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#define INCLUDE_COM
#include "always.h"

#include "campaign.h"

#include "assert.h"
#include "ccini.h"
#include "crc.h"
#include "globals.h"
#include "noinit.h"
#include "sun.h"
#include "tracker.h"
#include "vector.h"


/*
**	RA2: copies narrow text into a wide string, converting each character (limit -1: no limit).
*/
void Char_To_Wide(wchar_t * dest, char const * source, int limit);


/// <summary>
/// Creates a campaign and adds it to the campaign list.
/// The campaign starts out with the display description borrowed from its given
/// name; the rules will normally supply a proper one later.
/// </summary>
/// <param name="name">The name of the campaign as it appears in the rules.</param>
CampaignClass::CampaignClass(char const * name) :
	BASECLASS(name),
	CDNumber(-1),
	FinalMovie(VQ_NONE)
{
	Campaigns.Add(this);

	ScenarioName[0] = '\0';
	wcscpy(Description, L"");
}


/// <summary>
/// Destroys this campaign and removes it from the game.
/// Any object still pointing at this campaign is detached from it before the
/// campaign leaves the master campaign list.
/// </summary>
CampaignClass::~CampaignClass(void)
{
	Detach_This_From_All(this, true);
	Campaigns.Delete(this);
}


/// <summary>
/// Converts a campaign name into a campaign identifier.
/// </summary>
/// <param name="name">The campaign name to search for. Case is ignored.</param>
/// <returns>Returns with the campaign found, or CAMPAIGN_NONE if there is no
/// such campaign.</returns>
CampaignType CampaignClass::From_Name(char const * name)
{
	for (int index = 0; index < Campaigns.Count(); index++) {
		CampaignClass * ptr = Campaigns[index];
		if (stricmp(ptr->IniName, name) == 0) {
			return(CampaignType(index));
		}
	}
	return(CAMPAIGN_NONE);
}


/// <summary>
/// Fetches this campaign's settings from the rules.
/// This routine picks up the disc the campaign lives on, the scenario it starts
/// with, the movie that closes it out, and the expansion it requires.
/// </summary>
/// <param name="ini">The rules database to read the campaign settings from.</param>
/// <returns>bool; Was the campaign found and read?</returns>
bool CampaignClass::Read_INI(CCINIClass const & ini)
{
	if (BASECLASS::Read_INI(ini)) {
		CDNumber = ini.Get_Int(IniName, "CD", CDNumber);
		FinalMovie = ini.Get_VQType(IniName, "FinalMovie", FinalMovie);
		ini.Get_String(IniName, "Scenario", ScenarioName, ScenarioName, sizeof(ScenarioName));
		strupr(ScenarioName);
		if (!ini.Get_Bool(IniName, "DebugOnly", false)) {
			ini.ReadStringtableEntry(IniName, "Description", Description, sizeof(Description) / sizeof(Description[0]));
		} else {
			char buffer[1024];
			ini.Get_String(IniName, "Description", NULL, buffer, sizeof(buffer));
			strcat(buffer, " - for debug/testing");
			Char_To_Wide(Description, buffer, -1);
		}
		return(true);
	}
	return(false);
}


/// <summary>
/// Creates the campaign list from the rules.
/// This routine walks the Battles section and gives every campaign named there a
/// CampaignClass object, creating one where none exists yet and then letting each
/// campaign fetch its own settings.
/// </summary>
/// <param name="ini">The rules database to read the campaign list from.</param>
void Read_Battle_INI(CCINIClass const & ini)
{
	char name[32];

	int count = ini.Entry_Count("Battles");

	for (int index = 0; index < count; index++) {

		ini.Get_String("Battles", ini.Get_Entry("Battles", index), "", name, sizeof(name));

		CampaignType ctype = CampaignClass::From_Name(name);
		CampaignClass * campaign;

		if (ctype == CAMPAIGN_NONE) {
			campaign = new CampaignClass(name);
		} else {
			campaign = Campaigns[ctype];
		}

		campaign->Read_INI(ini);
	}
}


/// <summary>
/// Fetches the class identifier of this object.
/// This routine is required of every persistent object so that the save game loader
/// can recognize what to construct when the object is read back.
/// </summary>
/// <param name="retval">Pointer to the identifier to fill in.</param>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE CampaignClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_CampaignClass;
	return(S_OK);
}


/// <summary>
/// Submits this campaign to the CRC engine.
/// This routine is used by the multiplayer sync check to prove that every machine
/// is playing with the same campaign data.
/// </summary>
void CampaignClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);

	crc(CDNumber);
	crc(ScenarioName);
	crc(FinalMovie);
}


/// <summary>
/// Reads this campaign back in from a save game stream.
/// The object is reconstructed in place so that its virtual table pointer is valid
/// again before the game resumes using it.
/// </summary>
/// <returns>Returns with S_OK if the campaign was read, otherwise the failure code.</returns>
HRESULT STDMETHODCALLTYPE CampaignClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) CampaignClass(NoInitClass());

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Writes this campaign out to a save game stream.
/// </summary>
/// <param name="cleardirty">Should the dirty flag be cleared once written?</param>
/// <returns>Returns with S_OK if the campaign was written, otherwise the failure code.</returns>
HRESULT STDMETHODCALLTYPE CampaignClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Fetches the size of this object as it appears in a save file.
/// This routine is used by the persistence code to reserve room for the campaign
/// when it is written to, or read back from, a save game stream.
/// </summary>
/// <returns>Returns with the byte size of this object.</returns>
int CampaignClass::Fetch_Object_Size(void) const
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the RTTI identifier of this object.
/// </summary>
/// <returns>Returns with RTTI_CAMPAIGN.</returns>
RTTIType CampaignClass::Fetch_RTTI(void) const
{
	return(RTTI_CAMPAIGN);
}
