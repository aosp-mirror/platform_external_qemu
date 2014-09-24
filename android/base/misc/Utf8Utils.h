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

#ifndef ANDROID_BASE_UTF8_UTILS_H
#define ANDROID_BASE_UTF8_UTILS_H

#include <stddef.h>

namespace android {
namespace base {

// Return true iff |text| corresponds to |textLen| bytes of valid UTF-8
// encoded text.
bool utf8IsValid(const char* text, size_t textLen);

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_UTF8_UTILS_H
