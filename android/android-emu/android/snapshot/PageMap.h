// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/synchronization/Lock.h"

#include <vector>

#include <inttypes.h>

namespace android {
namespace snapshot {

// Class for looking up boolean properties of particular page-aligned
// addresses in a memory space.
class PageMap {
public:
    // Define a memory space starting from |start| with |size|
    // and aligned to |pageSize| (pageSize must be a power of 2)
    PageMap(void* start = 0,
            uint64_t size = 0x80000000,
            uint64_t pageSize = 4096);

    PageMap(const PageMap& other);

    // Set map values
    void set(void* ptr, bool val);

    // Look up into the map
    bool lookup(void* ptr, bool notFoundVal = true) const;

    // Existence check
    bool has(void* ptr) const;

    // Set/lookup by page index
    bool hasIndex(size_t index) const;
    void setByIndex(size_t index, bool val);
    bool lookupByIndex(size_t index, bool notFoundVal = true) const;

    // Counts the true entries
    uint64_t count() const;

    // Marks everything not dirty again
    void reset();
private:
    // Parameters and storage
    uintptr_t mBase;
    uint64_t mSize;
    uint64_t mPageSize;
    uint64_t mPageShift;
    std::vector<bool> mBitmap;
    // Lock to protect writes
    android::base::Lock mLock;
};

} // namespace android
} // namespace snapshot
