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

#include "android/snapshot/PageMap.h"

#include <stdio.h>
#include <string.h>

namespace android {
namespace snapshot {

static int sIlog2(uint64_t val) {
    int res = 0;
    if (val == 0) return 0;
    while (val >>= 1) ++res;
    return res;
}

PageMap::PageMap(void* start, uint64_t size, uint64_t pageSize) :
    mBase((uintptr_t)start),
    mSize(size),
    mPageSize(pageSize),
    mPageShift(sIlog2(pageSize)),
    mBitmap(size / pageSize, 0x0) { }

PageMap::PageMap(const PageMap& other) :
    mBase(other.mBase),
    mSize(other.mSize),
    mPageSize(other.mPageSize),
    mPageShift(other.mPageShift),
    mBitmap(other.mBitmap),
    mLock() { }

void PageMap::set(void* ptr, bool val) {
    uintptr_t iptr = (uintptr_t)ptr;

    if (!has(ptr)) return;

    // Possible to have a race condition on writing
    // single bits, e.g.,
    // one thread sets     0000 1000
    // and another         1000 0000
    // result might not be 1000 1000
    android::base::AutoLock lock(mLock);
    mBitmap[(iptr - mBase) >> mPageShift] = val;
}

bool PageMap::lookup(void* ptr, bool notFoundVal) const {
    uintptr_t iptr = (uintptr_t)ptr;

    if (!has(ptr)) return notFoundVal;

    return mBitmap[(iptr - mBase) >> mPageShift];
}

bool PageMap::has(void* ptr) const {
    uintptr_t iptr = (uintptr_t)ptr;
    return iptr >= mBase && iptr < mBase + mSize;
}

// Set/lookup by page index
bool PageMap::hasIndex(size_t index) const {
    return index < mBitmap.size();
}

void PageMap::setByIndex(size_t index, bool val) {
    if (!hasIndex(index)) return;
    android::base::AutoLock lock(mLock);
    mBitmap[index] = val;
}

bool PageMap::lookupByIndex(size_t index, bool notFoundVal) const {
    if (!hasIndex(index)) return notFoundVal;
    return mBitmap[index];
}

uint64_t PageMap::count() const {
    uint64_t total = 0;
    for (const auto b : mBitmap) {
        total += b ? 1 : 0;
    }
    return total;
}

void PageMap::reset() {
    android::base::AutoLock lock(mLock);
    size_t sz = mBitmap.size();
    mBitmap.resize(0);
    mBitmap.resize(sz, 0x0);
}

} // namespace android
} // namespace snapshot
