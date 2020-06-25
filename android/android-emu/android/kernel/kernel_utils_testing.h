// Copyright 2014 The Android Open Source Project
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

#include "kernel_utils.h"

ANDROID_BEGIN_HEADER

// The same as android_getKernelVersion, but operates with kernel bits
// loaded into memory (for testing).
bool android_getKernelVersionImpl(const char* const kernelBits, size_t size,
                                  KernelVersion* version);

ANDROID_END_HEADER
