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

#ifndef ANDROID_BASE_CONTAINERS_HASH_UTILS_H
#define ANDROID_BASE_CONTAINERS_HASH_UTILS_H

#include <stddef.h>
#include <stdint.h>

namespace android {
namespace base {
namespace internal {

extern const int kPrimes[32];

enum {
    kMinShift = 3,
    kMaxShift = 31,
    kMinCapacity = (1 << kMinShift),
    kLoadScale = 1024,
    kMinLoad = kLoadScale / 4,      // 25% minimum load.
    kMaxLoad = kLoadScale * 3 / 4,  // 75% maximum load.
};

// A hash table uses an array of |1 << shift| items. Determine a shift
// value that is large enough to hold |size| items with comfortable load.
// |oldShift| is the current shift of the array, or 0 if it is not known.
//
// If this function returns |oldShift|, then no resizing is needed.
// Otherwise, the array must be resized to |1 << result| items.
size_t hashShiftAdjust(size_t size, size_t oldShift);

// Hash a pointer value into something that can be used to implement
// hash tables.
inline size_t pointerHash(const void* ptr) {
    return static_cast<size_t>(
            reinterpret_cast<uintptr_t>(ptr));
}

}  // namespace internal
}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_CONTAINERS_HASH_UTILS_H
