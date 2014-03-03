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

#include "android/base/containers/PodVector.h"

#include "android/base/Log.h"
#include "android/base/memory/MallocUsableSize.h"

#include <stdlib.h>
#include <string.h>

namespace android {
namespace base {

static inline void swapPointers(char** p1, char** p2) {
    char* tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

PodVectorBase::PodVectorBase(const PodVectorBase& other) {
    initFrom(other.begin(), other.byteSize());
}

PodVectorBase& PodVectorBase::operator=(const PodVectorBase& other) {
    initFrom(other.begin(), other.byteSize());
    return *this;
}

PodVectorBase::~PodVectorBase() {
    if (mBegin) {
        // Sanity.
        ::memset(mBegin, 0xee, byteSize());
        ::free(mBegin);
        mBegin = NULL;
        mEnd = NULL;
        mLimit = NULL;
    }
}

void PodVectorBase::initFrom(const void* from, size_t fromLen) {
    if (!fromLen || !from) {
        mBegin = NULL;
        mEnd = NULL;
        mLimit = NULL;
    } else {
        mBegin = static_cast<char*>(::malloc(fromLen));
        PCHECK(mBegin)
                << LogString("Could not allocate %zd bytes.", fromLen);
        mEnd = mLimit = mBegin + fromLen;
        ::memcpy(mBegin, from, fromLen);
    }
}

void PodVectorBase::assignFrom(const PodVectorBase& other) {
    resize(other.byteSize(), 1U);
    ::memmove(begin(), other.begin(), byteSize());
}

void PodVectorBase::resize(size_t newSize, size_t itemSize) {
    const size_t kMaxSize = maxItemCapacity(itemSize);
    CHECK(newSize <= kMaxSize) << LogString(
            "Trying to resize vector to %zd items of %zd bytes "
            "(%zd max allowed)",
            newSize,
            kMaxSize);
    size_t oldCapacity = itemCapacity(itemSize);
    const size_t kMinCapacity = 256 / itemSize;

    if (newSize < oldCapacity) {
        // Only shrink if the new size is really small.
        if (newSize < oldCapacity / 2 && oldCapacity > kMinCapacity) {
            reserve(newSize, itemSize);
        }
    } else if (newSize > oldCapacity) {
        size_t newCapacity = oldCapacity;
        while (newCapacity < newSize) {
            size_t newCapacity2 = newCapacity + (newCapacity >> 2) + 8;
            if (newCapacity2 < newCapacity || newCapacity > kMaxSize) {
                newCapacity = kMaxSize;
            } else {
                newCapacity = newCapacity2;
            }
        }
        reserve(newCapacity, itemSize);
    }
    mEnd = mBegin + newSize * itemSize;
}

void PodVectorBase::reserve(size_t newSize, size_t itemSize) {
    const size_t kMaxSize = maxItemCapacity(itemSize);
    CHECK(newSize <= kMaxSize) << LogString(
            "Trying to allocate %zd items of %zd bytes (%zd max allowed)",
            newSize,
            kMaxSize);

    if (newSize == 0) {
        ::free(mBegin);
        mBegin = NULL;
        mEnd = NULL;
        mLimit = NULL;
        return;
    }

    size_t oldByteSize = byteSize();
    size_t newByteCapacity = newSize * itemSize;
    char* newBegin = static_cast<char*>(::realloc(mBegin, newByteCapacity));
    PCHECK(newBegin) << LogString(
            "Could not reallocate array from %zd tp %zd items of %zd bytes",
            oldByteSize / itemSize,
            newSize,
            itemSize);

    mBegin = newBegin;
    mEnd = newBegin + oldByteSize;
#if USE_MALLOC_USABLE_SIZE
    size_t usableSize = malloc_usable_size(mBegin);
    if (usableSize > newByteCapacity) {
        newByteCapacity = usableSize - (usableSize % itemSize);
    }
#endif
    mLimit = newBegin + newByteCapacity;
    // Sanity.
    if (newByteCapacity > oldByteSize) {
        ::memset(mBegin + oldByteSize, 0, newByteCapacity - oldByteSize);
    }
}

void PodVectorBase::removeAt(size_t itemPos, size_t itemSize) {
    size_t count = itemCount(itemSize);
    DCHECK(itemPos <= count) << "Item position is too large!";
    if (itemPos < count) {
        size_t  pos = itemPos * itemSize;
        ::memmove(mBegin + pos,
                  mBegin + pos + itemSize,
                  byteSize() - pos - itemSize);
        resize(count - 1U, itemSize);
    }
}

void* PodVectorBase::insertAt(size_t itemPos, size_t itemSize) {
    size_t count = this->itemCount(itemSize);
    DCHECK(itemPos <= count) << "Item position is too large";
    resize(count + 1, itemSize);
    size_t pos = itemPos * itemSize;
    if (itemPos < count) {
        ::memmove(mBegin + pos + itemSize,
                  mBegin + pos,
                  count * itemSize - pos);
        // Sanity to avoid copying pointers and other bad stuff.
        ::memset(mBegin + pos, 0, itemSize);
    }
    return mBegin + pos;
}

void PodVectorBase::swapAll(PodVectorBase* other) {
    swapPointers(&mBegin, &other->mBegin);
    swapPointers(&mEnd, &other->mEnd);
    swapPointers(&mLimit, &other->mLimit);
}

}  // namespace base
}  // namespace android
