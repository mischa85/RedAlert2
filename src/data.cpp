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
 *                     $Archive:: /G/wwlib/data.cpp                                           $*
 *                                                                                             *
 *                      $Author:: Neal_k                                                      $*
 *                                                                                             *
 *                     $Modtime:: 9/24/99 4:52p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Load_Alloc_Data -- Allocates a buffer and loads the file into it.                         *
 *   Load_Uncompress -- Loads and uncompresses data to a buffer.                               *
 *   Hires_Load -- Allocates memory for, and loads, a resolution dependant file.               *
 *   Fetch_String -- Fetches a string resource.                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "data.h"

#include "_mixfile.h"
#include "ccfile.h"
#include "mixfile.h"
#include "wstring.h"

#include <new.h>

HINSTANCE LanguageResources;

/***********************************************************************************************
 * Load_Alloc_Data -- Allocates a buffer and loads the file into it.                           *
 *                                                                                             *
 *    This is the C++ replacement for the Load_Alloc_Data function. It will allocate the       *
 *    memory big enough to hold the file and then read the file into it.                       *
 *                                                                                             *
 * INPUT:   file  -- The file to read.                                                         *
 *                                                                                             *
 *          mem   -- The memory system to use for allocation.                                  *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated and filled memory block.                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void * Load_Alloc_Data(FileClass & file)
{
	void * ptr = NULL;
	if (file.Is_Available()) {
		int size = file.Size();

		ptr = new char[size];
		if (ptr != NULL) {
			file.Read(ptr, size);
		}
	}
	return(ptr);
}


/// <summary>
/// RA2: fetches a whole file into memory by name.
/// A file held in a mixfile is used where it lies; any other file is read into a new buffer.
/// </summary>
/// <param name="name">The name of the file to load.</param>
/// <param name="allocated">Set when the data was read into a new buffer that the caller must free.</param>
/// <returns>Returns with a pointer to the file's data, or NULL if it could not be found.</returns>
void * Load_Alloc_Data(char const * name, bool & allocated)
{
	allocated = false;
	void * loaded = (void *)MFCD::Retrieve(name);
	if (loaded != NULL) return(loaded);

	CCFileClass file(name);
	FileClass * stream = &file;
	void * ptr = NULL;
	if (stream->Is_Available()) {
		int size = stream->Size();
		ptr = new char[size];
		if (ptr != NULL) {
			stream->Read(ptr, size);
		}
	}
	allocated = (ptr != NULL);
	return(ptr);
}


/// <summary>
/// RA2: fetches a whole file into memory by name, as Load_Alloc_Data above.
/// </summary>
void * Load_Alloc_Data(Wstring const & name, bool * allocated)
{
	char const * filename = name.text(); // RA2: text, not get
	*allocated = false;
	void * found = (void *)MFCD::Retrieve(filename);
	if (found != NULL) return(found);

	CCFileClass file(filename);
	FileClass * stream = &file;
	void * ptr = NULL;
	if (stream->Is_Available()) {
		int size = stream->Size();
		ptr = new char[size];
		if (ptr != NULL) {
			stream->Read(ptr, size);
		}
	}
	*allocated = (ptr != NULL);
	return(ptr);
}


/***********************************************************************************************
 * Load_Uncompress -- Loads and uncompresses data to a buffer.                                 *
 *                                                                                             *
 *    This is the C++ counterpart to the Load_Uncompress function. It will load the file       *
 *    specified into the graphic buffer indicated and uncompress it.                           *
 *                                                                                             *
 * INPUT:   file     -- The file to load and uncompress.                                       *
 *                                                                                             *
 *          uncomp_buff -- The graphic buffer that initial loading will use.                   *
 *                                                                                             *
 *          dest_buff   -- The buffer that will hold the uncompressed data.                    *
 *                                                                                             *
 *          reserved_data  -- This is an optional pointer to a buffer that will hold any       *
 *                            reserved data the compressed file may contain. This is           *
 *                            typically a palette.                                             *
 *                                                                                             *
 * OUTPUT:  Returns with the size of the uncompressed data in the destination buffer.          *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int Load_Uncompress(FileClass & file, Buffer & uncomp_buff, Buffer & dest_buff, void * reserved_data)
{
	unsigned short	size;
	void	* sptr = uncomp_buff.Get_Buffer();
	void	* dptr = dest_buff.Get_Buffer();
	int	opened = false;
	CompHeaderType	header;

	/*
	**	The file must be opened in order to be read from. If the file
	**	isn't opened, then open it. Record this fact so that it can be
	**	restored to its closed state at the end.
	*/
	if (!file.Is_Open()) {
		if (!file.Open()) {
			return(0);
		}
		opened = true;
	}

	/*
	**	Read in the size of the file (supposedly).
	*/
	file.Read(&size, sizeof(size));

	/*
	**	Read in the header block. This block contains the compression type
	**	and skip data (among other things).
	*/
	file.Read(&header, sizeof(header));
	size -= (unsigned short)sizeof(header);

	/*
	**	If there are skip bytes then they must be processed. Either read
	**	them into the buffer provided or skip past them. No check is made
	**	to ensure that the reserved data buffer is big enough (watch out!).
	*/
	if (header.Skip) {
		size -= header.Skip;
		if (reserved_data) {
			file.Read(reserved_data, header.Skip);
		} else {
			file.Seek(header.Skip, SEEK_CUR);
		}
		header.Skip = 0;
	}

	/*
	**	Determine where is the proper place to load the data. If both buffers
	**	specified are identical, then the data should be loaded at the end of
	**	the buffer and decompressed at the beginning.
	*/
	if (uncomp_buff.Get_Buffer() == dest_buff.Get_Buffer()) {
		sptr = (char *)sptr + uncomp_buff.Get_Size()-(size+sizeof(header));
	}

	/*
	**	Read in the bulk of the data.
	*/
	memmove(sptr, &header, sizeof(header));
//	Mem_Copy(&header, sptr, sizeof(header));
	file.Read((char *)sptr + sizeof(header), size);

	/*
	**	Decompress the data.
	*/
	size = (unsigned short) Uncompress_Data(sptr, dptr);

	/*
	**	Close the file if necessary.
	*/
	if (opened) {
		file.Close();
	}
	return((int)size);
}


/***********************************************************************************************
 * Fetch_String -- Fetches a string resource.                                                  *
 *                                                                                             *
 *    Fetches a string resource and returns a pointer to its text.                             *
 *                                                                                             *
 * INPUT:   id -- The ID number of the string resource to fetch.                               *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the actual text of the string resource.                  *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/25/1996 JLB : Created.                                                                 *
 *=============================================================================================*/
char const * Fetch_String(int id)
{
	/*
	**	RA2: the game's text comes from the string table; there are no string resources.
	*/
	return(NULL);
}


/// <summary>
/// Fetches a raw resource from the language library.
/// This routine locates the named resource and locks it down so that the caller may read
/// straight out of it. The data stays mapped for as long as the library is loaded, so
/// there is nothing to release afterwards.
/// </summary>
/// <param name="resname">Name or identifier of the resource to fetch.</param>
/// <param name="restype">Type of the resource to fetch.</param>
/// <returns>Returns with a pointer to the resource data. Otherwise, NULL is returned.</returns>
void const * Fetch_Resource(LPCSTR resname, LPCSTR restype)
{
	/// The superfluous MAKEINTRESOURCE cast is the game's, and the C4302 warning with it.
	HRSRC handle = FindResource(LanguageResources, MAKEINTRESOURCE(resname), restype);
	if (handle == NULL) {
		return(NULL);
	}

	HGLOBAL rhandle = LoadResource(LanguageResources, handle);
	if (rhandle == NULL) {
		return(NULL);
	}

	return(LockResource(rhandle));
}


/// <summary>
/// Loads a compressed picture into the buffer specified.
/// This routine is a convenience layer over Load_Uncompress for callers that deal in whole
/// screen images and would rather think in pages than in bytes.
/// </summary>
/// <param name="palette">Buffer to fill in with the picture's palette, or NULL to discard
/// it.</param>
/// <returns>Returns with the number of pages the loaded picture occupies.</returns>
int Load_Picture(FileClass & file, Buffer & scratchbuf, Buffer & destbuf, unsigned char * palette, PicturePlaneType )
{
	return(Load_Uncompress(file, scratchbuf, destbuf,  palette ) / 8000);
}


/***********************************************************************************************
 * Hires_Load -- Allocates memory for, and loads, a resolution dependant file.                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Name of file to load                                                              *
 *                                                                                             *
 * OUTPUT:   Ptr to loaded file                                                                *
 *                                                                                             *
 * WARNINGS: Caller is responsible for releasing the memory allocated                          *
 *                                                                                             *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/13/96 3:20PM ST : Created                                                              *
 *=============================================================================================*/
void * Hires_Load(FileClass & file)
{
	int 	length;
	void * return_ptr;

	if (file.Is_Available()) {

		length = file.Size();
		return_ptr = new char[length];
		file.Read(return_ptr, length);
		return(return_ptr);

	} else {
		return(NULL);
	}
}


/// <summary>
/// Loads the language resource library.
/// This routine brings in the library that every localized string and resource is fetched
/// from. It may be called as often as convenient -- the library is only loaded the first
/// time. If asked to, it will tell the player to reinstall when the library is missing.
/// </summary>
/// <param name="show_error">Should the player be told when the library cannot be loaded?</param>
/// <returns>bool; Are the language resources available?</returns>
bool Init_Language_Resources(bool show_error)
{
	/*
	**	RA2: there is no language resource library to load.
	*/
	return(true);
}


/// <summary>
/// Fetches a description of the language pack in use.
/// This routine composes a readable line naming the localized data set and its version,
/// taken from the version resource of the language library. It is used when reporting the
/// build the player is running. The string is left empty if the library is absent or
/// carries no version information of its own.
/// </summary>
/// <param name="version_string">Buffer to fill in with the description.</param>
/// <remarks>Be sure that the destination buffer is big enough to hold the composed text.</remarks>
void Get_Language_Version(char *version_string)
{
	/*
	**	RA2: there is no language resource library to report the version of.
	*/
	version_string[0] = '\0';
}
