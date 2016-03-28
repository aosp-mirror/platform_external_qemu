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

#include <sstream>

namespace android {
namespace base {

using std::get;  // for get<|index|>(|tuple|)

static constexpr StringView kInvalidVersion = "invalid";

Version::Version(StringView ver) : mData() {
    std::istringstream in(ver);
    char c[2];
    in >> std::noskipws >> get<kMajor>(mData) >> c[0] >> get<kMinor>(mData)
       >> c[1] >> get<kMicro>(mData);
    if (c[0] != '.' || c[1] != '.' || !in) {
        *this = invalid();
        return;
    }

    if (in.peek() != std::char_traits<char>::eof()) {
        // it has to be the build string - "-<build>"
        in >> c[0] >> get<kBuild>(mData);
        if (c[0] != '-' || !in) {
            *this = invalid();
        }
    }

    // make sure the stream is consumed to the end
    if (in.peek() != std::char_traits<char>::eof()) {
        *this = invalid();
    }
}

std::string Version::toString() const {
    if (!isValid()) {
        return kInvalidVersion;
    }

    std::string res = StringFormat("%u.%u.%u", get<kMajor>(mData),
                                   get<kMinor>(mData), get<kMicro>(mData));
    if (get<kBuild>(mData) != kNone && get<kBuild>(mData) != 0) {
        StringAppendFormat(&res, "-%u", get<kBuild>(mData));
    }
    return res;
}

}  // namespace android
}  // namespace base
