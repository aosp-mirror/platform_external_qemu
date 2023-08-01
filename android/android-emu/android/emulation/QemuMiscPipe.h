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
#include <stdint.h>

ANDROID_BEGIN_HEADER

void android_init_qemu_misc_pipe(void);
int get_guest_heart_beat_count(void);
void set_restart_when_stalled(void);
void signal_system_reset_was_requested(void);
int64_t get_uptime_since_reset(void);

ANDROID_END_HEADER

