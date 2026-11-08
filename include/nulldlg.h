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

#pragma once

#include "session.h"

bool Init_Null_Modem( SerialSettingsType *settings );
void Shutdown_Modem( void );
void Modem_Signoff( void );
int Test_Null_Modem( void );
bool Reconnect_Modem( void );
void Destroy_Null_Connection(int id, int error);
GameType Select_Serial_Dialog( void );


//
// how much time (ticks) to go by before thinking other system
// is not responding.
//
#define PACKET_SENDING_TIMEOUT	1800
#define PACKET_CANCEL_TIMEOUT		900
#define SIGNOFF_PACKET_CANCEL_TIMEOUT		300


//
// how much time (ticks) to go by before sending another packet
// of game options or serial connect.
//
#define PACKET_RETRANS_TIME	30
#define PACKET_REDRAW_TIME		60
