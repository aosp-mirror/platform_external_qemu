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

#include "android/utils/compiler.h"

#include <stddef.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

// An enum used to list of the types of Linux kernel images we need to
// handle. Unfortunately, this affects how we setup the kernel command-line
// when launching the system.
#define MAKE_KERNEL_VERSION(version, patchlevel, sublevel) \
    (((version) << 16) | ((patchlevel) << 8) | (sublevel))
typedef enum {
    KERNEL_VERSION_0 = 0,
    KERNEL_VERSION_2_6_29 = MAKE_KERNEL_VERSION(2, 6, 29),
    KERNEL_VERSION_3_4_0  = MAKE_KERNEL_VERSION(3, 4, 0),
    KERNEL_VERSION_3_4_67 = MAKE_KERNEL_VERSION(3, 4, 67),
    KERNEL_VERSION_3_10_0 = MAKE_KERNEL_VERSION(3, 10, 0),
    KERNEL_VERSION_4_4_124 = MAKE_KERNEL_VERSION(4, 4, 124),
    KERNEL_VERSION_4_14_112 = MAKE_KERNEL_VERSION(4, 14, 112),
    KERNEL_VERSION_4_14_150 = MAKE_KERNEL_VERSION(4, 14, 150),
    KERNEL_VERSION_5_4_0 = MAKE_KERNEL_VERSION(5, 4, 0),
} KernelVersion;
#undef MAKE_KERNEL_VERSION

// Gets the linux kernel version from |kernelPath| and stores it
// into |version|, e.g. "5.7.0-mainline-02031-..." is returned as 0x050700.
// On failure, returns false.
bool android_getKernelVersion(const char* kernelPath, KernelVersion* version);

// Return the serial device name prefix matching a given kernel type
// |kernelVersion|.  I.e. this should be "/dev/ttyS" for before 3.10.0 and
// "/dev/ttyGF" for 3.10.0 and later.
const char* android_kernelSerialDevicePrefix(KernelVersion kernelVersion);

ANDROID_END_HEADER
