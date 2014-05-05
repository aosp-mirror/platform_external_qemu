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

#include "android/cpu_accelerator.h"

// This source acts as a small C++ -> C bridge between android/emulation/
// and android/main.c

#include "android/base/String.h"
#include "android/emulation/CpuAccelerator.h"

#include "android/utils/system.h"

extern "C" bool android_hasCpuAcceleration(char** status_p) {
    android::CpuAccelerator accel = android::GetCurrentCpuAccelerator();

    if (status_p) {
        android::base::String status =
                android::GetCurrentCpuAcceleratorStatus();
        *status_p = ASTRDUP(status.c_str());
    }

    return accel != android::CPU_ACCELERATOR_NONE;
}
