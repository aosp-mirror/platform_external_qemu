// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/AlignedBuf.h"

#include <gtest/gtest.h>

#include <vector>

using android::AlignedBuf;

TEST(AlignedBuf, Basic) {

    {
        AlignedBuf<uint32_t, 64> buf(10);
        EXPECT_EQ(0, (uintptr_t)buf.data() & (64 - 1));
        EXPECT_EQ(10, buf.size());
    }

    {
        AlignedBuf<uint32_t, 256> buf(10);
        EXPECT_EQ(0, (uintptr_t)buf.data() & (256 - 1));
        EXPECT_EQ(10, buf.size());
    }

    {
        AlignedBuf<uint32_t, 4096> buf(10);
        EXPECT_EQ(0, (uintptr_t)buf.data() & (4096 - 1));
        EXPECT_EQ(10, buf.size());
    }
}
