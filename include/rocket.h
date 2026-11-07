// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.; 2026 OpenTS contributors
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
// Changed to RA2 1.006's layout by RedAlert2 contributors, 2026.
#pragma once

#include "face.h"
#include "ftimer.h"
#include "loco.h"
#include "matrix3d.h"
#include "timer.h"

#include "layer.hh"
#include "mark.hh"

struct RocketStruct;

class RocketLocomotionClass : public LocomotionClass
{
		typedef LocomotionClass BASECLASS;

	public:

		RocketLocomotionClass(void);
		RocketLocomotionClass(NoInitClass const & x) : BASECLASS(x), MissionTimer(x) {}
		virtual ~RocketLocomotionClass(void) override;

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;

		virtual boolean STDMETHODCALLTYPE Is_Moving(void) override;
		virtual CoordStruct STDMETHODCALLTYPE Destination(void) override;
		virtual Matrix3DStruct STDMETHODCALLTYPE Draw_Matrix(int * key) override;
		virtual boolean STDMETHODCALLTYPE Process(void) override;
		virtual void STDMETHODCALLTYPE Move_To(CoordStruct to) override;
		virtual void STDMETHODCALLTYPE Stop_Moving(void) override;
		virtual void STDMETHODCALLTYPE Do_Turn(DirStruct coord) override;
		virtual LayerType STDMETHODCALLTYPE In_Which_Layer(void) override;
		virtual boolean STDMETHODCALLTYPE Is_Moving_Now(void) override;
		virtual void STDMETHODCALLTYPE Mark_All_Occupation_Bits(int mark) override;

		virtual int Fetch_Object_Size(void) const override {return(sizeof(*this));}

	private:

		Coord MovingDestination;

		ProgressTimerClass<FrameTimerClass> MissionTimer;

		CDTimerClass<FrameTimerClass> TrailerTimer;

		int MissionState;

		int Unknown44;

		double CurrentSpeed;

		float CurrentPitch; // RA2: +0x50

		int CruiseDistance; // RA2: +0x54, YR: Unknown58; distance to MovingDestination when the climb ends

		Coord Position_At(int distance);
		bool Reached_Target(RocketStruct const * data);
		double Pitch_To_Target(void);
		void Detonate(void);
};
