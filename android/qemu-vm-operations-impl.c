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

#include "android/qemu-vm-operations-impl.h"

#include "cpu.h"
#include "monitor/monitor.h"
#include "sysemu/sysemu.h"

#include <stdlib.h>
#include <string.h>

static bool qemu_stop_vm_interrupt() {
    vm_stop(EXCP_INTERRUPT);
    return true;
}

static bool qemu_start_vm_interrupt() {
    vm_start();
    return true;
}

static bool qemu_is_vm_running() {
    return vm_running;
}

static void strncpy_safe(char* dst, const char* src, size_t n) {
    strncpy(dst, src, n);
    dst[n - 1] = '\0';
}

static int write_out_cb(void* opaque, const char* str, int strsize) {
    char** buf = opaque;
    *buf = malloc(strsize + 1);
    strncpy_safe(*buf, str, strsize + 1);
    return strsize;
}

static int write_err_cb(void* opaque, const char* str, int strsize) {
    static const char ko_prefix[] = "KO: ";
    char** buf = opaque;
    *buf = malloc(strlen(ko_prefix) + strsize + 1);
    strncpy(*buf, ko_prefix, strlen(ko_prefix) + 1);
    strncpy(*buf + strlen(ko_prefix), str, strsize + 1);
    return strlen(ko_prefix) + strsize;
}

static bool qemu_snapshot_list(char** const outMessage,
                               char** const errMessage) {
    assert(outMessage != NULL && *outMessage == NULL);
    assert(errMessage != NULL && *errMessage == NULL);
    Monitor* out = monitor_fake_new(outMessage, write_out_cb);
    Monitor* err = monitor_fake_new(errMessage, write_err_cb);
    do_info_snapshots(out, err);
    monitor_fake_free(err);
    monitor_fake_free(out);
    return (*errMessage == NULL);
}

static bool qemu_snapshot_save(const char* name, char** const errMessage) {
    assert(errMessage != NULL && *errMessage == NULL);
    Monitor* err = monitor_fake_new(errMessage, write_err_cb);
    do_savevm(err, name);
    monitor_fake_free(err);
    return (*errMessage == NULL);
}

static bool qemu_snapshot_load(const char* name, char** const errMessage) {
    assert(errMessage != NULL && *errMessage == NULL);
    Monitor* err = monitor_fake_new(errMessage, write_err_cb);
    do_loadvm(err, name);
    monitor_fake_free(err);
    return (*errMessage == NULL);
}

static bool qemu_snapshot_delete(const char* name, char** const errMessage) {
    assert(errMessage != NULL && *errMessage == NULL);
    Monitor* err = monitor_fake_new(errMessage, write_err_cb);
    do_delvm(err, name);
    monitor_fake_free(err);
    return (*errMessage != NULL);
}

QAndroidVMOperations* qemu_get_android_vm_operations() {
    QAndroidVMOperations* operations = malloc(sizeof(*operations));
    if (operations == NULL) {
        return operations;
    }

    operations->stopVMInterrupt = qemu_stop_vm_interrupt;
    operations->startVMInterrupt = qemu_start_vm_interrupt;
    operations->isVMRunning = qemu_is_vm_running;
    operations->snapshotList = qemu_snapshot_list;
    operations->snapshotSave = qemu_snapshot_save;
    operations->snapshotLoad = qemu_snapshot_load;
    operations->snapshotDelete = qemu_snapshot_delete;
    return operations;
}
