/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "cpoint.h"

#include "globals.h"
#include "scenario.h"
#include "vector.h"

#include "win.h"

extern ULONG COMRefCount;


/// <summary>
/// Determines if two connections differ.
/// This routine is required by the vector template that holds the connection list.
/// </summary>
/// <returns>bool; Does either the cookie or the sink pointer differ?</returns>
bool operator!=(CONNECTDATA const &left, CONNECTDATA const &right)
{
	return(left.dwCookie != right.dwCookie || left.pUnk != right.pUnk);
}


/// <summary>
/// Determines if two connections are the same connection.
/// This routine lets the connection list search for a sink by value.
/// </summary>
/// <returns>bool; Do both the cookie and the sink pointer match?</returns>
bool operator==(CONNECTDATA const &left, CONNECTDATA const &right)
{
	return(left.dwCookie == right.dwCookie && left.pUnk == right.pUnk);
}


/// <summary>
/// Fetches an interface pointer from the enumerator.
/// This routine serves IEnumConnections and IUnknown. A reference is taken on the
/// enumerator before the pointer is handed back.
/// </summary>
/// <param name="riid">The interface identifier being asked for.</param>
/// <param name="ppv">Pointer to fill in with the interface.</param>
/// <returns>Returns with S_OK if the interface was supplied, or E_NOINTERFACE if it is not
/// supported.</returns>
HRESULT EnumConnectionsClass::QueryInterface(REFIID riid, LPVOID *ppv)
{
	if (ppv == NULL) {
		return(E_POINTER);
	}

	*ppv = NULL;

	if (riid == IID_IUnknown) {
		*ppv = (IUnknown *)(this);
	}
	if (riid == IID_IEnumConnections) {
		*ppv = (IEnumConnections *)this;
	}


	if (NULL != *ppv) {
		this->AddRef();
		return(S_OK);
	}

	return(E_NOINTERFACE);
}


/// <summary>
/// Adds a reference to the enumerator.
/// </summary>
/// <returns>Returns with the new reference count.</returns>
ULONG EnumConnectionsClass::AddRef(void)
{
	COMRefCount++;
	return(InterlockedIncrement(&RefCount));
}


/// <summary>
/// Releases a reference on the enumerator.
/// The enumerator deletes itself once the last reference goes away.
/// </summary>
/// <returns>Returns with the reference count that remains.</returns>
ULONG EnumConnectionsClass::Release(void)
{
	COMRefCount--;

	LONG count = InterlockedDecrement(&RefCount);
	if (count == 0) {
		delete this;
	}

	return(count);
}


/// <summary>
/// Fetches the next batch of connections.
/// This routine is called by a client walking the sinks advised on a connection point.
/// Every sink handed back has been referenced on the client's behalf, so the client must
/// release them when it is done.
/// </summary>
/// <param name="cConnections">The number of connections asked for.</param>
/// <param name="rgpcd">Buffer to fill in with the connections fetched.</param>
/// <param name="pcFetched">Pointer to fill in with the number actually fetched. This may be
/// NULL only when a single connection was asked for.</param>
/// <returns>Returns with S_OK if any were fetched, or E_FAIL once the enumerator has run
/// dry.</returns>
HRESULT EnumConnectionsClass::Next(ULONG cConnections, CONNECTDATA * rgpcd, ULONG * pcFetched)
{
	if (rgpcd == NULL) {
		return(E_POINTER);
	}

	if (cConnections > 1 && pcFetched == NULL) {
		return(E_INVALIDARG);
	}

	if (pcFetched != NULL) {
		*pcFetched = 0;
	}

	int count = 0;
	for (unsigned int i = 0; i < cConnections; i++) {
		if (ListPosition >= List.Count()) {
			break;
		}

		List[ListPosition].pUnk->AddRef();
		*rgpcd++ = List[ListPosition++];
		count++;
	}

	if (pcFetched != NULL) {
		*pcFetched = count;
	}

	return(count ? S_OK : E_FAIL);
}


/// <summary>
/// Advances the enumerator past a run of connections.
/// </summary>
/// <param name="cConnections">The number of connections to step over.</param>
HRESULT EnumConnectionsClass::Skip(ULONG cConnections)
{
	ListPosition += cConnections;
	return(S_OK);
}


/// <summary>
/// Rewinds the enumerator back to the first connection.
/// </summary>
HRESULT EnumConnectionsClass::Reset(void)
{
	ListPosition = 0;
	return(S_OK);
}


/// <summary>
/// Creates a duplicate of this enumerator.
/// This routine hands the client an independent enumerator positioned exactly where this
/// one currently sits. The client owns the enumerator returned and must release it.
/// </summary>
/// <param name="ppEnum">Pointer to fill in with the clone.</param>
/// <returns>Returns with S_OK if the clone was created, or an error code otherwise.</returns>
HRESULT EnumConnectionsClass::Clone(IEnumConnections** ppEnum)
{
	if (ppEnum == NULL) {
		return(E_POINTER);
	}

	EnumConnectionsClass *enumerator = new EnumConnectionsClass(*this);

	if (enumerator == NULL) {
		return(E_OUTOFMEMORY);
	}

	enumerator->RefCount = 0;
	enumerator->AddRef();

	*ppEnum = enumerator;

	return(S_OK);
}


/// <summary>
/// Constructs an enumerator over the connections specified.
/// This routine is used by a connection point when a client asks to walk the sinks advised
/// on it. The list is snapshotted and a reference is taken on every sink in it, so the
/// enumerator survives sinks unadvising while the walk is in progress.
/// </summary>
/// <param name="list">The connections to enumerate.</param>
EnumConnectionsClass::EnumConnectionsClass(DynamicVectorClass<CONNECTDATA> &list) :
	List(list),
	ListPosition(0),
	RefCount(0)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i].pUnk->AddRef();
	}
}


/// <summary>
/// Copy constructs a connection enumerator.
/// This routine is used by Clone to hand out an independent enumerator that starts at the
/// same place in the same snapshot. A reference is taken on every sink listed.
/// </summary>
/// <param name="that">The enumerator to copy the list and position from.</param>
EnumConnectionsClass::EnumConnectionsClass(EnumConnectionsClass &that) :
	List(that.List),
	ListPosition(that.ListPosition),
	RefCount(0)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i].pUnk->AddRef();
	}
}


/// <summary>
/// Destroys the connection enumerator.
/// This routine releases the reference this enumerator took on every sink in the snapshot
/// it was handed.
/// </summary>
EnumConnectionsClass::~EnumConnectionsClass(void)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i].pUnk->Release();
	}

	List.Clear();
}


/// <summary>
/// Fetches an interface pointer from the connection point.
/// This routine serves IConnectionPoint and IUnknown. A reference is taken on the
/// connection point before the pointer is handed back.
/// </summary>
/// <param name="riid">The interface identifier being asked for.</param>
/// <param name="ppv">Pointer to fill in with the interface.</param>
/// <returns>Returns with S_OK if the interface was supplied, or E_NOINTERFACE if it is not
/// supported.</returns>
HRESULT ConnectionPointClass::QueryInterface(REFIID riid, void** ppv)
{
	if (ppv == NULL) {
		return(E_POINTER);
	}

	IUnknown * iface = NULL;


	if (riid == IID_IConnectionPoint || riid == IID_IUnknown) {
		iface = (IEnumConnections *)this;
	}

	*ppv = iface;

	if (iface != NULL) {
		iface->AddRef();
		return(S_OK);
	}

	return(E_NOINTERFACE);
}


/// <summary>
/// Adds a reference to the connection point.
/// </summary>
/// <returns>Returns with the new reference count.</returns>
ULONG ConnectionPointClass::AddRef(void)
{
	COMRefCount++;
	return(InterlockedIncrement(&RefCount));
}


/// <summary>
/// Releases a reference on the connection point.
/// The connection point deletes itself once the last reference goes away.
/// </summary>
/// <returns>Returns with the reference count that remains.</returns>
ULONG ConnectionPointClass::Release(void)
{
	COMRefCount--;

	LONG count = InterlockedDecrement(&RefCount);
	if (count == 0) {
		delete this;
	}

	return(count);
}


/// <summary>
/// Fetches the sink interface this connection point serves.
/// This routine lets a client discover what kind of sink it is expected to advise.
/// </summary>
/// <param name="pIID">Pointer to fill in with the interface identifier.</param>
/// <returns>Returns with S_OK, or E_POINTER if there was nowhere to put the answer.</returns>
HRESULT ConnectionPointClass::GetConnectionInterface(IID *pIID)
{
	if (pIID == NULL) {
		return(E_POINTER);
	}

	*pIID = InterfaceID;

	return(S_OK);
}


/// <summary>
/// Fetches the container that publishes this connection point.
/// </summary>
/// <param name="ppCPC">Pointer to fill in with the container.</param>
/// <returns>Returns with S_OK if the container was supplied, or E_NOINTERFACE if the
/// publishing object is not a connection point container after all.</returns>
HRESULT ConnectionPointClass::GetConnectionPointContainer(IConnectionPointContainer** ppCPC)
{
	return(Object->QueryInterface(IID_IConnectionPointContainer, (void**)ppCPC));
}


/// <summary>
/// Connects a client sink to this connection point.
/// This routine is called by a client that wants to be notified of the events this
/// connection point publishes. The sink is asked for the connection interface and held
/// until the client unadvises it.
/// </summary>
/// <param name="pUnkSink">The client sink to connect.</param>
/// <param name="pdwCookie">Pointer to fill in with the cookie identifying the new
/// connection.</param>
/// <returns>Returns with S_OK if the sink was connected, or a connection error code
/// otherwise.</returns>
HRESULT ConnectionPointClass::Advise(IUnknown * pUnkSink, DWORD * pdwCookie)
{
	if (pUnkSink != NULL && pdwCookie != NULL) {

		CONNECTDATA data;

		data.dwCookie = Scen->Get_Unique_ID();
		if (FAILED(pUnkSink->QueryInterface(InterfaceID, (void **)&data.pUnk))) {
			return(CONNECT_E_CANNOTCONNECT);
		}

		if (!List.Add(data)) {
			return(CONNECT_E_ADVISELIMIT);
		}

		*pdwCookie = data.dwCookie;
		return(S_OK);
	}

	return(E_POINTER);
}


/// <summary>
/// Breaks the connection identified by the cookie specified.
/// This routine is called by a client that no longer wants its sink notified.
/// </summary>
/// <param name="dwCookie">The cookie Advise handed out for the sink.</param>
/// <returns>Returns with S_OK if the connection was broken, or CONNECT_E_NOCONNECTION if
/// the cookie matches nothing.</returns>
HRESULT ConnectionPointClass::Unadvise(DWORD dwCookie)
{
	for (int i = 0; i < List.Count(); i++) {
		if (List[i].dwCookie == dwCookie) {
			List.Delete_Index(i);
			return(S_OK);
		}
	}

	return(CONNECT_E_NOCONNECTION);
}


/// <summary>
/// Creates an enumerator over the sinks advised on this connection point.
/// This routine is used by a client that wants to walk the connections currently
/// established. The client owns the enumerator returned and must release it.
/// </summary>
/// <param name="ppEnum">Pointer to fill in with the enumerator.</param>
/// <returns>Returns with S_OK if the enumerator was created, or an error code
/// otherwise.</returns>
HRESULT ConnectionPointClass::EnumConnections(IEnumConnections** ppEnum)
{
	if (ppEnum == NULL) {
		return(E_POINTER);
	}

	EnumConnectionsClass * enumerator = new EnumConnectionsClass(List);

	if (enumerator == NULL) {
		return(E_OUTOFMEMORY);
	}

	return(enumerator->QueryInterface(IID_IEnumConnections, (void**)ppEnum));
}


/// <summary>
/// Constructs a connection point for the sink interface specified.
/// This routine is used by an object that publishes events to create the connection point
/// its clients will advise their sinks on. A reference is taken on the publishing object.
/// </summary>
/// <param name="riid">The sink interface this connection point serves.</param>
/// <param name="obj">The object publishing this connection point.</param>
ConnectionPointClass::ConnectionPointClass(REFIID riid, IUnknown *obj) :
	InterfaceID(riid),
	Object(obj),
	RefCount(0)
{
	obj->AddRef();
}


/// <summary>
/// Destroys the connection point.
/// This routine drops the reference held on the object that published this connection
/// point and discards any sinks still advised on it.
/// </summary>
ConnectionPointClass::~ConnectionPointClass(void)
{
	Object->Release();
	List.Clear();
}


/// <summary>
/// Fetches an interface pointer from the enumerator.
/// This routine serves IEnumConnectionPoints and IUnknown. A reference is taken on the
/// enumerator before the pointer is handed back.
/// </summary>
/// <param name="riid">The interface identifier being asked for.</param>
/// <param name="ppv">Pointer to fill in with the interface.</param>
/// <returns>Returns with S_OK if the interface was supplied, or E_NOINTERFACE if it is not
/// supported.</returns>
HRESULT EnumConnectionPointsClass::QueryInterface(REFIID riid, LPVOID *ppv)
{
	if (ppv == NULL) {
		return(E_POINTER);
	}

	IUnknown * iface = NULL;

	if (riid == IID_IEnumConnectionPoints || riid == IID_IUnknown) {
		iface = (IEnumConnectionPoints *)this;
	}

	*ppv = iface;

	if (iface != NULL) {
		this->AddRef();
		return(S_OK);
	}

	return(E_NOINTERFACE);
}


/// <summary>
/// Adds a reference to the enumerator.
/// </summary>
/// <returns>Returns with the new reference count.</returns>
ULONG EnumConnectionPointsClass::AddRef(void)
{
	COMRefCount++;
	return(InterlockedIncrement(&RefCount));
}


/// <summary>
/// Releases a reference on the enumerator.
/// The enumerator deletes itself once the last reference goes away.
/// </summary>
/// <returns>Returns with the reference count that remains.</returns>
ULONG EnumConnectionPointsClass::Release(void)
{
	COMRefCount--;

	LONG count = InterlockedDecrement(&RefCount);
	if (count == 0) {
		delete this;
	}

	return(count);
}


/// <summary>
/// Fetches the next batch of connection points.
/// This routine is called by a client walking the connection points a container supports.
/// Every connection point handed back has been referenced on the client's behalf, so the
/// client must release them when it is done.
/// </summary>
/// <param name="cConnections">The number of connection points asked for.</param>
/// <param name="rgpcn">Buffer to fill in with the connection points fetched.</param>
/// <param name="pcFetched">Pointer to fill in with the number actually fetched. This may be
/// NULL only when a single connection point was asked for.</param>
/// <returns>Returns with S_OK if any were fetched, or E_FAIL once the enumerator has run
/// dry.</returns>
HRESULT EnumConnectionPointsClass::Next(ULONG cConnections, IConnectionPoint ** rgpcn, ULONG * pcFetched)
{
	if (rgpcn == NULL) {
		return(E_POINTER);
	}

	if (cConnections > 1 && pcFetched == NULL) {
		return(E_POINTER);
	}

	if (pcFetched != NULL) {
		*pcFetched = 0;
	}

	int count = 0;
	for (unsigned int i = 0; i < cConnections; i++) {
		if (ListPosition >= List.Count()) {
			break;
		}

		List[ListPosition]->AddRef();
		*rgpcn++ = List[ListPosition++];
		count++;
	}

	if (pcFetched != NULL) {
		*pcFetched = count;
	}

	return(count ? S_OK : E_FAIL);
}


/// <summary>
/// Advances the enumerator past a run of connection points.
/// </summary>
/// <param name="cConnections">The number of connection points to step over.</param>
HRESULT EnumConnectionPointsClass::Skip(ULONG cConnections)
{
	ListPosition += cConnections;
	return(S_OK);
}


/// <summary>
/// Rewinds the enumerator back to the first connection point.
/// </summary>
HRESULT EnumConnectionPointsClass::Reset(void)
{
	ListPosition = 0;
	return(S_OK);
}


/// <summary>
/// Creates a duplicate of this enumerator.
/// This routine hands the client an independent enumerator positioned exactly where this
/// one currently sits. The client owns the enumerator returned and must release it.
/// </summary>
/// <param name="ppEnum">Pointer to fill in with the clone.</param>
/// <returns>Returns with S_OK if the clone was created, or an error code otherwise.</returns>
HRESULT EnumConnectionPointsClass::Clone(IEnumConnectionPoints** ppEnum)
{
	if (ppEnum == NULL) {
		return(E_POINTER);
	}

	EnumConnectionPointsClass *enumerator = new EnumConnectionPointsClass(*this);

	if (enumerator == NULL) {
		return(E_OUTOFMEMORY);
	}

	enumerator->AddRef();

	*ppEnum = enumerator;

	return(S_OK);
}


/// <summary>
/// Constructs an enumerator over the connection points specified.
/// This routine is used when a client asks a container to walk the connection points it
/// supports. The list is snapshotted and a reference is taken on every connection point in
/// it, so the enumerator survives the container changing its mind afterwards.
/// </summary>
/// <param name="list">The connection points to enumerate.</param>
EnumConnectionPointsClass::EnumConnectionPointsClass(DynamicVectorClass<IConnectionPoint *> &list) :
	ListPosition(0),
	RefCount(0),
	List(list)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i]->AddRef();
	}
}


/// <summary>
/// Copy constructs a connection point enumerator.
/// This routine is used by Clone to hand out an independent enumerator that starts at the
/// same place in the same snapshot. A reference is taken on every connection point listed.
/// </summary>
/// <param name="that">The enumerator to copy the list and position from.</param>
EnumConnectionPointsClass::EnumConnectionPointsClass(EnumConnectionPointsClass &that) :
	ListPosition(that.ListPosition),
	RefCount(0),
	List(that.List)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i]->AddRef();
	}
}


/// <summary>
/// Destroys the connection point enumerator.
/// This routine releases the reference this enumerator took on every connection point in
/// the snapshot it was handed.
/// </summary>
EnumConnectionPointsClass::~EnumConnectionPointsClass(void)
{
	for (int i = 0; i < List.Count(); i++) {
		List[i]->Release();
	}

	List.Clear();
}
