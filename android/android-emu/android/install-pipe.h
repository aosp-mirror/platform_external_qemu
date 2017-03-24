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

ANDROID_BEGIN_HEADER

// Initialize the 'install' pipe.
void android_init_install_pipe(void);

// ask Install Pipe to send apks to guest
// if pipe is idle, this will return true;
// if pipe is still busy, this will return false;

bool android_kick_install_pipe(const char* filename);

ANDROID_END_HEADER
