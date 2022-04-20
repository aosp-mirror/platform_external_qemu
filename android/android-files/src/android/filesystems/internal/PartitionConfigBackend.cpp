// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/filesystems/internal/PartitionConfigBackend.h"

#include "android/filesystems/ext4_resize.h"
#include "android/filesystems/fstab_parser.h"
#include "android/filesystems/ramdisk_extractor.h"
#include "android/utils/filelock.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#include <cerrno>
#include <cstdlib>

namespace android::internal {

class DefaultPartitionConfigBackend : public PartitionConfigBackend {
public:
    auto pathExists(const char* path) -> bool override {
        return path_exists(path) != 0;
    }

    auto pathEmptyFile(const char* path) -> bool override {
        return path_empty_file(path) == 0;
    }

    auto pathCopyFile(const char* dst, const char* src) -> bool override {
        return path_copy_file(dst, src) == 0;
    }

    auto pathLockFile(const char* path) -> bool override {
        return filelock_create(path) != nullptr;
    }

    auto pathCreateTempFile(std::string* path) -> bool override {
        TempFile* temp_file = tempfile_create();
        if (temp_file == nullptr) {
            return false;
        }
        *path = tempfile_path(temp_file);
        return true;
    }

    auto probePartitionFileType(const char* path)
            -> AndroidPartitionType override {
        return androidPartitionType_probeFile(path);
    }

    auto extractRamdiskFile(const char* ramdisk_path,
                            const char* file_path,
                            std::string* out) -> bool override {
        char* out_data = nullptr;
        size_t out_size = 0U;
        if (!android_extractRamdiskFile(ramdisk_path, file_path, &out_data,
                                        &out_size)) {
            return false;
        }
        out->assign(out_data, out_size);
        free(out_data);
        return true;
    }

    auto parsePartitionFormat(const std::string& fstab,
                              const char* mountPath,
                              std::string* partitionFormat) -> bool override {
        char* out_format = nullptr;
        if (!android_parseFstabPartitionFormat(fstab.c_str(), fstab.size(),
                                               mountPath, &out_format)) {
            return false;
        }
        partitionFormat->assign(out_format);
        free(out_format);
        return true;
    }

    auto makeEmptyPartition(AndroidPartitionType partitionType,
                            uint64_t partitionSize,
                            const char* partitionFile) -> bool override {
        int ret = androidPartitionType_makeEmptyFile(
                partitionType, partitionSize, partitionFile);
        if (ret < 0) {
            errno = -ret;
            return false;
        }
        return true;
    }

    void resizeExt4Partition(const char* partitionPath,
                             uint64_t partitionSize) override {
        (void)::resizeExt4Partition(partitionPath,
                                    static_cast<int64_t>(partitionSize));
    }
};

// static
PartitionConfigBackend* PartitionConfigBackend::sInstance = nullptr;

// static
auto PartitionConfigBackend::get() -> PartitionConfigBackend* {
    if (sInstance == nullptr) {
        sInstance = new DefaultPartitionConfigBackend();
    }
    return sInstance;
}

// static
auto PartitionConfigBackend::setForTesting(PartitionConfigBackend* newBackend)
        -> PartitionConfigBackend* {
    PartitionConfigBackend* oldBackend = sInstance;
    sInstance = newBackend;
    return oldBackend;
}

}  // namespace android::internal
