/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "coord.h"
#include "ilocos.h"
#include "noinit.h"

class FootClass;

class LocomotionClass : public IPersistStream, public ILocomotion
{
	public:
		LocomotionClass(void);
		LocomotionClass(const NoInitClass &x) {}
		virtual ~LocomotionClass(void);

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObj) override;
		virtual ULONG STDMETHODCALLTYPE AddRef() override;
		virtual ULONG STDMETHODCALLTYPE Release() override;

		virtual LONG STDMETHODCALLTYPE IsDirty(void) override {return(Dirty ? S_OK : S_FALSE);}
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override = 0;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;
		virtual LONG STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize) override;

		virtual HRESULT STDMETHODCALLTYPE Link_To_Object(void *object) override;
		virtual boolean STDMETHODCALLTYPE Is_Moving(void) override;
		virtual CoordStruct STDMETHODCALLTYPE Destination(void) override;
		virtual CoordStruct STDMETHODCALLTYPE Head_To_Coord(void) override;
		virtual MoveType STDMETHODCALLTYPE Can_Enter_Cell(CellStruct cell) override;
		virtual boolean STDMETHODCALLTYPE Is_To_Have_Shadow(void) override;
		virtual Matrix3DStruct STDMETHODCALLTYPE Draw_Matrix(int *key) override;
		virtual Matrix3DStruct STDMETHODCALLTYPE Shadow_Matrix(int *key) override;
		virtual Point2DStruct STDMETHODCALLTYPE Draw_Point(void) override;
		virtual Point2DStruct STDMETHODCALLTYPE Shadow_Point(void) override;
		virtual VisualType STDMETHODCALLTYPE Visual_Character(boolean flag) override;
		virtual int STDMETHODCALLTYPE Z_Adjust(void) override;
		virtual ZGradientType STDMETHODCALLTYPE Z_Gradient(void) override;
		virtual boolean STDMETHODCALLTYPE Process(void) override;
		virtual void STDMETHODCALLTYPE Move_To(CoordStruct to) override;
		virtual void STDMETHODCALLTYPE Stop_Moving(void) override;
		virtual void STDMETHODCALLTYPE Do_Turn(DirStruct coord) override;
		virtual void STDMETHODCALLTYPE Unlimbo(void) override;
		virtual void STDMETHODCALLTYPE Tilt_Pitch_AI(void) override;
		virtual boolean STDMETHODCALLTYPE Power_On(void) override;
		virtual boolean STDMETHODCALLTYPE Power_Off(void) override;
		virtual boolean STDMETHODCALLTYPE Is_Powered(void) override;
		virtual boolean STDMETHODCALLTYPE Is_Ion_Sensitive(void) override;
		virtual boolean STDMETHODCALLTYPE Push(DirStruct dir) override;
		virtual boolean STDMETHODCALLTYPE Shove(DirStruct dir) override;
		virtual void STDMETHODCALLTYPE Force_Track(int track, CoordStruct coord) override;
		virtual void STDMETHODCALLTYPE Force_Immediate_Destination(CoordStruct coord) override;
		virtual void STDMETHODCALLTYPE Force_New_Slope(int ramp) override;
		virtual boolean STDMETHODCALLTYPE Is_Moving_Now(void) override {return(Is_Moving());}
		virtual int STDMETHODCALLTYPE Apparent_Speed(void) override;
		virtual int STDMETHODCALLTYPE Drawing_Code(void) override;
		virtual FireErrorType STDMETHODCALLTYPE Can_Fire(void) override;
		virtual int STDMETHODCALLTYPE Get_Status() override {return(0);}
		virtual void STDMETHODCALLTYPE Acquire_Hunter_Seeker_Target(void) override {}
		virtual boolean STDMETHODCALLTYPE Is_Surfacing() override {return(false);}
		virtual void STDMETHODCALLTYPE Mark_All_Occupation_Bits(int mark) override {}
		virtual boolean STDMETHODCALLTYPE Is_Moving_Here(CoordStruct to) override {return(false);}
		virtual boolean STDMETHODCALLTYPE Will_Jump_Tracks(void) override {return(false);}
		virtual boolean STDMETHODCALLTYPE Is_Really_Moving_Now(void) override {return(Is_Moving_Now());}
		virtual void STDMETHODCALLTYPE Stop_Movement_Animation(void) override {}
		virtual void STDMETHODCALLTYPE Lock(void) override {}
		virtual void STDMETHODCALLTYPE Unlock(void) override {}
		virtual int STDMETHODCALLTYPE Get_Track_Number(void) override {return(-1);}
		virtual int STDMETHODCALLTYPE Get_Track_Index(void) override {return(-1);}
		virtual int STDMETHODCALLTYPE Get_Speed_Accum(void) override {return(-1);}

		virtual int Fetch_Object_Size(bool oldsave = false) const = 0;

	protected:
		/*
		 * Declaring this overload const keeps a call made through a LocomotionClass from
		 * binding to the interface Load. The behavior it relies on is not guaranteed by the
		 * language, and the definitions in the source file must not be const, or the module
		 * will not link.
		 */
		HRESULT STDMETHODCALLTYPE Load(IStream * stream) const;

	protected:
		/*
		 * Pointer to the object this locomotor carries about. It is attached as the object
		 * is created, and every service the locomotor offers is performed through it.
		 */
		FootClass *LinkedTo;

		/*
		 * If this locomotor is able to move its object under its own means, then this flag
		 * will be true. It is cleared when an EM pulse or a loss of base power is to strand
		 * the object where it stands.
		 */
		bool IsPowered;

		/*
		 * If this locomotor has changed since it was last written out, then this flag will
		 * be true. It starts out set and is only cleared by a save that asks for it, so the
		 * persistence machinery never assumes a locomotor is already safely on disk.
		 */
		bool Dirty;

		/*
		 * This is the number of outstanding references to this locomotor. Releasing the
		 * last one destroys the locomotor, which is how its lifetime is managed through
		 * the COM interfaces it presents.
		 */
		LONG RefCount;
};
