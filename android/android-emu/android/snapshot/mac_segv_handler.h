// Copyright 2017 The Android Open Source Project
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

#include <inttypes.h>
#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef void (*mac_bad_access_callback_t)(void* faultvaddr);

void setup_mac_segv_handler(mac_bad_access_callback_t f);
void teardown_mac_segv_handler(void);

void register_segv_handling_range(void* ptr, uint64_t size);
bool is_ptr_registered(void* ptr);
void clear_segv_handling_ranges(void);

ANDROID_END_HEADER
