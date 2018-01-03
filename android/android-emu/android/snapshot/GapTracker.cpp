// Copyright 2017 The Android Ope Source Project
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

#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>

namespace android {
namespace snapshot {

// C++17's map<> got a try_emplace() function that's more efficient than
// a pure emplace() - it won't construct the value unless it is needed for
// insertion.
// This is as close as we can get to it with pre-C++17 STL.
template <class Key, class Value, class... ValueCtorArgs>
static auto tryEmplace(std::map<Key, Value>& map,
                       Key key,
                       ValueCtorArgs&&... valueCtorArgs)
        -> std::pair<typename std::map<Key, Value>::iterator, bool> {
    const auto pos = map.lower_bound(key);
    if (pos != map.end() && pos->first == key) {
        return {pos, false};
    }
    return {map.emplace_hint(pos, std::piecewise_construct,
                             std::forward_as_tuple(key),
                             std::forward_as_tuple(valueCtorArgs...)),
            true};
}

void GenericGapTracker::load(base::Stream& in) {
    mByStart.clear();
    mBySize.clear();
    auto gapsCount = in.getBe32();
    for (int i = 0; i < gapsCount; ++i) {
        const auto size = int(in.getPackedNum());
        assert(size != 0);
        const auto posCount = in.getPackedNum();
        assert(posCount != 0);
        auto pos = int64_t(in.getBe64());
        const auto bySizeIt =
                mBySize.emplace(std::piecewise_construct, std::make_tuple(size),
                                std::make_tuple())
                        .first;
        auto byStartIt = mByStart.emplace(pos, RangeAttributes{}).first;
        auto bySizeListIt =
                bySizeIt->second.insert(bySizeIt->second.end(), byStartIt);
        byStartIt->second.bySizeIt = bySizeIt;
        byStartIt->second.bySizeListIt = bySizeListIt;
        for (int j = 1; j < posCount; ++j) {
            const auto delta = in.getPackedSignedNum();
            pos += delta;
            auto byStartIt = mByStart.emplace(pos, RangeAttributes{}).first;
            auto bySizeListIt =
                    bySizeIt->second.insert(bySizeIt->second.end(), byStartIt);
            byStartIt->second.bySizeIt = bySizeIt;
            byStartIt->second.bySizeListIt = bySizeListIt;
        }
    }
    mEmpty.store(mByStart.empty(), std::memory_order_relaxed);
}

void GenericGapTracker::save(base::Stream& out) {
    out.putBe32(mBySize.size());
    for (auto&& pair : mBySize) {
        out.putPackedNum(pair.first);
        out.putPackedNum(pair.second.size());
        int64_t pos = (*pair.second.begin())->first;
        out.putBe64(pos);
        for (auto it = std::next(pair.second.begin()); it != pair.second.end();
             ++it) {
            const auto newPos = (*it)->first;
            out.putPackedSignedNum(newPos - pos);
            pos = newPos;
        }
    }
}

base::Optional<int64_t> GenericGapTracker::allocate(int size) {
    if (mEmpty.load(std::memory_order_relaxed)) {
        return {};
    }
    base::AutoLock lock(mLock);

    auto sizeIt = mBySize.lower_bound(size);
    if (sizeIt == mBySize.end()) {
        return {};
    }

    const auto rangeSize = sizeIt->first;
    assert(!sizeIt->second.empty());
    auto startIt = sizeIt->second.front();
    const auto pos = startIt->first;
    startIt = erase(startIt);
    if (rangeSize > size) {
        // We've taken away a part of existing range, this means the rest
        // of it won't get combined with anything and it's OK to just insert
        // it directly.
        auto byStartIt =
                mByStart.emplace_hint(startIt, pos + size, RangeAttributes{});
        auto bySizeIt = tryEmplace(mBySize, rangeSize - size).first;
        auto bySizeListIt =
                bySizeIt->second.insert(bySizeIt->second.end(), byStartIt);
        byStartIt->second.bySizeIt = bySizeIt;
        byStartIt->second.bySizeListIt = bySizeListIt;
    } else if (mByStart.empty()) {
        mEmpty.store(true, std::memory_order_relaxed);
    }
    return pos;
}

void GenericGapTracker::add(int64_t start, int size) {
    if (size <= 0) {
        return;
    }
    base::AutoLock lock(mLock);
    addLocked(start, size);
}

int64_t GenericGapTracker::wastedSpaceImpl() const {
    base::AutoLock lock(mLock);
    return std::accumulate(
            mBySize.begin(), mBySize.end(), int64_t(0),
            [](int64_t prev, const RangesBySize::value_type& pair) {
                return prev + pair.first * pair.second.size();
            });
}

void GenericGapTracker::addLocked(int64_t start, int size) {
    bool wasEmpty = mByStart.empty();

    // The gap we're adding by definition cannot exist in the current set of
    // gaps, and it may only get combined with its left and/or right neighbor.
    bool inserted = false;
    auto startIt = mByStart.lower_bound(start);
    if (startIt != mByStart.begin()) {
        // check if the previous range ends exactly at |start| and combine them
        auto prevIt = std::prev(startIt);
        auto prevSize = prevIt->second.bySizeIt->first;
        if (prevIt->first + prevSize == start) {
            eraseFromBySize(prevIt);
            size += prevSize;
            start = prevIt->first;
            startIt = prevIt;
            inserted = true;
        }
    }
    if (!inserted) {
        startIt = mByStart.emplace_hint(startIt, start, RangeAttributes{});
    }
    assert(startIt != mByStart.end());
    // now check if the next range can be merged in as well
    const auto nextIt = std::next(startIt);
    if (nextIt != mByStart.end()) {
        const auto nextStart = nextIt->first;
        if (start + size == nextStart) {
            const auto bySizeIt = nextIt->second.bySizeIt;
            size += bySizeIt->first;
            erase(nextIt);
        }
    }

    auto sizeIt = tryEmplace(mBySize, size).first;
    auto bySizeListIt = sizeIt->second.insert(sizeIt->second.end(), startIt);
    startIt->second.bySizeIt = sizeIt;
    startIt->second.bySizeListIt = bySizeListIt;
    if (wasEmpty) {
        mEmpty.store(false, std::memory_order_relaxed);
    }
}

GenericGapTracker::RangesByStart::iterator GenericGapTracker::erase(
        RangesByStart::iterator pos) {
    eraseFromBySize(pos);
    return mByStart.erase(pos);
}

void GenericGapTracker::eraseFromBySize(RangesByStart::iterator pos) {
    const auto bySizeIt = pos->second.bySizeIt;
    if (std::next(bySizeIt->second.begin()) == bySizeIt->second.end()) {
        mBySize.erase(bySizeIt);
    } else {
        bySizeIt->second.erase(pos->second.bySizeListIt);
    }
}

void OneSizeGapTracker::load(base::Stream& in) {
    mGapStarts.clear();
    mCurrentPos = 0;
    auto gapsCount = in.getBe32();
    assert(gapsCount == 0 || gapsCount == 1);
    if (gapsCount > 0) {
        mSize = int(in.getPackedNum());
        const auto posCount = in.getPackedNum();
        mGapStarts.reserve(posCount);
        assert(posCount != 0);
        auto pos = int64_t(in.getBe64());
        mGapStarts.push_back(pos);
        for (int j = 1; j < posCount; ++j) {
            const auto delta = in.getPackedSignedNum();
            pos += delta;
            mGapStarts.push_back(pos);
        }
    }
    mEmpty.store(mGapStarts.empty(), std::memory_order_relaxed);
}

void OneSizeGapTracker::save(base::Stream& out) {
    out.putBe32(empty() ? 0 : 1);
    if (!empty()) {
        out.putPackedNum(mSize);
        out.putPackedNum(mGapStarts.size() - mCurrentPos);
        int64_t pos = mGapStarts[mCurrentPos];
        out.putBe64(pos);
        for (auto it = std::next(mGapStarts.begin(), mCurrentPos + 1);
             it != mGapStarts.end(); ++it) {
            const auto newPos = *it;
            out.putPackedSignedNum(newPos - pos);
            pos = newPos;
        }
    }
}

base::Optional<int64_t> OneSizeGapTracker::allocate(int size) {
    // This is slightly racy: we might have a new gap added in another thread
    // while checking here, but it's ok to have this kind of a race: the gap
    // will be reused during the next incremental save.
    if (empty()) {
        return {};
    }

    base::AutoLock lock(mLock);
    // Another thread stole the last item after we've checked for empty.
    if (mCurrentPos == mGapStarts.size()) {
        return {};
    }
    if (!mSize) {
        mSize = size;
    }
    const auto res = mGapStarts[mCurrentPos++];
    if (mCurrentPos == mGapStarts.size()) {
        mEmpty.store(true, std::memory_order_relaxed);
        mGapStarts.clear();
        mCurrentPos = 0;
    }
    return res;
}

void OneSizeGapTracker::add(int64_t start, int size) {
    base::AutoLock lock(mLock);
    if (mCurrentPos == mGapStarts.size()) {
        mEmpty.store(false, std::memory_order_relaxed);
    }
    mGapStarts.push_back(start);
    if (!mSize) {
        mSize = size;
    }
}

int64_t OneSizeGapTracker::wastedSpaceImpl() const {
    base::AutoLock lock(mLock);
    return mSize * (mGapStarts.size() - mCurrentPos);
}

void NullGapTracker::load(base::Stream& in) {
    GenericGapTracker().load(in);
}

void NullGapTracker::save(base::Stream& out) {
    OneSizeGapTracker().save(out);
}

base::Optional<int64_t> NullGapTracker::allocate(int size) {
    return {};
}

void NullGapTracker::add(int64_t start, int size) {}

int64_t NullGapTracker::wastedSpaceImpl() const {
    return 0;
}

}  // namespace snapshot
}  // namespace android
