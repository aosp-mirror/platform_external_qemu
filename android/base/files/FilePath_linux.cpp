// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/FilePath.h"

#include "android/base/String.h"

namespace android {
namespace base {

FilePath::CharType FilePath::kSeparators[] = "/";

FilePath::FilePath(StringView path)
    : mPath(path.c_str()) {

}

FilePath::FilePath(const String& path)
    : mPath(path.c_str()) {

}

// Get the path encoded as UTF-8, may require conversion
String FilePath::asUtf8() const {
    return mPath.c_str();
}

bool FilePath::isPathSeparator(CharType c) const {
    for (auto separator : kSeparators) {
        if (c == separator) {
            return true;
        }
    }
    return false;
}

}  // namespace base
}  // namespace android
