/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "app.h"

#include "_map.h"
#include "globals.h"
#include "scenario.h"
#include "swizzle.h"

ApplicationClass ApplicationInstance;


/// <summary>
/// Fetches the requested interface from the class factory.
/// The factory hands out nothing but its own creation interface -- the objects it makes
/// are reached through CreateInstance.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied, or E_NOINTERFACE if the
/// factory does not implement it.</returns>
LONG STDMETHODCALLTYPE TiberianSunClassFactory::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
	if (ppvObject == NULL) {
		return(E_POINTER);
	}

	if (riid != IID_IClassFactory && riid != IID_IUnknown) {
		return(E_NOINTERFACE);
	}

	*ppvObject = static_cast<IClassFactory *>(this);

	reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();
	return(S_OK);
}


/// <summary>
/// Creates an object of the class this factory serves.
/// This routine is the entry point a scripting client uses to reach the game, handing out
/// either the application or the game map interface.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied. Otherwise a COM error code
/// describing the failure is returned.</returns>
/// <remarks>Aggregation is not supported, so no outer unknown may be supplied.</remarks>
HRESULT STDMETHODCALLTYPE TiberianSunClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObject)
{
	if (ppvObject == NULL) {
		return(E_INVALIDARG);
	}

	*ppvObject = NULL;

	if (pUnkOuter != NULL) {
		return(CLASS_E_NOAGGREGATION);
	}

	HRESULT res = E_NOINTERFACE;

	if (riid == IID_IApplication || riid == IID_IUnknown) {
		IUnknown * iterf = &ApplicationInstance;
		if (iterf == NULL) {
			return(E_OUTOFMEMORY);
		}
		res = iterf->QueryInterface(riid, ppvObject);
		if (FAILED(res)) {
			delete iterf;
		}
	}

	if (riid == IID_IGameMap) {
		IUnknown * iterf = &Map;
		if (iterf == NULL) {
			return(E_OUTOFMEMORY);
		}
		res = iterf->QueryInterface(riid, ppvObject);
		if (FAILED(res)) {
			delete iterf;
		}
	}
	return(res);
}


/// <summary>
/// Fetches the requested interface from the application object.
/// The application serves the game map interface as well as its own, so that a client
/// only needs to get hold of the application in order to reach the map.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied, or E_NOINTERFACE if the
/// application does not serve it.</returns>
LONG STDMETHODCALLTYPE ApplicationClass::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
	if (ppvObject == NULL) {
		return(E_POINTER);
	}

	if (riid == IID_IApplication || riid == IID_IUnknown) {
		*ppvObject = (IApplication*)this;
		((IUnknown*)(*ppvObject))->AddRef();
		return(S_OK);
	}

	if (riid == IID_IGameMap) {
		*ppvObject = (IGameMap*)&Map;
		return(S_OK);
	}

	return(E_NOINTERFACE);
}


/// <summary>
/// Adds a reference to the application object.
/// The application is a global rather than a heap object, so it is never really reference
/// counted.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE ApplicationClass::AddRef(void)
{
	return(1);
}


/// <summary>
/// Releases a reference to the application object.
/// The application is a global that lives for the duration of the game, so releasing it
/// never destroys anything.
/// </summary>
/// <returns>Returns with the reference count, which is always one.</returns>
ULONG STDMETHODCALLTYPE ApplicationClass::Release(void)
{
	return(1);
}


/// <summary>
/// Fetches the application interface.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied.</returns>
HRESULT STDMETHODCALLTYPE ApplicationClass::App_Interface(IApplication **pVal)
{
	return(QueryInterface(IID_IApplication, (void **)pVal));
}


/// <summary>
/// Fetches the full name of the application.
/// This routine is used by a scripting client that wishes to display which game it is
/// talking to.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination string was supplied.</returns>
/// <remarks>The caller takes ownership of the string returned and must free it.</remarks>
HRESULT STDMETHODCALLTYPE ApplicationClass::get_FullName(BSTR *retval)
{
	if (retval == NULL) {
		return(E_POINTER);
	}

	*retval = SysAllocString(L"Tiberian Sun");

	return(S_OK);
}


/// <summary>
/// Fetches the short name of the application.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination string was supplied.</returns>
/// <remarks>The caller takes ownership of the string returned and must free it.</remarks>
HRESULT STDMETHODCALLTYPE ApplicationClass::get_Name(BSTR *retval)
{
	if (retval == NULL) {
		return(E_POINTER);
	}

	*retval = SysAllocString(L"Sun");

	return(S_OK);
}


/// <summary>
/// Fetches the application interface.
/// This is the property named alias of App_Interface, provided so that a scripting client
/// can reach the application under either name.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied.</returns>
HRESULT STDMETHODCALLTYPE ApplicationClass::Application_Interface(IApplication **pVal)
{
	return(QueryInterface(IID_IApplication, (void **)pVal));
}


/// <summary>
/// Handles a request from a client to quit the game.
/// </summary>
/// <remarks>The request is accepted, but the game is not actually shut down by it.</remarks>
HRESULT STDMETHODCALLTYPE ApplicationClass::Quit(void)
{
	return(S_OK);
}


/// <summary>
/// Fetches the name of the scenario currently being played.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination string was supplied.</returns>
/// <remarks>The caller takes ownership of the string returned and must free it.</remarks>
HRESULT STDMETHODCALLTYPE ApplicationClass::get_ScenarioName(BSTR *retval)
{
	wchar_t psz[32];

	if (retval == NULL) {
		return(E_POINTER);
	}

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, Scen->ScenarioName, -1, psz, sizeof(psz)/2);
	*retval = SysAllocString(psz);

	return(S_OK);
}


/// <summary>
/// Fetches the elapsed game time of the scenario in progress.
/// </summary>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE ApplicationClass::get_FrameCount(LONG *retval)
{
	if (retval == NULL) {
		return(E_POINTER);
	}

	*retval = Scen->ElapsedTimer();

	return(S_OK);
}


/// <summary>
/// Fetches the pointer swizzler interface.
/// This routine hands a client access to the save game pointer remapper, so that it can
/// resolve the pointers held in a saved game.
/// </summary>
/// <returns>Returns with S_OK if the interface was supplied.</returns>
HRESULT STDMETHODCALLTYPE ApplicationClass::get_Swizzle_Interface(ISwizzle **pVal)
{
	return(Swizzler.QueryInterface(IID_ISwizzle, (void**)pVal));
}


/// Unlike every other interface ID, this one is defined here in the application module.
#define INITGUID
#undef DEFINE_GUID
#include <basetyps.h>
#include "iapp_i.c"
