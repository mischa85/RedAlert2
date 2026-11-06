/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "win.h"

/*
 * Interfaces.
 */
#include "iapp.h"
#include "iswizzle.h"


class TiberianSunClassFactory : public IClassFactory
{
	public:
		virtual LONG STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObject) override;
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override {return(1);}
		virtual ULONG STDMETHODCALLTYPE Release(void) override {return(1);}

		virtual HRESULT STDMETHODCALLTYPE CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject) override;
		virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override {return(S_OK);}

};

class ApplicationClass : public IApplication
{
	public:
		virtual LONG STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObject) override;
		virtual ULONG STDMETHODCALLTYPE AddRef(void) override;
		virtual ULONG STDMETHODCALLTYPE Release(void) override;

	public:
		ApplicationClass(void) {};

		virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR *retval) override;
		virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR *retval) override;
		virtual HRESULT STDMETHODCALLTYPE Quit(void) override;
		virtual HRESULT STDMETHODCALLTYPE get_ScenarioName(BSTR *retval) override;
		virtual HRESULT STDMETHODCALLTYPE get_FrameCount(LONG *retval) override;
		virtual HRESULT STDMETHODCALLTYPE get_Swizzle_Interface(ISwizzle **pVal) override;
		virtual HRESULT STDMETHODCALLTYPE App_Interface(IApplication **pVal);
		virtual HRESULT STDMETHODCALLTYPE Application_Interface(IApplication **pVal);
};

extern ApplicationClass ApplicationInstance;
