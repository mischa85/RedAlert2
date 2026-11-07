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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Sun/WSPIPX.h                                               $*
 *                                                                                             *
 *                      $Author:: Joe_b                                                       $*
 *                                                                                             *
 *                     $Modtime:: 8/12/97 5:42p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "wsproto.h"

/*
**	Include Windows specific extensions for Winsock that allow IPX over winsock 1.1
*/
#include <wsipx.h>
#include <wsnwlink.h>

/*
**	IPX interface class. This handles access to the IPX specific portions of the
**	Winsock interface.
**
*/
class IPXInterfaceClass : public WinsockInterfaceClass {
		typedef WinsockInterfaceClass BASECLASS;

	public:

		IPXInterfaceClass (void);
		//virtual ~IPXInterfaceClass(void){Close();};
		bool Get_Network_Card_Address (int card_number, SOCKADDR_IPX *addr);
		virtual int Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam) override;
		virtual bool Open_Socket ( SOCKET socketnum ) override;

		virtual ProtocolEnum Get_Protocol (void) override {
			return(PROTOCOL_IPX);
		};

		virtual int Protocol_Event_Message (void) override {
			return(WM_IPXASYNCEVENT);
		};

		bool Get_Network_Card_Address_As_String(int card_number, char *buffer, int length);

	private:
		/*
		**	The address of the network we will send broadcasts to. Normally you would expect
		**	this to be ff,ff,ff,ff but this fails under NT 4.0. Instead, we can use the network
		**	number of the net that this PC is attached to. This limits broadcasts to the current
		**	network.
		*/
		unsigned char	BroadcastNet[4];

		/*
		**	The node to use as a broadcast address. Normally ff,ff,ff,ff,ff,ff.
		*/
		unsigned char	BroadcastNode[6];

		/*
		**	The id of the network cars in this machine.
		*/
		unsigned char	MyNode[6];

		/*
		**	The socket number to connect with. Normally this will be virgins reserved socket
		**	number - VIRGIN_SOCKET (0x8813).
		*/
		SOCKET			IPXSocketNumber;

};
