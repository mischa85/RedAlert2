/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "ftimer.h"
#include "ipiggy.h"
#include "loco.h"
#include "timer.h"


class TeleportLocomotionClass : public LocomotionClass, public IPiggyback // RA2: IPiggyback
{
		typedef LocomotionClass BASECLASS;

	public:
		TeleportLocomotionClass(void);
		TeleportLocomotionClass(const NoInitClass & x) : BASECLASS(x), Timer(x), Piggybacker(NULL) {}

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObject) override;
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override {return(BASECLASS::AddRef());}
		virtual ULONG STDMETHODCALLTYPE Release(void) override {return(BASECLASS::Release());}

		virtual HRESULT STDMETHODCALLTYPE Begin_Piggyback(ILocomotion * pointer) override;
		virtual HRESULT STDMETHODCALLTYPE End_Piggyback(ILocomotion ** pointer) override;
		virtual boolean STDMETHODCALLTYPE Is_Ok_To_End(void) override;
		virtual HRESULT STDMETHODCALLTYPE Piggyback_CLSID(GUID * classid) override;
		virtual boolean STDMETHODCALLTYPE Is_Piggybacking(void) override {return(Piggybacker != NULL);}

		virtual boolean STDMETHODCALLTYPE Is_Moving(void) override;
		virtual CoordStruct STDMETHODCALLTYPE Destination(void) override;
		virtual void STDMETHODCALLTYPE Move_To(CoordStruct to) override;
		virtual void STDMETHODCALLTYPE Stop_Moving(void) override;
		virtual boolean STDMETHODCALLTYPE Process(void) override;
		virtual LayerType STDMETHODCALLTYPE In_Which_Layer(void) override;

		virtual int Fetch_Object_Size(void) const override; // RA2: no argument

		virtual boolean Is_Stationary(void);

	private:
		/*
		 * This is the coordinate that the object will be set down at the next time this
		 * locomotor is processed. If COORD_NONE, then no teleport is pending -- there is
		 * no separate moving flag, so this doubles as one.
		 */
		Coord DestinationCoord; // RA2: +0x1C

		Coord LastCoords; // RA2: +0x28
		bool IsMoving; // RA2: +0x34
		bool Unused_35; // RA2: only ever false; Is_Ok_To_End requires it clear
		bool Unused_36; // RA2: only ever set false (constructor, Stop_Moving), never read
		int State; // RA2: +0x38
		CDTimerClass<FrameTimerClass> Timer; // RA2: +0x3C
		ILocomotionPtr Piggybacker; // RA2: +0x48
};
