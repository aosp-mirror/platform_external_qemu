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

#include "android/base/Compiler.h"
#include "android/base/Optional.h"
#include "android/base/files/Stream.h"
#include "android/base/synchronization/Lock.h"

#include <atomic>
#include <map>
#include <set>

namespace android {
namespace snapshot {

class GapTracker final {
    DISALLOW_COPY_AND_ASSIGN(GapTracker);

public:
    GapTracker() = default;
    GapTracker(GapTracker&& other);
    GapTracker& operator=(GapTracker&& other);

    void load(base::Stream& in);
    void save(base::Stream& out);

    base::Optional<int64_t> allocate(int size);
    void add(int64_t start, int size);

    int64_t wastedSpace() const;

private:
    struct Range {
        int64_t start;
        int size;
        int64_t end() const { return start + size; }

        bool operator<(const Range& other) const { return start < other.start; }
    };

    using FreeRanges = std::set<Range>;
    struct IterCompare final {
        bool operator()(FreeRanges::iterator l, FreeRanges::iterator r) const {
            return l->start < r->start;
        }
    };
    using RangesBySize =
            std::map<int, std::set<FreeRanges::iterator, IterCompare>>;

    void addLocked(const Range& gap);

    FreeRanges mFreeRanges;
    RangesBySize mBySize;
    mutable base::Lock mLock;

    // Optimize the fast path - check for empty doesn't need to lock the mutex.
    std::atomic<bool> mEmpty{true};
};

}  // namespace snapshot
}  // namespace android
