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

#include "android/base/misc/Random.h"

#include <gtest/gtest.h>

#include <string.h>

namespace android {

TEST(Random, genRandomBytesWithSimpleBuffer) {
    const size_t kSize = 16;

    uint8_t buffer1[kSize] = {};

    // Verify that genRandomBytes doesn't generate an all-0 buffer.
    // Theorically, it's possible but 1/2^64 unlikely.
    android::genRandomBytes(buffer1, kSize);
    int numZeroes = 0;
    for (const auto x : buffer1) {
        if (x == 0)
            numZeroes++;
    }
    EXPECT_LT(numZeroes, kSize);
}

TEST(Random, genRandomBytesWithTwoBuffers) {
    const size_t kSize = 8;

    uint8_t buffer1[kSize] = {};
    uint8_t buffer2[kSize] = {};

    // Verify that genRandomBytes() doesn't generate the same sequence twice.
    android::genRandomBytes(buffer1, kSize);
    android::genRandomBytes(buffer2, kSize);
    EXPECT_TRUE(memcmp(buffer1, buffer2, kSize));
}

}  // namespace android
