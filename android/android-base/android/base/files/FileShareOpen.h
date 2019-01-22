// Copyright 2018 The Android Open Source Project
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

#include <cstdio>

namespace android {
namespace base {

enum class FileShare {
    Read,
    Write,
};

FILE* fsopen(const char* filename, const char* mode, FileShare fileshare);
FILE* fsopenWithTimeout(const char* filename, const char* mode,
        FileShare fileshare, int timeoutMs);

bool updateFileShare(FILE* file, FileShare fileshare);

// Create a file. This operation is safe for multi-processing if other process
// tries to fsopen the file during the creation.
void createFileForShare(const char* filename);

bool updateFileShare(FILE* file, FileShare fileshare);

}  // namespace base
}  // namespace android
