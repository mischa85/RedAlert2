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

EXTERN_C const IID IID_IGameMap;

/*
 * Map Interface.
 */
struct __declspec(novtable)
IGameMap : public IUnknown
{
public:
	/*
	 * Is location visible.
	 */
	virtual LONG STDMETHODCALLTYPE Is_Visible(CellStruct cell) = 0;
};


/*
 * IGameMap com smart pointer declaration.
 */
_COM_SMARTPTR_TYPEDEF(IGameMap, IID_IGameMap);
