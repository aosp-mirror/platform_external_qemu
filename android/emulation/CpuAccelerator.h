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

#ifndef ANDROID_EMULATION_CPU_EMULATOR_H
#define ANDROID_EMULATION_CPU_EMULATOR_H

#include "android/base/String.h"

namespace android {

using ::android::base::String;

// The list of CPU emulation acceleration technologies supported by the
// Android emulator.
//  CPU_ACCELERATOR_NONE means no acceleration is supported on this machine.
//
//  CPU_ACCELERATOR_KVM means Linux KVM, which requires a specific driver
//  to be installed and that /dev/kvm is properly accessible by the current
//  user.
//
//  CPU_ACCELERATOR_HAX means Intel's Hardware Accelerated eXecution,
//  which can be installed on Windows and OS X machines running on an
//  Intel processor.
//
enum CpuAccelerator {
    CPU_ACCELERATOR_NONE = 0,
    CPU_ACCELERATOR_KVM,
    CPU_ACCELERATOR_HAX,
};

// Return the CPU accelerator technology usable on the current machine.
// This only returns CPU_ACCELERATOR_KVM or CPU_ACCELERATOR_HAX if the
// corresponding accelerator can be used properly. Otherwise it will
// return CPU_ACCELERATOR_NONE.
CpuAccelerator  GetCurrentCpuAccelerator();

// Return an ASCII string describing the state of the current CPU
// acceleration on this machine. If GetCurrentCpuAccelerator() returns
// CPU_ACCELERATOR_NONE this will contain a small explanation why
// the accelerator cannot be used.
String GetCurrentCpuAcceleratorStatus();

// For unit testing/debugging purpose only, must be called before
// GetCurrentCpuAccelerator().
void SetCurrentCpuAcceleratorForTesting(CpuAccelerator accel,
                                        const char* status);

}  // namespace android

#endif  // ANDROID_EMULATION_CPU_EMULATOR_H
