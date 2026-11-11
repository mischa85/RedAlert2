// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Changed to RA2 1.006's layout by RedAlert2 contributors, 2026.
#pragma once

#include "abstract.h"
#include "ftimer.h"
#include "timer.h"
#include "vector.h"

class HouseClass;
class TechnoClass;

class CaptureManagerClass : public AbstractClass
{
		typedef AbstractClass BASECLASS;

	public:
		TechnoClass * Unit; // RA2: +0x24, the one unit under control (YR: ControlNode::Unit)
		HouseClass * OriginalOwner; // RA2: +0x28, its house before capture (YR: ControlNode::OriginalOwner)
		TechnoClass * Owner; // RA2: +0x2C

		CaptureManagerClass(void);
		CaptureManagerClass(TechnoClass * owner, int damage, bool infinite);
		CaptureManagerClass(TechnoClass * owner); // RA2: 0x467DB0
		CaptureManagerClass(NoInitClass const & x);
		virtual ~CaptureManagerClass(void) override;

		bool CaptureUnit(AbstractClass * target);
		void HandleOverload(void);
		void DecideUnitFate(TechnoClass * techno);
		void FreeAll(void);
		bool FreeUnit(TechnoClass * techno);
		bool FUN_00472330(TechnoClass * techno);
		bool FUN_004722E0(void) const;
		bool CannotControlAnyMore(void);
		bool FUN_00471F90(AbstractClass * ptr);
		int GetControlledCount(void) const;
		HouseClass * GetOriginalOwner(TechnoClass * techno) const;
		bool CanCapture(TechnoClass * techno);
		bool IsControllingSomething(void) const;
		bool IsOverloading(bool * flag);
		bool NeedsToDrawLinks(void) const;
		void DrawLinks(void) const;

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;

		virtual RTTIType Fetch_RTTI(void) const override;
		virtual int Fetch_Object_Size(void) const override;
		virtual void Compute_CRC(CRCEngine & crc) const override;
};

extern DynamicVectorClass<CaptureManagerClass *> CaptureManagerArray;
