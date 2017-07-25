// Copyright 2015 The Android Open Source Project
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
// limitations under the License.

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/callbacks.h"
#include "android/emulation/control/vm_operations.h"

extern "C" {
#include "qemu/osdep.h"

#include "migration/migration.h"
#include "migration/qemu-file.h"
#include "sysemu/sysemu.h"
}

#include <stdlib.h>
#include <string.h>

// TODO(zyy@): implement the VM snapshots support

static bool qemu_vm_stop() {
    vm_stop(RUN_STATE_DEBUG);
    return true;
}

static bool qemu_vm_start() {
    vm_start();
    return true;
}

static bool qemu_vm_is_running() {
    return runstate_is_running() != 0;
}

static bool qemu_snapshot_list(void* opaque,
                               LineConsumerCallback outConsumer,
                               LineConsumerCallback errConsumer) {
    //    Monitor* out = monitor_fake_new(opaque, outConsumer);
    //    Monitor* err = monitor_fake_new(opaque, errConsumer);
    //    do_info_snapshots(out, err);
    //    int ret = monitor_fake_get_bytes(err);
    //    monitor_fake_free(err);
    //    monitor_fake_free(out);
    //    return !ret;
    return false;
}

static bool qemu_snapshot_save(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    // Monitor* err = monitor_fake_new(opaque, errConsumer);
    // do_savevm(err, name);
    // int ret = monitor_fake_get_bytes(err);
    // monitor_fake_free(err);
    // return !ret;
    return false;
}

static bool qemu_snapshot_load(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    // Monitor* err = monitor_fake_new(opaque, errConsumer);
    // do_loadvm(err, name);
    // int ret = monitor_fake_get_bytes(err);
    // monitor_fake_free(err);
    // return !ret;
    return false;
}

static bool qemu_snapshot_delete(const char* name,
                                 void* opaque,
                                 LineConsumerCallback errConsumer) {
    // Monitor* err = monitor_fake_new(opaque, errConsumer);
    // do_delvm(err, name);
    // int ret = monitor_fake_get_bytes(err);
    // monitor_fake_free(err);
    // return !ret;
    return false;
}

static SnapshotCallbacks sSnapshotCallbacks = {};
static void* sSnapshotCallbacksOpaque = nullptr;

static int onSaveVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_SAVE].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onSaveVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_SAVE].onEnd(sSnapshotCallbacksOpaque, name, res);
}

static int onLoadVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_LOAD].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onLoadVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_LOAD].onEnd(sSnapshotCallbacksOpaque, name, res);
}

static int onDelVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_DEL].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onDelVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_DEL].onEnd(sSnapshotCallbacksOpaque, name, res);
}

static const QEMUSnapshotCallbacks sQemuSnapshotCallbacks = {
        .savevm = {onSaveVmStart, onSaveVmEnd},
        .loadvm = {onLoadVmStart, onLoadVmEnd},
        .delvm = {onDelVmStart, onDelVmEnd}};

static const QEMUFileHooks sSaveHooks = {
        // before_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
            if (flags == RAM_CONTROL_SETUP) {
                // Register all blocks for saving.
                qemu_ram_foreach_block(
                        [](const char* block_name, void* host_addr,
                           ram_addr_t offset, ram_addr_t length, void* opaque) {
                            SnapshotRamBlock block = {block_name, (int64_t)offset,
                                              (uint8_t*)host_addr,
                                              (int64_t)length};
                            sSnapshotCallbacks.ramOps.registerBlock(
                                    sSnapshotCallbacksOpaque, SNAPSHOT_SAVE,
                                    &block);
                            return 0;
                        },
                        nullptr);
            }
            return 0;
        },
        // after_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            if (flags == RAM_CONTROL_FINISH) {
                return sSnapshotCallbacks.ramOps.savingComplete(
                        sSnapshotCallbacksOpaque);
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
            sSnapshotCallbacks.ramOps.savePage(sSnapshotCallbacksOpaque,
                                               (int64_t)block_offset,
                                               (int64_t)offset, (int32_t)size);
            // Must set |bytes_sent| to non-zero, otherwise QEMU will save the
            // page in own way.
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
                    SnapshotRamBlock block;
                    block.id = static_cast<const char*>(data);
                    qemu_ram_foreach_block(
                            [](const char* block_name, void* host_addr,
                               ram_addr_t offset, ram_addr_t length,
                               void* opaque) {
                                auto block = static_cast<SnapshotRamBlock*>(opaque);
                                if (strcmp(block->id, block_name) != 0) {
                                    return 0;
                                }
                                block->startOffset = offset;
                                block->hostPtr = (uint8_t*)host_addr;
                                block->totalSize = length;
                                return 1;
                            },
                            &block);

                    RAMBlock* const qemuBlock =
                            qemu_ram_block_by_name(block.id);
                    block.pageSize = (int32_t)qemu_ram_pagesize(qemuBlock);
                    sSnapshotCallbacks.ramOps.registerBlock(
                            sSnapshotCallbacksOpaque, SNAPSHOT_LOAD, &block);
                    break;
                }
                case RAM_CONTROL_HOOK: {
                    return sSnapshotCallbacks.ramOps.startLoading(
                            sSnapshotCallbacksOpaque);
                }
            }
            return 0;
        }};

static void set_snapshot_callbacks(void* opaque,
                                   const SnapshotCallbacks* callbacks) {
    if (!opaque || !callbacks) {
        sSnapshotCallbacks = {};
        sSnapshotCallbacksOpaque = nullptr;
        qemu_set_snapshot_callbacks(nullptr);
        migrate_set_file_hooks(nullptr, nullptr);
    } else {
        sSnapshotCallbacks = *callbacks;
        sSnapshotCallbacksOpaque = opaque;
        qemu_set_snapshot_callbacks(&sQemuSnapshotCallbacks);
        migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);
    }
}

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = qemu_vm_stop,
        .vmStart = qemu_vm_start,
        .vmIsRunning = qemu_vm_is_running,
        .snapshotList = qemu_snapshot_list,
        .snapshotSave = qemu_snapshot_save,
        .snapshotLoad = qemu_snapshot_load,
        .snapshotDelete = qemu_snapshot_delete,
        .setSnapshotCallbacks = set_snapshot_callbacks,
};
const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;
