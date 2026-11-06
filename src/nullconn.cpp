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

/* $Header: /CounterStrike/NULLCONN.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLCONN.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : April 5, 1995                            *
 *                                                                         *
 *                  Last Update : April 20, 1995   [DRD]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   NullModemConnClass::NullModemConnClass -- class constructor           *
 *   NullModemConnClass::~NullModemConnClass -- class destructor           *
 *   NullModemConnClass::Init -- hardware-dependent initialization         *
 *   NullModemConnClass::Send -- hardware-dependent packet sending         *
 *   NullModemConnClass::Compute_CRC -- computes CRC for given buffer      *
 *   NullModemConnClass::Packet_Overhead_Size -- number of extra bytes     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "nullconn.h"

#include "wincomm.h"


/***************************************************************************
 * NullModemConnClass::NullModemConnClass -- class constructor             *
 *                                                                         *
 * INPUT:                                                                  *
 *      numsend         desired # send queue entries                       *
 *      numreceive      desired # send receive entries                     *
 *      maxlen         max length of application's packets                 *
 *      magicnum         application-defined magic # for the packets       *
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
NullModemConnClass::NullModemConnClass (int numsend, int numreceive,
	int maxlen, unsigned short magicnum) :
	BASECLASS (numsend, numreceive, maxlen, magicnum,
		60, 			// Retry Delta Time
		-1, 			// Max Retries (-1 means ignore this timeout parameter)
		1200)			// Timeout: 20 seconds
{
	/*------------------------------------------------------------------------
	Pre-set the port value to NULL, so Send won't send until we've been Init'd
	------------------------------------------------------------------------*/
	PortHandle = INVALID_HANDLE_VALUE;
	/*------------------------------------------------------------------------
	Allocate the Send Buffer; the parent constructor has set MaxPacketLen,
	so we can use it in our computation.
	------------------------------------------------------------------------*/
	SendBuf = new char [ Actual_Max_Packet() ];

}	/* end of NullModemConnClass */


/***************************************************************************
 * NullModemConnClass::~NullModemConnClass -- class destructor             *
 *                                                                         *
 * INPUT:                                                                  *
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
NullModemConnClass::~NullModemConnClass (void)
{
	delete [] SendBuf;

}	/* end of ~NullModemConnClass */


/***************************************************************************
 * NullModemConnClass::Init -- hardware-dependent initialization           *
 *                                                                         *
 * INPUT:                                                                  *
 *      port      GreenLeaf port handle                                    *
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
void NullModemConnClass::Init (HANDLE port_handle)
{
	BASECLASS::Init();
	PortHandle = port_handle;
}

/***************************************************************************
 * NullModemConnClass::Send -- hardware-dependent packet sending           *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf         buffer to send                                         *
 *      buflen      length of buffer to send                               *
 *      extrabuf      (not used by this class)                             *
 *      extralen      (not used by this class)                             *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      1 = OK, 0 = error                                                  *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int NullModemConnClass::Send (char *buf, int buflen, void *, int )
{
	int *ibuf;
	SerialHeaderType *header;
	unsigned int sendlen;

	/*------------------------------------------------------------------------
	Error if we haven't been properly initialized
	------------------------------------------------------------------------*/
	if ( PortHandle == INVALID_HANDLE_VALUE ) return(false);

	/*------------------------------------------------------------------------
	Package the data into the Send Buffer
	------------------------------------------------------------------------*/
	header = (SerialHeaderType *) SendBuf;
	header->MagicNumber	= PACKET_SERIAL_START;
	header->Length			= (short) buflen;
	header->MagicNumber2	= PACKET_SERIAL_VERIFY;

	sendlen = sizeof( SerialHeaderType );
	memcpy (SendBuf + sendlen, buf, buflen);
	sendlen += buflen;
	ibuf = (int *)(SendBuf + sendlen);
	*ibuf = Compute_CRC( buf, buflen );
	sendlen += sizeof( int );

	*(SendBuf + sendlen) = '\r';
	sendlen += 1;

	/*------------------------------------------------------------------------
	Send the data
	------------------------------------------------------------------------*/
	SerialPort->Write_To_Serial_Port((unsigned char *)SendBuf, (int)sendlen );
	return(true);
}	/* end of Send */


/***************************************************************************
 * NullModemConnClass::Compute_CRC -- computes CRC for given buffer        *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf      buffer to compute CRC for                                 *
 *      buflen   length of buffer in bytes                                 *
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
int NullModemConnClass::Compute_CRC (char *buf, int buflen)
{
	unsigned int sum, hibit;

	sum = 0;
	for (int i = 0; i < buflen; i++) {
		if ( sum & 0x80000000 ) {		// check hi bit to rotate into low bit
			hibit = 1;
		}
		else {
			hibit = 0;
		}

		sum <<= 1;
		sum += (hibit + (unsigned char)buf[i]);
	}

	return((int)sum);

}	/* end of Compute_CRC */


/***************************************************************************
 * NullModemConnClass::Packet_Overhead_Size -- number of extra bytes       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      number of bytes used for communications only.                      *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/20/1995 DRD : Created.                                             *
 *=========================================================================*/
int NullModemConnClass::Packet_Overhead_Size ( void )
{
	/*------------------------------------------------------------------------
	short for Null Modem Magic Number
	short for Null Modem length of packet
	int for Null Modem CRC check
	CommHeaderType for Queued packets
	------------------------------------------------------------------------*/
	return( (PACKET_SERIAL_OVERHEAD_SIZE + sizeof(CommHeaderType)) );

}	/* end of Packet_Overhead_Size */

/************************** end of nullconn.cpp ****************************/
