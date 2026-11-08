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

/* $Header: /CounterStrike/IPXCONN.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXCONN.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : April 9, 1995 [BRR]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   IPXConnClass::IPXConnClass -- class constructor                       *
 *   IPXConnClass::~IPXConnClass -- class destructor                       *
 *   IPXConnClass::Init -- hardware-specific initialization routine        *
 *   IPXConnClass::Configure -- One-time initialization routine            *
 *   IPXConnClass::Start_Listening -- commands IPX to listen               *
 *   IPXConnClass::Stop_Listening -- commands IPX to stop  listen          *
 *   IPXConnClass::Send -- sends a packet; invoked by SequencedConnection  *
 *   IPXConnClass::Open_Socket -- opens communications socket              *
 *   IPXConnClass::Close_Socket -- closes the socket                       *
 *   IPXConnClass::Send_To -- sends the packet to the given address        *
 *   IPXConnClass::Broadcast -- broadcasts the given packet                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "ipxconn.h"

#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "wsproto.h"


/*
********************************* Globals ***********************************
*/
#include "_wsproto.h"
#include "ipxgconn.h"
#include "ipxmgr.h"
#include "ownertalk.h"
#include "session.h"
#include "wspudp.h"
unsigned short 	IPXConnClass::Socket;
int			 		IPXConnClass::ConnectionNum;
//ECBType *			IPXConnClass::ListenECB;
IPXHeaderType *	IPXConnClass::ListenHeader;
char *				IPXConnClass::ListenBuf;
//ECBType *			IPXConnClass::SendECB;
IPXHeaderType *	IPXConnClass::SendHeader;
char *				IPXConnClass::SendBuf;
//long 					IPXConnClass::Handler;
int 					IPXConnClass::Configured = 0;
int 					IPXConnClass::SocketOpen = 0;
int 					IPXConnClass::Listening = 0;
int					IPXConnClass::PacketLen;


/***************************************************************************
 * IPXConnClass::IPXConnClass -- class constructor                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      numsend         desired # of entries for the send queue            *
 *      numreceive      desired # of entries for the receive queue         *
 *      maxlen         max length of an application packet                 *
 *      magicnum         the packet "magic number" for this connection     *
 *      address         address of destination (NULL = no address)         *
 *      id               connection's unique numerical ID                  *
 *      name            connection's name                                  *
 *      extralen         max size of app-specific extra bytes (optional)   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
IPXConnClass::IPXConnClass (int numsend, int numreceive, int maxlen,
	unsigned short magicnum, IPXAddressClass *address, int id, wchar_t const *name, // RA2: wide name
	int extralen) :
	BASECLASS (numsend, numreceive, maxlen, magicnum,
		2,          // retry delta
		-1,         // max retries
		60,         // timeout
		extralen)   // (currently, this is only used by the Global Channel)
{
	NetNumType net;
	NetNodeType node;

	/*------------------------------------------------------------------------
	Save the values passed in
	------------------------------------------------------------------------*/
	if (address)
		Address = (*address);
	ID = id;
	wcscpy (Name, name); // RA2: wide

	Address.Get_Address(net,node);
	memcpy(ImmediateAddress,node,6);
	Immed_Set = 0;
}	/* end of IPXConnClass */


/***************************************************************************
 * IPXConnClass::Init -- hardware-specific initialization routine          *
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
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Init (void)
{
	/*------------------------------------------------------------------------
	Invoke the parent's Init routine
	------------------------------------------------------------------------*/
	BASECLASS::Init();

}	/* end of Init */


/***************************************************************************
 * IPXConnClass::Configure -- One-time initialization routine              *
 *                                                                         *
 * This routine sets up static members that are shared by all IPX          *
 * connections (ie those variables used by the Send/Listen/Broadcast       *
 * routines).                                                              *
 *                                                                         *
 * INPUT:                                                                  *
 *      socket            socket ID for sending & receiving                *
 *      conn_num            local IPX Connection Number (0 = not logged in)*
 *      listen_ecb         ptr to ECBType for listening                    *
 *      send_ecb            ptr to ECBType for sending                     *
 *      listen_header      ptr to IPXHeaderType for listening              *
 *      send_header         ptr to IPXHeaderType for sending               *
 *      listen_buf         ptr to buffer for listening                     *
 *      send_buf            ptr to buffer for sending                      *
 *      handler_rm_ptr      REAL-MODE pointer to event service routine     *
 *                        (high word = segment, low word = offset)         *
 *      maxpacketlen      max packet size to listen for                    *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      - All pointers must be protected-mode pointers, but must point to  *
 *        DOS real-mode memory (except the Handler segment/offset)         *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Configure (unsigned short socket, int conn_num,
	/*ECBType *listen_ecb, ECBType *send_ecb, */IPXHeaderType *listen_header,
	IPXHeaderType *send_header, char *listen_buf, char *send_buf,
	/*long handler_rm_ptr, */int maxpacketlen)
{
	/*------------------------------------------------------------------------
	Save the values passed in
	------------------------------------------------------------------------*/
	Socket = socket;
	ConnectionNum = conn_num;
	//ListenECB = listen_ecb;
	//SendECB = send_ecb;
	ListenHeader = listen_header;
	SendHeader = send_header;
	ListenBuf = listen_buf;
	SendBuf = send_buf;
	//Handler = handler_rm_ptr;
	PacketLen = maxpacketlen;

	Configured = 1;

}	/* end of Configure */


/***************************************************************************
 * IPXConnClass::Start_Listening -- commands IPX to listen                 *
 *                                                                         *
 * This routine may be used to start listening in polled mode (if the      *
 * ECB's Event_Service_Routine is NULL), or in interrupt mode; it's        *
 * up to the caller to fill the ECB in.  If in polled mode, Listening      *
 * must be restarted every time a packet comes in.                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      - The ListenECB must have been properly filled in by the IPX Manager.*
 *      - Configure must be called before calling this routine.            *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Start_Listening(void)
{
	/*
	**	Open the socket.
	*/
	SocketOpen = Open_Socket(Socket);
	if (!SocketOpen)
		return(false);

	/*
	**	start listening on the socket.
	*/
	if ( PacketTransport->Start_Listening () ) {
		Listening =1;
		return(true);
	} else {
		Close_Socket(Socket);
		SocketOpen = false;
		return(false);
	}
}	/* end of Start_Listening */


/***************************************************************************
 * IPXConnClass::Stop_Listening -- commands IPX to stop  listen            *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      - This routine MUST NOT be called if IPX is not listening already! *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Stop_Listening(void)
{
	if ( PacketTransport ) PacketTransport->Stop_Listening();
	Listening = 0;

	// All done.
	return(1);
}	/* end of Stop_Listening */


/***************************************************************************
 * IPXConnClass::Send -- sends a packet; invoked by SequencedConnection    *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf         buffer to send                                         *
 *      buflen      length of buffer to send                               *
 *      extrabuf      (not used by this class)                             *
 *      extralen      (not used by this class)                             *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = OK, 0 = error                                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Send(char *buf, int buflen, void *, int, bool flag, unsigned short) // RA2: flag and port
{
	/*------------------------------------------------------------------------
	Invoke our own Send_To routine, filling in our Address as the destination.
	------------------------------------------------------------------------*/
	return(Send_To (buf, buflen, &Address, NULL, flag, 0)); // RA2: never through the immediate address
	}


/***************************************************************************
 * IPXConnClass::Open_Socket -- opens communications socket                *
 *                                                                         *
 * INPUT:                                                                  *
 *      socket      desired socket ID number                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = OK, 0 = error                                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Open_Socket(unsigned short socket)
{
	int rc;

	PacketTransport->Set_NetCard(Options.NetCard);
	rc = PacketTransport->Open_Socket(socket);

	SocketOpen = rc;
	return( rc );
}	/* end of Open_Socket */


/***************************************************************************
 * IPXConnClass::Close_Socket -- closes the socket                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      socket      desired socket ID number                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      Calling this routine when the sockets aren't open may crash!       *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Close_Socket(unsigned short socket)
{
	socket = socket;
	PacketTransport->Close_Socket();
	SocketOpen = 0;
}	/* end of Close_Socket */


/***************************************************************************
 * IPXConnClass::Send_To -- sends the packet to the given address          *
 *                                                                         *
 * The "ImmediateAddress" field of the SendECB must be filled in with the  *
 * address of a bridge, or the node address of the destination if there    *
 * is no bridge.  The NETX call to find this address will always crash     *
 * if NETX isn't loaded (ConnectionNum is 0), so this case is trapped &    *
 * prevented.                                                              *
 * Also, if the address of this IPX connection is known when the           *
 * constructor is called, and Configure has been called, Get_Local_Target  *
 * is called to precompute the ImmediateAddress; this case is detected &   *
 * if the value is already computed, it's just memcpy'd over.              *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf         buffer to send                                         *
 *      buflen      length of buffer                                       *
 *      address      Address to send to                                    *
 *      immed         ImmediateAddress value, NULL if none                 *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = OK, 0 = error                                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Send_To(char *buf, int buflen, IPXAddressClass *address,
	NetNodeType immed, bool flag, unsigned short port) // RA2: flag and port
{
	IPXAddressClass dest;
	unsigned short dest_port;
	dest = *address;


	if (immed) {
		NetNodeType node;
		NetNumType net;
		address->Get_Address(net, node);
		memcpy(net, immed, 4);
		dest.Set_Address(net, node, 0);
	}
	/*
	**	RA2: a packet of an internet game through the packet router goes to the router, with
	**	the mask of the player it is for; the host's own packets loop back.
	*/
	dest_port = 0;
	if (!flag && Session.ViaPacketRouter) {
		if (!((GlobalHeaderType *)buf)->Header.ForwardMask) {
			unsigned char result = OwnerTalk.Mask_From_ID(ID);
			if (!result) {
				return 0;
			}
			((GlobalHeaderType *)buf)->Header.ForwardMask = result;
		}
		dest = Session.PacketRouterAddress;
		dest_port = LoopbackPort;
		if (Session.unknown_2978 == Session.Players[0]->Address && dest.Is_Same_Node(Session.Players[0]->Address)) {
			((GlobalHeaderType *)buf)->Header.ForwardPort = 0;
			memset(&((GlobalHeaderType *)buf)->Header.ForwardAddress, 0, 4);
		}
		else {
			dest_port = Session.PacketRouterPort;
			((GlobalHeaderType *)buf)->Header.ForwardPort = LoopbackPort;
			memset(&((GlobalHeaderType *)buf)->Header.ForwardAddress, 0, 4);
		}
	}
	else {
		((GlobalHeaderType *)buf)->Header.ForwardPort = 0;
		memset(&((GlobalHeaderType *)buf)->Header.ForwardAddress, 0, 4);
	}

	/*
	**	RA2: a direct internet game sends to the port of the player on this connection.
	*/
	if (Session.Type == GAME_INTERNET && !Session.ViaPacketRouter) {
		int index = Ipx.Index_Of_Connection(this);
		switch (index) {
		case -3:
		case -2:
		case -1:
			goto none;
		default:
			if (index == 0 && Session.ViaPacketRouter) {
				if (Session.PacketRouterPort) {
					dest_port = Session.PacketRouterPort;
				}
			}
			else if (index >= 0 && index < Ipx.Num_Connections()) {
				dest_port = Session.Player_Port(Ipx.Connection_ID(index));
			}
			break;
		case -4:
			if (Session.PacketRouterPort) {
				dest_port = Session.PacketRouterPort;
			}
			break;
		}
		none:;
	}

	if (PacketTransport) {
		PacketTransport->WriteTo(buf, buflen, &dest, sizeof(dest), flag, false, dest_port);
	}
	return(1);
}	/* end of Send_To */


/***************************************************************************
 * IPXConnClass::Broadcast -- broadcasts the given packet                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      socket      desired socket ID number                               *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      1 = OK, 0 = error                                                  *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Broadcast(char *buf, int buflen)
{
	PacketTransport->Broadcast (buf, buflen, false);
	return(true);
}	/* end of Broadcast */


/************************** end of ipxconn.cpp *****************************/
