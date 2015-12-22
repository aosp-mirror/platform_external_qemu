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

#pragma once

#include <stdbool.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// don't change these numbers
// Android Studio depends on them
typedef enum {
    ANDROID_CPU_ACCELERATION_READY                    = 0, // Acceleration is available
    ANDROID_CPU_ACCELERATION_NESTED_NOT_SUPPORTED     = 1, // HAXM doesn't support nested VM
    ANDROID_CPU_ACCELERATION_INTEL_REQUIRED           = 2, // HAXM requires GeniuneIntel processor
    ANDROID_CPU_ACCELERATION_NO_CPU_SUPPORT           = 3, // CPU doesn't support required features (VT-x or SVM)
    ANDROID_CPU_ACCELERATION_NO_CPU_VTX_SUPPORT       = 4, // CPU doesn't support VT-x
    ANDROID_CPU_ACCELERATION_NO_CPU_NX_SUPPORT        = 5, // CPU doesn't support NX
    ANDROID_CPU_ACCELERATION_ACCEL_NOT_INSTALLED      = 6, // KVM/HAXM package not installed
    ANDROID_CPU_ACCELERATION_ACCEL_OBSOLETE           = 7, // HAXM package is obsolete
    ANDROID_CPU_ACCELERATION_DEV_NOT_FOUND            = 8, // /dev/kvm is not found: VT disabled in BIOS or KVM kernel module not loaded
    ANDROID_CPU_ACCELERATION_VT_DISABLED              = 9, // HAXM is installed but VT disabled in BIOS
    ANDROID_CPU_ACCELERATION_NX_DISABLED              = 10, // HAXM is installed but NX disabled in BIOS
    ANDROID_CPU_ACCELERATION_DEV_PERMISSION           = 11, // /dev/kvm or HAXM device: permission denied
    ANDROID_CPU_ACCELERATION_DEV_OPEN_FAILED          = 12, // /dev/kvm or HAXM device: open failed
    ANDROID_CPU_ACCELERATION_DEV_IOCTL_FAILED         = 13, // /dev/kvm or HAXM device: ioctl failed
    ANDROID_CPU_ACCELERATION_DEV_OBSOLETE             = 14, // KVM or HAXM supported API is too old
    ANDROID_CPU_ACCELERATION_HYPERV_ENABLED           = 15, // HyperV must be disabled
    ANDROID_CPU_ACCELERATION_ERROR                    = 138, // Some other error occurred
} AndroidCpuAcceleration;

// ditto
typedef enum {
    ANDROID_HYPERV_ABSENT       = 0,    // No hyper-V found
    ANDROID_HYPERV_INSTALLED    = 1,    // Hyper-V is installed but not running
    ANDROID_HYPERV_RUNNING      = 2,    // Hyper-V is up and running
    ANDROID_HYPERV_ERROR        = 100,  // Failed to detect status
} AndroidHyperVStatus;

/* Returns ANDROID_CPU_ACCELERATION_READY if CPU acceleration is
 *  possible on this machine.  If |status| is not NULL, on exit,
 * |*status| will be set to a heap-allocated string describing
 * the status of acceleration, to be freed by the caller.
 */
AndroidCpuAcceleration androidCpuAcceleration_getStatus(char** status);

ANDROID_END_HEADER
