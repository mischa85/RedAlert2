/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "iswizzle.h"
#include "vector.h"
#include "win.h"

class SwizzlePointerClass
{
	public:
		SwizzlePointerClass(LONG id = 0, void *pointer = NULL) : ID(id), Pointer(pointer) {}

		bool operator==(const SwizzlePointerClass &that) const { return(ID == that.ID); }
		bool operator!=(const SwizzlePointerClass &that) const { return(ID != that.ID); }
		bool operator<(const SwizzlePointerClass &that) const { return(ID < that.ID); }
		bool operator>(const SwizzlePointerClass &that) const { return(ID > that.ID); }
	public:
		/*
		 * This is the swizzle ID this entry is keyed by -- the address the object occupied
		 * when the game was saved. Both of the swizzler's tables are sorted on it so that
		 * they can be walked in step.
		 */
		LONG ID;

		/*
		 * This is the address this entry carries. In a request it is the pointer that needs
		 * fixing up; in an announcement it is where the object was loaded to.
		 */
		void *Pointer;
};


class SwizzleManagerClass : public ISwizzle
{
	public:
		SwizzleManagerClass(void);
		~SwizzleManagerClass(void);

		virtual LONG STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObject) override;
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return(1); }
		virtual ULONG STDMETHODCALLTYPE Release(void) override { return(1); }

		virtual HRESULT STDMETHODCALLTYPE Reset(void) override;

		virtual HRESULT STDMETHODCALLTYPE Swizzle(void **pointer);
		virtual HRESULT STDMETHODCALLTYPE Fetch_Swizzle_ID(void *pointer, LONG *id) override;
		virtual HRESULT STDMETHODCALLTYPE Here_I_Am(LONG id, void *pointer) override;
		virtual HRESULT STDMETHODCALLTYPE Save_Interface(IStream *stream, IUnknown *pointer) override { return(E_NOTIMPL); }
		virtual HRESULT STDMETHODCALLTYPE Load_Interface(IStream *stream, CLSID *riid, void **pointer) { return(E_NOTIMPL); }
		virtual HRESULT STDMETHODCALLTYPE Get_Save_Size(int *size) override;

	private:
		void Process_Tables(void);

		/*
		 * These are the pointers read back from the save file that still hold a swizzle ID
		 * instead of a real address. They are filled in and the list emptied when the
		 * tables are processed.
		 */
		DynamicVectorClass<SwizzlePointerClass> RequestTable;

		/*
		 * These are the addresses the loaded objects have announced themselves at. Every
		 * request is matched against this list to discover what its swizzle ID now stands
		 * for.
		 */
		DynamicVectorClass<SwizzlePointerClass> PointerTable;
};


extern SwizzleManagerClass Swizzler;

template<class T>
inline void Swizzle_Pointer(T **ptr)
{
	Swizzler.Swizzle((void**)ptr);
}

template<class T>
inline void Swizzle_Here_I_Am(LONG id, T *ptr)
{
	Swizzler.Here_I_Am(id, (void*)ptr);
}
