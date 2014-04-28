// Copyright (C) 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_CPU_ACCELERATOR_H
#define ANDROID_CPU_ACCELERATOR_H

#include <stdbool.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Returns true if CPU acceleration is possible on this machine.
// If |status| is not NULL, on exit, |*status| will be set to a
// heap-allocated string describing the status of acceleration,
// to be freed by the caller.
bool android_hasCpuAcceleration(char** status);

ANDROID_END_HEADER

#endif  // ANDROID_CPU_ACCELERATOR_H

