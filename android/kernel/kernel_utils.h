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

#ifndef ANDROID_KERNEL_KERNEL_UTILS_H
#define ANDROID_KERNEL_KERNEL_UTILS_H

#include "android/utils/compiler.h"
#include <stdint.h>

ANDROID_BEGIN_HEADER

// An enum used to list of the types of Linux kernel images we need to
// handle. Unfortunately, this affects how we setup the kernel command-line
// when launching the system.
//
// KERNEL_TYPE_LEGACY is any Linux kernel image before 3.10
// KERNEL_TYPE_3_10_OR_ABOVE is anything at 3.10 or above.
typedef enum {
    KERNEL_TYPE_LEGACY = 0,
    KERNEL_TYPE_3_10_OR_ABOVE = 1,
} KernelType;

// Probe the kernel image at |kernelPath| and returns the corresponding
// KernelType value. On success, returns true and sets |*ktype| appropriately.
// On failure (e.g. if the file doesn't exist or cannot be probed), return
// false and doesn't touch |*ktype|.
bool android_pathProbeKernelType(const char* kernelPath, KernelType *ktype);

// Return the serial device name prefix matching a given kernel type |ktype|.
// I.e. this should be "/dev/ttyS" for KERNEL_TYPE_LEGACY, and
// "/dev/ttyGF" for KERNEL_TYPE_3_10_OR_ABOVE.
const char* android_kernelSerialDevicePrefix(KernelType ktype);

ANDROID_END_HEADER

#endif  // ANDROID_KERNEL_KERNEL_UTILS_H
