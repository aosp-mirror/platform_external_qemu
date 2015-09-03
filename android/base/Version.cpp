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

#include "android/utils/debug.h"
#include "android/utils/misc.h"

#include <errno.h>
#include <limits.h>

namespace android {
namespace base {

const Version Version::Invalid(UINT_MAX, UINT_MAX, UINT_MAX);

// parse the current part of version string and return if there's anything else
// left
namespace {

enum VersionParseResult {
    Continue,
    Complete,
    Error,
};

}  // namespace

static VersionParseResult parseVersionPart(const char* start,
                                           char*& end,
                                           unsigned int* value) {
    const int val = strtoi(start, &end, 10);
    if (errno) {
        return Error;
    }
    if (*end != '.' && *end != 0) {
        return Error;
    }

    *value = val;
    return *end == '.' ? Continue : Complete;
}

Version::Version(const char* ver) : mMajor(0), mMinor(0), mMicro(0) {
    char* end;
    VersionParseResult res = parseVersionPart(ver, end, &mMajor);
    if (res != Continue) {
        if (res == Error) {
            *this = Invalid;
        }
        return;
    }

    res = parseVersionPart(end + 1, end, &mMinor);
    if (res != Continue) {
        if (res == Error) {
            *this = Invalid;
        }
        return;
    }

    res = parseVersionPart(end + 1, end, &mMicro);
    if (res == Error) {
        *this = Invalid;
    } else if (res != Complete) {
        dwarning("Version: invalid version string '%s'", ver);
    }
}

Version::Version(unsigned int major, unsigned int minor, unsigned int micro)
    : mMajor(major), mMinor(minor), mMicro(micro) {}

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

}  // namespace android
}  // namespace base
