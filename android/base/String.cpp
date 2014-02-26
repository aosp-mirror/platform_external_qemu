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

#include "android/base/String.h"

#include "android/base/Limits.h"
#include "android/base/Log.h"
#include "android/base/memory/MallocUsableSize.h"
#include "android/base/StringView.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace base {

String::String() : mStr(mStorage), mSize(0) {
    mStorage[0] = '\0';
}


String::String(const char* str) : mStr(mStorage), mSize(0) {
    assign(str, ::strlen(str));
}


String::String(const char* str, size_t len) : mStr(mStorage), mSize(0) {
    assign(str, len);
}


String::String(const String& other) : mStr(mStorage), mSize(0) {
    assign(other);
}


String::String(const StringView& other) : mStr(mStorage), mSize(0) {
    assign(other);
}


String::String(size_t count, char fill) : mStr(mStorage), mSize(0) {
    this->assign(count, fill);
}


String::~String() {
    reserve(0U);
    // Prevent misuse from dangling pointers.
    mStr = NULL;
    mSize = 0;
    mCapacity = 0;
}

String& String::assign(const char* str) {
    return this->assign(str, ::strlen(str));
}


String& String::assign(const char* str, size_t len) {
    this->resize(len);
    ::memmove(mStr, str, len);
    return *this;
}


String& String::assign(const String& other) {
    return this->assign(other.c_str(), other.size());
}


String& String::assign(const StringView& other) {
    return this->assign(other.str(), other.size());
}


String& String::assign(char ch) {
    return this->assign(&ch, 1U);
}


String& String::assign(size_t count, char fill) {
    this->resize(count);
    ::memset(mStr, fill, count);
    return *this;
}


String& String::append(const char* str, size_t len) {
    size_t oldSize = mSize;
    this->resize(mSize + len);
    ::memmove(mStr + oldSize, str, len);
    return *this;
}


String& String::append(const char* str) {
    return this->append(str, ::strlen(str));
}


String& String::append(const String& other) {
    return this->append(other.c_str(), other.size());
}


String& String::append(const StringView& other) {
    return this->append(other.str(), other.size());
}


String& String::append(char ch) {
    return this->append(&ch, 1U);
}


int String::compare(const char* str, size_t len) const {
    if (mSize == 0)
        return (len == 0) ? 0 : -1;

    if (len == 0)
        return +1;

    int ret = ::strncmp(mStr, str, len);
    if (ret < 0)
        return -1;
    if (ret > 0)
        return +1;

    if (mSize < len)
        return -1;
    if (mSize > len)
        return +1;
    return 0;
}


int String::compare(const char* str) const {
    return compare(str, ::strlen(str));
}


int String::compare(const String& other) const {
    return compare(other.c_str(), other.size());
}


int String::compare(const StringView& other) const {
    return compare(other.str(), other.size());
}


int String::compare(char ch) const {
    return compare(&ch, 1U);
}


bool String::equals(const char* str, size_t len) const {
    if (mSize == 0)
        return (len == 0);

    if (len != mSize)
        return false;

    return !::memcmp(mStr, str, len);
}


bool String::equals(const char* str) const {
    return equals(str, ::strlen(str));
}


bool String::equals(const String& other) const {
    return equals(other.c_str(), other.size());
}


bool String::equals(const StringView& other) const {
    return equals(other.str(), other.size());
}


bool String::equals(char ch) const {
    return equals(&ch, 1U);
}


void String::resize(size_t newSize) {
    if (!mStr)
        mStr = mStorage;

    size_t oldCapacity = capacity();
    if (newSize < oldCapacity) {
        if (oldCapacity >= 256U && newSize < oldCapacity / 2) {
            reserve(newSize);
        }
    } else if (newSize > oldCapacity) {
        const size_t kMaxCapacity = SIZE_MAX - 1U;
        CHECK(newSize < kMaxCapacity);

        size_t newCapacity = oldCapacity;
        while (newCapacity < newSize) {
            size_t newCapacity2 = newCapacity + (newCapacity >> 2) + 8;
            newCapacity = (newCapacity2 < newCapacity)
                    ? kMaxCapacity : newCapacity2;
        }
        reserve(newCapacity);
    }
    DCHECK(newSize <= capacity());
    mSize = newSize;
    mStr[newSize] = '\0';
}


void String::reserve(size_t newSize) {
    size_t minSize = (newSize < kMinCapacity) ? kMinCapacity : newSize;

    if (!mStr)
        mStr = mStorage;

    if (minSize == kMinCapacity) {
        if (mStr != mStorage) {
            // Copy the first bytes to mStorage, then free the heap
            // allocated buffer.
            ::memcpy(mStorage, mStr, newSize);
            ::free(mStr);
            mStr = mStorage;
        }
    } else /* newSize > kMinCapacity */ {
        char* oldStorage = (mStr == mStorage) ? NULL : mStr;
        size_t newStorageSize = newSize + 1U;
        mStr = static_cast<char*>(::realloc(oldStorage, newStorageSize));
#if xxxUSE_MALLOC_USABLE_SIZE
        size_t usableSize = malloc_usable_size(mStr);
        if (usableSize > newStorageSize)
            newStorageSize = usableSize;
#endif
        if (!oldStorage) {
            ::memcpy(mStr, mStorage, mSize);
        }
        if (newSize > mSize) {
            ::memset(mStr + mSize, 0, newSize - mSize);
        }
        mCapacity = newStorageSize - 1U;
    }
    mStr[newSize] = '\0';
}

void String::swap(String* other) {
    if (this == other)
        return;

    char* myStr = mStr;
    size_t mySize = mSize;
    size_t myCapacity = capacity();

    char* theirStr = other->mStr;
    size_t theirSize = other->mSize;
    size_t theirCapacity = other->capacity();

    if (myStr == mStorage) {
        if (theirStr == other->mStorage) {
            // Two small strings, swap buffer contents, no need to swap
            // pointers or capacities.
            for (size_t n = 0; n < kMinCapacity + 1U; ++n) {
                char tmp = myStr[n];
                myStr[n] = theirStr[n];
                theirStr[n] = tmp;
            }
        } else {
            // |this| is a small string, |other| is a long one.
            ::memcpy(other->mStorage, mStorage, mySize + 1U);
            other->mStr = other->mStorage;
            mStr = theirStr;
            mCapacity = theirCapacity;
        }
    } else if (theirStr == other->mStorage) {
        // |this| is a long string, |other| is a short string.
        ::memcpy(mStorage, other->mStorage, theirSize + 1U);
        other->mStr = myStr;
        other->mCapacity = myCapacity;
        mStr = mStorage;
    } else {
        // Both |this| and |other| are long strings.
        mStr = theirStr;
        mCapacity = theirCapacity;
        other->mStr = myStr;
        other->mCapacity = myCapacity;
    }
    // Always swap the sizes.
    mSize = theirSize;
    other->mSize = mySize;
}

// static
void String::adjustMovedSlice(String* fromStrings,
                              String* toStrings,
                              size_t count) {
    for (size_t n = 0; n < count; ++n) {
        if (toStrings[n].mStr == fromStrings[n].mStorage) {
            toStrings[n].mStr = toStrings[n].mStorage;
        }
    }
}

// static
void String::moveSlice(String* strings,
                       size_t from,
                       size_t to,
                       size_t count) {
    // First, move all slice items with ::memmove().
    ::memmove(strings + to, strings + from, count * sizeof(String));
    // Second, adjust mStorage pointers.
    adjustMovedSlice(strings + from, strings + to, count);
}

void String::finalizeSlice(String* strings, size_t count) {
    for (size_t n = count; n > 0; --n) {
        strings[n - 1U].reserve(0U);
    }
}

}  // namespace base
}  // namespace android
