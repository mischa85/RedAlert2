/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <comdef.h>

/// Names and comments from TLBs

EXTERN_C const IID IID_IRTTITypeInfo;

/*
 * RTTI Type Info Interface
 */
struct __declspec(novtable)
IRTTITypeInfo : public IUnknown
{
public:

	/*
	 * Fetch the RTTI type ID number.
	 */
	virtual int STDMETHODCALLTYPE What_Am_I(void) const = 0; /// these, in theory, cannot be const because the IDL compiler

	/*
	 * Fetch unique ID number for this RTTI typed object.
	 */
	virtual int STDMETHODCALLTYPE Fetch_ID(void) const = 0; /// doesn't support const. yet they somehow were

	/*
	 * Create unique ID number for this RTTI typed object.
	 */
	virtual void STDMETHODCALLTYPE Create_ID(void) = 0;
};
