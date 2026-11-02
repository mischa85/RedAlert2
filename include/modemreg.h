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

#include <windows.h>


class ModemRegistryEntryClass {

	public:

		ModemRegistryEntryClass (int modem_number);
		~ModemRegistryEntryClass (void);

		void Clear(void);


		char *Get_Modem_Name (void) { return(ModemName); }

		char *Get_Modem_Device_Name (void) { return(ModemDeviceName); }

		char *Get_Modem_Error_Correction_Enable (void) { return(ErrorCorrectionEnable); }

		char *Get_Modem_Error_Correction_Disable (void) { return(ErrorCorrectionDisable); }

		char *Get_Modem_Compression_Enable (void) { return(CompressionEnable); }

		char *Get_Modem_Compression_Disable (void) { return(CompressionDisable); }

		char *Get_Modem_Hardware_Flow_Control (void) { return(HardwareFlowControl); }

	private:

		char *ModemName;
		char *ModemDeviceName;
		char *ErrorCorrectionEnable;
		char *ErrorCorrectionDisable;
		char *CompressionEnable;
		char *CompressionDisable;
		char *HardwareFlowControl;

};
