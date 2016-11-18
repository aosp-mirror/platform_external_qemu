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

#include <stdint.h>

ANDROID_BEGIN_HEADER

// List of values describing how EGL/GLES emulation should work in a given
// Android virtual device.
//
// kAndroidGlesEmulationOff
//    Means there is no GPU emulation, equivalent to "-gpu off" and instructs
//    the guest system to use its old GLES 1.x software renderer.
//
// kAndroidGlesEmulationHost
//    Means Host GPU emulation is being used. All EGL/GLES commands are
//    sent to the host GPU or CPU through a simple wire protocol. This
//    corresponds to "-gpu host" and "-gpu mesa".
//
// kAndroidGlesEmulationGuest
//    Means a guest GLES 2.x library (e.g. SwiftShader) is being used in
//    the guest. This should only be used with accelerated emulation, or
//    results will be very very slow.
typedef enum {
    kAndroidGlesEmulationOff = 0,
    kAndroidGlesEmulationHost,
    kAndroidGlesEmulationGuest,
} AndroidGlesEmulationMode;

// Return a heap-allocated string containing the kernel parameter.
// |opts| corresponds to the command-line options after they have been
// processed by emulator_parseCommonCommandLineOptions().
// |targetArch| is the target architecture. (e.g. 'arm64')
// |apiLevel| is the AVD's API level.
// |kernelSerialPrefix| is the guest tty device prefix (e.g. 'ttyS')
// |avdKernelParameters| are the optional extra kernel parameters stored
// in the AVD's kernel.parameters hardware property, if any. They will
// be appended to the result.
// |glesMode| is the EGL/GLES emulation mode.
// |glesGuestCmaMB| is the size in megabytes of the contiguous memory
// allocation to be used when |glesMode| is kAndroidGlesEmulationGuest.
// A value of 0 also indicates to ignore this setting.
// |isQemu2| is true to indicate that this is called from QEMU2, otherwise
// QEMU1 is assumed.
char* emulator_getKernelParameters(const AndroidOptions* opts,
                                   const char* targetArch,
                                   int apiLevel,
                                   const char* kernelSerialPrefix,
                                   const char* avdKernelParameters,
                                   AndroidGlesEmulationMode glesMode,
                                   uint64_t glesGuestCmaMB,
                                   bool isQemu2);

ANDROID_END_HEADER
