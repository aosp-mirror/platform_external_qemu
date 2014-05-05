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

#include "android/emulation/CpuAccelerator.h"

#include <stdio.h>

#include <gtest/gtest.h>

namespace android {

class CpuAcceleratorTest : public ::testing::Test {
public:
    CpuAcceleratorTest() {
        saved_accel_ = GetCurrentCpuAccelerator();
        saved_status_ = GetCurrentCpuAcceleratorStatus();
    }

    ~CpuAcceleratorTest() {
        // Restore previous state.
        SetCurrentCpuAcceleratorForTesting(saved_accel_,
                                           saved_status_.c_str());
    }
private:
    CpuAccelerator saved_accel_;
    String saved_status_;
};

// Not really a test, but a simple way to print the current accelerator
// value for simple verification.
TEST_F(CpuAcceleratorTest, Default) {
    CpuAccelerator accel = GetCurrentCpuAccelerator();
    String status = GetCurrentCpuAcceleratorStatus();

    switch (accel) {
    case CPU_ACCELERATOR_NONE:
        printf("No acceleration possible on this machine!\n");
        break;

    case CPU_ACCELERATOR_KVM:
        printf("KVM acceleration usable on this machine!\n");
        break;

    case CPU_ACCELERATOR_HAX:
        printf("HAX acceleration usable on this machine!\n");
        break;

    default:
        ASSERT_FALSE(1) << "Invalid accelerator value: " << accel;
    }
    printf("Status: %s\n", status.c_str());
}

}  // namespace android
