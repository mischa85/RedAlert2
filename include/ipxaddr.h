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

/* $Header: /CounterStrike/IPXADDR.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXADDR.H                                *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : December 19, 1994   [BR]                 *
 *                                                                         *
 * This class is useful for any IPX-related code.  It's just a utility     *
 * to help manage those annoying IPX address fields.  This class lets you  *
 * compare addresses, copy addresses to & from the IPX header, etc.        *
 *                                                                         *
 * The class has no virtual functions, so you can treat this class just    *
 * like a data structure; it can be loaded & saved, and even transmitted   *
 * across the net.                                                         *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "ipx.h" // for NetNumType & NetNodeType


/*
***************************** Class Declaration *****************************
*/
class IPXAddressClass
{
	/*
	---------------------------- Public Interface ----------------------------
	*/
	public:
		/*.....................................................................
		Constructors:
		.....................................................................*/
		IPXAddressClass(void);
		IPXAddressClass(NetNumType net, NetNodeType node);
		IPXAddressClass(IPXHeaderType *header);

		/*.....................................................................
		Set the address from explicit variables, or from the SOURCE values
		in an IPX packet header.
		.....................................................................*/
		void Set_Address(NetNumType net, NetNodeType node);
		void Set_Address(IPXHeaderType *header);
		/*.....................................................................
		Get the address values explicitly, or copy them into the DESTINATION
		values in an IPX packet header.
		.....................................................................*/
		void Get_Address (NetNumType net, NetNodeType node);
		void Get_Address(IPXHeaderType *header);

		/*.....................................................................
		Tells if this address is a broadcast address
		.....................................................................*/
		int Is_Broadcast(void);

		/*.....................................................................
		Overloaded operators:
		.....................................................................*/
		bool operator == (IPXAddressClass & addr);
		bool operator != (IPXAddressClass & addr);
		bool operator > (IPXAddressClass &addr);
		bool operator < (IPXAddressClass &addr);
		bool operator >= (IPXAddressClass &addr);
		bool operator <= (IPXAddressClass &addr);

		/*.....................................................................
		Convert address to human readable string
		.....................................................................*/
		const char *As_String(void);

	/*
	-------------------------- Protected Interface ---------------------------
	*/
	protected:
	/*
	--------------------------- Private Interface ----------------------------
	*/
	private:
		NetNumType NetworkNumber;
		NetNodeType NodeAddress;
};


/**************************** end of ipxaddr.h *****************************/
