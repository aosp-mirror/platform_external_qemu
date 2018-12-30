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

#include "android/base/StringView.h"
#include "android/base/Optional.h"

#include <algorithm>

#include <inttypes.h>

namespace android {
namespace base {

void StringView::clear() {
    mSize = 0;
    mString = "";
}

void StringView::set(const char* data, size_t len) {
    mString = data ? data : "";
    mSize = len;
}

void StringView::set(const StringView& other) {
    mString = other.mString;
    mSize = other.mSize;
}

StringView& StringView::operator=(const StringView& other) {
    set(other);
    return *this;
}

size_t StringView::find(StringView other, size_t off) {
    // Trivial case
    if (!other.mSize) return 0;

    size_t safeOff = std::min(off, mSize);

    const char* searchStart = mString + safeOff;
    const char* searchEnd = searchStart + mSize - safeOff;

    const char* res =
        std::search(searchStart, searchEnd,
                    other.mString, other.mString + other.mSize);
    if (res == searchEnd) return std::string::npos;
    return (size_t)((uintptr_t)res - (uintptr_t)mString);
}

StringView StringView::getSubstr(StringView other, size_t off) {
    size_t loc = find(other, off);
    if (loc == std::string::npos) return StringView("");
    return { mString + loc, end() };
}

StringView StringView::substr(size_t begin, size_t len) {
    if (len == std::string::npos) {
        len = mSize - begin;
    }
    size_t safeOff = std::min(begin, mSize);
    size_t safeLen = std::min(len, mSize - safeOff);
    return { mString + safeOff, safeLen };
}

StringView StringView::substrAbs(size_t begin, size_t end) {
    if (end == std::string::npos) {
        end = begin + mSize;
    }
    return substr(begin, end - begin);
}

StringView::operator std::string() const { return std::string(mString, mSize); }

int StringView::compare(const StringView& other) const {
    size_t minSize = std::min(mSize, other.size());
    if (minSize == 0) {
        return mSize < other.size() ? -1 : (mSize > other.size() ? +1 : 0);
    }
    int ret = memcmp(mString, other.data(), minSize);
    if (ret) return ret;
    if (mSize < other.size()) return -1;
    if (mSize > other.size()) return +1;
    return 0;
}

bool operator==(const StringView& x, const StringView& y) {
    if (x.size() != y.size()) return false;
    return memcmp(x.data(), y.data(), x.size()) == 0;
}

bool operator!=(const StringView& x, const StringView& y) {
    return !(x == y);
}
bool operator<(const StringView& x, const StringView& y) {
    return x.compare(y) < 0;
}

bool operator>=(const StringView& x, const StringView& y) {
    return !(x < y);
}

bool operator >(const StringView& x, const StringView& y) {
    return x.compare(y) > 0;
}

bool operator<=(const StringView& x, const StringView& y) {
    return !(x > y);
}

// CStrWrapper implementation
class CStrWrapper::Impl {
public:
    Impl(StringView stringView) : mStringView(stringView) {}

    // Returns a null-terminated char*, potentially creating a copy to add a
    // null terminator.
    const char* get() {
        if (mStringView.isNullTerminated()) {
            return mStringView.data();
        } else {
            // Create the std::string copy on-demand.
            if (!mStringCopy) {
                mStringCopy.emplace(mStringView.str());
            }

            return mStringCopy->c_str();
        }
    }

    // Enable casting to const char*
    operator const char*() { return get(); }

private:
    const StringView mStringView;
    Optional<std::string> mStringCopy;
};

CStrWrapper::CStrWrapper(StringView stringView) :
    mImpl(new CStrWrapper::Impl(stringView)) {}
CStrWrapper::~CStrWrapper() { delete mImpl; }
const char* CStrWrapper::get() { return mImpl->get(); }
CStrWrapper::operator const char*() { return get(); }

}  // namespace base
}  // namespace android
