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

namespace android {
namespace base {

FilePath& FilePath::operator=(const FilePath& other) {
    if (this != &other) {
        mPath = other.mPath;
    }
    return *this;
}

bool FilePath::append(const FilePath& child, FilePath* path) const {
    if (path == nullptr || child.isAbsolute()) {
        return false;
    }
    *path = *this;
    while (!path->mPath.empty() && isPathSeparator(path->mPath.back())) {
        path->mPath.resize(path->mPath.size() - 1);
    }
    path->mPath += kSeparators[0];
    path->mPath += child.mPath;
    return true;
}

FilePath& FilePath::concat(const FilePath& other) {
    return concat(other.native());
}

FilePath& FilePath::concat(const StringType& other) {
    mPath += other;
    return *this;
}

// static
bool FilePath::isPathSeparator(CharType c) {
    for (size_t i = 0; i < kSeparatorsLength; ++i) {
        if (c == kSeparators[i]) {
            return true;
        }
    }
    return false;
}

}  // namespace base
}  // namespace android
