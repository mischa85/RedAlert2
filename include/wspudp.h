/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
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

		virtual void WriteTo (void *buffer, int buffer_len, void *address, int address_len, bool alternate, bool flag, unsigned short port) override; // RA2
		virtual void Broadcast (void *buffer, int buffer_len, bool alternate) override;
		virtual bool Start_Listening (void) override; // RA2
		virtual void Stop_Listening (void) override; // RA2
		virtual void Set_Broadcast_Address ( char const * address, unsigned short port ) override;
		virtual void Clear_Broadcast_Addresses(void) override;

		virtual ProtocolEnum Get_Protocol (void) override;
		virtual int Protocol_Event_Message (SOCKET socketnum) override;
		virtual bool Open_Socket ( SOCKET socketnum ) override;
		virtual int Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam) override;
		virtual int Get_Num_Local_Addresses(void) override;
		virtual unsigned char *Get_Local_Address(int index) override;

		virtual bool OpenSocket (unsigned short port = 0); // RA2
		virtual unsigned short Get_Port (void); // RA2
		virtual bool Init_Ports (void); // RA2
		virtual bool Open_Port_Socket (unsigned short port); // RA2
		virtual void Close_Port_Socket (unsigned short port); // RA2
		virtual void Close_All_Port_Sockets (void); // RA2
		virtual void Select_Port_Socket (unsigned short port); // RA2
		virtual void Add_Peer (unsigned char *address, unsigned short port); // RA2
		bool Is_Reachable (unsigned char *address, unsigned short port); // RA2

		static int UDPListenPort; // RA2: the port broadcasts go to when none is given

	private:

		/*
		**	RA2: the UDP port and the sockets bound to the ports tried.
		*/
		unsigned short Port; // RA2: +0x3F2FC
		SOCKET PortSockets[8];
		unsigned short PortNumbers[8];
		int PortCount;
		SOCKET SelectedSocket;

		/*
		**	Address to use when broadcasting a packet.
		*/
		DynamicVectorClass <unsigned char *> BroadcastAddresses;
		unsigned short BroadcastPorts[256]; // RA2

		/*
		**	List of local addresses.
		*/
		DynamicVectorClass <unsigned char *> LocalAddresses;

		/*
		**	RA2: the peers' addresses and ports.
		*/
		int PeerCount;
		unsigned int PeerAddresses[16];
		unsigned short PeerPorts[16];
};

/*
**	RA2: the ports of the alternate socket and the loopback port of the host in an
**	internet game, and whether a malformed packet was reported yet.
*/
extern int AlternateSendPort;
extern int AlternateListenPort;
extern int LoopbackPort;
extern bool IsMalformedPacketReported;
