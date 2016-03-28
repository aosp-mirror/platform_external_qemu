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

#pragma once

#include "android/base/StringView.h"

#include <string>
#include <tuple>

namespace android {
namespace base {

// Class |Version| is a class for software version manipulations
// it is able to parse, store, compare and convert version back to string
// Expected string format is "major.minor.micro[.build]", where all
// components are unsigned numbers (and, hopefully, reasonably small)
class Version {
public:
    // This is the value for an invalid/missing version component.
    static constexpr unsigned int kNone = static_cast<unsigned int>(-1);

    explicit Version(StringView ver);
    constexpr Version(unsigned int major, unsigned int minor,
                      unsigned int micro, unsigned int build = 0);

    constexpr bool isValid() const;

    constexpr bool operator<(const Version& other) const;
    constexpr bool operator==(const Version& other) const;
    constexpr bool operator!=(const Version& other) const;

    static constexpr Version invalid();

    std::string toString() const;

private:
    std::tuple<unsigned int, unsigned int, unsigned int, unsigned int> mData;

    enum {
        kMajor, kMinor, kMicro, kBuild
    };
};

// all constexpr functions have to be defined in the header, just like templates

constexpr Version::Version(unsigned int major,
                           unsigned int minor,
                           unsigned int micro,
                           unsigned int build)
    : mData(major, minor, micro, build) {}

constexpr bool Version::isValid() const {
    return *this != invalid();
}

constexpr bool Version::operator<(const Version& other) const {
    return mData < other.mData;
}

constexpr bool Version::operator==(const Version& other) const {
    return mData == other.mData;
}

constexpr bool Version::operator!=(const Version& other) const {
    return !(*this == other);
}

constexpr Version Version::invalid() {
    return Version(kNone, kNone, kNone);
}

}  // namespace android
}  // namespace base
