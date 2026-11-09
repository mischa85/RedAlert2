// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// RA2: the rest of the per-module geometry constants; include after globals.h and coord.h.
#pragma once

const double CELL_SLOPE_ANGLE = fastmath::atan(LEVEL_LEPTON_H * (1 / 256.0));
const double CELL_DIAG_SLOPE_ANGLE = fastmath::atan((LEVEL_LEPTON_H * 2) / CELL_LEPTON_DIAG);
const int BRIDGE_LEPTON_HEIGHT = (int)((LEVEL_LEPTON_H * 4) + 0.5);
const Cell CELL_NONE(0, 0);
const Coord COORD_NONE(0, 0, 0);
