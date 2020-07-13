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

#include <vector>
#include <gtest/gtest.h>
#include "android/kernel/kernel_utils.h"
#include "android/kernel/kernel_utils_testing.h"

namespace android {
namespace kernel {

TEST(KernelUtils, GetKernelSerialDevicePrefix) {
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_2_6_29));
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_4_0));
    EXPECT_STREQ("ttyS",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_4_67));
    EXPECT_STREQ("ttyGF",
                 android_kernelSerialDevicePrefix(KERNEL_VERSION_3_10_0));
}

TEST(KernelUtils, getKernelVersionOffset) {
    std::vector<char> kernelBits(1024);
    KernelVersion version;

    const unsigned kVersionOffset = 803; // random number
    const unsigned kOffsetToWrite = kVersionOffset - 512;
    const unsigned kOffsetOffset = 526;

    uint8_t* offsetBits = reinterpret_cast<uint8_t *>(&kernelBits[kOffsetOffset]);
    offsetBits[0] = kOffsetToWrite & 0xff;
    offsetBits[1] = kOffsetToWrite >> CHAR_BIT;

    strcpy(&kernelBits[kVersionOffset], "3.4.67-something");
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(), 0, &version));
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(),
                                                  kOffsetOffset,
                                                  &version));
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(),
                                                  kVersionOffset,
                                                  &version));
    EXPECT_EQ(true, android_getKernelVersionImpl(kernelBits.data(),
                                                 kernelBits.size(),
                                                 &version));
    EXPECT_EQ(version, KERNEL_VERSION_3_4_67);

    strcpy(&kernelBits[kVersionOffset], "not a kernel version");
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(),
                                                  kernelBits.size(),
                                                  &version));

    strcpy(&kernelBits[kVersionOffset], "3.4.280-something");
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(),
                                                  kernelBits.size(),
                                                  &version));

    strcpy(&kernelBits[kVersionOffset], "4.14.150-something");
    EXPECT_EQ(true, android_getKernelVersionImpl(kernelBits.data(),
                                                 kernelBits.size(),
                                                 &version));
    EXPECT_EQ(version, KERNEL_VERSION_4_14_150);
}

TEST(KernelUtils, getKernelVersionPrefix) {
    std::string kernelBits;
    KernelVersion version;

    kernelBits = "random bits Linux version 3.4.67-something";
    EXPECT_EQ(true, android_getKernelVersionImpl(kernelBits.data(),
                                                 kernelBits.size(),
                                                 &version));
    EXPECT_EQ(version, KERNEL_VERSION_3_4_67);

    kernelBits = "random bits Linux version something";
    EXPECT_EQ(false, android_getKernelVersionImpl(kernelBits.data(),
                                                  kernelBits.size(),
                                                  &version));

    kernelBits = "random bits Linux version something Linux version 4.14.150-something";
    EXPECT_EQ(true, android_getKernelVersionImpl(kernelBits.data(),
                                                 kernelBits.size(),
                                                 &version));
    EXPECT_EQ(version, KERNEL_VERSION_4_14_150);
}

}  // namespace kernel
}  // namespace android
