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

#include "android/snapshot/GapTracker.h"

#include <cassert>
#include <iterator>
#include <numeric>

namespace android {
namespace snapshot {

GapTracker::GapTracker(GapTracker&& other)
    : mFreeRanges(std::move(other.mFreeRanges)),
      mBySize(std::move(other.mBySize)),
      mEmpty(other.mEmpty.exchange(true, std::memory_order_relaxed)) {}

GapTracker& GapTracker::operator=(GapTracker&& other) {
    mFreeRanges = std::move(other.mFreeRanges);
    mBySize = std::move(other.mBySize);
    mEmpty = other.mEmpty.exchange(true, std::memory_order_relaxed);
    return *this;
}

void GapTracker::load(base::Stream& in) {
    mFreeRanges.clear();
    mBySize.clear();
    auto gapsCount = in.getBe32();
    for (int i = 0; i < gapsCount; ++i) {
        const auto size = int(in.getPackedNum());
        assert(size != 0);
        const auto posCount = in.getPackedNum();
        assert(posCount != 0);
        auto& positions = mBySize[size];
        auto pos = int64_t(in.getBe64());
        positions.insert(mFreeRanges.insert({pos, size}).first);
        for (int j = 1; j < posCount; ++j) {
            const auto delta = in.getPackedSignedNum();
            pos += delta;
            positions.insert(mFreeRanges.insert({pos, size}).first);
        }
    }
    mEmpty.store(mFreeRanges.empty(), std::memory_order_relaxed);
}

void GapTracker::save(base::Stream& out) {
    out.putBe32(mBySize.size());
    for (auto&& pair : mBySize) {
        out.putPackedNum(pair.first);
        out.putPackedNum(pair.second.size());
        int64_t pos = (*pair.second.begin())->start;
        out.putBe64(pos);
        for (auto it = std::next(pair.second.begin()); it != pair.second.end();
             ++it) {
            const auto newPos = (*it)->start;
            out.putPackedSignedNum(newPos - pos);
            pos = newPos;
        }
    }
}

base::Optional<int64_t> GapTracker::allocate(int size) {
    if (mEmpty.load(std::memory_order_relaxed)) {
        return {};
    }
    base::AutoLock lock(mLock);

    auto sizeIt = mBySize.upper_bound(size);
    if (sizeIt != mBySize.begin() && std::prev(sizeIt)->first == size) {
        --sizeIt;
    }
    if (sizeIt != mBySize.end()) {
        assert(!sizeIt->second.empty());

        const auto rangeIt = *sizeIt->second.begin();
        const auto pos = rangeIt->start;

        // update the maps: remove the old range and add its remainder if the
        // requested size is smaller than the whole range we found.
        sizeIt->second.erase(sizeIt->second.begin());
        if (sizeIt->second.empty()) {
            mBySize.erase(sizeIt);
        }
        Range remainingRange = {rangeIt->start + size, rangeIt->size - size};
        mFreeRanges.erase(rangeIt);
        if (remainingRange.size > 0) {
            // We've taken away a part of existing range, this means the rest
            // of it won't get combined with anything and it's OK to just insert
            // it directly.
            const auto remainingInsertRes = mFreeRanges.insert(remainingRange);
            assert(remainingInsertRes.second);
            mBySize[remainingRange.size].insert(remainingInsertRes.first);
        }
        if (mFreeRanges.empty()) {
            mEmpty.store(true, std::memory_order_relaxed);
        }
        return pos;
    }
    return {};
}

void GapTracker::add(int64_t start, int size) {
    if (size <= 0) {
        return;
    }
    base::AutoLock lock(mLock);
    addLocked({start, size});
}

int64_t GapTracker::wastedSpace() const {
    if (mEmpty.load(std::memory_order_relaxed)) {
        return 0;
    }

    base::AutoLock lock(mLock);
    return std::accumulate(
            mBySize.begin(), mBySize.end(), int64_t(0),
            [](int64_t prev, const RangesBySize::value_type& pair) {
                return prev + pair.first * pair.second.size();
            });
}

void GapTracker::addLocked(const Range& gap) {
    // The gap we're adding by definition cannot exist in the current set of
    // gaps, and it may only get combined with its left or right neighbor.
    bool inserted = false;
    auto it = mFreeRanges.lower_bound(gap);
    if (it != mFreeRanges.begin()) {
        // check if the previous range ends exactly at |start| and combine them
        auto prevIt = std::prev(it);
        if (prevIt->end() == gap.start) {
            auto bySizeIt = mBySize.find(prevIt->size);
            assert(bySizeIt != mBySize.end());
            bySizeIt->second.erase(prevIt);
            if (bySizeIt->second.empty()) {
                mBySize.erase(bySizeIt);
            }
            const_cast<Range&>(*prevIt).size += gap.size;
            it = prevIt;
            inserted = true;
        }
    }
    if (!inserted) {
        it = mFreeRanges.insert(it, gap);
    }
    assert(it != mFreeRanges.end());
    // now check if the next range can be merged in as well
    const auto nextIt = std::next(it);
    if (nextIt != mFreeRanges.end() && it->end() == nextIt->start) {
        auto bySizeIt = mBySize.find(nextIt->size);
        assert(bySizeIt != mBySize.end());
        bySizeIt->second.erase(nextIt);
        if (bySizeIt->second.empty()) {
            mBySize.erase(bySizeIt);
        }
        const_cast<Range&>(*it).size += nextIt->size;
        mFreeRanges.erase(nextIt);
    }
    mBySize[it->size].insert(it);
    mEmpty.store(false, std::memory_order_relaxed);
}

}  // namespace snapshot
}  // namespace android
