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

#include "android/base/Optional.h"
#include "android/base/StringView.h"

#include <string>

#include <stddef.h>

namespace android {

// Encode binary string |src| of |srcLen| bytes into its Base64 representation.
std::string base64Encode(const void* src, size_t srcLen);

// Encode binary string |str| into its Base64 representation.
inline std::string base64Encode(android::base::StringView str) {
    return base64Encode(str.c_str(), str.size());
}

// Return the size in bytes of the base-64 encoding of any stream of
// |inputSize| bytes.
constexpr size_t base64EncodedSizeForInputSize(size_t inputSize) {
    return (inputSize + 2) / 3 * 4;
}

// Decode base-64 encoded |str|. On success, return true and sets |*dst|
// to the decoded byte stream. On failure (i.e. invalid input), return false.
base::Optional<std::string> base64Decode(const void* src, size_t srcLen);

inline base::Optional<std::string> base64Decode(android::base::StringView str) {
    return base64Decode(str.c_str(), str.size());
}

}  // namespace
