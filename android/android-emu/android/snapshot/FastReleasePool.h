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

#include "android/base/system/System.h"

#include <array>
#include <atomic>
#include <cassert>

namespace android {
namespace snapshot {

//
// FastReleasePool<> - a resource pool that optimizes for fast object releasing.
//
//  The primary usecase is when there's a big number of fast producers and a
// single / slow consumer, that needs to release the resource after it was
// consumed as fast as possible.
//
//  Both allocate() and release() calls are blocking, and allocate() may force
// the calling thread to go into sleeping mode. release() won't ever sleep, but
// may instead spin forever if one has tried to release more resources than the
// pool is supposed to contain.
//

template <class T, int N>
class FastReleasePool {
public:
    using value_type = T*;
    static constexpr int capacity = N;

    FastReleasePool() = default;

    template <class Iter>
    FastReleasePool(Iter begin, Iter end)
        : mAvailable(std::distance(begin, end)) {
        assert(mAvailable <= N);
        for (auto out = mResources.begin(); begin != end; ++begin, ++out) {
            out->store(&*begin, std::memory_order_relaxed);
        }
    }

    T* allocate();
    void release(T* t);

private:
    std::atomic<int> mAvailable{0};
    std::array<std::atomic<T*>, N> mResources = {};
    base::System* mSystem = base::System::get();
};

template <class T, int N>
T* FastReleasePool<T, N>::allocate() {
    // Spin for a while before starting to go into real sleep.
    int spinLimit = 2 * mResources.size();
    T* resource = nullptr;
    for (;;) {
        for (auto& current : mResources) {
            while (!mAvailable.load(std::memory_order_relaxed)) {
                if (spinLimit-- > 0) {
                    mSystem->yield();
                } else {
                    mSystem->sleepMs(1);
                }
            }
            resource = current.exchange(nullptr, std::memory_order_relaxed);
            if (resource) {
                mAvailable.fetch_sub(1, std::memory_order_relaxed);
                return resource;
            }
        }
    }
}

template <class T, int N>
void FastReleasePool<T, N>::release(T* resource) {
    assert(mAvailable < N);
    auto current = mResources.begin();
    for (;;) {
        T* needed = nullptr;
        if (current->compare_exchange_strong(needed, resource,
                                             std::memory_order_relaxed,
                                             std::memory_order_relaxed)) {
            mAvailable.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (++current == mResources.end()) {
            current = mResources.begin();
        }
    }
}

}  // namespace snapshot
}  // namespace android
