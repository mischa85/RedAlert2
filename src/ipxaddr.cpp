/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/IPXADDR.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXADDR.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : December 19, 1994   [BR]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   IPXAddressClass::IPXAddressClass -- class constructor                 *
 *   IPXAddressClass::IPXAddressClass -- class constructor form 2          *
 *   IPXAddressClass::IPXAddressClass -- class constructor form 3          *
 *   IPXAddressClass::Set_Address -- sets the IPX address values           *
 *   IPXAddressClass::Set_Address -- sets the IPX values from a header     *
 *   IPXAddressClass::Get_Address -- retrieves the IPX address values      *
 *   IPXAddressClass::Get_Address -- copies address into an IPX header     *
 *   IPXAddressClass::Is_Broadcast -- tells if this is a broadcast address *
 *   IPXAddressClass::operator== -- overloaded comparison operator         *
 *   IPXAddressClass::operator!= -- overloaded comparison operator         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "ipxaddr.h"

#include "wsproto.h"

#include <cstdio>
#include <cstring>


/***************************************************************************
 * IPXAddressClass::IPXAddressClass -- class constructor                   *
 *                                                                         *
 * This default constructor generates a broadcast address.                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      net      Network Number for this address                           *
 *      node      Node Address for this address                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
IPXAddressClass::IPXAddressClass(void)
{
	NetworkNumber[0] = 0xff;
	NetworkNumber[1] = 0xff;
	NetworkNumber[2] = 0xff;
	NetworkNumber[3] = 0xff;
	NodeAddress[0] = 0xff;
	NodeAddress[1] = 0xff;
	NodeAddress[2] = 0xff;
	NodeAddress[3] = 0xff;
	NodeAddress[4] = 0xff;
	NodeAddress[5] = 0xff;

}	/* end of IPXAddressClass */


/*
**	RA2: an address of either protocol; a UDP address keeps the port where IPX keeps the network number.
*/
IPXAddressClass::IPXAddressClass(NetNumType net, NetNodeType node, unsigned short port)
{
	memcpy(NodeAddress, node, 6);
	if (AddressProtocol == 1) {
		memcpy(NetworkNumber, net, 4);
	} else {
		*(unsigned long *)NetworkNumber = 0;
		*(unsigned short *)NetworkNumber = port;
	}
}


/*
**	RA2: sets an address of either protocol.
*/
void IPXAddressClass::Set_Address(NetNumType net, NetNodeType node, unsigned short port)
{
	memcpy(NodeAddress, node, 6);
	if (AddressProtocol == 1) {
		memcpy(NetworkNumber, net, 4);
	} else {
		*(unsigned long *)NetworkNumber = 0;
		*(unsigned short *)NetworkNumber = port;
	}
}


/*
**	RA2: gets an address of either protocol.
*/
void IPXAddressClass::Get_Address(NetNumType net, NetNodeType node, unsigned short * port)
{
	memcpy(node, NodeAddress, 6);
	if (AddressProtocol == 1) {
		memcpy(net, NetworkNumber, 4);
	} else {
		*port = *(unsigned short *)NetworkNumber;
	}
}


/***************************************************************************
 * IPXAddressClass::Get_Address -- retrieves the IPX address values        *
 *                                                                         *
 * INPUT:                                                                  *
 *      net      Network Number for this address                           *
 *      node      Node Address for this address                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXAddressClass::Get_Address(NetNumType net, NetNodeType node)
{
	memcpy(node,NodeAddress,6); // RA2: the node first
	memcpy(net,NetworkNumber,4);

}	/* end of Get_Address */


/*
**	RA2: gets the node address alone.
*/
void IPXAddressClass::Get_Node(NetNodeType node)
{
	memcpy(node, NodeAddress, 6);
}


/*
**	RA2: sets the node address alone.
*/
void IPXAddressClass::Set_Node(NetNodeType node)
{
	memcpy(NodeAddress, node, 6);
}


/***************************************************************************
 * IPXAddressClass::Is_Broadcast -- tells if this is a broadcast address   *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXAddressClass::Is_Broadcast(void)
{
	if (AddressProtocol == 1) { // RA2: IPX
		if (	NetworkNumber[0] == 0xff &&
				NetworkNumber[1] == 0xff &&
				NetworkNumber[2] == 0xff &&
				NetworkNumber[3] == 0xff &&
				NodeAddress[0] == 0xff &&
				NodeAddress[1] == 0xff &&
				NodeAddress[2] == 0xff &&
				NodeAddress[3] == 0xff &&
				NodeAddress[4] == 0xff &&
				NodeAddress[5] == 0xff) {
			return(1);
		}
		else {
			return(0);
		}
	}

	/*
	**	RA2: a UDP address has no network number; only its node address counts.
	*/
	if (	NodeAddress[0] == 0xff &&
			NodeAddress[1] == 0xff &&
			NodeAddress[2] == 0xff &&
			NodeAddress[3] == 0xff &&
			NodeAddress[4] == 0xff &&
			NodeAddress[5] == 0xff) {
		return(1);
	}
	return(0);

}	/* end of Is_Broadcast */


/***************************************************************************
 * IPXAddressClass::operator== -- overloaded comparison operator           *
 *                                                                         *
 * Since, if NETX isn't running, the network number on a received packet   *
 * can be bogus (all 0's), only the node address is used for comparison    *
 * purposes here.                                                          *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = not equal, 1 = equal                                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator == (IPXAddressClass & addr)
{
	/*
	**	RA2: UDP addresses compare the IP address and the port.
	*/
	if (AddressProtocol == 2) {
		if (memcmp(NodeAddress, addr.NodeAddress, 4) == 0 && memcmp(NetworkNumber, addr.NetworkNumber, 2) == 0) {
			return(1);
		}
		return(0);
	}

	//------------------------------------------------------------------------
	// If either Network Number is all 0's (which can happen if the system is
	// not running NETX), compare only the Node Addresses.
	//------------------------------------------------------------------------
	if ( (NetworkNumber[0]==0 &&
			NetworkNumber[1]==0 &&
			NetworkNumber[2]==0 &&
			NetworkNumber[3]==0) ||
		  (addr.NetworkNumber[0]==0 &&
			addr.NetworkNumber[1]==0 &&
			addr.NetworkNumber[2]==0 &&
			addr.NetworkNumber[3]==0) ) {

		if (memcmp(NodeAddress,addr.NodeAddress,6)==0) {
			return(1);
		}
		else {
			return(0);
		}

	}
	//------------------------------------------------------------------------
	// Otherwise, compare both the Network Numbers and Node Addresses
	//------------------------------------------------------------------------
	else {
		if (memcmp(NodeAddress,addr.NodeAddress,6)==0 &&
			memcmp(NetworkNumber,addr.NetworkNumber,4)==0) {
			return(1);
		}
		else {
			return(0);
		}
	}

}	/* end of operator== */


/*
**	RA2: compares the node addresses alone (the IP address for UDP).
*/
bool IPXAddressClass::Is_Same_Node(IPXAddressClass & addr)
{
	return(memcmp(NodeAddress, addr.NodeAddress, (AddressProtocol == 2) ? 4 : 6) == 0);
}


/***************************************************************************
 * IPXAddressClass::operator!= -- overloaded comparison operator           *
 *                                                                         *
 * Since, if NETX isn't running, the network number on a received packet   *
 * can be bogus (all 0's), only the node address is used for comparison    *
 * purposes here.                                                          *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = equal, 1 = not equal                                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator != (IPXAddressClass & addr)
{
	/*
	**	RA2: UDP addresses compare the IP address and the port.
	*/
	if (AddressProtocol == 2) {
		if (memcmp(NodeAddress, addr.NodeAddress, 4) == 0 && memcmp(NetworkNumber, addr.NetworkNumber, 2) == 0) {
			return(0);
		}
		return(1);
	}

	//------------------------------------------------------------------------
	// If either Network Number is all 0's (which can happen if the system is
	// not running NETX), compare only the Node Addresses.
	//------------------------------------------------------------------------
	if ( (NetworkNumber[0]==0 &&
			NetworkNumber[1]==0 &&
			NetworkNumber[2]==0 &&
			NetworkNumber[3]==0) ||
		  (addr.NetworkNumber[0]==0 &&
			addr.NetworkNumber[1]==0 &&
			addr.NetworkNumber[2]==0 &&
			addr.NetworkNumber[3]==0) ) {

		if (memcmp(NodeAddress,addr.NodeAddress,6)==0) {
			return(0);
		}
		else {
			return(1);
		}
	}
	//------------------------------------------------------------------------
	// Otherwise, compare both the Network Numbers and Node Addresses
	//------------------------------------------------------------------------
	else {
		if (memcmp(NodeAddress,addr.NodeAddress,6)==0 &&
			memcmp(NetworkNumber,addr.NetworkNumber,4)==0) {
			return(0);
		}
		else {
			return(1);
		}
	}

}	/* end of operator!= */


/***************************************************************************
 * IPXAddressClass::operator > -- overloaded comparison operator           *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      TRUE = greater, FALSE = not                                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator > (IPXAddressClass & addr)
{
	return(memcmp(this, &addr, 10) > 0);

}	/* end of operator> */


/***************************************************************************
 * IPXAddressClass::operator < -- overloaded comparison operator           *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      TRUE = less, FALSE = not                                           *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator < (IPXAddressClass & addr)
{
	return(memcmp(this, &addr, 10) < 0);

}	/* end of operator< */


/***************************************************************************
 * IPXAddressClass::operator >= -- overloaded comparison operator          *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      TRUE = greater or equal, FALSE = not                               *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator >= (IPXAddressClass & addr)
{
	return(memcmp(this, &addr, 10) >= 0);

}	/* end of operator>= */


/***************************************************************************
 * IPXAddressClass::operator <= -- overloaded comparison operator          *
 *                                                                         *
 * INPUT:                                                                  *
 *      addr      address to compare to                                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      TRUE = less or equal, FALSE = not                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXAddressClass::operator <= (IPXAddressClass & addr)
{
	return(memcmp(this, &addr, 10) <= 0);

}	/* end of operator<= */


/// <summary>
/// Converts this address into a displayable string.
/// This routine is used by the debug output when a network address has to be shown in a
/// human readable form. The network number and the node address are printed as pairs of
/// hexadecimal digits, separated by a colon.
/// RA2: a UDP address prints as its IP address and port instead.
/// </summary>
/// <param name="address">RA2: The address to convert; NULL converts this one.</param>
/// <returns>Returns with a pointer to the formatted address text.</returns>
/// <remarks>The text is built in a shared static buffer, so it only survives until the next
/// call to this routine.</remarks>
const char *IPXAddressClass::As_String(IPXAddressClass * address)
{
	static char _addr_str[128];

	NetNumType num;
	NetNodeType node;

	if (!address) address = this; // RA2

	memcpy(node,address->NodeAddress,6);

	if (AddressProtocol == 1) {
		memcpy(num,address->NetworkNumber,4);
		sprintf (_addr_str, "%02X %02X %02X %02X : %02X %02X %02X %02X %02X %02X",
				 num[0], num[1], num[2], num[3], node[0], node[1], node[2], node[3], node[4], node[5]
		);
	} else {
		/*
		**	RA2: a UDP address prints as the IP address and the port.
		*/
		sprintf(_addr_str, "%d.%d.%d.%d ; %d", node[0], node[1], node[2], node[3], *(unsigned short *)NetworkNumber);
	}

	return(_addr_str);
}


/*
**	RA2: sets the protocol the addresses are for.
*/
void Set_Address_Protocol(int protocol)
{
	AddressProtocol = protocol;
}


/*
**	RA2: sets the port of a UDP address.
*/
void IPXAddressClass::Set_Port(unsigned short port)
{
	if (AddressProtocol == 2) {
		*(unsigned short *)NetworkNumber = port;
	}
}

/************************** end of ipxaddr.cpp *****************************/
