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

#include "android/base/files/PathUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/emulation/control/callbacks.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/emulation/VmLock.h"
#include "android/snapshot/common.h"
#include "android/snapshot/interface.h"
#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshotter.h"

extern "C" {
#include "qemu/osdep.h"

#include "exec/cpu-common.h"
#include "migration/qemu-file.h"
#include "qapi/error.h"
#include "qapi/qapi-commands-misc.h"
#include "qapi/qmp/qobject.h"
#include "qapi/qmp/qstring.h"
#include "sysemu/hvf.h"
#include "sysemu/kvm.h"
#include "sysemu/hax.h"
#include "sysemu/whpx.h"
#include "sysemu/sysemu.h"
#include "sysemu/cpus.h"
#include "exec/cpu-common.h"
#include "exec/memory-remap.h"
}

#include <string>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

using android::base::PathUtils;
using android::base::StringAppendFormatWithArgs;
using android::base::StringFormatWithArgs;
using android::base::StringView;

static bool qemu_vm_stop() {
    vm_stop(RUN_STATE_PAUSED);
    return true;
}

static bool qemu_vm_start() {
    vm_start();
    return true;
}

static bool qemu_vm_is_running() {
    return runstate_is_running() != 0;
}

namespace {

// A custom callback class to correctly format and forward messages
// into the user-supplied line callbacks.
struct MessageCallback : QEMUMessageCallback {
    MessageCallback(void* opaque,
                    LineConsumerCallback out,
                    LineConsumerCallback err)
        : userOpaque(opaque), userOut(out), userErr(err) {
        this->opaque = this;
        this->out = outCb;
        this->err = errCb;
    }

    MessageCallback(const MessageCallback& other)
        : QEMUMessageCallback(other),
          userOpaque(other.userOpaque),
          userOut(other.userOut),
          userErr(other.userErr) {
        this->opaque = this;
    }

    void operator=(const MessageCallback&) = delete;

    operator const QEMUMessageCallback*() const {
        return (const QEMUMessageCallback*)this;
    }

private:
    // GCC doesn't support converting lambdas with '...' to function pointers,
    // so these have to be real functions.
    static void outCb(void* opaque, const char* fmt, ...) {
        auto self = (MessageCallback*)opaque;
        if (self->userOut) {
            va_list ap;
            va_start(ap, fmt);
            auto msg = StringFormatWithArgs(fmt, ap);
            self->userOut(self->userOpaque, msg.c_str(), msg.size());
            va_end(ap);
        }
    }

    static void errCb(void* opaque, Error* err, const char* fmt, ...) {
        auto self = (MessageCallback*)opaque;
        if (self->userErr) {
            std::string msg;
            if (fmt) {
                va_list ap;
                va_start(ap, fmt);
                msg = StringFormatWithArgs(fmt, ap);
                va_end(ap);
            }
            if (err) {
                msg += StringView(error_get_pretty(err));
                error_free(err);
            }
            msg += '\n';  // QEMU's error printing always appends this.
            self->userErr(self->userOpaque, msg.c_str(), msg.size());
        }
    }

    void* userOpaque;
    LineConsumerCallback userOut;
    LineConsumerCallback userErr;
};

}  // namespace

static android::snapshot::FailureReason sFailureReason =
    android::snapshot::FailureReason::Empty;
static bool sExiting = false;

static bool qemu_snapshot_list(void* opaque,
                               LineConsumerCallback outConsumer,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;
    return qemu_listvms(nullptr, nullptr,
                        MessageCallback(opaque, outConsumer, errConsumer)) == 0;
}

static bool qemu_snapshot_save(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;
    bool wasVmRunning = runstate_is_running() != 0;
    vm_stop(RUN_STATE_SAVE_VM);

    int res = qemu_savevm(name, MessageCallback(opaque, nullptr, errConsumer));

    if (wasVmRunning &&
        !sExiting) {
        vm_start();
    }

    return res == 0;
}

static bool qemu_snapshot_load(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;

    bool wasVmRunning = runstate_is_running() != 0;

    vm_stop(RUN_STATE_RESTORE_VM);

    int loadVmRes =
        qemu_loadvm(name, MessageCallback(opaque, nullptr, errConsumer));

    bool failed = loadVmRes != 0;

    // loadvm may have failed, but try to restart the current vm anyway, to
    // prevent hanging on generic snapshot load errors such as snapshots not
    // existing.

    if (wasVmRunning) {
        if (failed) {

            std::string failureStr("Snapshot load failure: ");

            failureStr +=
                android::snapshot::failureReasonToString(
                    sFailureReason, SNAPSHOT_LOAD);

            failureStr += "\n";

            if (errConsumer) {
                errConsumer(opaque, failureStr.data(), failureStr.length());
            }

            if (sFailureReason < android::snapshot::FailureReason::ValidationErrorLimit) {
                // load failed, but it is OK to resume VM
                vm_start();
            } else {
                // load failed weirdly, don't resume
                if (errConsumer) {
                    const char* fatalErrStr = "fatal error, VM stopped.\n";
                    errConsumer(opaque, fatalErrStr, strlen(fatalErrStr));
                }
            }
        } else {
            // Normal load, resume
            vm_start();
        }
    }

    return !failed;
}

static bool qemu_snapshot_delete(const char* name,
                                 void* opaque,
                                 LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;
    return qemu_delvm(name, MessageCallback(opaque, nullptr, errConsumer)) == 0;
}

static bool qemu_snapshot_remap(bool shared,
                                void* opaque,
                                LineConsumerCallback errConsumer) {

    android::RecursiveScopedVmLock vmlock;

    // We currently only support remap of the Quickboot snapshot,
    // and only if file-backed.
    //
    // It's good to skip doing anything if the use case
    // is currently out of scope, or it would have no semantic effect
    // (going from auto saving to continuing to auto-save).
    auto currentRamFileStatus = androidSnapshot_getRamFileInfo();

    // If the backing file is not "default_boot" then we cannot use file-backed
    // ram.
    // If it doesn't have a backing file then we will create a default_boot
    // snapshot.
    if (android::snapshot::Snapshotter::get().loadedSnapshotFile() != "" &&
        android::snapshot::Snapshotter::get().loadedSnapshotFile() !=
                "default_boot") {
        return true;
    }

    if (currentRamFileStatus == SNAPSHOT_RAM_FILE_SHARED && shared) {
        return true;
    }

    bool wasVmRunning = runstate_is_running() != 0;

    if (currentRamFileStatus != SNAPSHOT_RAM_FILE_PRIVATE && !shared) {
        vm_stop(RUN_STATE_SAVE_VM);
        android::snapshot::Snapshotter::get().setRemapping(true);
        qemu_savevm("default_boot", MessageCallback(opaque, nullptr, errConsumer));
        android::snapshot::Snapshotter::get().setRemapping(false);
        ram_blocks_remap_shared(shared);
    } else {
        vm_stop(RUN_STATE_RESTORE_VM);
        ram_blocks_remap_shared(shared);
        qemu_loadvm("default_boot", MessageCallback(opaque, nullptr, errConsumer));
    }

    android::snapshot::Snapshotter::get().setRamFileShared(shared);
    androidSnapshot_setRamFileDirty("default_boot", false);

    if (wasVmRunning) {
        vm_start();
    }

    return true;
}

static SnapshotCallbacks sSnapshotCallbacks = {};
static void* sSnapshotCallbacksOpaque = nullptr;

static int onSaveVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_SAVE].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onSaveVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_SAVE].onEnd(sSnapshotCallbacksOpaque, name,
                                                res);
}

static void onSaveVmQuickFail(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_SAVE].onQuickFail(sSnapshotCallbacksOpaque,
                                                      name, res);
}

static bool saveVmQueryCanceled(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_SAVE].isCanceled(
            sSnapshotCallbacksOpaque, name);
}

static int onLoadVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_LOAD].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onLoadVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_LOAD].onEnd(sSnapshotCallbacksOpaque, name,
                                                res);
}

static void onLoadVmQuickFail(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_LOAD].onQuickFail(sSnapshotCallbacksOpaque,
                                                      name, res);
}

static bool loadVmQueryCanceled(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_LOAD].isCanceled(
            sSnapshotCallbacksOpaque, name);
}

static int onDelVmStart(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_DEL].onStart(
            sSnapshotCallbacksOpaque, name);
}

static void onDelVmEnd(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_DEL].onEnd(sSnapshotCallbacksOpaque, name,
                                               res);
}

static void onDelVmQuickFail(const char* name, int res) {
    sSnapshotCallbacks.ops[SNAPSHOT_DEL].onQuickFail(sSnapshotCallbacksOpaque,
                                                     name, res);
}

static bool delVmQueryCanceled(const char* name) {
    return sSnapshotCallbacks.ops[SNAPSHOT_DEL].isCanceled(
            sSnapshotCallbacksOpaque, name);
}

static const QEMUSnapshotCallbacks sQemuSnapshotCallbacks = {
        .savevm = {onSaveVmStart, onSaveVmEnd, onSaveVmQuickFail,
                   saveVmQueryCanceled},
        .loadvm = {onLoadVmStart, onLoadVmEnd, onLoadVmQuickFail,
                   loadVmQueryCanceled},
        .delvm = {onDelVmStart, onDelVmEnd, onDelVmQuickFail,
                  delVmQueryCanceled}};

static const QEMUFileHooks sSaveHooks = {
        // before_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
            if (flags == RAM_CONTROL_SETUP) {
                // Register all blocks for saving.
                qemu_ram_foreach_migrate_block_with_file_info(
                        [](const char* block_name, void* host_addr,
                           ram_addr_t offset, ram_addr_t length,
                           uint32_t flags, const char* path,
                           bool readonly, void* opaque) {
                            auto relativePath =
                                PathUtils::relativeTo(android::snapshot::getSnapshotBaseDir(), path);

                            SnapshotRamBlock block = {
                                block_name, (int64_t)offset,
                                (uint8_t*)host_addr, (int64_t)length,
                                0 /* page size to fill in later */,
                                flags,
                                relativePath,
                                readonly, false /* init need restore to false */
                            };

                            block.pageSize = (int32_t)qemu_ram_pagesize(
                                    qemu_ram_block_by_name(block_name));
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
                    qemu_ram_foreach_migrate_block_with_file_info(
                            [](const char* block_name, void* host_addr,
                               ram_addr_t offset, ram_addr_t length,
                               uint32_t flags, const char* path,
                               bool readonly, void* opaque) {

                                auto block =
                                        static_cast<SnapshotRamBlock*>(opaque);
                                if (strcmp(block->id, block_name) != 0) {
                                    return 0;
                                }

                                // Rebase path as relative to support migration
                                auto relativePath =
                                    PathUtils::relativeTo(android::snapshot::getSnapshotBaseDir(), path);

                                block->startOffset = offset;
                                block->hostPtr = (uint8_t*)host_addr;
                                block->totalSize = length;
                                block->flags = flags;
                                block->path = relativePath;
                                block->readonly = readonly;
                                block->needRestoreFromRamFile = false;
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
        qemu_set_ram_load_callback([](void* hostRam, uint64_t size) {
            sSnapshotCallbacks.ramOps.loadRam(sSnapshotCallbacksOpaque, hostRam,
                                              size);
        });

        switch (android::GetCurrentCpuAccelerator()) {
        case android::CPU_ACCELERATOR_HVF:
            set_address_translation_funcs(hvf_hva2gpa, hvf_gpa2hva);
            set_memory_mapping_funcs(hvf_map_safe, hvf_unmap_safe,
                                     hvf_protect_safe, hvf_remap_safe, NULL);
            break;
        case android::CPU_ACCELERATOR_HAX:
            set_address_translation_funcs(hax_hva2gpa, hax_gpa2hva);
            set_memory_mapping_funcs(NULL, NULL, hax_gpa_protect, NULL,
                                     hax_gpa_protection_supported);
            break;
        default:
            break;
        }

        migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);
    }
}

static void map_user_backed_ram(uint64_t gpa, void* hva, uint64_t size) {
    android::RecursiveScopedVmLock vmlock;

    const bool wasRunning = runstate_is_running() != 0;
    if (wasRunning) { vm_stop(RUN_STATE_SAVE_VM); }

    qemu_user_backed_ram_map(gpa, hva, size, USER_BACKED_RAM_FLAGS_READ | USER_BACKED_RAM_FLAGS_WRITE);

    if (wasRunning) { vm_start(); }
}

static void unmap_user_backed_ram(uint64_t gpa, uint64_t size) {
    android::RecursiveScopedVmLock vmlock;

    const bool wasRunning = runstate_is_running() != 0;
    if (wasRunning) { vm_stop(RUN_STATE_SAVE_VM); }

    qemu_user_backed_ram_unmap(gpa, size);

    if (wasRunning) { vm_start(); }
}

static void get_vm_config(VmConfiguration* out) {
    out->numberOfCpuCores = smp_cpus * smp_cores * smp_threads;
    out->ramSizeBytes = int64_t(ram_size);
    if (whpx_enabled()) {
        out->hypervisorType = HV_WHPX;
    } else if (hax_enabled()) {
        out->hypervisorType = HV_HAXM;
    } else if (hvf_enabled()) {
        out->hypervisorType = HV_HVF;
    } else if (kvm_enabled()) {
        out->hypervisorType = HV_KVM;
    } else if (tcg_enabled()) {
        out->hypervisorType = HV_NONE;
    } else {
        out->hypervisorType = HV_UNKNOWN;
    }
}

static void set_failure_reason(const char* name, int failure_reason) {
    (void)name; // TODO
    sFailureReason = (android::snapshot::FailureReason)failure_reason;
}

static void set_exiting() {
    sExiting = true;
}


static void system_reset_request() {
    qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
}

static void system_shutdown_request() {
    qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
}

static bool vm_pause() {
    qmp_stop(nullptr);
    return true;
}

static bool vm_resume() {
    Error *err = nullptr;
    qmp_cont(&err);
    return err == nullptr;;
}

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = qemu_vm_stop,
        .vmStart = qemu_vm_start,
        .vmReset = system_reset_request,
        .vmShutdown = system_shutdown_request,
        .vmPause = vm_pause,
        .vmResume = vm_resume,
        .vmIsRunning = qemu_vm_is_running,
        .snapshotList = qemu_snapshot_list,
        .snapshotSave = qemu_snapshot_save,
        .snapshotLoad = qemu_snapshot_load,
        .snapshotDelete = qemu_snapshot_delete,
        .snapshotRemap = qemu_snapshot_remap,
        .setSnapshotCallbacks = set_snapshot_callbacks,
        .mapUserBackedRam = map_user_backed_ram,
        .unmapUserBackedRam = unmap_user_backed_ram,
        .getVmConfiguration = get_vm_config,
        .setFailureReason = set_failure_reason,
        .setExiting = set_exiting,
};
const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;
