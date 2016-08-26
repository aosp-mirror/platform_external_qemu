// Copyright 2016 The Android Open Source Project
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
#include <inttypes.h>

ANDROID_BEGIN_HEADER

void pipe_control_add_pipe(void*);
void pipe_control_remove_pipe(void*);
void pipe_control_stop_pipe(uint64_t);
void pipe_control_list_pipes(char** out);

ANDROID_END_HEADER
