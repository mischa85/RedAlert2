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
 *                     $Archive:: /Sun/WSPUDP.h                                               $*
 *                                                                                             *
 *                      $Author:: Joe_b                                                       $*
 *                                                                                             *
 *                     $Modtime:: 8/05/97 6:45p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "wsproto.h"

#include <nspapi.h>


/*
**	Class to allow access to UDP specific portions of the Winsock interface.
**
*/
class UDPInterfaceClass : public WinsockInterfaceClass {
		typedef WinsockInterfaceClass BASECLASS;

	public:

		UDPInterfaceClass (void);
		virtual ~UDPInterfaceClass(void) override;

		virtual int Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam) override;
		virtual bool Open_Socket ( SOCKET socketnum ) override;
		virtual void Set_Broadcast_Address ( void *address ) override;
		virtual void Clear_Broadcast_Addresses(void) override;
		virtual void Broadcast (void *buffer, int buffer_len) override;

		virtual ProtocolEnum Get_Protocol (void) override {
			return(PROTOCOL_UDP);
		};

		virtual int Protocol_Event_Message (void) override {
			return(WM_UDPASYNCEVENT);
		};

		virtual int Get_Num_Local_Addresses(void) override {
			return(LocalAddresses.Count());
		};

		virtual unsigned char *Get_Local_Address(int index) override {
			return(LocalAddresses[index]);
		};

	private:

		/*
		**	Address to use when broadcasting a packet.
		*/
		DynamicVectorClass <unsigned char *> BroadcastAddresses;

		/*
		**	List of local addresses.
		*/
		DynamicVectorClass <unsigned char *> LocalAddresses;
};
