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

#include "android/base/containers/StringVector.h"

#include "android/base/StringView.h"

#include <stdio.h>

namespace android {
namespace base {

StringVector::StringVector(const StringVector& other) : PodVector<String>() {
    size_t count = other.size();
    // We rely on the fact that an all-0 String instance is properly
    // initialized to the empty string.
    resize(count);
    for (size_t n = 0; n < count; ++n) {
        (*this) [n] = other[n];
    }
}

StringVector& StringVector::operator=(const StringVector& other) {
    reserve(0U);
    resize(other.size());
    for (size_t n = 0; n < other.size(); ++n) {
        (*this)[n] = other[n];
    }
    return *this;
}

StringVector::~StringVector() {
    reserve(0U);
}

void StringVector::resize(size_t newSize) {
    size_t oldSize = size();
    String* oldStrings = begin();
    if (newSize < oldSize) {
        String::finalizeSlice(oldStrings + newSize, oldSize - newSize);
    }
    PodVectorBase::resize(newSize, sizeof(String));
    if (oldStrings != begin()) {
        String::adjustMovedSlice(oldStrings, begin(), newSize);
    }
}

void StringVector::reserve(size_t newSize) {
    size_t oldSize = size();
    String* oldStrings = begin();
    if (newSize < oldSize) {
        String::finalizeSlice(oldStrings + newSize, oldSize - newSize);
    }
    PodVectorBase::reserve(newSize, sizeof(String));
    if (oldStrings != begin()) {
        String::adjustMovedSlice(oldStrings, begin(), newSize);
    }
}

void StringVector::remove(size_t index) {
    size_t oldSize = size();
    if (index >= oldSize)
        return;
    String::finalizeSlice(begin(), 1U);
    String::moveSlice(begin(), index + 1, index, oldSize - index);
}

String* StringVector::emplace(size_t index) {
    size_t oldSize = size();
    DCHECK(index <= oldSize);
    resize(oldSize + 1U);
    String::moveSlice(begin(), index, index + 1, oldSize - index);
    String* result = begin() + index;
    ::memset(result, 0, sizeof(String));
    return result;
}

void StringVector::append(const String& str) {
    *(this->emplace(this->size())) = str;
}

void StringVector::prepend(const String& str) {
    *(this->emplace(0U)) = str;
}

void StringVector::insert(size_t index, const String& str) {
    *(this->emplace(index)) = str;
}

void StringVector::append(const StringView& view) {
    *(this->emplace(this->size())) = view;
}

void StringVector::prepend(const StringView& view) {
    *(this->emplace(0U)) = view;
}

void StringVector::insert(size_t index, const StringView& view) {
    *(this->emplace(index)) = view;
}

void StringVector::swap(StringVector* other) {
    size_t mySize = size();
    size_t otherSize = other->size();
    PodVectorBase::swapAll(other);
    String::adjustMovedSlice(this->begin(), other->begin(), mySize);
    String::adjustMovedSlice(other->begin(), this->begin(), otherSize);
}

}  // namespace base
}  // namespace android
