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
//#include "cpu.h"
//#include "monitor/monitor.h"
#include "sysemu/sysemu.h"

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
    //Monitor* err = monitor_fake_new(opaque, errConsumer);
    //do_savevm(err, name);
    //int ret = monitor_fake_get_bytes(err);
    //monitor_fake_free(err);
    //return !ret;
    return false;
}

static bool qemu_snapshot_load(const char* name,
                               void* opaque,
                               LineConsumerCallback errConsumer) {
    //Monitor* err = monitor_fake_new(opaque, errConsumer);
    //do_loadvm(err, name);
    //int ret = monitor_fake_get_bytes(err);
    //monitor_fake_free(err);
    //return !ret;
    return false;
}

static bool qemu_snapshot_delete(const char* name,
                                 void* opaque,
                                 LineConsumerCallback errConsumer) {
    //Monitor* err = monitor_fake_new(opaque, errConsumer);
    //do_delvm(err, name);
    //int ret = monitor_fake_get_bytes(err);
    //monitor_fake_free(err);
    //return !ret;
    return false;
}

static const QAndroidVmOperations sQAndroidVmOperations = {
    .vmStop = qemu_vm_stop,
    .vmStart = qemu_vm_start,
    .vmIsRunning = qemu_vm_is_running,
    .snapshotList = qemu_snapshot_list,
    .snapshotLoad = qemu_snapshot_load,
    .snapshotSave = qemu_snapshot_save,
    .snapshotDelete = qemu_snapshot_delete
};
const QAndroidVmOperations * const gQAndroidVmOperations = &sQAndroidVmOperations;
