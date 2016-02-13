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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

namespace android {
namespace base {

FilePath::CharType FilePath::kSeparators[] = "/";
size_t FilePath::kSeparatorsLength = ARRAY_SIZE(FilePath::kSeparators);

std::string FilePath::asUtf8() const {
    return mPath.c_str();
}

bool FilePath::isAbsolute() const {
    return mPath.size() > 0 &&  mPath[0] == kSeparators[0];
}

}  // namespace base
}  // namespace android
