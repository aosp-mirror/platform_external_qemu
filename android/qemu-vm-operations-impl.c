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

#include "android/qemu-control-impl.h"

#include "cpu.h"
#include "monitor/monitor.h"
#include "sysemu/sysemu.h"

#include <stdlib.h>
#include <string.h>

static bool qemu_vm_stop() {
    vm_stop(EXCP_INTERRUPT);
    return true;
}

static bool qemu_vm_start() {
    vm_start();
    return true;
}

static bool qemu_vm_is_running() {
    return vm_running;
}

static int write_line_cb(char** buf,
                         const char* prefix,
                         const char* payload,
                         int payloadlen) {
    const int buflen = (*buf != NULL) ? strlen(*buf) : 0;
    const int prefixlen = (prefix != NULL) ? strlen(prefix) : 0;
    // Extra space for '\0'
    const int strlen = buflen + prefixlen + payloadlen + 1;
    *buf = realloc(*buf, strlen);

    char* str = *buf + buflen;
    if (prefix != NULL) {
        strncpy(str, prefix, prefixlen);
        str += prefixlen;
    }
    if (payload != NULL) {
        strncpy(str, payload, payloadlen);
        str += payloadlen;
    }
    str[0] = 0;
    // We want to return the lenght of string, so don't count '\0'.
    return (strlen - 1);
}

static int write_out_cb(void* opaque, const char* str, int strsize) {
    char** buf = opaque;
    return write_line_cb(buf, NULL, str, strsize);
}

static int write_err_cb(void* opaque, const char* str, int strsize) {
    static const char ko_prefix[] = "KO: ";
    char** buf = opaque;
    return write_line_cb(buf, ko_prefix, str, strsize);
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

static const QAndroidVmOperations sQAndroidVmOperations = {
    .vmStop = qemu_vm_stop,
    .vmStart = qemu_vm_start,
    .vmIsRunning = qemu_vm_is_running,
    .snapshotList = qemu_snapshot_list,
    .snapshotLoad = qemu_snapshot_load,
    .snapshotSave = qemu_snapshot_save,
    .snapshotDelete = qemu_snapshot_delete
};
const QAndroidVmOperations *gQAndroidVmOperations = &sQAndroidVmOperations;
