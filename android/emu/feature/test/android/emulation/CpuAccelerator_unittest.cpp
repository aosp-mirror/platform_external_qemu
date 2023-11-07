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
#include "android/emulation/internal/CpuAccelerator.h"

#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <stdio.h>

using android::base::System;
using android::base::Version;


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
                                           saved_status_code_,
                                           saved_status_.c_str());
    }
private:
    CpuAccelerator saved_accel_;
    AndroidCpuAcceleration saved_status_code_;
    std::string saved_status_;
};

// Not really a test, but a simple way to print the current accelerator
// value for simple verification.
TEST_F(CpuAcceleratorTest, Default) {
    CpuAccelerator accel = GetCurrentCpuAccelerator();
    std::string status = GetCurrentCpuAcceleratorStatus();

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

    case CPU_ACCELERATOR_HVF:
        printf("HVF acceleration usable on this machine!\n");
        break;
    case CPU_ACCELERATOR_WHPX:
        printf("WHPX acceleration usable on this machine!\n");
        break;
    case CPU_ACCELERATOR_AEHD:
        printf("AEHD acceleration usable on this machine!\n");
        break;
    case  CPU_ACCELERATOR_MAX:
        ASSERT_FALSE(1) << "Invalid accelerator value: " << accel;
    }
    printf("Status: %s\n", status.c_str());
}

TEST(CpuAccelerator, GetCpuInfo) {
    auto info = android::GetCpuInfo();
    EXPECT_NE(info.first, ANDROID_CPU_INFO_FAILED);
    EXPECT_GT(info.second.size(), 0U);
}

#ifdef __APPLE__

TEST(CpuAccelerator, GetMacOSVersionOnMac) {
    std::string osVersionStr = System::get()->getOsName();

    printf("Mac OS version string: length %zu string: [%s]\n",
           osVersionStr.size(), osVersionStr.c_str());

    std::string status;
    EXPECT_GT(parseMacOSVersionString(osVersionStr, &status), Version(9,0,0));
    EXPECT_EQ(0, status.size());
}

#endif // __APPLE__

TEST(CpuAccelerator, GetMacOSVersionGeneral) {
    std::string osVersionStr = "Mac OS X 10.13.6";
    std::string status;
    EXPECT_GT(parseMacOSVersionString(osVersionStr, &status), Version(9,0,0));

    EXPECT_EQ(0, status.size());
}

}  // namespace android
