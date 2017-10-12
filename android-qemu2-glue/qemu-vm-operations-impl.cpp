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

#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/emulation/control/callbacks.h"
#include "android/emulation/control/vm_operations.h"
#include "android/snapshot/MemoryWatch.h"

extern "C" {
#include "qemu/osdep.h"

#include "exec/cpu-common.h"
#include "migration/migration.h"
#include "migration/qemu-file.h"
#include "qapi/error.h"
#include "qapi/qmp/qobject.h"
#include "qapi/qmp/qstring.h"
#include "sysemu/hvf.h"
#include "sysemu/kvm.h"
#include "sysemu/sysemu.h"
#include "sysemu/cpus.h"
#include "exec/cpu-common.h"
}

#include <string>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

static bool qemu_snapshot_list(void* opaque,
                               LineConsumerCallback outConsumer,
                               LineConsumerCallback errConsumer) {
    return qemu_listvms(nullptr, nullptr,
                        MessageCallback(opaque, outConsumer, errConsumer)) == 0;
}

static bool qemu_snapshot_save(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    return qemu_savevm(name, MessageCallback(opaque, nullptr, errConsumer)) ==
           0;
}

static bool qemu_snapshot_load(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    bool wasVmRunning = runstate_is_running() != 0;
    vm_stop(RUN_STATE_RESTORE_VM);
    if (qemu_loadvm(name, MessageCallback(opaque, nullptr, errConsumer)) != 0) {
        return false;
    }
    if (wasVmRunning) {
        vm_start();
    }
    return true;
}

static bool qemu_snapshot_delete(const char* name,
                                 void* opaque,
                                 LineConsumerCallback errConsumer) {
    return qemu_delvm(name, MessageCallback(opaque, nullptr, errConsumer)) == 0;
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

static const QEMUSnapshotCallbacks sQemuSnapshotCallbacks = {
        .savevm = {onSaveVmStart, onSaveVmEnd, onSaveVmQuickFail},
        .loadvm = {onLoadVmStart, onLoadVmEnd, onLoadVmQuickFail},
        .delvm = {onDelVmStart, onDelVmEnd, onDelVmQuickFail}};

static const QEMUFileHooks sSaveHooks = {
        // before_ram_iterate
        [](QEMUFile* f, void* opaque, uint64_t flags, void* data) {
            qemu_put_be64(f, RAM_SAVE_FLAG_HOOK);
            if (flags == RAM_CONTROL_SETUP) {
                // Register all blocks for saving.
                qemu_ram_foreach_block(
                        [](const char* block_name, void* host_addr,
                           ram_addr_t offset, ram_addr_t length, void* opaque) {
                            SnapshotRamBlock block = {
                                    block_name, (int64_t)offset,
                                    (uint8_t*)host_addr, (int64_t)length};
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
                    qemu_ram_foreach_block(
                            [](const char* block_name, void* host_addr,
                               ram_addr_t offset, ram_addr_t length,
                               void* opaque) {
                                auto block =
                                        static_cast<SnapshotRamBlock*>(opaque);
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
        qemu_set_ram_load_callback([](void* hostRam, uint64_t size) {
            sSnapshotCallbacks.ramOps.loadRam(sSnapshotCallbacksOpaque, hostRam,
                                              size);
        });
        set_address_translation_funcs(hvf_hva2gpa, hvf_gpa2hva);
        migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);
    }
}

// These are QEMU's functions to check for each specific hypervisor status.
#ifdef CONFIG_HAX
  extern "C" int hax_enabled(void);
#else
  // Under configurations where CONFIG_HAX is not defined we will not
  // have hax_enabled() defined during link time.
  #define hax_enabled() (0)
#endif

extern "C" int hvf_enabled(void);

static void get_vm_config(VmConfiguration* out) {
    out->numberOfCpuCores = smp_cpus * smp_cores * smp_threads;
    out->ramSizeBytes = int64_t(ram_size);
    if (hax_enabled()) {
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

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = qemu_vm_stop,
        .vmStart = qemu_vm_start,
        .vmReset = qemu_system_reset_request,
        .vmIsRunning = qemu_vm_is_running,
        .snapshotList = qemu_snapshot_list,
        .snapshotSave = qemu_snapshot_save,
        .snapshotLoad = qemu_snapshot_load,
        .snapshotDelete = qemu_snapshot_delete,
        .setSnapshotCallbacks = set_snapshot_callbacks,
        .getVmConfiguration = get_vm_config,
};
const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;
