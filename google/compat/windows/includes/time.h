// Copyright 2023 The Android Open Source Project
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
#include_next <time.h>

#include "compat_compiler.h"

ANDROID_BEGIN_HEADER

#define 	CLOCK_MONOTONIC   1
typedef int clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *tp);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

ANDROID_END_HEADER