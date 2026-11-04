/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "swizzle.h"

#include "vector.h"


SwizzleManagerClass Swizzler;


/// <summary>
/// Constructor for the pointer swizzle manager.
/// The request and pointer tables are given a generous growth step, since a save game
/// hands the swizzler thousands of pointers in a single pass.
/// </summary>
SwizzleManagerClass::SwizzleManagerClass(void)
{
	RequestTable.Set_Growth_Step(1000);
	PointerTable.Set_Growth_Step(1000);
}


/// <summary>
/// Destructor for the pointer swizzle manager.
/// Any requests still pending are resolved on the way out.
/// </summary>
SwizzleManagerClass::~SwizzleManagerClass(void)
{
	Process_Tables();
}


/// <summary>
/// Resolves any outstanding pointer requests and starts over.
/// The load code calls this routine once every object has been read in and has announced
/// itself, so that the saved pointers become real addresses again.
/// </summary>
HRESULT STDMETHODCALLTYPE SwizzleManagerClass::Reset(void)
{
	Process_Tables();

	return(S_OK);
}


/// <summary>
/// Registers a saved pointer to be resolved later.
/// The load code calls this routine for every pointer it reads back, since the value on
/// disk is a swizzle ID rather than an address. The request is remembered and the pointer
/// is cleared until the tables are processed and the real address is known.
/// </summary>
/// <param name="pointer">Pointer to the pointer that needs resolving.</param>
/// <returns>Returns with S_OK, or E_POINTER if no pointer was supplied.</returns>
HRESULT STDMETHODCALLTYPE SwizzleManagerClass::Swizzle(void **pointer)
{
	if (pointer == NULL) {
		return(E_POINTER);
	}

	LONG id = LONG(*pointer);
	if (!id) {
		return(S_OK);
	}

	SwizzlePointerClass pair(id, pointer);
	RequestTable.Add(pair);

	*pointer = NULL;

	return(S_OK);
}


/// <summary>
/// Announces the real address of a swizzle ID.
/// Objects call this routine as they are loaded, to say where they ended up. The requests
/// gathered by Swizzle are matched against these announcements when the tables are
/// processed.
/// </summary>
/// <param name="id">The swizzle ID the object was saved under.</param>
/// <param name="pointer">The address the object now resides at.</param>
HRESULT STDMETHODCALLTYPE SwizzleManagerClass::Here_I_Am(LONG id, void *pointer)
{
	SwizzlePointerClass pair;
	pair.Pointer = pointer;
	pair.ID = id;
	PointerTable.Add(pair);

	return(S_OK);
}


/// <summary>
/// Compares two swizzle entries by their ID.
/// This routine is the sort callback used on the request and pointer tables, so that the
/// two can be walked in step when the swizzler resolves them.
/// </summary>
/// <param name="ptr1">The first swizzle entry to compare.</param>
/// <param name="ptr2">The second swizzle entry to compare.</param>
/// <returns>Returns with -1, 0 or 1 according to whether the first entry's ID sorts
/// before, with, or after the second entry's ID.</returns>
int __cdecl ptr_compare_func(const void *ptr1, const void *ptr2)
{
	const SwizzlePointerClass *p1 = static_cast<const SwizzlePointerClass *>(ptr1);
	const SwizzlePointerClass *p2 = static_cast<const SwizzlePointerClass *>(ptr2);

	if (p1->ID == p2->ID) {
		return(0);
	}
	if (p1->ID < p2->ID) {
		return(-1);
	}
	return(1);
}


/// <summary>
/// Resolves every outstanding pointer request.
/// This is the heart of the swizzler. The requests registered by Swizzle are matched
/// against the addresses announced by Here_I_Am, and each real address is written into the
/// pointer that asked for it. Both tables are emptied once the matching is done.
/// </summary>
/// <remarks>Every ID handed to Swizzle must have been announced by Here_I_Am. An unmatched
/// request is fatal and this routine will deliberately fault rather than leave a dangling
/// pointer in the game.</remarks>
void SwizzleManagerClass::Process_Tables(void)
{
	if (RequestTable.Count() != 0) {

		if (PointerTable.Count() > 0) {
			qsort(&PointerTable[0], PointerTable.Count(), sizeof(SwizzlePointerClass), ptr_compare_func);
		}
		if (RequestTable.Count() > 0) {
			qsort(&RequestTable[0], RequestTable.Count(), sizeof(SwizzlePointerClass), ptr_compare_func);
		}

		int pointer_count = PointerTable.Count();
		SwizzlePointerClass *pointer_entry = &PointerTable[0];

		int request_count = RequestTable.Count();
		SwizzlePointerClass *request_entry = &RequestTable[0];

		while (request_count > 0) {

			LONG req_id = request_entry->ID;
			LONG ptr_id = pointer_entry->ID;

			if (req_id == ptr_id) {
				LONG *ptr = (LONG *)request_entry->Pointer;
				*ptr = (LONG)pointer_entry->Pointer;

				request_entry++;
				request_count--;
				continue;

			}

			while (req_id > ptr_id) {
				pointer_entry++;
				pointer_count--;
				ptr_id = pointer_entry->ID;
			}

			ptr_id = pointer_entry->ID;

			if (req_id != ptr_id) {
				volatile int div_zero = 0;
				div_zero = 1 / div_zero;
			}

		}

		RequestTable.Clear();
		PointerTable.Clear();
	}

}


/// <summary>
/// Fetches the room a swizzled pointer takes up in a save file.
/// The persistence code calls this routine to size the space a pointer will need once it
/// has been reduced to a swizzle ID.
/// </summary>
/// <param name="size">Pointer to the variable to fill in with the byte count.</param>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE SwizzleManagerClass::Get_Save_Size(int *size)
{
	if (size == NULL) {
		return(E_POINTER);
	}

	*size = sizeof(int);
	return(S_OK);
}


/// <summary>
/// Fetches the requested interface from the swizzle manager.
/// Only the swizzler's own interface is supported. The manager is a single global object
/// rather than a reference counted one, so the caller never really owns what it gets back.
/// </summary>
/// <param name="riid">The interface identifier being asked for.</param>
/// <param name="ppvObject">Pointer to the variable to fill in with the interface pointer.</param>
/// <returns>Returns with S_OK, E_POINTER if no destination was supplied, or E_NOINTERFACE
/// if the interface asked for is not one the manager provides.</returns>
LONG STDMETHODCALLTYPE SwizzleManagerClass::QueryInterface(REFIID riid, LPVOID * ppvObject)
{
	if (ppvObject == NULL) {
		return(E_POINTER);
	}

	if ( riid != IID_ISwizzle && riid != IID_IUnknown) {
		return(E_NOINTERFACE);
	}

	*ppvObject = reinterpret_cast<ISwizzle *>(this);

	reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();
	return(S_OK);
}


/// <summary>
/// Fetches the swizzle ID that stands in for an object.
/// The save code calls this routine to turn a live pointer into the value that will be
/// written to the file in its place. An object's current address serves as its ID.
/// </summary>
/// <param name="pointer">The object to fetch the ID of.</param>
/// <param name="id">Pointer to the variable to fill in with the ID.</param>
/// <returns>Returns with S_OK, or E_POINTER if the object or the destination was missing.</returns>
HRESULT STDMETHODCALLTYPE SwizzleManagerClass::Fetch_Swizzle_ID(void *pointer, LONG *id)
{
	if (pointer != NULL && id != 0) {
		*id = (LONG)pointer;
		return(S_OK);
	}

	return(E_POINTER);
}
