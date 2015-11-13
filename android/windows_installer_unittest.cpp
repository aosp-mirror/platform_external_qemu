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

#include "android/windows_installer.h"

#include <gtest/gtest.h>
#include <stdio.h>

namespace android {

TEST(WindowsInstaller, getVersion) {
    EXPECT_EQ(0, WindowsInstaller::getVersion("Google Play"));

    int32_t version = WindowsInstaller::getVersion(u8"Intel® Hardware Accelerated Execution Manager");
    printf("HAXM version: %08x\n", version);
}

}  // namespace android

