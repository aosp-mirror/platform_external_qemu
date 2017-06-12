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
#include "android/globals.h"
#include "android/snapshot/RamLoader.h"
#include "android/snapshot/RamSaver.h"
#include "android/snapshot/common.h"
#include "android-qemu2-glue/vm_mem_mappings.h"

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

static std::string makeRamSaveFilename(const std::string& uuid) {
    auto dir = avdInfo_getContentPath(android_avdInfo);
    auto path = PathUtils::join(
            dir, android::base::StringFormat("snapshot_ram_%s.bin", uuid));
    return path;
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

static const QEMUFileHooks sSaveHooks = {
        // before_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
            if (flags == RAM_CONTROL_SETUP) {
                auto uuid = Uuid::generateFast().toString();
                auto name = makeRamSaveFilename(uuid);
                qemu_put_byte(f, uuid.size());
                qemu_put_buffer(f, (const uint8_t*)uuid.data(), uuid.size());
                sRamSaver->reset(
                        new RamSaver(StdioStream(fopen(name.c_str(), "wb"),
                                                 StdioStream::kOwner),
                                     isPageZeroed));

                // Now register all blocks for saving.
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
            sRamSaver.get()->savePage((int64_t)block_offset, (int64_t)offset,
                                      (int32_t)size);
            *bytes_sent = size;
            return size_t(RAM_SAVE_CONTROL_DELAYED);
        },
};

void load_page_right_now(uint64_t len, uint64_t gpa) {
    // bool found;
    // void* hva = gpa2hva(gpa, &found);
    // if (!found) return;
    // if (!found) { fprintf(stderr, "%s: wtf? not found 0x%llx\n", __func__, gpa); }
    // else { fprintf(stderr, "%s: fault in gpa 0x%llx -> hva %p\n", __func__, gpa, hva); }
    // sRamLoader->get()->readPageAtPtr(hva);
    // fprintf(stderr, "%s: fault in gpa 0x%llx -> hva %p\n", __func__, gpa,
    //         sRamLoader->get()->getHostRamAddrFromGuestPhysical(gpa));

    sRamLoader->get()->readAtGpa(len, gpa);
}

static const QEMUFileHooks sLoadHooks = {
        // before_ram_iterate
        nullptr,
        // after_ram_iterate
        nullptr,
        // hook_ram_load
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            switch (flags) {
                case RAM_CONTROL_BLOCK_REG: {
                    if (!sRamLoader->get() || sRamLoader->get()->wasStarted()) {
                        fprintf(stderr, "%s: first ram load hook.\n", __func__);
                        // register callback
                        android::snapshot::set_hva2gpa_func(hva2gpa);
                        // First call to the load hook - create a new loader.
                        sRamLoader->reset(new RamLoader(isPageZeroed));
                    }
                    RamBlock block;
                    block.id = static_cast<const char*>(data);
                    qemu_ram_foreach_block2(
                            [](const char* block_name, void* host_addr,
                               ram_addr_t offset, ram_addr_t length, uint64_t gpa,
                               void* opaque) {
                                auto block = static_cast<RamBlock*>(opaque);
                                if (block->id != block_name) {
                                    return 0;
                                }
                                block->startOffset = offset;
                                block->hostPtr = (uint8_t*)host_addr;
                                block->totalSize = length;
                                block->guestPhysicalAddress = gpa;
                                if (offset != gpa) {
                                    fprintf(stderr, "offset doesn't match gpa: id %s hva %p offset: 0x%llx gpa: 0x%llx size 0x%llx\n", block->id.c_str(), block->hostPtr, offset, gpa, (uint64_t)length);
                                } else {
                                    fprintf(stderr, "offset matches gpa: id %s hva %p offset: 0x%llx gpa: 0x%llx size 0x%llx\n", block->id.c_str(), block->hostPtr, offset, gpa, (uint64_t)length);
                                }
                                return 1;
                            },
                            &block);

                    RAMBlock* const qemuBlock =
                            qemu_ram_block_by_name(block.id.c_str());
                    block.pageSize = (int32_t)qemu_ram_pagesize(qemuBlock);
                    fprintf(stderr, "%s: register block %s\n", __func__, block.id.c_str());
                    sRamLoader->get()->registerBlock(block);
                    break;
                }
                case RAM_CONTROL_HOOK:
                    assert(sRamLoader->get());
                    if (!sRamLoader->get()->wasStarted()) {
                        auto uuid = readRamFileUuid(f);
                        auto name = makeRamSaveFilename(uuid);
                        sRamLoader->get()->startLoading(
                                StdioStream(fopen(name.c_str(), "rb"),
                                            StdioStream::kOwner));
                    }
                    break;
            }
            return 0;
        }};

void qemu_snapshot_hooks_setup() {
    migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);

    // qemu_add_vm_change_state_handler(
    //        [](void* opaque, int running, RunState state) {
    //            if (!running) {
    //                return;
    //            }
    //            if (auto rl = sRamLoader->get()) {
    //                rl->done();
    //            }
    //        },
    //        nullptr);
}
