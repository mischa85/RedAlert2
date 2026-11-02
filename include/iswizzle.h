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

EXTERN_C const IID IID_ISwizzle;

struct __declspec(novtable)
ISwizzle : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Swizzle(void *__RPC_FAR *pointer) = 0;
	virtual HRESULT STDMETHODCALLTYPE Fetch_Swizzle_ID(void *pointer, LONG *id) = 0;
	virtual HRESULT STDMETHODCALLTYPE Here_I_Am(LONG id, void *pointer) = 0;
	virtual HRESULT STDMETHODCALLTYPE Save_Interface(IStream *stream, IUnknown *pointer) = 0;
	virtual HRESULT STDMETHODCALLTYPE Load_Interface(IStream *stream, GUID *riid, void **pointer) = 0;
	virtual HRESULT STDMETHODCALLTYPE Get_Save_Size(int *size) = 0;

};
