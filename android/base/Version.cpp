// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Version.h"

#include "android/base/StringFormat.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <sstream>

namespace android {
namespace base {

// parse the current part of the version string and return if there's anything
// else left
namespace {

enum VersionParseResult {
    kContinue,
    kComplete,
    kError,
};

}  // namespace

static VersionParseResult parseVersionPart(const char* start,
                                           char** end,
                                           unsigned int* value) {
    errno = 0;
    const int val = strtol(start, end, 10);
    if (errno) {
        return kError;
    }
    if (*end == start || (**end != '.' && **end != 0)) {
        return kError;
    }

    *value = val;
    return **end == '.' ? kContinue : kComplete;
}

Version::Version(const char* ver) : mMajor(0), mMinor(0), mMicro(0) {
    char* end;
    VersionParseResult res = parseVersionPart(ver, &end, &mMajor);
    if (res != kContinue) {
        if (res == kError) {
            *this = Invalid();
        }
        return;
    }

    res = parseVersionPart(end + 1, &end, &mMinor);
    if (res != kContinue) {
        if (res == kError) {
            *this = Invalid();
        }
        return;
    }

    res = parseVersionPart(end + 1, &end, &mMicro);
    if (res != kComplete) {
        *this = Invalid();
    }
}

Version::Version(unsigned int major, unsigned int minor, unsigned int micro)
    : mMajor(major), mMinor(minor), mMicro(micro) {
}

bool Version::isValid() const {
    return *this != Invalid();
}

bool Version::operator<(const Version& other) const {
    if (mMajor < other.mMajor) {
        return true;
    }
    if (mMajor > other.mMajor) {
        return false;
    }

    if (mMinor < other.mMinor) {
        return true;
    }
    if (mMinor > other.mMinor) {
        return false;
    }

    return mMicro < other.mMicro;
}

bool Version::operator==(const Version& other) const {
    return mMajor == other.mMajor
            && mMinor == other.mMinor
            && mMicro == other.mMicro;
}

bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

String Version::toString() const {
    if (!isValid()) {
        return String("invalid");
    }

    const String res = StringFormat("%u.%u.%u", mMajor, mMinor, mMicro);
    return res;
}

Version Version::Invalid() {
    static const Version invalid(UINT_MAX, UINT_MAX, UINT_MAX);
    return invalid;
}

}  // namespace android
}  // namespace base
