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

#include <popcntintrin.h>

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
    mBitmap(size / pageSize / 8 + 1, 0x0) { }

void PageMap::set(void* ptr, bool val) {
    uintptr_t iptr = (uintptr_t)ptr;

    if (!has(ptr)) return;

    iptr -= mBase;
    iptr >>= mPageShift;

    uintptr_t iptrByteIndex = iptr >> 3;
    uintptr_t iptrBit = iptr & 0x7;

    if (val) {
        mBitmap[iptrByteIndex] |= (1 << iptrBit);
    } else {
        mBitmap[iptrByteIndex] &= ~(1 << iptrBit);
    }
}

bool PageMap::lookup(void* ptr, bool notFoundVal) const {
    uintptr_t iptr = (uintptr_t)ptr;

    if (!has(ptr)) return notFoundVal;

    iptr -= mBase;
    iptr >>= mPageShift;

    uintptr_t iptrByteIndex = iptr >> 3;
    uintptr_t iptrBit = iptr & 0x7;

    return (1 << iptrBit) & mBitmap[iptrByteIndex];
}

bool PageMap::has(void* ptr) const {
    uintptr_t iptr = (uintptr_t)ptr;
    return iptr >= mBase && iptr < mBase + mSize;
}

uint64_t PageMap::count() const {
    uint64_t total = 0;
    for (const auto byte : mBitmap) {
        total += __builtin_popcount((unsigned int)byte);
    }
    return total;
}

PageMap::~PageMap() { }
