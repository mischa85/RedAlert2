/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "mschoice.h"

#include "_mixfile.h"
#include "addon.h"
#include "ccfile.h"
#include "ccini.h"
#include "data.h"
#include "dbgprint.h"
#include "dsaudio.h"
#include "globals.h"
#include "goptions.h"
#include "mixfile.h"
#include "wstring.h"
#include "stringtable.h"

int __fastcall AudioDeviceAvailable(void); // RA2: whether the audio library has a device

#define RA2_SOURCE "D:\\Ra2\\MSChoice.cpp"

/// <summary>
/// Creates an empty map choice database.
/// The stage, animation and sound lists start out empty. Call Initialize to fill them
/// in from the map selection control file.
/// </summary>
MapChoice::MapChoice(void)
{
	Stages.Clear();
	AnimPaletteName.clear(); // RA2
	AnimEntries.Clear();
}


/// <summary>
/// Destroys the map choice database.
/// Everything the database loaded is released by way of Deinit.
/// </summary>
MapChoice::~MapChoice(void)
{
	Deinit();
}


/// <summary>
/// Loads the map selection data for the specified house.
/// This routine reads the map selection control file belonging to whichever addon is
/// required and builds the stages, background animations and sound effects that the
/// map selection screen presents. Any previously loaded data is discarded first.
/// </summary>
/// <param name="house_name">Name of the house whose campaign is being presented.</param>
/// <returns>bool; Was the map selection data loaded?</returns>
bool MapChoice::Initialize(Wstring const & side_name) // RA2
{
	CCFileClass file;
	INIClass ini;
	Wstring section; // RA2
	Wstring buffer;
	int index;

	Deinit();

	file.Open("MAPSEL.INI"); // RA2

	if (!ini.Load(file)) {
		DebugString("Failed to load MAPSEL.INI\n");
		return(false);
	}

	for (index = 1; index <= 100; index++) {

		if (!ini.Get_String(side_name, Wstring_FromInt(index), NULL, buffer)) { // RA2
			break;
		}

		MapStage * stage = new MapStage(ini, buffer);
		if (stage == NULL) {
			DebugString("MapSelect: Failed to create stage %s\n", buffer);
			return(false);
		}

		Stages.Add(stage);
	}

	if (Stages.Count() == 0) {
		DebugString("MapSelect: There isn't any stages!\n");
		return(false);
	}

	if (ini.Get_String(side_name, "Anims", NULL, section) > 0) { // RA2

		if (ini.Get_String(section, "TextRect", NULL, buffer) > 0) { // RA2

			Wstring token; // RA2
			int pos = buffer.getToken(0, ",", token);
			if (token.length() > 0) {
				TextRect.X = token.toInt();
			}

			pos = buffer.getToken(pos, ",", token); // RA2
			if (token.length() > 0) {
				TextRect.Y = token.toInt();
			}

			pos = buffer.getToken(pos, ",", token); // RA2
			if (token.length() > 0) {
				TextRect.Width = token.toInt();
			}

			buffer.getToken(pos, ",", token); // RA2
			if (token.length() > 0) {
				TextRect.Height = token.toInt();
			}
		}

		if (ini.Get_String(section, "Palette", NULL, buffer) > 0) { // RA2
			AnimPaletteName = buffer;
		}

		for (index = 1; index <= 100; index++) {

			if (!ini.Get_String(section, Wstring_FromInt(index), NULL, buffer)) { // RA2
				break;
			}

			MSAnimEntry * anim = new MSAnimEntry(buffer);
			if (anim == NULL) {
				DebugString("Failed to create anim %s\n", buffer);
				return(false);
			}

			AnimEntries.Add(anim);
		}
	}

	if (AudioDeviceAvailable()) { // RA2
		Wstring entry;

		if (ini.Get_String(side_name, "Sounds", NULL, section) > 0) {
			for (index = 0; index < ini.Entry_Count(section); index++) {
				entry = ini.Get_Entry(section, index); // RA2

				if (entry != (char const *)NULL && ini.Get_String(section, entry, NULL, buffer) > 0) { // RA2

					MSSfxEntry * sfx = new MSSfxEntry(entry, buffer);

					if (sfx->Get_Name() == (char const *)NULL) { // RA2
						delete sfx;
					} else {
						Add_Sound(sfx);
					}
				}
			}
		}
	}

	return(true);
}


/// <summary>
/// Discards the loaded map selection data.
/// This routine releases the stages, animations and sound effects and puts the
/// database back into the empty state it was constructed in. Initialize calls it
/// first, so it is safe to call more than once.
/// </summary>
void MapChoice::Deinit(void)
{
	int i;

	for (i = 0; i < SoundEntries.Count(); i++) {
		MSSfxEntry * sfx = SoundEntries[i];
		if (sfx != NULL) {
			delete sfx;
		}
	}
	SoundEntries.Clear();
	TextRect.Set(0, 0, 640, 400); // RA2
	for (i = 0; i < AnimEntries.Count(); i++) {
		MSAnimEntry * anim = AnimEntries[i];
		if (anim != NULL) {
			delete anim;
		}
	}
	AnimEntries.Clear();
	for (i = 0; i < Stages.Count(); i++) {
		MapStage * stage = Stages[i];
		if (stage != NULL) {
			delete stage;
		}
	}
	Stages.Clear();
}


/// <summary>
/// Fetches the stage with the specified label.
/// The label is the control file section name, so the comparison ignores case and a
/// selection may spell its destination however it likes.
/// </summary>
/// <param name="name">Label of the stage to search for.</param>
/// <returns>Returns with a pointer to the stage. Otherwise, NULL is returned.</returns>
MapStage * MapChoice::Find_Stage_By_Name(Wstring const & name) // RA2
{
	if (name != NULL) {
		for (int index = 0; index < Stages.Count(); index++) {
			Wstring wanted = name;
			wanted.toLower();
			Wstring label = Stages[index]->Get_Stage_Label();
			label.toLower();
			if (wanted == label) {
				return(Stages[index]);
			}
		}
	}
	return(NULL);
}


/// <summary>
/// RA2
/// </summary>
MapStage * MapChoice::Find_Stage_By_Scenario(Wstring const & name)
{
	if (name != NULL) {
		for (int index = 0; index < Stages.Count(); index++) {
			Wstring wanted = name;
			wanted.toLower();
			Wstring scenario = Stages[index]->Get_Scenario_Name();
			scenario.toLower();
			if (wanted == scenario) {
				return(Stages[index]);
			}
		}
	}
	return(NULL);
}


/// <summary>
/// Fetches the stage with the specified identifier.
/// This is the counterpart of Get_Stage_ID and is how the scenario record's remembered
/// stage is turned back into something the map selection screen can present.
/// </summary>
/// <returns>Returns with a pointer to the stage. Otherwise, NULL is returned.</returns>
MapStage * MapChoice::Find_Stage_By_ID(unsigned short index)
{
	if (Stages.Count() <= index) {
		return(NULL);
	}
	return(Stages[index]);
}


/// <summary>
/// Fetches the identifier of the specified stage.
/// The scenario record remembers which stage the player reached as a number rather
/// than as a pointer, so this is how a stage is turned into something storable.
/// </summary>
/// <returns>Returns with the stage identifier, or -1 if the stage is not one of ours.</returns>
short MapChoice::Get_Stage_ID(MapStage * stage)
{
	return(Stages.ID(stage));
}


/// <summary>
/// Fetches the sound effect registered under the specified name.
/// The map selection screen asks for its sounds by name rather than by file, so that
/// the control file may re-point them without the screen caring.
/// </summary>
/// <param name="name">Name of the sound effect as the control file spells it.</param>
/// <returns>Returns with a pointer to the sound effect. Otherwise, NULL is returned.</returns>
MSSfxEntry * MapChoice::Find_Sound(Wstring const & name) // RA2
{
	if (!AudioDeviceAvailable()) return(NULL); // RA2
	for (int i = 0; i < SoundEntries.Count(); i++) {
		MSSfxEntry * sfx = SoundEntries[i];
		Wstring have = sfx->Name; // RA2
		have.toLower();
		Wstring wanted = name;
		wanted.toLower();
		if (have == wanted) {
			return(sfx);
		}
	}
	return(NULL);
}


/// <summary>
/// Creates a map selection stage from its control file section.
/// This routine gathers everything the map selection screen needs in order to present
/// one stage -- the scenario it launches, its description and voice over, the movie
/// and overlay artwork, the target markers, and the click map regions the player may
/// pick from.
/// </summary>
/// <param name="ini">The map selection control file to read the stage from.</param>
/// <param name="label">Name of the section that describes this stage.</param>
MapStage::MapStage(INIClass const & ini, Wstring const & label) : // RA2
	Targets(),
	TextEntries(),
	Selections()
{
	Wstring buffer; // RA2
	int index;
	WideWstring text; // RA2
	Wstring key;
	int pos;
	int count;
	Point2D point;

	StageLabel = label; // RA2

	ScenarioName = (char const *)NULL; // RA2
	if (ini.Get_String(label, "Scenario", NULL, buffer) > 0) {
		ScenarioName = buffer;
	}

	Description = (wchar_t const *)NULL;
	if (ini.Get_String(label, "Description", NULL, buffer) > 0) {
		Description = StringTable::LoadString(buffer.get(), NULL, RA2_SOURCE, 482);
	}

	VoiceOver = (char const *)NULL; // RA2
	if (ini.Get_String(label, "VoiceOver", NULL, buffer) > 0) {
		VoiceOver = buffer;
	}

	MapVQName = (char const *)NULL; // RA2
	if (ini.Get_String(label, "MapVQ", NULL, buffer) > 0) {
		MapVQName = buffer;
	}

	OverlayCount = 0;
	for (index = 0; index < 2; index++) { // RA2
		OverlayNames[index] = (char const *)NULL;
	}

	if (ini.Get_String(label, "Overlays", NULL, buffer) > 0) { // RA2
		index = 0;
		Wstring token; // RA2
		pos = buffer.getToken(0, ",", token);
		while (token.length() > 0 && index < 2) {
			OverlayNames[index] = token;
			pos = buffer.getToken(pos, ",", token);
			index++;
		}

		OverlayCount = index;
	}

	ClickMapName = (char const *)NULL; // RA2
	if (ini.Get_String(label, "ClickMap", NULL, buffer) > 0) {
		ClickMapName = buffer;
	}

	for (index = 1; index < 8; index++) {
		if (ini.Get_String(label, "Text" + Wstring_FromInt(index), NULL, buffer) > 0) { // RA2
			text = Wstring_ToWide(buffer);
			MSTextEntry * entry = new MSTextEntry(text);
			if (entry) {
				TextEntries.Add(entry);
			}
		}
	}

	if (ini.Get_String(label, "Targets", NULL, buffer) > 0) { // RA2

		count = 0; // RA2
		Wstring token;
		pos = buffer.getToken(0, ",", token);
		if (token.length() > 0) {
			count = token.toInt();
		}

		for (index = 0; index < count; index++) {
			point.X = -1; // RA2
			point.Y = -1;

			pos = buffer.getToken(pos, ",", token); // RA2
			if (token.length() > 0) {
				point.X = token.toInt();
			}

			pos = buffer.getToken(pos, ",", token); // RA2
			if (token.length() > 0) {
				point.Y = token.toInt();
			}

			Targets.Add(point); // RA2
		}
	}

	for (index = 0; index < 256; index++) { // RA2
		if (ini.Get_String(label, Wstring_FromInt(index), NULL, buffer) > 0) {

			MapSelection * selection = new MapSelection(buffer, index); // RA2
			if (selection) {
				Selections.Add(selection);
			}
		}
	}
}


/// <summary>
/// Destroys the map selection stage.
/// The strings this stage copied out of the control file, along with its text entries
/// and its selections, are all released here.
/// </summary>
MapStage::~MapStage(void)
{
	int index;

	for (index = 0; index < TextEntries.Count(); index++) {
		MSTextEntry * entry = TextEntries[index];
		if (entry != NULL) {
			delete entry;
		}
	}
	for (index = 0; index < Selections.Count(); index++) {
		MapSelection * selection = Selections[index];
		if (selection != NULL) {
			delete selection;
		}
	}
}


/// <summary>
/// Fetches the selection that leads to the named stage.
/// This routine is used to get at the target animation of a click map region once
/// Find_Selection has named the stage that region selects.
/// </summary>
/// <param name="name">Label of the stage the selection leads to.</param>
/// <returns>Returns with a pointer to the selection. Otherwise, NULL is returned.</returns>
MapSelection * MapStage::Find_Selection_By_Name(char const * name) const
{
	Wstring wanted = name; // RA2
	wanted.toLower();
	for (int i = 0; i < Selections.Count(); i++) {
		if (Selections[i]->Get_Stage_Label() != NULL) { // RA2
			Wstring label = Selections[i]->Get_Stage_Label();
			label.toLower();
			if (wanted == label) {
				return(Selections[i]);
			}
		}
	}
	return(NULL);
}


/// <summary>
/// Determines which stage the specified click map region leads to.
/// The map selection screen reads a pixel out of the stage's click map artwork and
/// hands the color here to learn whether the player is pointing at something, and if
/// so, at what.
/// </summary>
/// <param name="index">Click map color of the region the player is pointing at.</param>
/// <returns>Returns with the label of the stage that region selects, or NULL if the region
/// is not selectable.</returns>
Wstring MapStage::Find_Selection_By_Index(int index) const // RA2
{
	for (int i = 0; i < Selections.Count(); i++) {
		if ((char)index == Selections[i]->Get_Index()) {
			return(Selections[i]->Get_Stage_Label());
		}
	}
	return(Wstring((char const *)NULL)); // RA2
}


/// <summary>
/// Creates a stage text entry from its control file line.
/// The line gives the screen position, the moment the text is due to appear, and the
/// text itself.
/// </summary>
/// <param name="string">The control file line to parse; it is chopped up in place.</param>
MSTextEntry::MSTextEntry(WideWstring const & string) : // RA2
	XPos(0),
	YPos(0),
	StartTime(0),
	String((wchar_t const *)NULL) // RA2
{
	WideWstring token; // RA2
	int pos = string.getToken(0, L",", token);
	if (token.length() > 0) {
		XPos = token.toInt();
	}
	pos = string.getToken(pos, L",", token); // RA2
	if (token.length() > 0) {
		YPos = token.toInt();
	}
	pos = string.getToken(pos, L",", token); // RA2
	if (token.length() > 0) {
		StartTime = token.toInt();
	}
	string.getToken(pos, L",", token); // RA2
	if (token.length() > 0) {
		String = token;
	}
}


/// <summary>
/// Creates a map selection animation entry from its control file line.
/// The line names the animation to play and gives the screen position and the rate to
/// play it at.
/// </summary>
/// <param name="string">The control file line to parse; it is chopped up in place.</param>
MSAnimEntry::MSAnimEntry(Wstring const & string) : // RA2
	Filename((char const *)NULL),
	XPos(0),
	YPos(0),
	Rate(0)
{
	Wstring token; // RA2
	int pos = string.getToken(0, ",", token);
	if (token.length() > 0) {
		Filename = token;
	}
	pos = string.getToken(pos, ",", token); // RA2
	if (token.length() > 0) {
		XPos = token.toInt();
	}
	pos = string.getToken(pos, ",", token); // RA2
	if (token.length() > 0) {
		YPos = token.toInt();
	}
	string.getToken(pos, ",", token); // RA2
	if (token.length() > 0) {
		Rate = token.toInt();
	}
}


/// <summary>
/// Creates a map selection sound effect from its control file line.
/// The line names the sample file and the volume to play it at. The sample comes from
/// the mixfiles, or from a loose file if the mixfiles do not carry it. Nothing is
/// loaded when the audio system is unavailable or the volume works out to silence.
/// </summary>
/// <param name="name">Name the map selection screen will ask for this sound by.</param>
/// <param name="string">The control file line to parse; it is chopped up in place.</param>
/// <remarks>Check Get_Name for NULL to tell whether the sound effect is usable.</remarks>
MSSfxEntry::MSSfxEntry(Wstring const & name, Wstring const & string) : // RA2
	Name((char const *)NULL),
	AllocLoaded(false),
	Sample(NULL),
	Volume(255)
{
	if (name != NULL && AudioDeviceAvailable()) { // RA2
		Wstring file_name;
		int pos = string.getToken(0, ",", file_name);
		Wstring volume_str;
		string.getToken(pos, ",", volume_str);

		if (volume_str != NULL) {
			Volume = volume_str.toInt(); // RA2
			if (Volume > 100) {
				Volume = 100;
			} else if (Volume < 0) {
				Volume = 0;
			}
			Volume = 255 * Volume / 100;
		}

		if (Volume > 0 && file_name != NULL) {
			Name = name; // RA2
		}
	}
}


/// <summary>
/// Destroys the sound effect.
/// Any playback in progress is stopped first, and the sample data is released only if
/// this entry is the one that loaded it.
/// </summary>
MSSfxEntry::~MSSfxEntry(void)
{
}


/// <summary>
/// Plays the sound effect.
/// The volume this entry was configured with is scaled by the player's sound volume
/// option before the sample is handed to the audio system.
/// </summary>
void MSSfxEntry::Play(void)
{
}

