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

#include "android/cmdline-option.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Return a heap-allocated string containing the kernel parameter.
// |opts| corresponds to the command-line options after they have been
// processed by emulator_parseCommonCommandLineOptions().
// |is_qemu2| is true to indicate that this is called from QEMU2, otherwise
// QEMU1 is assumed.
char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   const char* kernelSerialPrefix,
                                   bool is_qemu2);

ANDROID_END_HEADER
