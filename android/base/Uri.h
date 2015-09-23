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

// Some free functions for manipulating Strings as URIs. Wherever possible,
// these functions take const references to StringView to avoid unnecessary
// copies.

#include "android/base/String.h"
#include "android/base/StringView.h"

namespace android {
namespace base {

class Uri {
public:
    // |Encode| is aggressive -- it will always encode a reserved character,
    // disregarding a possibly included URL scheme.
    static String Encode(const StringView& uri);

    // |Decode| is aggressive. It will decode every occurance of %XX in a single
    // pass -- even for unreserved characters.
    // Returns empty string on error.
    static String Decode(const StringView& uri);
};

}  // namespace base
}  // namespace android
