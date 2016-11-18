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

#include "android/base/containers/PointerSet.h"

#include "android/base/containers/HashUtils.h"
#include "android/base/Log.h"

#include <stdlib.h>

namespace android {
namespace base {

namespace {

// Special values used in the set's items array to indicate either
// unused slots, or tombstone ones.
#define UNUSED_SLOT NULL
#define TOMBSTONE ((void*)(~(uintptr_t)0))

// Returns true if value |obj| corresponds to a real object in the set,
// false otherwise.
bool validValue(void* obj) {
    return obj != UNUSED_SLOT && obj != TOMBSTONE;
}

// Probe the array |items| holding |1U << shift| items for object |obj|.
// |objHash| is the object hash.
// If |obj| is found, return its index in the array.
// If |obj| is not found, return the index of a unused or tombstone slot
// that can be used to add it.
size_t probeItems(const void* obj, size_t objHash, void** items, size_t shift) {
    DCHECK(obj);
    DCHECK(shift < 32U);

    size_t probe_mask = ((1U << shift) - 1U);
    const size_t probe_index0 = (objHash % internal::kPrimes[shift]) & probe_mask;
    size_t probe_index = probe_index0;
    size_t step = 0;

    intptr_t tombstone = -1;

    for (;;) {
        void* item = items[probe_index];
        if (item == obj) {
            return probe_index;
        }
        if (item == UNUSED_SLOT) {
            break;
        }
        if (item == TOMBSTONE && tombstone < 0) {
            tombstone = (intptr_t)probe_index;
        }
        step++;
        if (step > probe_mask) {
            break;
        }
        probe_index = (probe_index + step) & probe_mask;
    }
    if (tombstone >= 0) {
        return (size_t)tombstone;
    }
    return probe_index;
}

}  // namespace

PointerSetBase::PointerSetBase() :
        mShift(internal::kMinShift),
        mCount(0),
        mItems(NULL) {
    size_t capacity = 1U << mShift;
    mItems = static_cast<void**>(::calloc(capacity, sizeof(mItems[0])));
}

PointerSetBase::~PointerSetBase() {
    mCount = 0;
    mShift = 0;
    ::free(mItems);
}

PointerSetBase::Iterator::Iterator(PointerSetBase* set) :
        mItems(set->mItems),
        mCapacity(1U << set->mShift),
        mPos(0U) {}

void* PointerSetBase::Iterator::next() {
    while (mPos < mCapacity) {
        void* item = mItems[mPos++];
        if (validValue(item)) {
            return item;
        }
    }
    return NULL;
}

bool PointerSetBase::contains(const void* obj, HashFunction hashFunc) const {
    if (!obj) {
        return false;
    }
    size_t objHash = hashFunc(obj);
    size_t pos = probeItems(obj, objHash, mItems, mShift);
    DCHECK(pos < (1U << mShift));

    return validValue(mItems[pos]);
}

void PointerSetBase::clear() {
    mCount = 0;
    mShift = internal::kMinShift;
    size_t capacity = 1U << mShift;
    mItems = static_cast<void**>(
            ::realloc(mItems, sizeof(mItems[0]) * capacity));
    for (size_t n = 0; n < capacity; ++n) {
        mItems[n] = UNUSED_SLOT;
    }
}

void* PointerSetBase::addItem(void* obj, HashFunction hashFunc) {
    if (!validValue(obj)) {
        return NULL;
    }
    if(mCount >= (1U << mShift)) maybeResize(hashFunc);
    size_t objHash = hashFunc(obj);
    size_t pos = probeItems(obj, objHash, mItems, mShift);
    DCHECK(pos < (1U << mShift));

    void* result = mItems[pos];
    if (validValue(result)) {
        // Simple replacement.
        DCHECK(result == obj);
        return result;
    }
    mItems[pos] = obj;
    mCount++;

    if (result != TOMBSTONE) {
        // No need to resize when replacing tombstone.
        maybeResize(hashFunc);
    }

    return NULL;
}

void* PointerSetBase::removeItem(void* obj, HashFunction hashFunc) {
    if (!validValue(obj)) {
        return NULL;
    }
    size_t objHash = hashFunc(obj);
    size_t pos = probeItems(obj, objHash, mItems, mShift);
    DCHECK(pos < (1U << mShift));

    void* result = mItems[pos];
    if (!validValue(result)) {
        // Item was not in the array.
        return NULL;
    }
    mItems[pos] = TOMBSTONE;
    mCount--;
    return result;
}

void** PointerSetBase::toArray() const {
    if (!mCount) {
        return NULL;
    }
    void** result = static_cast<void**>(::calloc(mCount, sizeof(mItems[0])));
    size_t capacity = 1U << mShift;
    size_t count = 0;
    for (size_t n = 0; n < capacity; ++n) {
        void* item = mItems[n];
        if (!validValue(item)) {
            continue;
        }
        result[count++] = item;
    }

    DCHECK(count == mCount);
    return result;
}

void PointerSetBase::maybeResize(HashFunction hashFunc) {
    size_t newShift = internal::hashShiftAdjust(mCount, mShift);
    if (newShift == mShift) {
        // Nothing to resize
        return;
    }

    size_t capacity = 1U << mShift;
    size_t newCapacity = 1U << newShift;
    void** newItems = static_cast<void**>(
            ::calloc(newCapacity, sizeof(newItems[0])));

    for (size_t n = 0; n < capacity; ++n) {
        void* item = mItems[n];
        if (!validValue(item)) {
            continue;
        }
        size_t itemHash = hashFunc(item);
        size_t slot = probeItems(item, itemHash, newItems, newShift);
        DCHECK(!validValue(newItems[slot]));
        newItems[slot] = item;
    }

    ::free(mItems);
    mItems = newItems;
    mShift = newShift;
}

}  // namespace base
}  // namespace android
