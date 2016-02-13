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

#include "android/base/system/Win32UnicodeString.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

namespace android {
namespace base {

FilePath::CharType FilePath::kSeparators[] = L"\\/";
size_t FilePath::kSeparatorsLength = ARRAY_SIZE(FilePath::kSeparators);

FilePath::FilePath(const char* path)
    : mPath(Win32UnicodeString(path).c_str()) {

}

FilePath::FilePath(const std::string& path)
    : mPath(Win32UnicodeString(path.c_str()).c_str()) {

}

std::string FilePath::asUtf8() const {
    return Win32UnicodeString::convertToUtf8StdString(mPath.c_str());
}

bool FilePath::isAbsolute() const {
    if (mPath.size() > 2) {
        if (mPath[1] == L':' && isPathSeparator(mPath[2])) {
            // Regular path with a letter followed by colon and separator
            return (mPath[0] >= L'A' && mPath[0] <= L'Z') ||
                   (mPath[0] >= L'a' && mPath[0] <= L'z');
        }
    }
    if (mPath.size() > 1) {
        // UNC path starting with double path separators
        return isPathSeparator(mPath[0]) && isPathSeparator(mPath[1]);
    }
    return false;
}

}  // namespace base
}  // namespace android
