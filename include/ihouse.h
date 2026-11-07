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

EXTERN_C const IID IID_IHouse;

struct IApplication;

struct __declspec(novtable)
IHouse : public IUnknown
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
	 * Returns IApplication interface.
	 */
	virtual IApplication *STDMETHODCALLTYPE Get_Application(void) = 0;

	/*
	 * Available money.
	 */
	virtual LONG STDMETHODCALLTYPE Available_Money(void) = 0;

	/*
	 * Available tiberium storage.
	 */
	virtual LONG STDMETHODCALLTYPE Available_Storage(void) = 0;

	/*
	**	Current power output.
	*/
	virtual LONG STDMETHODCALLTYPE Power_Output(void) = 0;

	/*
	 * Current power drain.
	 */
	virtual LONG STDMETHODCALLTYPE Power_Drain(void) = 0;

	/*
	 * Quantity owned by category.
	 */
	virtual LONG STDMETHODCALLTYPE Category_Quantity(CategoryType category) = 0;

	/*
	 * Total strength available by category.
	 */
	virtual LONG STDMETHODCALLTYPE Category_Power(CategoryType category) = 0;

	/*
	 * Center location of base.
	 */
	virtual CellStruct STDMETHODCALLTYPE Base_Center(void) = 0;

	/*
	 * Sell all buildings.
	 */
	virtual HRESULT STDMETHODCALLTYPE Fire_Sale(void) = 0;

	/*
	 * All units to attack enemy.
	 */
	virtual HRESULT STDMETHODCALLTYPE All_To_Hunt(void) = 0;
};
