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

#include "tube.h"

#include "_map.h"
#include "ccini.h"
#include "cell.h"
#include "coord.h"
#include "crc.h"
#include "globals.h"
#include "inline.h"
#include "tracker.h"

#include <cstdio>


char const * TubeClass::INI_NAME = "Tubes";


/// <summary>
/// Creates a tunnel and adds it to the tunnel list.
/// The tunnel starts out with no length at all -- it enters and leaves at the same cell
/// until directions are added to it. The cell it is entered from is stamped with this
/// tunnel's index so that anything standing there can find its way underground.
/// </summary>
/// <param name="cell">The cell that the tunnel is entered from.</param>
/// <param name="dir">The facing that the tunnel is entered by.</param>
TubeClass::TubeClass(Cell const &cell, FacingType dir) :
	Enter(cell),
	Exit(cell),
	EnterDir(dir),
	Count(0)
{
	Create_ID();

	for (int i = 0; i < 100; i++) {
		Dirs[i] = FACING_NONE;
	}

	Tubes.Add(this);

	if (cell != Cell(0, 0)) {
		CellClass *cptr = &Map[cell];
		cptr->Tube = Tubes.ID(this);
	}
}


/// <summary>
/// Removes this tunnel from the game.
/// Everything that was holding on to this tunnel is told to let go of it, and the cell
/// the tunnel was entered from forgets about it as well.
/// </summary>
TubeClass::~TubeClass(void)
{
	Detach_This_From_All(this, true);

	CellClass *cptr = &Map[Enter];
	if (cptr) {
		if (cptr->Tube == Tubes.ID(this)) {
			cptr->Tube = -1;
		}
	}

	Tubes.Delete(this);
}


/// <summary>
/// Loads this tunnel from the save game stream.
/// The object is rebuilt in place once the base class has read the raw image back over
/// it, so that the tunnel has a working virtual table again.
/// </summary>
/// <returns>Returns with S_OK if the tunnel was read successfully.</returns>
HRESULT STDMETHODCALLTYPE TubeClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) TubeClass(NoInitClass());

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Saves this tunnel to the save game stream.
/// </summary>
/// <param name="cleardirty">Should the dirty flag be cleared once the object is written?</param>
/// <returns>Returns with S_OK if the tunnel was written successfully.</returns>
HRESULT STDMETHODCALLTYPE TubeClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Tidies up the tunnel list and renumbers it.
/// A tunnel whose entrance cell no longer holds a tunnel is discarded outright, and the
/// survivors are numbered in order so that every entrance cell points at the right one.
/// This routine is used before the tunnels are written out to a map file.
/// </summary>
void TubeClass::Assign_Tubes(void)
{
	int tube_count = Tubes.Count();
	int current_tube = -1;
	for (int index = 0; index < tube_count; index++) {
		TubeClass * tube = Tubes[index];
		CellClass * cptr = &Map[tube->Enter];

		if (!cptr->Has_Tunnel()) {
			delete tube;
			tube_count--;
			cptr->Tube = -1;
			index--;
		} else {
			cptr->Tube = ++current_tube;
		}
	}
}


/// <summary>
/// Writes all of the tunnels out to the map file.
/// Each tunnel becomes a single entry holding the cell it is entered from, the facing it
/// is entered by, the cell it comes out of, and the chain of directions that traces its
/// path underground. The tunnel list is tidied up first so that the indices written here
/// agree with the ones stamped into the cells.
/// </summary>
void TubeClass::Write_INI(CCINIClass & ini)
{
	char key[12];
	char buffer[550];

	ini.Clear(INI_NAME);
	Assign_Tubes();

	for (int index = 0; index < Tubes.Count(); index++) {

		TubeClass * tube = Tubes[index];
		if (tube != NULL) {

			sprintf(key, "%d", index);
			sprintf(buffer, "%d,%d,%d,%d,%d", tube->Enter.X, tube->Enter.Y, tube->EnterDir, tube->Exit.X, tube->Exit.Y);
			for (int dir = 0; dir < 100; dir++) {
				sprintf(buffer + strlen(buffer), ",%d", tube->Dirs[dir]);
			}
			ini.Put_String(INI_NAME, key, buffer);
		}
	}
}


/// <summary>
/// Reads the tunnels in from the map file.
/// A tunnel is created for every entry found, and the cell each one is entered from is
/// stamped with the index of the tunnel that leads out of it.
/// </summary>
void TubeClass::Read_INI(CCINIClass const & ini)
{
	char buffer[INIClass::MAX_LINE_LENGTH];

	int count = ini.Entry_Count(INI_NAME);

	for (int index = 0; index < count; index++) {

		char const * entry = ini.Get_Entry(INI_NAME, index);
		ini.Get_String(INI_NAME, entry, NULL, buffer, sizeof(buffer));

		TubeClass * tube = new TubeClass(Cell(0,0));

		tube->Enter.X = atoi(strtok(buffer, ","));
		tube->Enter.Y = atoi(strtok(NULL, ","));
		tube->EnterDir = (FacingType)atoi(strtok(NULL, ","));
		tube->Exit.X = atoi(strtok(NULL, ","));
		tube->Exit.Y = atoi(strtok(NULL, ","));

		tube->Count = -1;
		for (int dir = 0; dir < 100; dir++) {
			tube->Dirs[dir] = (FacingType)atoi(strtok(NULL, ","));
			tube->Count++;
			if (tube->Dirs[dir] == FACING_NONE) break;
		}

		Map[tube->Enter].Tube = index;
	}
}


/// <summary>
/// Is the specified cell clear of this tunnel?
/// The tunnel's path is traced from its entrance and the cell is checked against every
/// step along the way. The entrance cell itself does not count as being inside the
/// tunnel -- an object standing there is still above ground.
/// </summary>
/// <returns>bool; Is the cell outside of this tunnel?</returns>
bool TubeClass::Not_In_Tube(Cell const & cell) const
{
	Cell tubecell = Enter;

	if (tubecell == cell) {
		return(true);
	}

	int dir = 0;
	while (Dirs[dir] != FACING_NONE) {
		tubecell = Adjacent_Cell(tubecell, Dirs[dir++]);
		if (tubecell == cell) {
			return(false);
		}
	}

	return(true);
}


/// <summary>
/// Extends this tunnel by one step.
/// The direction is appended to the path that traces the tunnel underground, and the
/// exit cell moves along with it. The map editor builds a tunnel up one step at a time
/// this way.
/// </summary>
/// <param name="dir">The direction to extend the tunnel in.</param>
/// <remarks>A tunnel can hold no more than 99 steps. Beyond that the direction is
/// dropped, although the exit cell still moves.</remarks>
void TubeClass::Add_Direction(FacingType dir)
{
	if (Count < 99) {
		Dirs[Count] = dir;
		Dirs[++Count] = FACING_NONE;
	}

	Exit = Adjacent_Cell(Exit, dir);
}


/// <summary>
/// Folds this tunnel's state into a running CRC.
/// The multiplayer sync check uses this to prove that every machine still agrees on
/// where the tunnels run.
/// </summary>
/// <param name="crc">The running CRC to fold this tunnel's state into.</param>
void TubeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);

	crc(Enter.X);
	crc(Enter.Y);
	crc(Exit.X);
	crc(Exit.Y);
	crc(EnterDir);
	for (int i = 0; i < 100; i++) {
		crc(Dirs[i]);
	}
	crc(Count);
}


/// <summary>
/// Fetches the size of this object as it is written to a save game.
/// </summary>
/// <returns>Returns with the size of the tunnel record in bytes.</returns>
int TubeClass::Fetch_Object_Size(void) const // RA2: no argument
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the RTTI type of this object.
/// </summary>
/// <returns>Returns with RTTI_TUBE.</returns>
RTTIType TubeClass::Fetch_RTTI(void) const
{
	return(RTTI_TUBE);
}


/// <summary>
/// Fetches the class identifier of this object.
/// This routine is used by the save game system so that it knows what kind of object to
/// construct when the stream is read back in.
/// </summary>
/// <param name="retval">Pointer to the place to store the class identifier.</param>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE TubeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_TubeClass;
	return(S_OK);
}
