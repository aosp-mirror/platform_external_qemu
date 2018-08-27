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
#pragma once

#include "android/base/AlignedBuf.h"

#include <unordered_map>
#include <vector>

#include <assert.h>

namespace android {
namespace base {

// Creates and deletes multiple POD or trivially copyable objects of type |T|
// whose addresses are aligned on |align|.  Since we might reclaim or resize the
// pool while in operation, the user only interacts with |Handle|'s.  This is
// externally synchronized, intended to be used from only one thread. 
//
// The intended use case (for cases like Vulkan at least) is that there won't be too
// many possible different array sizes with which objects can be allocated.
// We only need to allocate a few Vulkan structs of each type.
//
// Therefore we specialize on having multiple AlignedBufs, one for each array size
// requested. This allows straightforward "swap with last" reclamation.
//
// We also keep an AlignedBuf of such AlignedBufs for faster access. That's
// because we also expect not to have to deal with any large arrays directly
// (once we have DMA working) or if we do, we only pay a one-time cost
// proportional to the largest buffer sent.
//
// This also assumes that the total set of live objects in use won't be too
// big, so it never de-allocates unless it is deconstructed itself.
template <typename T, size_t align>
class Pool {
public:
    using Handle = size_t;

    Pool() {
        // Start up some common sizes
        static constexpr size_t kHighestStartingSize = 256;
        mAllocsWithSize.resize(kHighestStartingSize);
        memset(mAllocsWithSize.data(), 0x0,
               kHighestStartingSize * sizeof(SameSizedAllocs*));

        size_t i = 1;
        while (i <= kHighestStartingSize) {
            mAllocsWithSize[i - 1] = new SameSizedAllocs(i);
            i <<= 1;
        }
    }

    ~Pool() {
        for (size_t i = 0; i < mAllocsWithSize.size(); i++) {
            if (mAllocsWithSize[i]) {
                delete mAllocsWithSize[i];
                mAllocsWithSize[i] = nullptr;
            }
        }
    }

    Handle gen(size_t wantedCount = 1) {
        SameSizedAllocs* buf = nullptr;

        if (wantedCount > mAllocsWithSize.size()) {
            size_t oldSize = mAllocsWithSize.size();
            mAllocsWithSize.resize(wantedCount);
            for (size_t needZero = oldSize;
                 needZero < wantedCount;
                 ++needZero) {
                mAllocsWithSize[needZero] = nullptr;
            }
        }

        size_t allocsWithSizeKey = wantedCount - 1;

        buf = mAllocsWithSize[allocsWithSizeKey];

        if (!buf) {
            buf = new SameSizedAllocs(wantedCount);
            mAllocsWithSize[allocsWithSizeKey] = buf;
        }

        size_t id = buf->add();

        Key key = {
            allocsWithSizeKey,
            id,
        };

        ++mCount;

        if (mCount > mAlloced) {
            mAlloced = 2 * mCount;
            mAllocs.resize(mAlloced);
            mAllocLookup.resize(mAlloced);
        }

        Handle res = mCount - 1;

        mAllocs[res] = key;
        mAllocLookup[res] = res;

        return res;
    }

    T* acquire(Handle obj) {
        const auto& key = mAllocs[mAllocLookup[obj]];
        auto granularity = key.granularity;
        auto id = key.id;
        return mAllocsWithSize[granularity]->ptr(id);
    }

    void del(Handle obj) {
        assert(mAllocs.size() > 0);

        size_t indexInAllocs = mAllocLookup[obj];

        // perform the storage deallocation
        const auto& key = mAllocs[indexInAllocs];
        auto granularity = key.granularity;
        auto id = key.id;
        mAllocsWithSize[granularity]->remove(id);

        // swap with last in mAllocs
        if (indexInAllocs == mCount - 1) {
            --mCount;
        } else {
            mAllocs[indexInAllocs] = mAllocs[mCount - 1];
            mAllocLookup[mCount - 1] = indexInAllocs;
            --mCount;
        }
    }

private:

    struct Key {
        size_t granularity;
        size_t id;
    };

    class SameSizedAllocs {
    public:
        SameSizedAllocs(size_t _granularity) :
            granularity(_granularity) { }

        size_t add() {
            size_t res = count;

            ++count;

            if (alloced < count) {
                alloced = 2 * count;
                buffer.resize(granularity * alloced);
                refs.resize(alloced);
            }

            refs[res] = res * granularity;
            return res;
        }

        void remove(size_t toRemove) {
            assert(count > 0);
            assert(toRemove < count);

            size_t last = count - 1;

            if (toRemove == last) {
                --count;
                return;
            }

            T* data = buffer.data();

            memcpy(data + toRemove * granularity,
                   data + last * granularity,
                   sizeof(T) * granularity);


            refs[last] = toRemove * granularity;

            --count;
        }

        T* ptr(size_t obj) {
            return buffer.data() + refs[obj];
        }

        AlignedBuf<T, align> buffer { 0 };
        AlignedBuf<size_t, 64> refs { 0 };

        size_t granularity = 0;
        size_t alloced = 0;
        size_t count = 0;
    };

    AlignedBuf<SameSizedAllocs*, 64> mAllocsWithSize { 0 };
    AlignedBuf<Key, 64> mAllocs { 0 };
    AlignedBuf<size_t, 64> mAllocLookup { 0 };

    size_t mAlloced = 0;
    size_t mCount = 0;
};

} // namespace base
} // namespace android
