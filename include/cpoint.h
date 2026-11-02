/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "timer.h"
#include "vector.h"

#include <comdef.h>

bool operator!=(CONNECTDATA const &left, CONNECTDATA const &right);
bool operator==(CONNECTDATA const &left, CONNECTDATA const &right);

class EnumConnectionsClass : public IEnumConnections
{
public:

	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	STDMETHOD(Next)(ULONG cConnections, CONNECTDATA * rgpcd, ULONG * pcFetched);
	STDMETHOD(Skip)(ULONG cConnections);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumConnections ** ppEnum);

	EnumConnectionsClass(DynamicVectorClass<CONNECTDATA> &list);
	EnumConnectionsClass(EnumConnectionsClass &that);

	~EnumConnectionsClass(void);

private:
	/*
	 * This is the snapshot of the connection point's sinks that the enumerator walks. A
	 * reference is held on every sink listed, so the walk survives a client unadvising
	 * while it is in progress.
	 */
	DynamicVectorClass<CONNECTDATA> List;

	/*
	 * This is the index of the next connection the enumerator will hand out, counting from
	 * the front of the snapshot.
	 */
	int ListPosition;

	/*
	 * This is the COM reference count for the enumerator. The object destroys itself as
	 * soon as the last client releases it.
	 */
	LONG RefCount;
};


class ConnectionPointClass : public IConnectionPoint
{
public:

	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	STDMETHOD(GetConnectionInterface)(IID * pIID);
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
	STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(LPENUMCONNECTIONS * ppEnum);

	ConnectionPointClass(REFIID iid, IUnknown *container);
	~ConnectionPointClass(void);

private:
	/*
	 * This is the sink interface that this connection point publishes. A client advising a
	 * sink must supply one that answers to this identifier, or the connection is refused.
	 */
	IID InterfaceID;

	/*
	 * This is the COM reference count for the connection point. The object destroys itself
	 * as soon as the last client releases it.
	 */
	LONG RefCount;

	/*
	 * Pointer to the object that publishes this connection point. A reference is held on it
	 * for as long as the connection point lives, and it is the object a client is handed
	 * when it asks which container the connection point belongs to.
	 */
	IUnknown * Object;

	/*
	 * These are the client sinks currently advised on this connection point, each paired
	 * with the cookie handed out when it was connected. A client breaks its connection by
	 * quoting that cookie back.
	 */
	DynamicVectorClass<CONNECTDATA> List;
};


class EnumConnectionPointsClass : public IEnumConnectionPoints
{
public:

	STDMETHOD(QueryInterface)(REFIID, void **);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	STDMETHOD(Next)(ULONG cConnections, IConnectionPoint ** rgpcn, ULONG * pcFetched);
	STDMETHOD(Skip)(ULONG cConnections);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumConnectionPoints ** ppEnum);

	EnumConnectionPointsClass(DynamicVectorClass<IConnectionPoint *> &list);
	EnumConnectionPointsClass(EnumConnectionPointsClass &that);
	~EnumConnectionPointsClass(void);

private:
	/*
	 * This is the index of the next connection point the enumerator will hand out,
	 * counting from the front of the snapshot.
	 */
	int ListPosition;

	/*
	 * This is the COM reference count for the enumerator. The object destroys itself as
	 * soon as the last client releases it.
	 */
	LONG RefCount;

	/*
	 * This is the snapshot of the container's connection points that the enumerator walks.
	 * A reference is held on every point listed, so the walk survives the container
	 * changing its mind while the client is still working through it.
	 */
	DynamicVectorClass<IConnectionPoint *> List;
};
