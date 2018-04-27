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

namespace android {
namespace internal {

class DefaultPartitionConfigBackend : public PartitionConfigBackend {
public:
    bool pathExists(const char* path) override { return path_exists(path); }

    bool pathEmptyFile(const char* path) override {
        return path_empty_file(path) == 0;
    }

    bool pathCopyFile(const char* dst, const char* src) override {
        return path_copy_file(dst, src) == 0;
    }

    bool pathLockFile(const char* path) override {
        return filelock_create(path) != nullptr;
    }

    bool pathCreateTempFile(std::string* path) override {
        TempFile* temp_file = tempfile_create();
        if (!temp_file) {
            return false;
        }
        *path = tempfile_path(temp_file);
        return true;
    }

    AndroidPartitionType probePartitionFileType(const char* path) override {
        return androidPartitionType_probeFile(path);
    }

    bool extractRamdiskFile(const char* ramdisk_path,
                            const char* file_path,
                            std::string* out) override {
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

    bool parsePartitionFormat(const std::string& fstab,
                              const char* mountPath,
                              std::string* partitionFormat) override {
        char* out_format = nullptr;
        if (!android_parseFstabPartitionFormat(fstab.c_str(), fstab.size(),
                                               mountPath, &out_format)) {
            return false;
        }
        partitionFormat->assign(out_format);
        free(out_format);
        return true;
    }

    bool makeEmptyPartition(AndroidPartitionType partitionType,
                            uint64_t partitionSize,
                            const char* partitionFile) override {
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
PartitionConfigBackend* PartitionConfigBackend::get() {
    if (!sInstance) {
        sInstance = new DefaultPartitionConfigBackend();
    }
    return sInstance;
}

// static
PartitionConfigBackend* PartitionConfigBackend::setForTesting(
        PartitionConfigBackend* newBackend) {
    PartitionConfigBackend* oldBackend = sInstance;
    sInstance = newBackend;
    return oldBackend;
}

}  // namespace internal
}  // namespace android
