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

#include "android/filesystems/partition_types.h"

#include "android/filesystems/ext4_utils.h"
#include "android/utils/panic.h"
#include "android/utils/path.h"

#include <errno.h>

const char* androidPartitionType_toString(AndroidPartitionType part_type) {
    switch (part_type) {
        case ANDROID_PARTITION_TYPE_UNKNOWN:
            return "unknown";
        case ANDROID_PARTITION_TYPE_YAFFS2:
            return "yaffs2";
        case ANDROID_PARTITION_TYPE_EXT4:
            return "ext4";
        default:
            APANIC("Invalid partition type value %d", part_type);
    }
}


AndroidPartitionType androidPartitionType_probeFile(const char* image_file) {
    if (!path_exists(image_file)) {
        return ANDROID_PARTITION_TYPE_UNKNOWN;
    }
    if (android_pathIsExt4PartitionImage(image_file)) {
        return ANDROID_PARTITION_TYPE_EXT4;
    }
    // Assume YAFFS2, since there is little way to be sure for now.
    // NOTE: An empty file is a valid Yaffs2 file!
    return ANDROID_PARTITION_TYPE_YAFFS2;
}


int androidPartitionType_makeEmptyFile(AndroidPartitionType part_type,
                                       uint64_t part_size,
                                       const char* part_file) {
    switch (part_type) {
        case ANDROID_PARTITION_TYPE_YAFFS2:
            // Any empty file is a valid YAFFS2 partition, |part_size|
            // can be ignored here.
            if (path_empty_file(part_file) < 0) {
                return -errno;
            }
            return 0;

        case ANDROID_PARTITION_TYPE_EXT4:
            return android_createEmptyExt4Image(part_file, part_size, NULL);

        default:
            return -EINVAL;
    }
}
