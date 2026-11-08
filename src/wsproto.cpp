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
 *                     $Archive:: /Sun/WSProto.cpp                                            $*
 *                                                                                             *
 *                      $Author:: Joe_b                                                       $*
 *                                                                                             *
 *                     $Modtime:: 8/20/97 10:54a                                              $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *  WSProto.CPP WinsockInterfaceClass to provide an interface to Winsock protocols             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *                                                                                             *
 * WIC::WinsockInterfaceClass -- constructor for the WinsockInterfaceClass                     *
 * WIC::~WinsockInterfaceClass -- destructor for the WinsockInterfaceClass                     *
 * WIC::Close -- Releases any currently in use Winsock resources.                              *
 * WIC::Close_Socket -- Close the communication socket if its open                             *
 * WIC::Start_Listening -- Enable callbacks for read/write events on our socket                *
 * WIC::Stop_Listening -- Disable the winsock event callback                                   *
 * WIC::Discard_In_Buffers -- Discard any packets in our incoming packet holding buffers       *
 * WIC::Discard_In_Buffers -- Discard any packets in our outgoing packet holding buffers       *
 * WIC::Init -- Initialised Winsock and this class for use.                                    *
 * WIC::Read -- read any pending input from the communications socket                          *
 * WIC::WriteTo -- Send data via the Winsock socket                                            *
 * WIC::Broadcast -- Send data via the Winsock socket                                          *
 * WIC::Clear_Socket_Error -- Clear any outstanding erros on the socket                        *
 * WIC::Set_Socket_Options -- Sets default socket options for Winsock buffer sizes             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "wsproto.h"

#include "_keyboar.h"
#include "assert.h"
#include "dbgprint.h"
#include "globals.h"
#include "keyboard.h"
#include "vector.h"

#include <cstdio>
#include <nspapi.h>


/*
**	RA2: the protocols to ask Winsock about for the largest packet size: UDP and IPX.
*/
static int WS_Protocol_List[] = { 17, 1000, 0 };

extern void Add_CRC(unsigned int *crc, unsigned int val);

/***********************************************************************************************
 * WIC::WinsockInterfaceClass -- constructor for the WinsockInterfaceClass                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:51PM ST : Created                                                              *
 *=============================================================================================*/
WinsockInterfaceClass::WinsockInterfaceClass(void) :
Socket(INVALID_SOCKET), // RA2
AlternateSocket(INVALID_SOCKET)
{
	WinsockInitialised = false;

	for (int i = 0; i < WS_MAX_STATIC_BUFFERS; i++) {
		StaticInBuffers[i].InUse = false;
		StaticInBuffers[i].IsAllocated = false;

		StaticOutBuffers[i].InUse = false;
		StaticOutBuffers[i].IsAllocated = false;
		StaticOutBuffers[i].IsAlternate = false; // RA2
		StaticOutBuffers[i].Unused_18 = false;
		StaticAltOutBuffers[i].InUse = false;
		StaticAltOutBuffers[i].IsAllocated = false;
		StaticAltOutBuffers[i].IsAlternate = true;
		StaticAltOutBuffers[i].Unused_18 = false;
	}

	InBuffersUsed = 0;
	OutBuffersUsed = 0;

	AltOutBuffersUsed = 0; // RA2
	InBufferArrayPos = 0;
	OutBufferArrayPos = 0;

	MaxPacketSize = 576; // RA2
	DebugString("WinsockInterface constructed\n");

}


/***********************************************************************************************
 * WIC::~WinsockInterfaceClass -- destructor for the WinsockInterfaceClass                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:52PM ST : Created                                                              *
 *=============================================================================================*/
WinsockInterfaceClass::~WinsockInterfaceClass(void)
{
	Close();
}


/***********************************************************************************************
 * WIC::Close -- Releases any currently in use Winsock resources.                              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:52PM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Close(void)
{
	/*
	**	If we never initialised the class in the first place then just return
	*/
	if (!WinsockInitialised) return;

	/*
	**	Cancel any outstaning asyncronous events
	*/
	Stop_Listening();

	/*
	**	Close any open sockets
	*/
	Close_Socket();

	/*
	**	Call the Winsock cleanup function to say we are finished using Winsock
	*/
	WSACleanup();

	WinsockInitialised = false;
}


/***********************************************************************************************
 * WIC::Close_Socket -- Close the communication socket if its open                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 11:53AM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Close_Socket (void)
{
	if ( Socket != INVALID_SOCKET ) {
		closesocket (Socket);
		Socket = INVALID_SOCKET;
	}
	if (AlternateSocket != INVALID_SOCKET) { // RA2
		closesocket(AlternateSocket);
		AlternateSocket = INVALID_SOCKET;
}
}


/***********************************************************************************************
 * WIC::Start_Listening -- Enable callbacks for read/write events on our socket                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 11:54AM ST : Created                                                              *
 *=============================================================================================*/
bool WinsockInterfaceClass::Start_Listening (void)
{
	/*
	**	Enable asynchronous events on the socket
	*/
	if (WSAAsyncSelect(Socket, MainWindow, Protocol_Event_Message(INVALID_SOCKET), FD_READ | FD_WRITE) == SOCKET_ERROR) { // RA2
		DebugString("Async select failed on primary socket - error code %d.\n", LAST_ERROR);
		return(false);
	}
	if (AlternateSocket != INVALID_SOCKET) { // RA2
		if (WSAAsyncSelect(AlternateSocket, MainWindow, Protocol_Event_Message(AlternateSocket), FD_READ | FD_WRITE) == SOCKET_ERROR) {
			DebugString("Async select failed on alternate socket - error code %d.\n", LAST_ERROR);
			return(false);
		}
	}
	return(true);
}


/***********************************************************************************************
 * WIC::Stop_Listening -- Disable the winsock event callback                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 12:06PM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Stop_Listening (void)
{
	if (Socket != INVALID_SOCKET && MainWindow) { // RA2
		WSAAsyncSelect(Socket, MainWindow, 0, 0);
	}
	if (AlternateSocket != INVALID_SOCKET && MainWindow) {
		WSAAsyncSelect(AlternateSocket, MainWindow, 0, 0);
	}
}


/***********************************************************************************************
 * WIC::Discard_In_Buffers -- Discard any packets in our incoming packet holding buffers       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 11:55AM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Discard_In_Buffers(void)
{
	WinsockBufferType *packet;

	while (InBuffers.Count()) {
		packet = InBuffers[0];
		if (packet->IsAllocated) {
			delete packet;
		} else {
			packet->InUse = false;
			InBuffersUsed--;
		}
		InBuffers.Delete_Index(0);
	}
	InBuffersUsed = 0;
	InBufferArrayPos = 0;
}


/***********************************************************************************************
 * WIC::Discard_In_Buffers -- Discard any packets in our outgoing packet holding buffers       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 11:55AM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Discard_Out_Buffers(void)
{
	WinsockBufferType *packet;

	while (OutBuffers.Count()) {
		packet = OutBuffers[0];
		if (packet->IsAllocated) {
			delete packet;
		} else {
			packet->InUse = false;
			OutBuffersUsed--;
		}
		OutBuffers.Delete_Index(0);
	}
	OutBuffersUsed = 0;
	OutBufferArrayPos = 0;
	while (AltOutBuffers.Count()) { // RA2
		packet = AltOutBuffers[0];
		if (packet->IsAllocated) {
			delete packet;
		} else {
			packet->InUse = false;
			AltOutBuffersUsed--;
}
		AltOutBuffers.Delete_Index(0);
	}
	AltOutBuffersUsed = 0;
	AltOutBufferArrayPos = 0;
}


/***********************************************************************************************
 * WIC::Init -- Initialised Winsock and this class for use.                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   true if Winsock is available and was initialised                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:54PM ST : Created                                                              *
 *=============================================================================================*/
bool WinsockInterfaceClass::Init(void)
{
	short version;
	int 	rc;

	DebugString("WinsockInterface init.\n");
	/*
	**	Just return true if we are already set up
	*/
	if (WinsockInitialised) {
		DebugString("WinsockInterface already initialised\n");
		return(true);
	}

	/*
	**	Create a buffer much larger than the sizeof (WSADATA) would indicate since Bounds Checker
	**	says that a buffer of that size gets overrun.
	*/
	if (PacketLossPercent > 0 && !IsPacketLossReported) { // RA2
		IsPacketLossReported = true;
		MessageBox(MainWindow, "Warning - Packet Loss Simulation mode enabled", "Note", MB_OK);
	}
	char	*buffer = new char [sizeof (WSADATA) + 1024];
	WSADATA *winsock_info = (WSADATA*) (&buffer[0]);

	/*
	**	Initialise socket and event handle to null
	*/
	Socket =INVALID_SOCKET;
	Discard_In_Buffers();
	Discard_Out_Buffers();

	DebugString("About to call WSAStartup\n");

	/*
	**	Start WinSock, and fill in our Winsock info structure
	*/
	version = (WINSOCK_MINOR_VER << 8) | WINSOCK_MAJOR_VER;
	rc = WSAStartup(version, winsock_info);
	if (rc != 0) {
		DebugString("Winsock failed to initialise - error code %d.\n", rc );
		delete [] buffer;
		return(false);
	}

	DebugString("Winsock initialised OK\n");

	/*
	**	Check the Winsock version number
	*/
	if ((winsock_info->wVersion & 0x00ff) != (version & 0x00ff) ||
		(winsock_info->wVersion >> 8) != (version >> 8)) {
		DebugString("Winsock version is less than 1.1\n" );
		delete [] buffer;
		return(false);
	}

	DebugString("Winsock version is %d.%d\n", winsock_info->wVersion & 0x00ff, winsock_info->wVersion >> 8);

	/*
	**	Everything is OK so return success
	*/
	MaxPacketSize = 546; // RA2
	char protocol_buffer[4096];
	unsigned long protocol_buffer_len = sizeof(protocol_buffer);
	int count = EnumProtocolsA(WS_Protocol_List, protocol_buffer, &protocol_buffer_len);
	if (count != -1) {
		int min_size = 0x7fffffff;
		PROTOCOL_INFOA *info = (PROTOCOL_INFOA *)protocol_buffer;
		for (int i = 0; i < count; i++) {
			DebugString("Found protocol %s, max frame size is %d\n", info[i].lpProtocol, info[i].dwMessageSize);
			if (info[i].dwMessageSize != 0 && info[i].dwMessageSize != -1 && min_size >= (int)info[i].dwMessageSize) {
				min_size = info[i].dwMessageSize;
			}
		}
		if (min_size != 0x7fffffff && min_size > 400) {
			MaxPacketSize = min_size;
		}
	} else {
		DebugString("Failed to get protocol enumeration\n");
	}
	WinsockInitialised = true;

	delete [] buffer;
	return(true);

}


/***********************************************************************************************
 * WIC::Build_Packet_CRC -- Create a CRC value for a packet.                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to packet                                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/5/99 1:26PM ST : Created                                                               *
 *=============================================================================================*/
void WinsockInterfaceClass::Build_Packet_CRC(WinsockBufferType * packet)
{
	fw_assert (packet->InUse);
	fw_assert (packet->BufferLen);

	packet->CRC = 0;

	unsigned int *crc_ptr = &(packet->CRC);
	unsigned int *packetptr = (unsigned int*) &(packet->Buffer[0]);

	for (int i=0 ; i<packet->BufferLen/4 ; i++) {
		Add_CRC (crc_ptr, *packetptr++);
	}

	int leftover = packet->BufferLen & 3;
	if (leftover) {
		unsigned int val = *packetptr;
		val = val & (0xffffffff >> ((4-leftover) << 3));
		Add_CRC (crc_ptr, val);
	}
}


/***********************************************************************************************
 * WIC::Passes_CRC_Check -- Checks the CRC for a packet                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to packet                                                                     *
 *                                                                                             *
 * OUTPUT:   true if packet passes CRC check                                                   *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/5/99 1:26PM ST : Created                                                               *
 *=============================================================================================*/
bool WinsockInterfaceClass::Passes_CRC_Check(WinsockBufferType * packet)
{
	fw_assert (packet->InUse);
	fw_assert (packet->BufferLen < WS_INTERNET_BUFFER_LEN);

	if (packet->BufferLen >= WS_INTERNET_BUFFER_LEN) {
		return(false);
	}

	unsigned int crc = 0;

	unsigned int *crc_ptr = &crc;
	unsigned int *packetptr = (unsigned int*) &(packet->Buffer[0]);

	for (int i=0 ; i<packet->BufferLen/4 ; i++) {
		Add_CRC (crc_ptr, *packetptr++);
	}

	int leftover = packet->BufferLen & 3;
	if (leftover) {
		unsigned int val = *packetptr;
		val = val & (0xffffffff >> ((4-leftover) << 3));
		Add_CRC (crc_ptr, val);
	}

	if (crc == packet->CRC) {
		return(true);
	}

	fw_assert (crc == packet->CRC);
	DebugString("Error in Winsock packet CRC\n");
	return(false);
}


/***********************************************************************************************
 * WIC::Get_New_Out_Buffer -- Get a holding buffer for an outgoing packet                      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   ptr to out buffer                                                                 *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/2/00 12:39PM ST : Created                                                               *
 *=============================================================================================*/
void *WinsockInterfaceClass::Get_New_Out_Buffer(bool alternate) // RA2
{
	WinsockBufferType *buffer = NULL;
	int pos;

	if (!alternate) { // RA2

	/*
	**	If there are no more free buffers in the heap then allocate one.
	*/
	if (OutBuffersUsed == WS_MAX_STATIC_BUFFERS) {
		buffer = new WinsockBufferType;
		buffer->IsAllocated = true;
		buffer->InUse = true;
	}else{
		/*
		**	Find the next free buffer in the heap.
		*/
		for (int i=0 ; i<WS_MAX_STATIC_BUFFERS ; i++) {

			pos = OutBufferArrayPos++;
			if (OutBufferArrayPos > WS_MAX_STATIC_BUFFERS-1) {
				OutBufferArrayPos = 0;
			}

			if (StaticOutBuffers[pos].InUse == false) {
				buffer = &StaticOutBuffers[pos];
				buffer->InUse = true;
				OutBuffersUsed++;
				break;
			}
		}
	}

		if (buffer != NULL) { // RA2
			buffer->IsAlternate = false;
			buffer->Unused_18 = false;
		}
	} else {
		if (AltOutBuffersUsed == WS_MAX_STATIC_BUFFERS) {
			buffer = new WinsockBufferType;
			buffer->IsAllocated = true;
			buffer->InUse = true;
		} else {
			for (int i = 0; i < WS_MAX_STATIC_BUFFERS; i++) {
				pos = AltOutBufferArrayPos++;
				if (AltOutBufferArrayPos > WS_MAX_STATIC_BUFFERS - 1) {
					AltOutBufferArrayPos = 0;
				}
				if (StaticAltOutBuffers[pos].InUse == false) {
					buffer = &StaticAltOutBuffers[pos];
					buffer->InUse = true;
					AltOutBuffersUsed++;
					break;
				}
			}
		}
		if (buffer != NULL) {
			buffer->IsAlternate = true;
			buffer->Unused_18 = false;
		}
	}
	return(buffer);
}


/***********************************************************************************************
 * WIC::Get_New_In_Buffer -- Get a holding buffer for an incoming packet                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   ptr to in buffer                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/2/00 12:39PM ST : Created                                                               *
 *=============================================================================================*/
void *WinsockInterfaceClass::Get_New_In_Buffer(void)
{
	WinsockBufferType *buffer = NULL;
	int pos;


	/*
	**	If there are no more free buffers in the heap then allocate one.
	*/
	if (InBuffersUsed == WS_MAX_STATIC_BUFFERS) {
		buffer = new WinsockBufferType;
		buffer->IsAllocated = true;
		buffer->InUse = true;
	}else{
		/*
		**	Find the next free buffer in the heap.
		*/
		for (int i=0 ; i<WS_MAX_STATIC_BUFFERS ; i++) {

			pos = InBufferArrayPos++;
			if (InBufferArrayPos > WS_MAX_STATIC_BUFFERS-1) {
				InBufferArrayPos = 0;
			}

			if (StaticInBuffers[pos].InUse == false) {
				buffer = &StaticInBuffers[pos];
				buffer->InUse = true;
				InBuffersUsed++;
				break;
			}
		}
	}

	if (buffer != NULL) { // RA2
		buffer->IsAlternate = false;
		buffer->Unused_18 = false;
	}
	return(buffer);
}


/***********************************************************************************************
 * WIC::Read -- read any pending input from the communications socket                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer to receive input                                                    *
 *           length of buffer                                                                  *
 *           ptr to address to fill with address that packet was sent from                     *
 *           length of address buffer                                                          *
 *                                                                                             *
 * OUTPUT:   number of bytes transfered to buffer                                              *
 *                                                                                             *
 * WARNINGS: The format of the address is dependent on the protocol in use.                    *
 *                                                                                             *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 2:58PM ST : Created                                                              *
 *=============================================================================================*/
int WinsockInterfaceClass::Read(void *buffer, int &buffer_len, void *address, int &address_len, unsigned short &port) // RA2
{
	/*
	**	Call the message loop in case there are any outstanding winsock READ messages.
	*/
	Keyboard->Check();

	/*
	**	If there are no available packets then return 0
	*/
	if ( InBuffers.Count() == 0 ) return(0);

	/*
	**	Get the oldest packet for reading
	*/
	int packetnum = 0;
	WinsockBufferType *packet = InBuffers[packetnum];



	/*
	**	Copy the data and the address it came from into the supplied buffers.
	*/
	memcpy(buffer, packet->Buffer, packet->BufferLen);
	unsigned int copy_len = address_len; // RA2
	if (copy_len > sizeof(packet->Address)) {
		copy_len = sizeof(packet->Address);
	}
	address_len = copy_len;
	memcpy(address, packet->Address, copy_len);
	port = packet->SrcPort;

	/*
	**	Return the length of the packet in buffer_len.
	*/
	buffer_len = packet->BufferLen;

	/*
	**	Delete the temporary storage for the packet now that it is being passed to the game.
	*/
	InBuffers.Delete_Index(packetnum);
	if (packet->IsAllocated) {
		delete packet;
	}else{
		packet->InUse = false;
		InBuffersUsed--;
	}

	return(buffer_len);
}


/***********************************************************************************************
 * WIC::WriteTo -- Send data via the Winsock socket                                            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer containing data to send                                             *
 *           length of data to send                                                            *
 *           address to send data to.                                                          *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: The format of the address is dependent on the protocol in use.                    *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:00PM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::WriteTo(void *buffer, int buffer_len, void *address, int address_len, bool alternate, bool flag, unsigned short port) // RA2
{
	/*
	**	Create a temporary holding area for the packet.
	*/
	WinsockBufferType *packet = (WinsockBufferType *)Get_New_Out_Buffer(alternate); // RA2

	/*
	**	Copy the packet into the holding buffer.
	*/
	memcpy ( packet->Buffer, buffer, buffer_len );
	packet->BufferLen = buffer_len;
	packet->IsBroadcast = false;
//	memcpy ( packet->Address, address, sizeof (packet->Address) );
	packet->Unused_18 = flag; // RA2
	memcpy ( packet->Address, address, address_len );

	packet->SrcPort = port; // RA2
	Build_Packet_CRC(packet);

	/*
	**	Add it to our out list.
	*/
	SOCKET sock; // RA2
	if (alternate) {
		AltOutBuffers.Add(packet);
		sock = AlternateSocket;
	} else {
	OutBuffers.Add ( packet );

	/*
	**	Send a message to ourselves so that we can initiate a write if Winsock is idle.
	*/
		sock = Socket; // RA2
	}
	SendMessage(MainWindow, Protocol_Event_Message(sock), 0, (LONG)FD_WRITE);

	/*
	**	Make sure the message loop gets called.
	*/
	Keyboard->Check();
}


/***********************************************************************************************
 * WIC::Broadcast -- Send data via the Winsock socket                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer containing data to send                                             *
 *           length of data to send                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:00PM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Broadcast(void *buffer, int buffer_len, bool alternate) // RA2
{
	/*
	**	Create a temporary holding area for the packet.
	*/
	WinsockBufferType *packet = (WinsockBufferType *)Get_New_Out_Buffer(alternate); // RA2

	/*
	**	Copy the packet into the holding buffer.
	*/
	memcpy ( packet->Buffer, buffer, buffer_len );
	packet->BufferLen = buffer_len;
	Build_Packet_CRC(packet);

	/*
	**	Indicate that this packet should be broadcast.
	*/
	packet->IsBroadcast = true;

	/*
	**	Add it to our out list.
	*/
	SOCKET sock; // RA2
	if (alternate) {
		AltOutBuffers.Add(packet);
		sock = AlternateSocket;
	} else {
	OutBuffers.Add ( packet );

	/*
	**	Send a message to ourselves so that we can initiate a write if Winsock is idle.
	*/
		sock = Socket; // RA2
	}
	SendMessage(MainWindow, Protocol_Event_Message(sock), 0, (LONG)FD_WRITE);

	/*
	**	Make sure the message loop gets called.
	*/
	Keyboard->Check();
}


/***********************************************************************************************
 * WIC::Clear_Socket_Error -- Clear any outstanding erros on the socket                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Socket                                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 12:05PM ST : Created                                                              *
 *=============================================================================================*/
void WinsockInterfaceClass::Clear_Socket_Error(SOCKET socket)
{
	unsigned int error_code;
	int length = 4;

	{
		getsockopt (socket, SOL_SOCKET, SO_ERROR, (char*)&error_code, &length);
		error_code = 0;
		setsockopt (socket, SOL_SOCKET, SO_ERROR, (char*)&error_code, length);
	}
}


/***********************************************************************************************
 * WIC::Set_Socket_Options -- Sets default socket options for Winsock buffer sizes             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 12:07PM ST : Created                                                              *
 *=============================================================================================*/
bool WinsockInterfaceClass::Set_Socket_Options(SOCKET socket) // RA2
{
	static int		socket_transmit_buffer_size = SOCKET_BUFFER_SIZE;
	static int		socket_receive_buffer_size = SOCKET_BUFFER_SIZE;

	/*
	**	Specify the size of the receive buffer.
	*/
	if (socket == INVALID_SOCKET) { // RA2
		socket = Socket;
	}
	int err = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&socket_receive_buffer_size, sizeof(socket_receive_buffer_size));
	if ( err == INVALID_SOCKET ) {
		DebugString("Failed to set socket option SO_RCVBUF - error code %d.\n", LAST_ERROR );
	} else {
		DebugString("Socket option SO_RCVBUF set OK\n");
	}

	/*
	**	Specify the size of the send buffer.
	*/
	err = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&socket_transmit_buffer_size, sizeof(socket_transmit_buffer_size)); // RA2
	if ( err == INVALID_SOCKET ) {
		DebugString("Failed to set socket option SO_SNDBUF - error code %d.\n", LAST_ERROR );
	} else {
		DebugString("Socket option SO_SNDBUF set OK\n");
	}

	return( true );
}


/// <summary>
/// Fetches the name of the local host machine.
/// This routine is used when the interface needs to look up the local ip addresses,
/// since the host name is what the address lookup is keyed on.
/// </summary>
/// <param name="name">Buffer that will be filled in with the host name.</param>
/// <param name="len">Size of the buffer supplied.</param>
/// <returns>bool; Was the host name fetched?</returns>
bool WinsockInterfaceClass::Get_Host_Name(char *name, int len)
{
	/*
	**	Use gethostbyname to find the name of the local host. We will need this to look up
	**	the local ip address.
	*/
	if (gethostname(name, len) == -1) {
		DebugString("Error - WinsockInterface Unable to get host name. Error code %d\n", LAST_ERROR );
		return(false);
	}

	DebugString("WinsockInterface Host name is %s\n", name);
	return(true);
}


