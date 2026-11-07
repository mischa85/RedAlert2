/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "comtypes.h"

#include <comdef.h>

/// Names and comments from TLBs

EXTERN_C const IID IID_IAIHouse;

struct IHouse;

struct __declspec(novtable)
IAIHouse : public IUnknown
{
public:
	/*
	 * Links house to an advisor object.
	 */
	virtual LONG STDMETHODCALLTYPE Link_House(IHouse * house) = 0;

	/*
	 * AI processing entry function.
	 */
	virtual LONG STDMETHODCALLTYPE AI(LONG * framedelay) = 0;
};

/*
 * IAIHouse com smart pointer declaration.
 */
_COM_SMARTPTR_TYPEDEF(IAIHouse, IID_IAIHouse);
