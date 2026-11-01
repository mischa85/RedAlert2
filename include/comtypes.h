/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C"{
#endif

#pragma pack(1)

struct CellStruct {
	short X;
	short Y;
};

struct CoordStruct {
	int X;
	int Y;
	int Z;
};

struct DirStruct{
	union {
		struct {
			short Value;
			short Padding;
		};

		int Raw;
	};
};

struct Point2DStruct {
	int X;
	int Y;
};

struct Matrix3DStruct {
	float Data[12];
};

#pragma pack()

#ifdef __cplusplus
}
#endif
