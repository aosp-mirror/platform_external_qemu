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
#include <memory>
#include <set>
#include <vector>

namespace android {
namespace snapshot {

class GapTracker {
    DISALLOW_COPY_AND_ASSIGN(GapTracker);

public:
    using Ptr = std::unique_ptr<GapTracker>;

    virtual ~GapTracker() = default;

    virtual void load(base::Stream& in) = 0;
    virtual void save(base::Stream& out) = 0;

    virtual base::Optional<int64_t> allocate(int size) = 0;
    virtual void add(int64_t start, int size) = 0;

    int64_t wastedSpace() const {
        if (empty()) {
            return 0;
        }
        return wastedSpaceImpl();
    }

protected:
    GapTracker() = default;
    virtual int64_t wastedSpaceImpl() const = 0;

    bool empty() const { return mEmpty.load(std::memory_order_relaxed); }

    mutable base::Lock mLock;
    // Optimize the fast path - check for empty doesn't need to lock the mutex.
    std::atomic<bool> mEmpty{true};
};

class OneSizeGapTracker final : public GapTracker {
public:
    OneSizeGapTracker();

    void load(base::Stream& in) override;
    void save(base::Stream& out) override;

    base::Optional<int64_t> allocate(int size) override;
    void add(int64_t start, int size) override;
    int64_t wastedSpaceImpl() const override;

private:
    std::vector<int64_t> mGapStarts;
    int32_t mCurrentPos = 0;
    int32_t mSize = 0;
};

class GenericGapTracker final : public GapTracker {
public:
    GenericGapTracker() = default;

    void load(base::Stream& in) override;
    void save(base::Stream& out) override;

    base::Optional<int64_t> allocate(int size) override;
    void add(int64_t start, int size) override;

    int64_t wastedSpaceImpl() const override;

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
};

}  // namespace snapshot
}  // namespace android
