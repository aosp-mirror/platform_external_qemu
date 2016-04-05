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

#pragma once

// This file defines a macro ARRAY_SIZE() for static array size detection
// If used from C, it's just a standard division of sizes. In C++ though, it
// would give you a compile-time error if used with something other than
// a built-in array or std::array<>
// Also, C++ defines a constexpr function arraySize() for the same purpose

#ifndef __cplusplus
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#else

#include <array>

namespace android {
namespace base {

template <class T, size_t N>
static constexpr size_t arraySize(const T (&arr)[N]) {
    return N;
}

template <class T, size_t N>
static constexpr size_t arraySize(const std::array<T, N>& arr) {
    return N;
}

}  // namespace base
}  // namespace android

// for those who like macros, define it to be a simple function call
#define ARRAY_SIZE(arr) (::android::base::arraySize(arr))

#endif  // __cplusplus
