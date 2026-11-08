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
 *                     $Archive:: /Sun/WSPUDP.cpp                                             $*
 *                                                                                             *
 *                      $Author:: Joe_b                                                       $*
 *                                                                                             *
 *                     $Modtime:: 8/05/97 6:45p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *  WSProto.CPP WinsockInterfaceClass to provide an interface to Winsock protocols             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 * UDPInterfaceClass::UDPInterfaceClass -- Class constructor.                                  *
 * UDPInterfaceClass::Set_Broadcast_Address -- Sets the address to send broadcast packets to   *
 * UDPInterfaceClass::Open_Socket -- Opens a socket for communications via the UDP protocol    *
 * TMC::Message_Handler -- Message handler function for Winsock related messages               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "wspudp.h"

#include "assert.h"
#include "dbgprint.h"
#include "misc.h"
#include "msgloop.h"
#include "vector.h"

#include <cstdio>
#include "session.h"
#include "_rand.h"
#include "random.h"


extern int WestwoodOnline_PortNumber;

/***********************************************************************************************
 * UDPInterfaceClass::UDPInterfaceClass -- Class constructor.                                  *
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
 *    8/5/97 12:11PM ST : Created                                                              *
 *=============================================================================================*/
UDPInterfaceClass::UDPInterfaceClass (void) : BASECLASS()
{
	Port = 0;
	PortCount = 0;
	SelectedSocket = INVALID_SOCKET;
	PeerCount = 0;
}


/***********************************************************************************************
 * UDPIC::~UDPInterfaceClass -- UDPInterface class destructor                                  *
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
 *    10/9/97 12:17PM ST : Created                                                             *
 *=============================================================================================*/
UDPInterfaceClass::~UDPInterfaceClass (void)
{
	Clear_Broadcast_Addresses();

	while ( LocalAddresses.Count() ) {
		delete LocalAddresses[0];
		LocalAddresses.Delete_Index(0);
	}

	Close();
}


/// <summary>
/// Discards every broadcast address the interface knows about.
/// This routine is used to forget the addresses handed over by Set_Broadcast_Address,
/// so that a later broadcast will not reach a stale set of destinations.
/// </summary>
void UDPInterfaceClass::Clear_Broadcast_Addresses(void)
{
	while ( BroadcastAddresses.Count() ) {
		delete BroadcastAddresses[0];
		BroadcastAddresses.Delete_Index(0);
	}
}


/***********************************************************************************************
 * UDPInterfaceClass::Set_Broadcast_Address -- Sets the address to send broadcast packets to   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to address in decimal dot format. i.e. xxx.xxx.xxx.xxx                        *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 12:12PM ST : Created                                                              *
 *=============================================================================================*/
void UDPInterfaceClass::Set_Broadcast_Address (char const *address, unsigned short port)
{
	int addr[4];
	unsigned char *baddr = new unsigned char[4];

	sscanf ( address, "%d.%d.%d.%d", (unsigned char *)&addr[0], (unsigned char *)&addr[1], (unsigned char *)&addr[2], (unsigned char *)&addr[3] );
	baddr[0] = addr[0];
	baddr[1] = addr[1];
	baddr[2] = addr[2];
	baddr[3] = addr[3];
	if (port == 0) port = UDPListenPort;
	BroadcastPorts[BroadcastAddresses.Count()] = port;
	BroadcastAddresses.Add (baddr);
}


/// <summary>
/// RA2
/// </summary>
bool UDPInterfaceClass::OpenSocket (unsigned short port)
{
	LINGER ling;
	struct sockaddr_in addr;

	if (Socket != INVALID_SOCKET) {
		closesocket(Socket);
		Socket = INVALID_SOCKET;
	}

	DebugString("About to open primary UDP socket\n");

	Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (Socket == INVALID_SOCKET) {
		return(false);
	}

	addr.sin_family = AF_INET;
	if (port) {
		Port = port;
		addr.sin_port = htons(port);
	} else {
		Port = 0;
		addr.sin_port = htons((unsigned short) UDPListenPort);
	}
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	DebugString("About to bind the UDP socket to port %d\n", port);

	if (bind(Socket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
		DebugString("Bind failed with error code %d\n", LAST_ERROR);
		Close_Socket();
		return(false);
	}

	ling.l_onoff = 0;
	ling.l_linger = 0;
	setsockopt(Socket, SOL_SOCKET, SO_LINGER, (LPSTR)&ling, sizeof(ling));

	BASECLASS::Set_Socket_Options();

	DebugString("Primary UDP Socket init complete\n");

	return(true);
}


/// <summary>
/// RA2
/// </summary>
bool UDPInterfaceClass::Open_Port_Socket (unsigned short port)
{
	struct sockaddr_in addr;

	if (port == 0) {
		return(false);
	}

	for (int i = 0; i < PortCount; i++) {
		if (PortNumbers[i] == port) {
			return(false);
		}
	}

	DebugString("About to open spare UDP socket\n");

	SOCKET spare = socket(AF_INET, SOCK_DGRAM, 0);
	if (spare == INVALID_SOCKET) {
		return(false);
	}

	DebugString("Got spare socket %d\n", spare);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	DebugString("About to bind the UDP socket to port %d\n", port);

	if (bind(spare, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
		closesocket(spare);
		DebugString("Failed to bind spare socket to port %d\n", port);
		return(false);
	}

	PortNumbers[PortCount] = port;
	PortSockets[PortCount] = spare;
	PortCount++;

	BASECLASS::Set_Socket_Options(spare);

	DebugString("Spare UDP Socket init complete\n");

	return(true);
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::Close_Port_Socket (unsigned short port)
{
	if (port) {
		for (int i = 0; i < PortCount; i++) {
			if (PortNumbers[i] == port) {
				DebugString("Closing spare socket %d\n", PortSockets[i]);
				closesocket(PortSockets[i]);
				if (PortSockets[i] == SelectedSocket) {
					SelectedSocket = INVALID_SOCKET;
				}
				for (int j = i; j < PortCount - 1; j++) {
					PortSockets[j] = PortSockets[j + 1];
					PortNumbers[j] = PortNumbers[j + 1];
				}
				PortCount--;
				return;
			}
		}
	}
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::Select_Port_Socket (unsigned short port)
{
	if (port && port != UDPListenPort) {
		for (int i = 0; i < PortCount; i++) {
			if (PortNumbers[i] == port) {
				SelectedSocket = PortSockets[i];
				return;
			}
		}
	}
	SelectedSocket = INVALID_SOCKET;
}


/// <summary>
/// RA2
/// </summary>
bool UDPInterfaceClass::Init_Ports (void)
{
	if (Init()) {
		PortCount = 0;
		return(true);
	}
	return(false);
}


/***********************************************************************************************
 * UDPInterfaceClass::Open_Socket -- Opens a socket for communications via the UDP protocol    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Socket number to use. Not required for this protocol.                             *
 *                                                                                             *
 * OUTPUT:   True if socket was opened OK                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    8/5/97 12:13PM ST : Created                                                              *
 *=============================================================================================*/
bool UDPInterfaceClass::Open_Socket ( SOCKET )
{
	LINGER ling;
	struct sockaddr_in addr;

	if ( !WinsockInitialised ) {
		if ( !Init_Ports()) return( false );
	}

	OpenSocket((unsigned short) UDPListenPort);

	DebugString("About to query the host name\n");

	char hostname[128];

	if (gethostname(hostname, 128) != 0) {
		DebugString("Host name is '%s'\n", hostname);
	}

	DebugString("About to call gethostbyname\n");

	struct hostent *host_info = gethostbyname ( hostname );

	if (host_info == NULL) {
		DebugString("gethostbyname failed! Error code %d\n", LAST_ERROR);
	}

	while ( LocalAddresses.Count() ) {
		delete LocalAddresses[0];
		LocalAddresses.Delete_Index(0);
	}

	unsigned int **addresses = (unsigned int**) (host_info->h_addr_list);

	for ( ;; ) {
		if ( !*addresses ) break;

		unsigned int address = **addresses++;

		DebugString("Found local address: %d.%d.%d.%d\n", address & 0xff, (address & 0xff00) >> 8, (address & 0xff0000) >> 16, (address & 0xff000000) >> 24);

		unsigned char *a = new unsigned char [4];
		* ((unsigned int*) a) = address;
		LocalAddresses.Add (a);
	}

	DebugString("About to open alternate UDP socket\n");

	AlternateSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (AlternateSocket == INVALID_SOCKET) {
		return(false);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons ( (unsigned short) AlternateListenPort);
	addr.sin_addr.s_addr = htonl (INADDR_ANY);

	DebugString("About to bind the UDP socket\n");

	if ( bind (AlternateSocket, (LPSOCKADDR)&addr, sizeof(addr) ) == SOCKET_ERROR) {
		Close_Socket ();
		return(false);
	}

	ling.l_onoff = 0;
	ling.l_linger = 0;
	setsockopt (AlternateSocket, SOL_SOCKET, SO_LINGER, (LPSTR)&ling, sizeof(ling));

	BASECLASS::Set_Socket_Options(AlternateSocket);

	DebugString("Alternate UDP Socket init complete\n");

	return(true);
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::WriteTo (void *buffer, int buffer_len, void *address, int address_len, bool alternate, bool flag, unsigned short port)
{
	if ( address && ((IPXAddressClass *)address)->Is_Broadcast() ) {
		Broadcast ( buffer, buffer_len, alternate );
		return;
	}
	BASECLASS::WriteTo ( buffer, buffer_len, address, address_len, alternate, flag, port );
}


/***********************************************************************************************
 * UDPIC::Broadcast -- Send data via the Winsock socket                                        *
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
void UDPInterfaceClass::Broadcast (void *buffer, int buffer_len, bool alternate)
{
	for ( int i=0 ; i<BroadcastAddresses.Count() ; i++ ) {
		WinsockBufferType *packet = (WinsockBufferType *)Get_New_Out_Buffer(alternate);

		memcpy ( packet->Buffer, buffer, buffer_len );
		packet->BufferLen = buffer_len;
		packet->IsBroadcast = true;

		memset (packet->Address, 0, sizeof (packet->Address));
		memcpy (packet->Address+4, BroadcastAddresses[i], 4);
		packet->SrcPort = BroadcastPorts[i];

		Build_Packet_CRC(packet);

		SOCKET send_socket;
		if ( alternate ) {
			AltOutBuffers.Add ( packet );
			send_socket = AlternateSocket;
		} else {
			OutBuffers.Add ( packet );
			send_socket = Socket;
		}

		SendMessage ( MainWindow, Protocol_Event_Message(send_socket), 0, (LONG)FD_WRITE );

		Windows_Message_Handler();
	}
}


/// <summary>
/// RA2
/// </summary>
bool UDPInterfaceClass::Start_Listening (void)
{
	if (BASECLASS::Start_Listening()) {
		for (int i = 0; i < PortCount; i++) {
			if (WSAAsyncSelect(PortSockets[i], MainWindow, Protocol_Event_Message(INVALID_SOCKET), FD_READ | FD_WRITE) == SOCKET_ERROR) {
				DebugString("Async select failed on spare socket.\n");
				return(false);
			}
		}
	}
	Windows_Message_Handler();
	return(true);
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::Stop_Listening (void)
{
	BASECLASS::Stop_Listening();
	Close_All_Port_Sockets();
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::Add_Peer (unsigned char *address, unsigned short port)
{
	if (PeerCount < 16) {
		for (int i = 0; i < PeerCount; i++) {
			if (port == PeerPorts[i] && *(unsigned int *)address == PeerAddresses[i]) {
				return;
			}
		}
		DebugString("Marking address %d.%d.%d.%d ; %d as unreachable\n", address[0], address[1], address[2], address[3], port);
		PeerAddresses[PeerCount] = *(unsigned int *)address;
		PeerPorts[PeerCount] = port;
		PeerCount++;
	}
}


/// <summary>
/// RA2
/// </summary>
bool UDPInterfaceClass::Is_Reachable (unsigned char *address, unsigned short port)
{
	for (int i = 0; i < PeerCount; i++) {
		if (port == PeerPorts[i] && !memcmp(address, &PeerAddresses[i], 4)) {
			return(false);
		}
	}
	return(true);
}


/// <summary>
/// RA2
/// </summary>
void UDPInterfaceClass::Close_All_Port_Sockets (void)
{
	for (int i = 0; i < PortCount; i++) {
		DebugString("Closing spare socket %d\n", PortSockets[i]);
		WSAAsyncSelect(PortSockets[i], MainWindow, 0, 0);
		closesocket(PortSockets[i]);
	}
	PortCount = 0;
	SelectedSocket = INVALID_SOCKET;
}


/***********************************************************************************************
 * TMC::Message_Handler -- Message handler function for Winsock related messages               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Windows message handler stuff                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 3:05PM ST : Created                                                              *
 *=============================================================================================*/
int UDPInterfaceClass::Message_Handler(HWND, UINT message, UINT wParam, LONG lParam)
{
	struct sockaddr_in addr;
	int rc;
	int addr_len;
	WinsockBufferType *packet;
	bool alternate = false;
	SOCKET socket;

	if (message == WM_UDPASYNCEVENT) {
		socket = Socket;
		if (wParam != socket) {
			for (int i = 0; i < PortCount; i++) {
				if (PortSockets[i] == wParam) {
					socket = PortSockets[i];
					break;
				}
			}
		}
	} else if (message == WM_UDPASYNCEVENT + 1) {
		socket = AlternateSocket;
		alternate = true;
	} else {
		return(1);
	}

	switch (WSAGETSELECTEVENT(lParam)) {

		case FD_READ:
			rc = WSAGETSELECTERROR(lParam);
			if (rc != 0) {
				DebugString("FD_READ - Got error code %d\n", rc);
				Clear_Socket_Error(socket);
				return(0);
			}

			addr_len = sizeof(addr);
			rc = recvfrom(socket, (char *)ReceiveBuffer, sizeof(ReceiveBuffer), 0, (LPSOCKADDR)&addr, &addr_len);
			if (rc == SOCKET_ERROR) {
				Clear_Socket_Error(socket);
				return(0);
			}

			if (rc) {
				in_addr from = addr.sin_addr; // RA2
				DebugString("recvfrom %d.%d.%d.%d ; %d\n", from.S_un.S_un_b.s_b1, from.S_un.S_un_b.s_b2, from.S_un.S_un_b.s_b3, from.S_un.S_un_b.s_b4, ntohs(addr.sin_port)); // RA2
				if (!alternate && !Session.ViaPacketRouter) {
					for (int i = 0; i < LocalAddresses.Count(); i++) {
						if (!memcmp(LocalAddresses[i], &addr.sin_addr.s_addr, 4)) return(0);
					}
				}

				int loss = Get_Packet_Loss_Percent();
				if (loss > 0) {
					if (NonCriticalRandomNumber(0, 99) <= loss) {
						return(0);
					}
				}

				packet = (WinsockBufferType *)Get_New_In_Buffer();
				packet->BufferLen = rc - sizeof(packet->CRC);
				packet->CRC = *((unsigned int *)(&ReceiveBuffer[0]));
				packet->IsAlternate = alternate;
				packet->Unused_18 = false;
				memcpy(packet->Buffer, ReceiveBuffer + sizeof(packet->CRC), packet->BufferLen);

				if (!Passes_CRC_Check(packet)) {
					memcpy(packet->Address + 4, &addr.sin_addr.s_addr, 4);
					unsigned int address = addr.sin_addr.s_addr;
					DebugString("Throwing away malformed packet from %d . %d . %d . %d ; %d\n", address & 0xff, (address >> 8) & 0xff, (address >> 16) & 0xff, address >> 24, ntohs(addr.sin_port));
					if (!IsMalformedPacketReported) {
						IsMalformedPacketReported = true;
					}
					if (packet->IsAllocated) {
						delete packet;
					} else {
						packet->InUse = false;
						OutBuffersUsed--;
					}
					return(0);
				} else {
					memset(packet->Address, 0, sizeof(packet->Address));
					if (addr.sin_addr.s_addr == htonl(INADDR_LOOPBACK)) {
						packet->SrcPort = 0;
						*(IPXAddressClass *)packet->Address = Session.unknown_2978;
					} else {
						packet->SrcPort = ntohs(addr.sin_port);
						memcpy(packet->Address + 4, &addr.sin_addr.s_addr, 4);
					}
					InBuffers.Add(packet);
				}
			}
			return(0);

		case FD_WRITE:
			if (SelectedSocket != INVALID_SOCKET) {
				for (int i = 0; i < PortCount; i++) {
					if (PortSockets[i] == SelectedSocket) {
						socket = SelectedSocket;
						DebugString("FD_WRITE - Using socket %d\n", socket);
					}
				}
			}

			rc = WSAGETSELECTERROR(lParam);
			if (rc != 0) {
				DebugString("FD_WRITE - Got error code %d\n", rc);
				Clear_Socket_Error(socket);
				return(0);
			}

			if (alternate) {
				if (AltOutBuffers.Count() == 0) {
					DebugString("FD_WRITE - No AltOutBuffers left - nothing to do\n");
					return(0);
				}
				packet = AltOutBuffers[0];
			} else {
				if (OutBuffers.Count() == 0) {
					DebugString("FD_WRITE - No OutBuffers left - nothing to do\n");
					return(0);
				}
				packet = OutBuffers[0];
			}

			memcpy(&addr.sin_addr.s_addr, packet->Address + 4, 4);
			addr.sin_port = htons(packet->SrcPort);
			addr.sin_family = AF_INET;

			if (alternate) {
				if (addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
					addr.sin_port = htons((unsigned short)AlternateSendPort);
					addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				}
			} else {
				if (packet->SrcPort) {
					addr.sin_port = htons(packet->SrcPort);
				} else {
					addr.sin_port = htons((unsigned short)UDPListenPort);
				}
				if (!memcmp(packet->Address + 4, (unsigned char *)&Session.unknown_2978 + 4, 4)
					&& (Session.Players.Count() == 0 || Session.unknown_2978 == Session.Players[0]->Address)) {
					addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
					addr.sin_port = htons((unsigned short)LoopbackPort);
				}
			}

			{
				unsigned char ip[4];
				memcpy(ip, &addr.sin_addr.s_addr, 4);
				if (Is_Reachable(ip, ntohs(addr.sin_port))) {
					DebugString("sendto %d.%d.%d.%d ; %d\n", ip[0], ip[1], ip[2], ip[3], ntohs(addr.sin_port));
					rc = sendto(socket, ((char const *)packet->Buffer) - sizeof(packet->CRC), packet->BufferLen + sizeof(packet->CRC), 0, (LPSOCKADDR)&addr, sizeof(addr));
					if (rc == SOCKET_ERROR) {
						if (LAST_ERROR != WSAEWOULDBLOCK) {
							DebugString("sendto returned error %d\n", LAST_ERROR);
							Clear_Socket_Error(socket);
							return(0);
						}
					}
				} else {
					DebugString("FD_WRITE - %d.%d.%d.%d ; %d is unreachable\n", ip[0], ip[1], ip[2], ip[3], ntohs(addr.sin_port));
				}
			}

			if (alternate) {
				AltOutBuffers.Delete_Index(0);
				if (packet->IsAllocated) {
					delete packet;
				} else {
					packet->InUse = false;
					AltOutBuffersUsed--;
				}
			} else {
				OutBuffers.Delete_Index(0);
				if (packet->IsAllocated) {
					delete packet;
				} else {
					packet->InUse = false;
					OutBuffersUsed--;
				}
			}
			return(0);
	}

	return(0);
}


/// <summary>
/// RA2
/// </summary>
ProtocolEnum UDPInterfaceClass::Get_Protocol (void)
{
	return(PROTOCOL_UDP);
}


/// <summary>
/// RA2
/// </summary>
int UDPInterfaceClass::Protocol_Event_Message (SOCKET socketnum)
{
	if (socketnum == Socket || socketnum == INVALID_SOCKET) {
		return(WM_UDPASYNCEVENT);
	}
	return(socketnum == AlternateSocket ? WM_UDPASYNCEVENT + 1 : 0);
}


/// <summary>
/// RA2
/// </summary>
int UDPInterfaceClass::Get_Num_Local_Addresses (void)
{
	return(LocalAddresses.Count());
}


/// <summary>
/// RA2
/// </summary>
unsigned char *UDPInterfaceClass::Get_Local_Address (int index)
{
	return(LocalAddresses[index]);
}


/// <summary>
/// RA2
/// </summary>
unsigned short UDPInterfaceClass::Get_Port (void)
{
	return(Port);
}

