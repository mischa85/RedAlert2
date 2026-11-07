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
#include "abstype.h"

#include "campaign.hh"
#include "vq.hh"

class CCINIClass;

class CampaignClass : public AbstractTypeClass
{
		typedef AbstractTypeClass BASECLASS;

	public:
		CampaignClass(char const * name = NULL);
		CampaignClass(NoInitClass const & x) : BASECLASS(x) {}
		virtual ~CampaignClass(void) override;

		virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * retval) override;
		virtual HRESULT STDMETHODCALLTYPE Load(IStream * stream) override;
		virtual HRESULT STDMETHODCALLTYPE Save(IStream * stream, BOOL cleardirty) override;

		virtual RTTIType Fetch_RTTI(void) const override;
		virtual int Fetch_Object_Size(bool oldsave) const override;
		virtual void Compute_CRC(CRCEngine & crc) const override;

		virtual bool Read_INI(CCINIClass const & ini) override;

		static CampaignType From_Name(char const * name);

	public:
		/*
		 * This is the number of the game disc that this campaign's scenarios are found on. The
		 * CD handling code will demand that disc before the campaign is allowed to start.
		 */
		int CDNumber;

		/*
		 * This is the file name of the scenario that the campaign opens with. It is forced to
		 * upper case as it is read from the rules.
		 */
		char ScenarioName[_MAX_FNAME+_MAX_EXT];

		/*
		 * This is the movie that plays once the last mission of the campaign has been won.
		 */
		VQType FinalMovie;

		/*
		 * This is the campaign's title as it appears in the mission selection list. Until the
		 * rules supply one, it is borrowed from the campaign's given name.
		 */
		wchar_t Description[128]; // RA2: wide, read from Description unless DebugOnly is set
};

void Read_Battle_INI(CCINIClass const & ini);
