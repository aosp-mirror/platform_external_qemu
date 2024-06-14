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
#include <assert.h>                                   // for assert
#include <errno.h>                                    // for errno
#include <stdint.h>                                   // for uint64_t, int64_t

#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif

#include <nlohmann/json.hpp>                          // for basic_json, bas...
#include "android/console.h"
#include "aemu/base/Log.h"                         // for LOG, LogMessage
#include "aemu/base/Optional.h"                    // for Optional
#include "aemu/base/StringFormat.h"                // for StringFormatWit...

#include "aemu/base/files/PathUtils.h"             // for pj, PathUtils
#include "android/base/system/System.h"               // for System
#include "android/emulation/CpuAccelerator.h"         // for GetCurrentCpuAc...
#include "host-common/HostmemIdMapping.h"       // for android_emulati...
#include "host-common/VmLock.h"                 // for RecursiveScoped...
#include "android/emulation/control/callbacks.h"      // for LineConsumerCal...
#include "host-common/vm_operations.h"  // for SnapshotCallbacks
#include "android/snapshot/MemoryWatch.h"             // for set_address_tra...
#include "android/snapshot/PathUtils.h"               // for getSnapshotBaseDir
#include "android/snapshot/Snapshotter.h"             // for Snapshotter
#include "snapshot/common.h"                  // for SnapshotRamBlock
#include "snapshot/interface.h"               // for androidSnapshot...
#include "android/utils/path.h"                       // for path_copy_file
#include "android/utils/debug.h"

extern "C" {

#include "qemu/osdep.h"
#include "block/aio.h"                                // for aio_context_acq...
#include "migration/qemu-file-types.h"                // for qemu_put_be64
#include "qapi/qapi-types-run-state.h"                // for RUN_STATE_SAVE_VM
#include "qemu-common.h"                              // for tcg_enabled
#include "qemu/config-file.h"
#include "qemu/typedefs.h"                            // for QEMUFile, Error

#include "block/block.h"                              // for bdrv_get_aio_co...
#include "exec/cpu-common.h"                          // for qemu_ram_block_...
#include "exec/memory-remap.h"                        // for ram_blocks_rema...
#include "migration/qemu-file.h"                      // for migrate_set_fil...
#include "qapi/error.h"                               // for error_get_pretty
#include "qapi/qapi-commands-block-core.h"            // for qmp_query_block
#include "qapi/qapi-commands-misc.h"                  // for qmp_cont, qmp_stop
#include "qapi/qapi-types-block-core.h"               // for BlockInfoList
#include "qapi/qmp/qdict.h"                           // for qdict_set_defau...
#include "sysemu/block-backend.h"                     // for blk_flush, blk_...
#include "sysemu/cpus.h"                              // for smp_cores, smp_...
#include "sysemu/aehd.h"                               // for aehd_gpa2hva
#include "sysemu/hax.h"                               // for hax_gpa2hva
#include "sysemu/hvf.h"                               // for hvf_enabled
#include "sysemu/kvm.h"                               // for kvm_enabled
#include "sysemu/sysemu.h"                            // for vm_start, vm_stop
#include "sysemu/whpx.h"                              // for whpx_gpa2hva
#include "ui/console.h"
}

// Qemu includes can redefine some windows behavior..
// clang-format off
#include "android/snapshot/Snapshot.h"                // for Snapshot
// clang-format on

// As well as introduce some workarounds we don't want..
#ifdef accept
#undef accept
#endif
#include <stdarg.h>                                   // for va_end, va_list
#include <string.h>                                   // for strcmp, strlen
#include <algorithm>                                  // for find_if
#include <cstdio>                                     // for NULL, rename
#include <string>                                     // for string, operator+
#include <string_view>
#include <vector>                                     // for vector

#define  DD(...)    VERBOSE_PRINT(snapshot,__VA_ARGS__)

using android::base::PathUtils;
using android::base::pj;
using android::base::StringAppendFormatWithArgs;
using android::base::StringFormatWithArgs;
using android::base::System;
using android::snapshot::Snapshot;

using json = nlohmann::json;

static const char* kImportSuffix = "_import.qcow2";
static std::string sLastLoadedSnapshot = "";

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
        return static_cast<const QEMUMessageCallback*>(this);
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
                msg += std::string_view(error_get_pretty(err));
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

static bool qemu_snapshot_last_loaded(void* opaque,
                               LineConsumerCallback outConsumer,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;
    if (sLastLoadedSnapshot == "") {
        const char* kNullSnapshot = "(null)";
        outConsumer(opaque, kNullSnapshot, strlen(kNullSnapshot));
    } else {
        outConsumer(opaque, sLastLoadedSnapshot.c_str(), sLastLoadedSnapshot.length());
    }
    const char* kNewLine = "\r\n";
    outConsumer(opaque, kNewLine, strlen(kNewLine));
    return true;
}

static bool qemu_snapshot_save(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;
    bool wasVmRunning = runstate_is_running() != 0;
    vm_stop(RUN_STATE_SAVE_VM);

    int res = qemu_savevm(name, MessageCallback(opaque, nullptr, errConsumer));

    if (wasVmRunning && !sExiting) {
        vm_start();
    }

    return res == 0;
}

// TODO(jansene): Remove qemu-img dependency and use direct calls instead.
// See static int img_rebase(int argc, char **argv) in qemu-img.c
// |src| should live in the avd dir and must not be an absolute path
// |baseimg| should  live in the avd dir  and must not be an absolute path
static bool rebase_on_top_of(std::string src,
                             std::string baseimg,
                             void* opaque,
                             LineConsumerCallback errConsumer) {
    assert(!qemu_vm_is_running());
    DD("Rebase %s -> %s", src.c_str(), baseimg.c_str());
    auto qemu_img = System::get()->findBundledExecutable("qemu-img");
    auto dst_img = pj(android::snapshot::getAvdDir(), baseimg);
    if (!System::get()->pathExists(dst_img)) {
        // TODO(jansene): replace with internal functions, vs calling qemu-img.
        // See qemu-img.c: static int img_create(int argc, char **argv)
        auto res = android::base::System::get()->runCommandWithResult(
                {qemu_img, "create", "-f", "qcow2", dst_img, "0"});
        if (!res) {
            std::string msg = "Could not create empty qcow2: " + dst_img;
            errConsumer(opaque, msg.c_str(), msg.size());
            return false;
        }
    }

    // TODO(jansene): replace with internal functions, vs calling qemu-img.
    // See qemu-img.c: static int img_rebase(int argc, char **argv)
    auto res = System::get()->runCommandWithResult(
            {qemu_img, "rebase", "-u", "-f", "qcow2", "-b", dst_img, src});

    return res;
}

static bool extract_snapshot(const char* srcImg,
                             const char* dstImg,
                             const char* snapshot,
                             void* opaque,
                             LineConsumerCallback errConsumer) {
    auto qemu_img = System::get()->findBundledExecutable("qemu-img");
    // Extract the snapshot into a standalone qcow2 file.
    auto res = System::get()->runCommandWithResult({qemu_img, "convert", "-O",
                                                    "qcow2", "-s", snapshot,
                                                    srcImg, dstImg});
    if (!res) {
        std::string err =
                std::string("Failed to extract snapshot. Source image name ") +
                srcImg + " destination image name " + dstImg +
                " snapshot name " + snapshot + ".\n";
        DD("extract error %s\n", err.c_str());
        errConsumer(opaque, err.c_str(), err.size());
        return false;
    }
    // Create an empty snapshot in the standalone qcow2 file.
    res = System::get()->runCommandWithResult(
            {qemu_img, "snapshot", "-c", snapshot, dstImg});
    if (!res) {
        std::string err =
                std::string("Failed to create snapshot. Image name ") + dstImg +
                " snapshot name " + snapshot + ".\n";
        DD("extract error %s\n", err.c_str());
        errConsumer(opaque, err.c_str(), err.size());
        path_delete_file(dstImg);
        return false;
    }
    return true;
}

// Swaps out the given blockdriver with a new blockdriver
// backed by the provided qcow2 file.
// |drive| Device to be swapped out
// |qcow2| The new image we are going to insert.
static bool qemu_swap_blockdriver(const json drive,
                                  const char* qcow2,
                                  void* opaque,
                                  LineConsumerCallback errConsumer) {
    assert(!qemu_vm_is_running());
    bool res = false;
    auto device = drive["device"].get<std::string>();
    BlockBackend* blk = blk_by_name(device.c_str());
    if (blk == nullptr)
        return res;

    Error* errp = nullptr;
    BlockDriverState* bs = blk_bs(blk);
    auto oldflags = bdrv_get_flags(bs);

    // Eject
    DD("Ejecting %s", device.c_str());
    AioContext* aioCtx = bdrv_get_aio_context(bs);
    aio_context_acquire(aioCtx);
    blk_flush(blk);
    blk_remove_bs(blk);
    aio_context_release(aioCtx);

    // Inject.
    DD("Injecting %s", qcow2);
    auto dict = qdict_new();
    qdict_put_str(dict, "overlap-check",
                  drive["overlap-check"].get<std::string>().c_str());
    qdict_put_str(dict, "l2-cache-size",
                  drive["l2-cache-size"].get<std::string>().c_str());
    qdict_set_default_str(dict, BDRV_OPT_CACHE_DIRECT, "off");
    qdict_set_default_str(dict, BDRV_OPT_CACHE_NO_FLUSH, "off");
    qdict_set_default_str(dict, BDRV_OPT_READ_ONLY, "off");

    auto replacement = bdrv_open(qcow2, nullptr, dict, oldflags, &errp);
    if (replacement) {
        blk_insert_bs(blk, replacement, &errp);
        bdrv_unref(replacement);
    }

    if (errp) {
        const char* msg = error_get_pretty(errp);
        errConsumer(opaque, msg, strlen(msg));
        return false;
    }

    return true;
}

// Returns a json description of all the mounted qcow2 drives. This is partial
// representation that you would retrieve from using the query_block command
// on the qemuy qmp-shell.
// Notably you will find: {'device' : 'name-of-device', 'backing' :
// 'backing-file' }
static std::vector<json> qcow2_drives() {
    Error* errp = nullptr;
    const char json_str[6] = "json:";
    std::vector<json> drives;
    BlockInfoList* info = qmp_query_block(&errp);
    while (info) {
        if (info->value->has_inserted) {
            char* js = strstr(info->value->inserted->file, json_str);
            if (js) {
                json drive =
                        json::parse(js + sizeof(json_str) - 1, nullptr, false);

                if (!drive.is_discarded() && drive.count("driver") > 0 &&
                    drive["driver"] == "qcow2" && drive.count("file") > 0 &&
                    drive["file"].count("filename") > 0) {
                    drive["device"] = info->value->device;
                    drive["backing"] =
                            info->value->inserted->has_backing_file
                                    ? info->value->inserted->backing_file
                                    : "";
                    DD("Found %s", drive.dump().c_str());
                    drives.push_back(drive);
                }
            }
        }
        info = info->next;
    }
    DD("Found %d drives", drives.size());
    return drives;
}

static void set_skip_snapshot_save(bool used);

static bool import_snapshot(const char* name,
                            void* opaque,
                            LineConsumerCallback errConsumer) {
    assert(!qemu_vm_is_running());
    auto snapshot = Snapshot::getSnapshotById(name);
    if (!snapshot || !snapshot->isImported()) {
        // Ready to roll, this is not an imported snapshot..
        return true;
    }

    bool success = true;
    auto drives = qcow2_drives();

    // Copy and replace the avds from the snapshot directory..
    auto avd = android::snapshot::getAvdDir();
    auto datadir = snapshot->dataDir();
    for (auto qcow2 : android::snapshot::getQcow2Files(datadir.data())) {
        // Check if we have a device mapping this qcow.
        auto it = std::find_if(
                drives.begin(), drives.end(), [qcow2](const json object) {
                    return object["file"]["filename"].get<std::string>().find(
                                   qcow2) != std::string::npos;
                });

        if (it == drives.end()) {
            DD("%s is not mounted, ignoring", qcow2.c_str());
            continue;
        }
        auto drive = *it;

        DD("Copying %s", qcow2.c_str());
        auto dest = pj(avd, qcow2) + kImportSuffix;
        auto src = pj(datadir.data(), qcow2);
        // There is a corner case if we are on an imported snapshot, we might
        // not be able to delete dest. path_delete_file returns 0 on success
        for (int i = 0;
             path_exists(dest.c_str()) && path_delete_file(dest.c_str()) != 0;
             i++) {
            dest = pj(avd, qcow2) + "_" + std::to_string(i) + kImportSuffix;
        }

        // We do not symlink as we do not want to modify the existing snapshot.

        if (path_copy_file(dest.c_str(), src.c_str()) != 0) {
            std::string msg = "Failed to copy " + qcow2 + " to: " + dest +
                              " due to " + std::to_string(errno);
            errConsumer(opaque, msg.c_str(), msg.size());
            success = false;
        };

        auto baseimg = drive["backing"];
        DD("backing file %s\n", ((std::string)baseimg).c_str());
        rebase_on_top_of(dest, baseimg, opaque, errConsumer);

        // Now we are going to replace the block driver associated with it.
        success = qemu_swap_blockdriver(drive, dest.c_str(), opaque,
                                        errConsumer) &&
                  success;
        set_skip_snapshot_save(true);
    }

    return success;
}

static int myStdErrCallback(void* opaque, const char* buff, int len) {
    (void)opaque;
    char mybuf[1024] = {};
    len = std::min(len, (int)(sizeof(mybuf)) - 1);
    memcpy(mybuf, buff, len);
    LOG(WARNING) << mybuf;
    return len;
}

static bool qemu_snapshot_load(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    android::RecursiveScopedVmLock vmlock;

    bool wasVmRunning = runstate_is_running() != 0;

    vm_stop(RUN_STATE_RESTORE_VM);

    Error* errp = nullptr;

    // Okay, so it could be that this is an imported snapshot..
    // An imported snapshot has the qcow2s stored in its own directory.
    // in that case we need to swap out our current qcow2s, and use them
    // for a reload.

    // TODO(jansen): We should make it possible to swap back, as
    // swapping in an imported snapshots invalidates all the existing
    // snapshots, and we cannot load local snapshots anymore as they
    // live in the swapped out snapshot.
    import_snapshot(name, opaque, errConsumer);

    int loadVmRes = qemu_loadvm(
            name,
            MessageCallback(opaque, NULL,
                            errConsumer ? errConsumer : myStdErrCallback));
    DD("Snapshot load vm result %d success %d", loadVmRes, loadVmRes != 0);

    bool failed = loadVmRes != 0;

    // loadvm may have failed, but try to restart the current vm anyway, to
    // prevent hanging on generic snapshot load errors such as snapshots not
    // existing.

    if (wasVmRunning) {
        if (failed) {
            std::string failureStr("Snapshot load failure: ");

            failureStr += android::snapshot::failureReasonToString(
                    sFailureReason, SNAPSHOT_LOAD);

            failureStr += "\n";

            if (errConsumer) {
                errConsumer(opaque, failureStr.data(), failureStr.length());
            }

            if (sFailureReason <
                android::snapshot::FailureReason::ValidationErrorLimit) {
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
            DD("Snapshot restarted vm");
        }
    }

    if (failed) {
        sLastLoadedSnapshot = "";
    } else {
        sLastLoadedSnapshot = name;
        getConsoleAgents()->settings->set_guest_boot_completed(true);
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
        qemu_savevm("default_boot",
                    MessageCallback(opaque, nullptr, errConsumer));
        android::snapshot::Snapshotter::get().setRemapping(false);
        ram_blocks_remap_shared(shared);
    } else {
        vm_stop(RUN_STATE_RESTORE_VM);
        ram_blocks_remap_shared(shared);
        qemu_loadvm("default_boot",
                    MessageCallback(opaque, nullptr, errConsumer));
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
                           ram_addr_t offset, ram_addr_t length, uint32_t flags,
                           const char* path, bool readonly, void* opaque) {
                            auto relativePath = PathUtils::relativeTo(
                                    android::snapshot::getSnapshotBaseDir(),
                                    path ? path : "");

                            SnapshotRamBlock block = {
                                    block_name,
                                    (int64_t)offset,
                                    (uint8_t*)host_addr,
                                    (int64_t)length,
                                    0 /* page size to fill in later */,
                                    flags,
                                    relativePath,
                                    readonly,
                                    false /* init need restore to false */
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
                               uint32_t flags, const char* path, bool readonly,
                               void* opaque) {
                                auto block =
                                        static_cast<SnapshotRamBlock*>(opaque);
                                if (block->id != block_name) {
                                    return 0;
                                }

                                // Rebase path as relative to support migration
                                auto relativePath = PathUtils::relativeTo(
                                        android::snapshot::getSnapshotBaseDir(),
                                        path ? path : "");

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
                            qemu_ram_block_by_name(block.id.data());
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

static void* tcg_gpa2hva(uint64_t gpa, bool *found) {
    void* res = cpu_physical_memory_get_addr(gpa);
    if (res != nullptr) *found = true;
    return res;
}

static void set_snapshot_protobuf(void* pb) {
    qemu_set_snapshot_protobuf(pb);
}

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
                                         hvf_protect_safe, hvf_remap_safe,
                                         NULL);
                break;
            case android::CPU_ACCELERATOR_HAX:
                set_address_translation_funcs(hax_hva2gpa, hax_gpa2hva);
                set_memory_mapping_funcs(NULL, NULL, hax_gpa_protect, NULL,
                                         hax_gpa_protection_supported);
                break;
            case android::CPU_ACCELERATOR_WHPX:
                set_address_translation_funcs(0, whpx_gpa2hva);
                break;
            case android::CPU_ACCELERATOR_AEHD:
                set_address_translation_funcs(aehd_hva2gpa, aehd_gpa2hva);
                break;
            case android::CPU_ACCELERATOR_KVM:
#ifdef __linux__
                set_address_translation_funcs(0, kvm_gpa2hva);
#else
                set_address_translation_funcs(0, tcg_gpa2hva);
#endif
                break;
            default:  // No acceleration
                set_address_translation_funcs(0, tcg_gpa2hva);
                break;
        }

        migrate_set_file_hooks(&sSaveHooks, &sLoadHooks);
    }
}

static void map_user_backed_ram(uint64_t gpa, void* hva, uint64_t size) {
    android::RecursiveScopedVmLock vmlock;
    qemu_user_backed_ram_map(
            gpa, hva, size,
            USER_BACKED_RAM_FLAGS_READ | USER_BACKED_RAM_FLAGS_WRITE);
}

static void unmap_user_backed_ram(uint64_t gpa, uint64_t size) {
    android::RecursiveScopedVmLock vmlock;
    qemu_user_backed_ram_unmap(gpa, size);
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
    (void)name;  // TODO
    sFailureReason = (android::snapshot::FailureReason)failure_reason;
}

static void set_exiting() {
    sExiting = true;
}

static void allow_real_audio(bool allow) {
    qemu_allow_real_audio(allow);
}

static bool is_real_audio_allowed() {
    return qemu_is_real_audio_allowed();
}

static bool need_skip_snapshot_save = false;

static void set_skip_snapshot_save(bool skip) {
    need_skip_snapshot_save = skip;
}

static SnapshotSkipReason skip_snapshot_save_reason = SNAPSHOT_SKIP_UNKNOWN;

static void set_skip_snapshot_save_reason(SnapshotSkipReason reason) {
    skip_snapshot_save_reason = reason;
}

static SnapshotSkipReason get_skip_snapshot_save_reason() {
    return skip_snapshot_save_reason;
}

static bool does_snapshot_use_vulkan = false;

static void set_stat_snasphot_use_vulkan() {
    does_snapshot_use_vulkan = true;
}

static bool snapshot_use_vulkan() {
    return does_snapshot_use_vulkan;
}

static bool is_snapshot_save_skipped() {
    return need_skip_snapshot_save;
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
    Error* err = nullptr;
    qmp_cont(&err);
    return err == nullptr;
    ;
}

static void* physical_memory_get_addr(uint64_t gpa) {
    if (!gpa2hva_call) {
        derror("%s: No gpa2hva!", __func__);
        return nullptr;
    }
    bool found;
    void* res = gpa2hva_call(gpa, &found);
    return found ? res : nullptr;
}

static bool flush_all_drives() {
    bool success = true;
    BlockBackend* blk = NULL;

    while ((blk = blk_all_next(blk)) != NULL) {
        AioContext* aio_context = blk_get_aio_context(blk);
        int ret;
        aio_context_acquire(aio_context);
        if (blk_is_inserted(blk)) {
            ret = blk_flush(blk);
            if (ret < 0) {
                success = false;
                LOG(WARNING) << "Failed to flush: " << blk_name(blk);
            }
        }
        aio_context_release(aio_context);
    }

    return success;
}

bool qemu_snapshot_export_qcow(const char* snapshot,
                               const char* dest,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    // Exporting works like this:
    // 1. Pause the emulator, we don't want any writes to happen
    // 2. Flush all the drives, so we don't have a messed up qcow2 state
    //
    // Then it takes different workflows depending on the image:
    // Workflow A:
    // a.1. convert the snapshot to a standalone qcow2 file
    // a.2. create an empty snapshot in the standalone qcow2 file
    // Workflow B:
    // b.1. Copy all the qcow2 to a temporary
    // b.2. Rebase on top of an empty image, forcing us to pull all relevant
    //      data from the read only sections of the base image (likely nop)
    // b.3. Move the standalone qcows out of the way..
    //
    // At last, we wake up the emulator.
    //
    // TODO(jansene) Qemu supports live migration, so this should be possible
    // without pausing the emulator.

    android::RecursiveScopedVmLock vmlock;
    bool wasVmRunning = runstate_is_running() != 0;
    bool success = true;
    vm_stop(RUN_STATE_SAVE_VM);

    // Next we flush all the drives, this guarantees that the qcow2's are in a
    // consistent state, and we can safely copy them.
    flush_all_drives();

    // Now we copy over the qcow2 overlays..
    auto drives = qcow2_drives();
    static const int kSuffixSize = strlen(kImportSuffix);

    for (auto drive : drives) {
        auto overlay = drive["file"]["filename"].get<std::string>();
        std::string qcow;
        if (!PathUtils::split(overlay.data(), nullptr, &qcow)) {
            success = false;
            std::string err = "Failed to extract basename from " + overlay;
            errConsumer(opaque, err.c_str(), err.size());
            break;
        }
        auto final_overlay = pj(dest, qcow);

        // Remove the suffix if there is any.
        // This would happen if you import then export.
        while (final_overlay.length() > kSuffixSize &&
               0 == strcmp(kImportSuffix, final_overlay.c_str() +
                                                  final_overlay.length() -
                                                  kSuffixSize)) {
            final_overlay.resize(final_overlay.length() - kSuffixSize);
        }
        DD("final overlay name %s\n", final_overlay.c_str());
        path_delete_file(final_overlay.c_str());
        // Extract the target snapshot from qcow2.
        // It doesn't work with cache.img.
        if (std::string_view(qcow).find("cache") == std::string::npos) {
#ifdef _WIN32
            // Windows has file access issues when accessing the same file from
            // multiple processes simultaneously (probably a bad file share option
            // setup). Thus we need to unplug the drive and replug it later.
            std::string id = drive["device"].get<std::string>();
            DD("unplugging %s\n", id.c_str());
            BlockBackend* blk = blk_by_name(id.c_str());

            BlockDriverState* oldbs = blk_bs(blk);
            AioContext* aioCtx = bdrv_get_aio_context(oldbs);
            aio_context_acquire(aioCtx);
            blk_remove_bs(blk);
            aio_context_release(aioCtx);
#endif
            if (!extract_snapshot(overlay.c_str(), final_overlay.c_str(),
                                  snapshot, opaque, errConsumer)) {
                success = false;
            }
#ifdef _WIN32
            // Plug it back.
            QemuOpts* opts = qemu_opts_find(qemu_find_opts("drive"), id.c_str());
            QDict* bs_opts = qdict_new();
            qemu_opts_to_qdict(opts, bs_opts);
            QemuOpts* legacy_opts =
                    qemu_opts_create(&qemu_legacy_drive_opts, nullptr, 0, nullptr);
            qemu_opts_absorb_qdict(legacy_opts, bs_opts, nullptr);
            QemuOpts* drive_opts = qemu_opts_find(&qemu_common_drive_opts, id.c_str());
            if (drive_opts) {
                qemu_opts_del(drive_opts);
                drive_opts = nullptr;
            }
            drive_opts = qemu_opts_create(&qemu_common_drive_opts, id.c_str(), 1, nullptr);
            qemu_opts_absorb_qdict(drive_opts, bs_opts, nullptr);

            qdict_set_default_str(bs_opts, BDRV_OPT_CACHE_DIRECT, "off");
            qdict_set_default_str(bs_opts, BDRV_OPT_CACHE_NO_FLUSH, "off");
            qdict_set_default_str(bs_opts, BDRV_OPT_READ_ONLY, "off");
            qdict_del(bs_opts, "id");

            Error* local_err = NULL;
            BlockDriverState* bs =
                    bdrv_open(overlay.c_str(), nullptr, bs_opts, 0, &local_err);
            if (bs) {
                int res = blk_insert_bs(blk, bs, &local_err);
                bdrv_unref(bs);
            }
            if (local_err) {
                success = false;
                LOG(WARNING) << "drive " << overlay << "open failure: ",
                    error_get_pretty(local_err);
                error_free(local_err);
            }
#endif
            if (!success) {
                break;
            }
        } else {
            // For cache image, we use fallback path to extract all contents.
            auto tmp_overlay =
                    pj(android::snapshot::getAvdDir(), "tmp_" + std::string(qcow));

            // Copy and rebase on an empty image...
            if (path_copy_file(tmp_overlay.c_str(), overlay.c_str()) != 0) {
                success = false;
                std::string err = std::string("Failed to copy ") + overlay +
                                  " to " + tmp_overlay + " due to " +
                                  std::to_string(errno);
                errConsumer(opaque, err.c_str(), err.size());
                break;
            }

            success = rebase_on_top_of(tmp_overlay, "empty.qcow2", opaque,
                                       errConsumer);

            // And move it to the export destination..
            if (success &&
                !android::base::PathUtils::move(tmp_overlay, final_overlay)) {
                std::string err = "Failed to rename " + tmp_overlay + " to " +
                                  final_overlay;

                errConsumer(opaque, err.c_str(), err.size());
                success = false;
            }
            if (!success) {
                path_delete_file(tmp_overlay.c_str());
                break;
            }
        }
    }

    if (wasVmRunning && !sExiting) {
        vm_start();
    }

    return success;
}

static EmuRunState qemu_get_runstate() {
    return (EmuRunState) get_runstate();
};

static bool qemu_set_display(int32_t idx, int32_t w, int32_t h, uint32_t dpi) {
    int found= -1, graphicConsoleIdx = -1;
    for (int i = 0;; i++) {
        QemuConsole* const c = qemu_console_lookup_by_index(i);
        if (!c) {
            break;
        }
        if (qemu_console_is_graphic(c)) {
            graphicConsoleIdx++;    // The first graphic console is fb_dev
            if (graphicConsoleIdx == 1 + idx) {
                found = i;
                break;
            }
        }
    }

    if (found < 0) {
        return false;
    }
    console_select(found);
    QemuConsole* con = qemu_console_lookup_by_index(found);
    QemuUIInfo info = {
        0, 0,
        (uint32_t)w,
        (uint32_t)h,
        dpi,
    };
    dpy_set_ui_info(con, &info);
    return true;
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
        .snapshotExport = qemu_snapshot_export_qcow,
        .snapshotLastLoaded = qemu_snapshot_last_loaded,
        .setSnapshotCallbacks = set_snapshot_callbacks,
        .setSnapshotProtobuf = set_snapshot_protobuf,
        .mapUserBackedRam = map_user_backed_ram,
        .unmapUserBackedRam = unmap_user_backed_ram,
        .getVmConfiguration = get_vm_config,
        .setFailureReason = set_failure_reason,
        .setExiting = set_exiting,
        .allowRealAudio = allow_real_audio,
        .physicalMemoryGetAddr = physical_memory_get_addr,
        .isRealAudioAllowed = is_real_audio_allowed,
        .setSkipSnapshotSave = set_skip_snapshot_save,
        .isSnapshotSaveSkipped = is_snapshot_save_skipped,
        .hostmemRegister = android_emulation_hostmem_register,
        .hostmemUnregister = android_emulation_hostmem_unregister,
        .hostmemGetInfo = android_emulation_hostmem_get_info,
        .getRunState = qemu_get_runstate,
        .setDisplay = qemu_set_display,
        .system_shutdown_request =
                [](QemuShutdownCause c) {
                    qemu_system_shutdown_request((ShutdownCause)c);
                },
        .setSkipSnapshotSaveReason = set_skip_snapshot_save_reason,
        .getSkipSnapshotSaveReason = get_skip_snapshot_save_reason,
        .setStatSnapshotUseVulkan = set_stat_snasphot_use_vulkan,
        .snapshotUseVulkan = snapshot_use_vulkan,
};

extern "C" const QAndroidVmOperations* const gQAndroidVmOperations =
        &sQAndroidVmOperations;
