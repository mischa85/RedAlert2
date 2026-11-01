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
 *                 Project Name : Command & Conquer/ WW Library                                *
 *                                                                                             *
 *                    File Name : WINCOMM.H                                                    *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 1/10/96                                                      *
 *                                                                                             *
 *                  Last Update : January 10th 1996 [ST]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *                                                                                             *
 *   These classes was created to replace the greenleaf comms functions used in C&C DOS with   *
 *  WIN32 API calls.                                                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "win.h"

enum WinCommDialMethodType {
	WC_TOUCH_TONE = 0,
	WC_PULSE
};



#define	COMMSUCCESS		0
#define 	ASTIMEOUT      -10
#define	COMMUSERABORT	-16


/*
**	The size of our serial buffer within the class.
**
** !!!!!! THIS MUST BE  A POWER OF 2 !!!!!!
**
*/
#define SIZE_OF_WINDOWS_SERIAL_BUFFER 2048 * 4



/*
**	WinModemClass.
**
**	This class provides access to modems under Win95. The functions are designed to be more or less
**	drop in replacements for the Grenleaf comms functions.
*/

class WinModemClass
{

	public:

					WinModemClass (void);   //WinModemClass Contructor
		virtual	~WinModemClass (void);      //WinModemClass Destructor


					/*
					**	Serial port open should be called to get a handle to the COM port
					**	This needs to be called first as other class members rely on the handle
					**
					**	Replacement for Greenleaf function: PortOpenGreenleafFast
					*/
		//virtual	HANDLE	Serial_Port_Open (int port, int baud, int parity, int wordlen, int stopbits);
		virtual	HANDLE	Serial_Port_Open (char *device_name, int baud, int parity, int wordlen, int stopbits);

					/*
					**	This function releases the COM port handle and should be called after
					**	communications have finished
					**
					**	Replacement for Greenleaf function: PortClose
					*/
					void		Serial_Port_Close (void);

					/*
					**	This member copies any bytes from the internal class serial buffer
					**	into your user buffer.
					**
					**	Replacement for Greenleaf function: ReadBuffer
					*/
					int		Read_From_Serial_Port (unsigned char *dest_ptr, int buffer_len);

					/*
					**	Write chars to the serial port
					**
					**	Replacement for Greenleaf function: WriteBuffer
					*/
					void		Write_To_Serial_Port (unsigned char *buffer, int length);

					/*
					**	Wait for the outgoing buffer to empty
					*/
					void		Wait_For_Serial_Write (void);

					/*
					**	Set the dial type to DIAL_TOUCH_TONE or DIAL_PULSE
					**
					**	Replacement for Greenleaf function: HMSetDiallingMethod
					*/
		virtual	void		Set_Modem_Dial_Type (WinCommDialMethodType method);

					/*
					**	Get the status of the modem control lines
					**	Possible flags are: CTS_SET DSR_SET RI_SET & CD_SET
					**
					**	Replacement for Greenleaf function: GetModemStatus
					*/
		virtual	unsigned	Get_Modem_Status (void);

					/*
					**	Set the DTR line to the given state
					**
					**	Replacement for Greenleaf function: SetDtr
					*/
		virtual	void		Set_Serial_DTR (bool state);

					/*
					**	Get the result code from the modem after issuing an 'AT' command
					**
					**	Replacement for Greenleaf function: HMInputLine
					*/
		virtual	int		Get_Modem_Result (int delay, char *buffer, int buffer_len, int inter_char_delay);

					/*
					**	Issue a dial command to the modem.
					**	Use Set_Modem_Dial_Type to select pulse or tone dial
					**
					**	Replacement for Greenleaf function: HMDial
					*/
		virtual	void		Dial_Modem (char *dial_number);

					/*
					**	Send a command to the modem. This is usually an 'AT' command.
					**	Function will optionally retry until 'OK' is received.
					*/
		virtual	int		Send_Command_To_Modem (char const *command, char terminator, char *buffer, int buflen, int delay, int retries, int inter_char_delay);

					/*
					**	Sets a pointer to a function that will be called for each incoming serial char
					**
					**	Replacement for Greenleaf function: HMSetUpEchoRoutine
					*/
		virtual	void		Set_Echo_Function (void(*func)(char c));

					/*
					**	Sets a pointer to a function that will be called if ESC is pressed during a dial
					**
					**	Replacement for Greenleaf function: HMSetUpAbortKey
					*/
		virtual	void		Set_Abort_Function (int (*func)(void));

					/*
					**	Member to allow access to the serial port handle
					*/
					HANDLE	Get_Port_Handle(void);

		bool				Enter_Failsafe_Mode(void);
		void				Leave_Failsafe_Mode(void);

		void				Debug_Status(DWORD error, COMSTAT status);

		/*
		**	Status vars for debugging purposes
		*/
		int FramingErrors;
		int IOErrors;
		int BufferOverruns;
		int InBufferOverflows;
		int ParityErrors;
		int OutBufferOverflows;
		int InQueue;
		int OutQueue;

		/*
		**	Modem send result codes
		*/
		enum SendModemEnum {
			MODEM_CMD_TIMEOUT = 0,
			MODEM_CMD_ERROR,
			MODEM_CMD_OK,
			MODEM_CMD_0,
		};


		/*
		**	Enums for modem status flags
		*/
		enum {
			CTS_SET  = 0x10,
			DSR_SET  = 0x20,
			RI_SET   = 0x40,
			CD_SET   = 0x80
		};

		enum {
			COM1 = 0,
			COM2 = 1,
			COM3 = 2,
			COM4 = 3,
			COM5 = 4,
		};


	protected:


		/*
		**	Copy incoming data from the windows file buffer into the internal class buffer
		*/
		bool						Read_Serial_Chars(void);

		/*
		**	Pointer to the internal class circular buffer for incoming data
		*/
		unsigned char				*SerialBuffer;

		/*
		**	Overlap object for asyncronous reads from the serial port
		*/
		OVERLAPPED					ReadOverlap;

		/*
		**	Overlap object for asyncronous writes to the serial port
		*/
		OVERLAPPED					WriteOverlap;

		/*
		**	Flag that there is no outstanding incoming data in the windows buffer
		*/
		bool							WaitingForSerialCharRead;

		/*
		**	Flag that we are waiting for the last write to port operation to complete
		*/
		bool							WaitingForSerialCharWrite;

		/*
		**	Head and Tail pointers for our internal serial buffer
		*/
		int							SerialBufferReadPtr;
		int							SerialBufferWritePtr;

		/*
		**	Windows handle to the COM port device
		*/
		HANDLE						PortHandle;

		/*
		**	Dialing method - DIAL_TOUCH_TONE or DIAL_PULSE
		*/
		WinCommDialMethodType	DialingMethod;

		/*
		**	Pointer to function for echoing incoming data - can be NULL
		*/
		void 							(*EchoFunction)(char c);

		/*
		**	Pointer to function for aborting when ESC pressed - can be NULL
		*/
		int							(*AbortFunction)(void);

		/*
		**	Serial buffer for asyncronous reads
		*/
		char							TempSerialBuffer[SIZE_OF_WINDOWS_SERIAL_BUFFER];

		/*
		 * These are the baud rate, parity and handshaking settings the port is opened with.
		 * They are kept so that the port can be reconfigured without gathering them again.
		 */
		DCB							DeviceControl;
};


/*
**	WinNullModemClass.
**
**	This class provides access to serial ports under Win95. The functions are designed to be more or less
**	drop in replacements for the Grenleaf comms functions.
**
**	This class just overloads the WinModemClass members that arent required for direct serial communications
**	via a 'null modem' cable.
*/
class WinNullModemClass : public WinModemClass
{
		typedef WinModemClass BASECLASS;

	public:

		virtual	inline	void		Set_Modem_Dial_Type (int){};
		virtual	inline	unsigned	Get_Modem_Status (void) override {return(0);};
		virtual	inline	void		Set_Serial_DTR (BOOL){};
		virtual	inline	int		Get_Modem_Result (int, char*, int){return(0);};
		virtual	inline	void		Dial_Modem (char*){};
		virtual	inline	int		Send_Command_To_Modem (char const*, char, char*, int, int, int){return(0);};
		virtual	inline	void		Set_Echo_Function (void(*)(char)){};
		virtual	inline	void		Set_Abort_Function (int(*)(void)){};

};


extern WinModemClass *SerialPort;
