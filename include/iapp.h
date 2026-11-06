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

interface ISwizzle;

EXTERN_C const IID IID_IApplication;

struct __declspec(novtable)
IApplication : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR *retval) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *retval) = 0;
	virtual HRESULT STDMETHODCALLTYPE Quit(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_ScenarioName(BSTR *retval) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_FrameCount(LONG *retval) = 0;
	virtual HRESULT STDMETHODCALLTYPE get_Swizzle_Interface(ISwizzle **pVal) = 0;
};
