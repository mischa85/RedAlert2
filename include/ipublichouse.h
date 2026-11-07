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

#include "category.hh"

#include <comdef.h>

/// Names and comments from TLBs

EXTERN_C const IID IID_IPublicHouse;

struct IHouse;

struct __declspec(novtable)
IPublicHouse : public IUnknown
{
public:
	/*
	 * Unique House ID.
	 */
	virtual LONG STDMETHODCALLTYPE ID_Number(void) = 0;

	/*
	 * House name.
	 */
	virtual BSTR STDMETHODCALLTYPE Name(void) = 0;

	/*
	 * Quantity owned by category.
	 */
	virtual LONG STDMETHODCALLTYPE Apparent_Category_Quantity(CategoryType category) = 0;

	/*
	 * Total strength available by category.
	 */
	virtual LONG STDMETHODCALLTYPE Apparent_Category_Power(CategoryType category) = 0;

	/*
	 * Center location of base.
	 */
	virtual CellStruct STDMETHODCALLTYPE Apparent_Base_Center(void) = 0;

	/*
	 * Is base visibly powered?
	 */
	virtual bool STDMETHODCALLTYPE Is_Powered(void) = 0;
};

/*
 * IPublicHouse com smart pointer declaration.
 */
_COM_SMARTPTR_TYPEDEF(IPublicHouse, IID_IPublicHouse);
