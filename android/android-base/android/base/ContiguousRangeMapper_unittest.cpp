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

#include "android/base/ContiguousRangeMapper.h"

#include <gtest/gtest.h>

#include <vector>

using android::base::ContiguousRangeMapper;

namespace android {
namespace base {

TEST(ContiguousRangeMapper, Basic) {
    std::vector<uintptr_t> elements = {
        1, 2, 3, 5, 6, 7,
    };

    int numTotalRanges = 0;

    ContiguousRangeMapper rm(
        [&numTotalRanges](uintptr_t start, uintptr_t size) {
            ++numTotalRanges;
        });

    EXPECT_EQ(0, numTotalRanges);

    rm.finish();
    EXPECT_EQ(0, numTotalRanges);

    for (auto elt : elements) {
        rm.add(elt, 1);
    }

    EXPECT_EQ(1, numTotalRanges);

    rm.finish();

    EXPECT_EQ(2, numTotalRanges);

    rm.finish();

    EXPECT_EQ(2, numTotalRanges);
}

TEST(ContiguousRangeMapper, Pages) {
    std::vector<uintptr_t> elements = {
        0x1000,
        0x2000,
        0x3000,
        0x5000,
        0x6000,
        0x7000,
        0xa000,
        0xc000,
    };

    int numTotalRanges = 0;

    ContiguousRangeMapper rm(
        [&numTotalRanges](uintptr_t start, uintptr_t size) {
            ++numTotalRanges;
        });

    for (auto elt : elements) {
        rm.add(elt, 0x1000);
    }

    rm.finish();

    EXPECT_EQ(4, numTotalRanges);
}

TEST(ContiguousRangeMapper, PagesBatched) {
    std::vector<uintptr_t> elements = {
        0x1000,
        0x2000,

        0x3000,

        0x5000,
        0x6000,

        0x7000,

        0xa000,

        0xc000,
    };

    int numTotalRanges = 0;

    ContiguousRangeMapper rm(
        [&numTotalRanges](uintptr_t start, uintptr_t size) {
            ++numTotalRanges;
        }, 0x2000); // 2 page batch

    for (auto elt : elements) {
        rm.add(elt, 0x1000);
    }

    rm.finish();

    EXPECT_EQ(6, numTotalRanges);
}

}  // namespace base
}  // namespace android
