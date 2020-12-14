// Copyright (C) 2016 The Android Open Source Project
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

namespace android {

// Reads from |fd| into |file_contents|
// Returns false if something went wrong
bool readFileIntoString(int fd, std::string* file_contents);

// Reads |name| file
base::Optional<std::string> readFileIntoString(base::StringView name);

// Writes |file_contents| to |fd|
// Returns false if something went wrong
bool writeStringToFile(int fd, const std::string& file_contents);

// Sets the file size to |size|, could either extend or truncate it.
bool setFileSize(int fd, int64_t size);

}  // namespace android
