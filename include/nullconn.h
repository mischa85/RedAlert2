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

/* $Header: /CounterStrike/NULLCONN.H 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLCONN.H                               *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : April 3, 1995   [BR]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This is the Connection Class for a NULL-Modem connection.  It inherits  *
 * a Queue, PacketBuf, timeout variables from ConnectionClass.  It         *
 * inherits its Send_/Receive_/Get_Packet functions, and the non-sequenced *
 * ACK/Retry logic in Service_Send_Queue & Service_Receive_Queue from      *
 * ConnectionClass.                                                        *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "win.h"


/*
********************************* Includes **********************************
*/
#include "connect.h"

/*
********************************** Defines **********************************
*/
#define	PACKET_SERIAL_START				0xDABD
#define	PACKET_SERIAL_VERIFY				0xDEAF

#define	PACKET_SERIAL_OVERHEAD_SIZE	(sizeof( SerialHeaderType ) + sizeof( SerialCRCType ))

struct SerialHeaderType {
	unsigned short MagicNumber;
	unsigned short Length;
	unsigned short MagicNumber2;
};

struct SerialCRCType {
	int SerialCRC;
};

struct PORT {
	/// Opaque stand-in for the serial library's port structure; only ever used as a pointer.
};


/*
***************************** Class Declaration *****************************
*/
class NullModemConnClass : public ConnectionClass
{
		typedef ConnectionClass BASECLASS;

	/*
	---------------------------- Public Interface ----------------------------
	*/
	public:
		/*.....................................................................
		Constructor/destructor.
		.....................................................................*/
		NullModemConnClass (int numsend, int numrecieve, int maxlen,
			unsigned short magicnum);
		virtual ~NullModemConnClass () override;

		/*.....................................................................
		Initialization.
		.....................................................................*/
		void Init (HANDLE port_handle);
		virtual void Init (void) override { BASECLASS::Init(); }

		/*.....................................................................
		Utility routines.
		.....................................................................*/
		unsigned int Actual_Max_Packet (void) { return (MaxPacketLen +
			(sizeof(SerialHeaderType)) + sizeof(int) + sizeof (char)); }

		/*.....................................................................
		This routine computes a CRC value for the given buffer.
		.....................................................................*/
		static int Compute_CRC(char *buf, int buflen);

		/*.....................................................................
		This routine returns the number of bytes extra added the packet
		for communication.
		.....................................................................*/
		static int Packet_Overhead_Size( void );

	/*
	--------------------------- Private Interface ----------------------------
	*/
	protected:
		/*.....................................................................
		This routine actually performs a hardware-dependent data send.
		.....................................................................*/
		virtual int Send (char *buf, int buflen, void *extrabuf, int extralen) override;

		/*
		**	This is the winsoze port handle
		*/
		HANDLE PortHandle;

		/*.....................................................................
		This buffer is a staging area for data sent out; it includes the
		packet sent by the parent class (which includes the application's
		packet, plus the CommHeaderType header), plus:
		- 2-byte buffer start ID
		- 2-byte length
		- 4-byte CRC value (at the end of the buffer)
		This is the actual packet that gets sent across the serial line.
		.....................................................................*/
		char *SendBuf;
};

/************************** end of nullconn.h ******************************/
