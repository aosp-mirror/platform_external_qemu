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

#ifndef ANDROID_BASE_MISC_HTTP_UTILS_H
#define ANDROID_BASE_MISC_HTTP_UTILS_H

#include <stddef.h>

namespace android {
namespace base {

// Return true iff the content of |line|, or |lineLen| characters,
// matches an HTTP request definition. See section 5.1 or RFC 2616.
bool httpIsRequestLine(const char* line, size_t lineLen);

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_MISC_HTTP_UTILS_H
