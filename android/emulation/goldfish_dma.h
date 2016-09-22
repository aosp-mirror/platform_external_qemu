/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "android/utils/compiler.h"

#include <inttypes.h>
#include <stdbool.h>

ANDROID_BEGIN_HEADER

// GOLDFISH DMA DEVICE
// The Goldfish dma virtual device is designed to do one thing:
// transfer data from guest to host.
void* getGoldfishDMA();
void setGoldfishDMA(void* x);

ANDROID_END_HEADER
