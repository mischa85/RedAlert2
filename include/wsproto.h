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
 *                     $Archive:: /Sun/WSProto.h                                              $*
 *                                                                                             *
 *                      $Author:: Joe_b                                                       $*
 *                                                                                             *
 *                     $Modtime:: 8/12/97 5:42p                                               $*
 *                                                                                             *
 *                    $Revision:: 4                                                          $ *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "_WSProto.h"
#include "vector.h"

/*
**	Include standard Winsock 1.0 header file.
*/
#include <winsock.h>

#ifndef fw_assert
#define fw_assert assert
#endif

#ifndef LAST_ERROR
#define LAST_ERROR WSAGetLastError()
#endif

/*
**	Misc defines
*/
#define WINSOCK_MINOR_VER		1   // Version of Winsock
#define WINSOCK_MAJOR_VER		1   //    that we require

#define WS_RECEIVE_BUFFER_LEN	640		// Length of our temporary receive buffer. RA2: 640
#define SOCKET_BUFFER_SIZE		1024*128	// Length of winsocks internal buffer.

#define WS_INTERNET_BUFFER_LEN	640	// RA2: 640

#define WS_MAX_STATIC_BUFFERS	128

#define PLANET_WESTWOOD_HANDLE_MAX 20	// Max length of a WChat handle

/*
**	Define events for Winsock callbacks
*/
#define WM_IPXASYNCEVENT		(WM_USER + 115)	// IPX socket Async event
#define WM_UDPASYNCEVENT		(WM_USER + 116)	// UDP socket Async event


/*
**	Enum to identify the protocols supported by the Winsock interface.
*/
enum ProtocolEnum {
	PROTOCOL_NONE,
	PROTOCOL_IPX,
	PROTOCOL_UDP
};

/*
**
**	Class to interface with Winsock. This interface only supports connectionless packet protocols
**	like UDP & IPX. Connection orientated or streaming protocols like TCP are not supported by this
**	class.
**
*/
class WinsockInterfaceClass {

	public:

		WinsockInterfaceClass(void);
		virtual ~WinsockInterfaceClass(void);

		bool Init(void);
		void Close(void);


		virtual void Close_Socket(void);

		virtual int  Read(void *buffer, int &buffer_len, void *address, int &address_len, unsigned short &port); // RA2: port
		virtual void WriteTo (void *buffer, int buffer_len, void *address, int address_len, bool alternate, bool flag, unsigned short port); // RA2: alternate socket, flag and port
		virtual void Broadcast (void *buffer, int buffer_len, bool alternate); // RA2: alternate socket

		virtual void Discard_In_Buffers (void);
		virtual void Discard_Out_Buffers (void);

		virtual bool Start_Listening (void);
		virtual void Stop_Listening (void);

		virtual void Clear_Socket_Error(SOCKET socket);

		virtual bool Set_Socket_Options ( SOCKET socket = INVALID_SOCKET ); // RA2: socket

		virtual void Set_Broadcast_Address ( char const * address, unsigned short port ) {}; // RA2: address and port
		virtual void Clear_Broadcast_Addresses(void) {};

		virtual ProtocolEnum Get_Protocol (void) {
			return(PROTOCOL_NONE);
		};

		virtual int Protocol_Event_Message (SOCKET) { // RA2: socket
			return(0);
		};

		virtual bool Open_Socket ( SOCKET ) {
			return(false);
		};

		virtual int Message_Handler(HWND, UINT, UINT, LONG) {
			return(1);
		}

		virtual bool Get_Host_Name(char *name, int len);

		virtual int Get_Num_Local_Addresses(void) { return(0); }
		virtual unsigned char *Get_Local_Address(int index) { return(NULL); }

		virtual void Set_NetCard(int card) { NetCard = card; }

		enum ConnectStatusEnum {
			CONNECTED_OK = 0,
			NOT_CONNECTING,
			CONNECTING,
			UNABLE_TO_CONNECT_TO_SERVER,
			CONTACTING_SERVER,
			SERVER_ADDRESS_LOOKUP_FAILED,
			RESOLVING_HOST_ADDRESS,
			UNABLE_TO_ACCEPT_CLIENT,
			UNABLE_TO_CONNECT,
			CONNECTION_LOST
		};

		inline ConnectStatusEnum Get_Connection_Status(void) {return(ConnectStatus);}

	protected:

		/*
		**	This struct contains the information needed for each incoming and outgoing packet.
		**	It acts as a temporary control for these packets.
		*/
		struct WinsockBufferType {
			unsigned char		Address [16];                   // Address. IN_ADDR, IPXAddressClass etc. RA2: 16 bytes
			int					BufferLen;                      // Length of data in buffer
			bool				IsBroadcast;                    // Flag to broadcast this packet
			bool				InUse;                          // Useage state of buffer
			bool				IsAllocated;                    // false means statically allocated.
			bool				IsAlternate;                    // RA2: +0x17, the packet uses the alternate socket; never read
			bool				Unused_18;                      // RA2: set from WriteTo's flag, never read
			unsigned short		SrcPort;                        // RA2: +0x1A, the peer's port
			unsigned int		CRC;                            // CRC of packet for extra sanity.
			unsigned char		Buffer[WS_INTERNET_BUFFER_LEN]; // Buffer to store packet in.
		};

		/*
		**	Packet buffer allocation.
		*/
		void *Get_New_Out_Buffer(bool alternate); // RA2: alternate
		void *Get_New_In_Buffer(void);

		/*
		**	Packet CRCs.
		*/
		virtual void Build_Packet_CRC(WinsockBufferType *packet);
		virtual bool Passes_CRC_Check(WinsockBufferType *packet);

		/*
		**	Array of buffers to temporarily store incoming and outgoing packets.
		*/
		unsigned int MaxPacketSize; // RA2: +0x04

		DynamicVectorClass <WinsockBufferType *> InBuffers;
		DynamicVectorClass <WinsockBufferType *> OutBuffers;
		DynamicVectorClass <WinsockBufferType *> AltOutBuffers; // RA2: the queue of the alternate socket

		/*
		**	Array of buffers that are always available for incoming packets.
		*/
		WinsockBufferType StaticInBuffers[WS_MAX_STATIC_BUFFERS];
		WinsockBufferType StaticOutBuffers[WS_MAX_STATIC_BUFFERS];
		WinsockBufferType StaticAltOutBuffers[WS_MAX_STATIC_BUFFERS]; // RA2

		/*
		**	Pointers to allow circular use of the buffer arrays.
		*/
		int InBufferArrayPos;
		int OutBufferArrayPos;
		int AltOutBufferArrayPos; // RA2

		/*
		**	Usage count for each array.
		*/
		int InBuffersUsed;
		int OutBuffersUsed;
		int AltOutBuffersUsed; // RA2

		/*
		**	Is Winsock present and initialised?
		*/
		bool 					WinsockInitialised;

		/*
		**	Socket that communications will take place over.
		*/
		SOCKET				Socket;
		SOCKET				AlternateSocket; // RA2: there is no ASync handle

		/*
		**	Temporary receive buffer to use when querying Winsock for incoming packets.
		*/
		unsigned char		ReceiveBuffer[WS_RECEIVE_BUFFER_LEN];

		/*
		**	Current connection status.
		*/
		ConnectStatusEnum	ConnectStatus;

		/*
		 * This is the index of the network adapter the player chose in the options dialog.
		 * A machine with more than one card needs telling which to play over.
		 */
		int NetCard;
};

int __cdecl Get_Packet_Loss_Percent(void); // RA2: the percentage of received packets to drop on purpose
