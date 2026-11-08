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


/***************************************************************************
 * IPXAddressClass::IPXAddressClass -- class constructor form 2            *
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
IPXAddressClass::IPXAddressClass(NetNumType net, NetNodeType node)
{
	memcpy(NetworkNumber, net, 4);
	memcpy(NodeAddress, node, 6);

}	/* end of IPXAddressClass */


/***************************************************************************
 * IPXAddressClass::IPXAddressClass -- class constructor form 3            *
 *                                                                         *
 * This form of the constructor takes an IPX header as an argument.  It    *
 * extracts the address from the Source address fields in the header.      *
 *                                                                         *
 * INPUT:                                                                  *
 *      header   Header from which to extract the address                  *
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
IPXAddressClass::IPXAddressClass(IPXHeaderType *header)
{
	memcpy(NetworkNumber,header->SourceNetworkNumber,4);
	memcpy(NodeAddress,header->SourceNetworkNode,6);

}	/* end of IPXAddressClass */


/***************************************************************************
 * IPXAddressClass::Set_Address -- sets the IPX address values             *
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
void IPXAddressClass::Set_Address(NetNumType net, NetNodeType node)
{
	memcpy(NetworkNumber,net,4);
	memcpy(NodeAddress,node,6);

}	/* end of Set_Address */


/***************************************************************************
 * IPXAddressClass::Set_Address -- sets the IPX values from a header       *
 *                                                                         *
 * This routine extracts the source addresses from the given IPX header.   *
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
void IPXAddressClass::Set_Address(IPXHeaderType *header)
{
	ProtocolEnum protocol = PROTOCOL_IPX;
	if ( PacketTransport ) protocol = PacketTransport->Get_Protocol();

	switch ( protocol ) {

		case PROTOCOL_IPX:
			memcpy(NetworkNumber,header->SourceNetworkNumber,4);
			memcpy(NodeAddress,header->SourceNetworkNode,6);
			break;

		case PROTOCOL_UDP:
			unsigned char *addr = (unsigned char*) header;
			memset (NodeAddress, 0, 6);
			memcpy (NodeAddress, addr, 4);
			memset (NetworkNumber, 0, 4);
			break;
	}
}	/* end of Set_Address */


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
	memcpy(net,NetworkNumber,4);
	memcpy(node,NodeAddress,6);

}	/* end of Get_Address */


/***************************************************************************
 * IPXAddressClass::Get_Address -- copies address into an IPX header       *
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
void IPXAddressClass::Get_Address(IPXHeaderType *header)
{
	memcpy(header->DestNetworkNumber,NetworkNumber,4);
	memcpy(header->DestNetworkNode,NodeAddress,6);

}	/* end of Get_Address */


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
/// </summary>
/// <returns>Returns with a pointer to the formatted address text.</returns>
/// <remarks>The text is built in a shared static buffer, so it only survives until the next
/// call to this routine.</remarks>
const char *IPXAddressClass::As_String(void)
{
	static char _addr_str[128];

	NetNumType num;
	NetNodeType node;

	memcpy(num,NetworkNumber,4);
	memcpy(node,NodeAddress,6);

	sprintf (_addr_str, "%02X %02X %02X %02X : %02X %02X %02X %02X %02X %02X",
			 num[0], num[1], num[2], num[3], node[0], node[1], node[2], node[3], node[4], node[5]
	);

	return(_addr_str);
}

/************************** end of ipxaddr.cpp *****************************/
