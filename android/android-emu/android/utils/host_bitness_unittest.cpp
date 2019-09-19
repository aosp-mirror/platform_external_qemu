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

#include "android/utils/host_bitness.h"

#include <stdio.h>                  // for printf
#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult
#include <ostream>                  // for operator<<

#include "gtest/gtest_pred_impl.h"  // for Test, AssertionResult, SuiteApiRe...

TEST(HostBitness, android_getHostBitness) {
    int host_bitness = android_getHostBitness();
    printf("Found host bitness: %d\n", host_bitness);
    EXPECT_TRUE(host_bitness == 32 || host_bitness == 64) <<
        "Invalid host bitness: " << host_bitness;
}
