// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// RA2: modules including this hold their own copy of these constants; include before globals.h, which defines LEVEL_LEPTON_H after them.
#pragma once

#include "geozero.h"

#include <cmath>
#include "fastmath.h"

const double ISO_TILE_SIZE = 60.0;
const double ONE_RAD = M_PI / 180.0;
const double CELL_LEPTON_DIAG = fastmath::sqrt(pow(256.0, 2) * 2);
const double LEPTONS_PER_PIXEL = CELL_LEPTON_DIAG / ISO_TILE_SIZE;
const double PIXELS_PER_LEPTON = 1.0 / LEPTONS_PER_PIXEL;
const double RAD_45 = ONE_RAD * 45;
const double RAD_60 = ONE_RAD * 60;
const double RAD_90 = ONE_RAD * 90;
