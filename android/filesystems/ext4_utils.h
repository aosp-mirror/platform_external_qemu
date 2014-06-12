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

#ifndef ANDROID_FILESYSTEMS_EXT4_UTILS_H
#define ANDROID_FILESYSTEMS_EXT4_UTILS_H

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <inttypes.h>

ANDROID_BEGIN_HEADER

// Create a new empty EXT4 partition image file at |filePath|
// of |size| bytes. |mountPoint| is the name of the corresponding
// mount point, e.g. 'cache' for the cache partition.
// Returns 0 on success, or -errno on failure.
int android_createEmptyExt4Image(const char *filePath,
                                 uint64_t size,
                                 const char *mountpoint);

// Returns true iff the file at |filePath| is an actual EXT4 partition image.
bool android_pathIsExt4PartitionImage(const char* filePath);


ANDROID_END_HEADER

#endif  // ANDROID_FILESYSTEMS_EXT4_UTILS_H
