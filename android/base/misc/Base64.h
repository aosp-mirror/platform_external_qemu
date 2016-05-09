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

#include "android/base/StringView.h"

#include <string>

namespace android {

// Encode binary string |str| into its Base64 representation.
std::string base64Encode(android::base::StringView str);

std::string base64Encode(const uint8_t* bytes, size_t byteLen);

// Decode base-64 encoded |str|. On success, return true and sets |*dst|
// to the decoded byte stream. On failure (i.e. invalid input), return false.
bool base64Decode(android::base::StringView str, std::string* dst);

}  // namespace
