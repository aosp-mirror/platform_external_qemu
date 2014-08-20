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

namespace android {
namespace base {
namespace internal {

namespace {

inline int hashCapacityCompare(size_t size, size_t shift) {
    size_t capacity = 1U << shift;
    // Essentially, one can rewrite:
    //   load < minLoad
    // as:
    //   size / capacity < minLoad
    //   capacity * minLoad > size
    if (capacity * kMinLoad  > size * kLoadScale)
        return +1;

    // Similarly, one can rewrite:
    //   load > maxLoad
    // as:
    //   size / capacity > maxLoad
    //   capacity * maxLoad < size
    if (capacity * kMaxLoad < size * kLoadScale)
        return -1;

    return 0;
}

}  // namespace

const int kPrimes[32] = {
    1,          /* For 1 << 0 */
    2,
    3,
    7,
    13,
    31,
    61,
    127,
    251,
    509,
    1021,
    2039,
    4093,
    8191,
    16381,
    32749,
    65521,      /* For 1 << 16 */
    131071,
    262139,
    524287,
    1048573,
    2097143,
    4194301,
    8388593,
    16777213,
    33554393,
    67108859,
    134217689,
    268435399,
    536870909,
    1073741789,
    2147483647  /* For 1 << 31 */
    };


size_t hashShiftAdjust(size_t size, size_t shift) {
    size_t newShift = shift;
    int ret = hashCapacityCompare(size, shift);
    if (ret < 0) {
        // Capacity is too small and must be increased.
        do {
            if (newShift == kMaxShift) {
                break;
            }
            ++newShift;
        } while (hashCapacityCompare(size, newShift) < 0);
    } else if (ret > 0) {
        // Capacity is too large and must be decreased.
        do {
            if (newShift == kMinShift) {
                break;
            }
            newShift--;
        } while (hashCapacityCompare(size, newShift) > 0);
    }

    return newShift;
}

}  // namespace internal
}  // namespace base
}  // namespace android
