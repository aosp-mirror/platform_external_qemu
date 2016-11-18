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

#include "android/base/containers/HashUtils.h"

#include <gtest/gtest.h>

#include <math.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

namespace {

// Return true iff |x| is a prime integer.
bool isPrime(int x) {
    if (x == 1 || x == 2)
        return true;

    int max_n = (int)ceil(sqrt(x));
    for (int n = 2; n <= max_n; ++n) {
        if ((x % n) == 0) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(HashUtils, kPrimes) {
    for (size_t n = 0; n < 32U; ++n) {
        int prime = internal::kPrimes[n];
        if (n > 0) {
            EXPECT_LT(internal::kPrimes[n - 1], prime)
                    << "Prime #" << n << " (" << prime << ")";
        }
        EXPECT_TRUE(isPrime(prime))
                << "Prime #" << n << " (" << prime << ")";
    }
}

TEST(HashUtils, hashShiftAdjust) {
    static const struct {
        size_t size;
        size_t shift;
        size_t expected;
    } kData[] = {
        { 0, 3, 3 },
        { 1, 3, 3 },
        { 2, 3, 3 },
        { 3, 3, 3 },
        { 4, 3, 3 },
        { 4, 3, 3 },
        { 6, 3, 3 },
        { 7, 3, 4 },
        { 8, 3, 4 },
        { 8, 4, 4 },
        { 3, 4, 3 },
        { 15, 4, 5 },
        { 15000, 16, 15 },
        { 32768, 16, 16 },
        { 55000, 16, 17 },
    };
    for (size_t n = 0; n < ARRAYLEN(kData); ++n) {
        EXPECT_EQ(kData[n].expected,
                  internal::hashShiftAdjust(kData[n].size,
                                            kData[n].shift))
                << "For size="
                << kData[n].size
                << " and shift="
                << kData[n].shift;
    }
}


}  // namespace base
}  // namespace android
