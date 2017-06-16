// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License

#include "android-qemu2-glue/snapshot_hooks.h"

#include "android/avd/info.h"
#include "android/base/StringFormat.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/RamSaver.h"
#include "android/snapshot/common.h"
#include "android/utils/path.h"

extern "C" {
#include "qemu/osdep.h"
}

extern "C" {
#include "migration/migration.h"
#include "migration/qemu-file.h"
#include "qemu/cutils.h"
#include "sysemu/sysemu.h"
}

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::StdioStream;
using android::base::Uuid;

using android::snapshot::RamBlock;
using android::snapshot::RamLoader;
using android::snapshot::RamSaver;

static LazyInstance<std::unique_ptr<RamSaver>> sRamSaver = {};
static LazyInstance<std::unique_ptr<RamLoader>> sRamLoader = {};

static std::string getSnapshotDir(const char* snapshotName, bool create) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path = PathUtils::join(dir, "snapshots", snapshotName);
    if (create) {
        if (path_mkdir_if_needed(path.c_str(), 0777) != 0) {
            return {};
        }
    }
    return path;
}

static std::string getRamFileName(const std::string& snapshotDir) {
    return PathUtils::join(snapshotDir, "ram.bin");
}

static bool isPageZeroed(const uint8_t* ptr, int size) {
    return buffer_is_zero(ptr, size);
}

static std::string readRamFileUuid(QEMUFile* f) {
    int uuidSize = qemu_get_byte(f);
    std::string uuid(uuidSize, '\0');
    qemu_get_buffer(f, (uint8_t*)&uuid[0], uuidSize);
    return uuid;
}

static int onSaveVmStart(const char* name) {
    auto dir = getSnapshotDir(name, true);
    if (dir.empty()) {
        return -EINVAL;
    }

    auto ramFile = fopen(getRamFileName(dir).c_str(), "wb");
    if (!ramFile) {
        return errno ? -errno : -EINVAL;
    }
    sRamSaver->reset(new RamSaver(
            StdioStream(ramFile, StdioStream::kOwner), isPageZeroed));
    return 0;
}

static void onSaveVmEnd(const char* name, int res) {
    if (res != 0) {
        sRamSaver->reset();
        // The original snapshot is already screwed, so let's at least clean up
        // the current one.
        path_delete_file(getRamFileName(getSnapshotDir(name, false)).c_str());
    }
}

static int onLoadVmStart(const char* name) {
    auto& loader = sRamLoader.get();
    if (loader) {
        loader.reset();
    }

    // First call to the load hook - create a new loader.
    auto dir = getSnapshotDir(name, false);
    if (!path_is_dir(dir.c_str())) {
        // No such directory - a raw QEMU snapshot?
        return 0;
    }

    auto ramFileName = getRamFileName(dir);
    auto ramFile = fopen(ramFileName.c_str(), "rb");
    if (!ramFile) {
        const auto err = errno ? -errno : -EINVAL;
        sRamLoader->reset();
        return err;
    }
    loader.reset(new RamLoader(StdioStream(ramFile, StdioStream::kOwner), isPageZeroed));
    return 0;
}

static void onLoadVmEnd(const char* name, int res) {
    if (res != 0) {
        sRamLoader->reset();
    }
}

static int onDelVmStart(const char* name) {
    auto snapshotDir = getSnapshotDir(name, false);
    if (!path_is_dir(snapshotDir.c_str())) {
        return 0;
    }
    return path_delete_dir(snapshotDir.c_str());
}

static void onDelVmEnd(const char* name, int res) {
    ;
}

static const QEMUFileHooks sSaveHooks = {
        // before_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
            if (flags == RAM_CONTROL_SETUP) {
                // Register all blocks for saving.
                qemu_ram_foreach_block(
                        [](const char* block_name, void* host_addr,
                           ram_addr_t offset, ram_addr_t length, void* opaque) {
                            auto ramSaver = static_cast<RamSaver*>(opaque);
                            RamBlock block = {block_name, (int64_t)offset,
                                              (uint8_t*)host_addr,
                                              (int64_t)length};
                            ramSaver->registerBlock(block);
                            return 0;
                        },
                        sRamSaver->get());
            }
            return 0;
        },
        // after_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            if (flags == RAM_CONTROL_FINISH) {
                sRamSaver->reset();
            }
            return 0;
        },
        // hook_ram_load
        nullptr,
        // save_page
        [](QEMUFile* f,
           void* opaque,
           ram_addr_t block_offset,
           ram_addr_t offset,
           size_t size,
           uint64_t* bytes_sent) {
            auto saver = sRamSaver->get();
            if (!saver) {
                qemu_file_set_error(f, -EINVAL);
                return size_t(-1);
            }
            saver->savePage((int64_t)block_offset, (int64_t)offset,
                            (int32_t)size);
            *bytes_sent = size;
            return size_t(RAM_SAVE_CONTROL_DELAYED);
        },
};

static const QEMUFileHooks sLoadHooks = {
        // before_ram_iterate
        nullptr,
        // after_ram_iterate
        nullptr,
        // hook_ram_load
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            switch (flags) {
                case RAM_CONTROL_BLOCK_REG: {
                    auto& loader = sRamLoader.get();
                    if (!loader || loader->wasStarted()) {
                        qemu_file_set_error(f, -EINVAL);
                        return -1;
                    }
                    RamBlock block;
                    block.id = static_cast<const char*>(data);
                    qemu_ram_foreach_block(
                            [](const char* block_name, void* host_addr,
                               ram_addr_t offset, ram_addr_t length,
                               void* opaque) {
                                auto block = static_cast<RamBlock*>(opaque);
                                if (block->id != block_name) {
                                    return 0;
                                }
                                block->startOffset = offset;
                                block->hostPtr = (uint8_t*)host_addr;
                                block->totalSize = length;
                                return 1;
                            },
                            &block);

                    RAMBlock* const qemuBlock =
                            qemu_ram_block_by_name(block.id.c_str());
                    block.pageSize = (int32_t)qemu_ram_pagesize(qemuBlock);
                    loader->registerBlock(block);
                    break;
                }
                case RAM_CONTROL_HOOK: {
                    auto loader = sRamLoader->get();
                    if (!loader) {
                        qemu_file_set_error(f, -EINVAL);
                        return -1;
                    }
                    if (!loader->wasStarted()) {
                        loader->startLoading();
                    }
                    break;
                }
            }
            return 0;
        }};

void qemu_snapshot_hooks_setup() {
    migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);
    QEMUSnapshotCallbacks snapshotCallbacks = {
        .savevm = { onSaveVmStart, onSaveVmEnd },
        .loadvm = { onLoadVmStart, onLoadVmEnd },
        .delvm = { onDelVmStart, onDelVmEnd }
    };
    qemu_set_snapshot_callbacks(&snapshotCallbacks);
}
