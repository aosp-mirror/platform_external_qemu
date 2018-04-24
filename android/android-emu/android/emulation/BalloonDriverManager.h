// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>
ANDROID_BEGIN_HEADER

typedef bool (*balloon_driver_t) (int64_t);
void android_start_balloon(unsigned int hwRamSize, balloon_driver_t func);

ANDROID_END_HEADER
