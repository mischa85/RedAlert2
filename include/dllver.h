/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/


#pragma once

#include "win.h"

#define PACKVERSION(major, minor) MAKELONG(minor, major)

DWORD GetDllVersion(LPCSTR dllName);
