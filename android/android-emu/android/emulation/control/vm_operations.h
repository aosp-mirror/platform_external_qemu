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

#pragma once

#include "android/emulation/control/callbacks.h"
#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef struct {
    int (*onStart)(void* opaque, const char* name);
    void (*onEnd)(void* opaque, const char* name, int res);
    void (*onQuickFail)(void* opaque, const char* name, int res);
    bool (*isCanceled)(void* opaque, const char* name);
} SnapshotCallbackSet;

typedef enum {
    SNAPSHOT_SAVE,
    SNAPSHOT_LOAD,
    SNAPSHOT_DEL,
    SNAPSHOT_OPS_COUNT
} SnapshotOperation;

struct SnapshotRamBlock;

typedef struct {
    void (*registerBlock)(void* opaque,
                          SnapshotOperation operation,
                          const struct SnapshotRamBlock* block);
    int (*startLoading)(void* opaque);
    void (*savePage)(void* opaque,
                     int64_t blockOffset,
                     int64_t pageOffset,
                     int32_t size);
    int (*savingComplete)(void* opaque);
    void (*loadRam)(void* opaque,
                    void* hostRamPtr,
                    uint64_t size);
} SnapshotRamCallbacks;

typedef struct {
    SnapshotCallbackSet ops[SNAPSHOT_OPS_COUNT];
    SnapshotRamCallbacks ramOps;
} SnapshotCallbacks;

typedef enum {
    HV_UNKNOWN,
    HV_NONE,
    HV_KVM,
    HV_HAXM,
    HV_HVF,
    HV_WHPX,
} VmHypervisorType;

typedef struct {
    VmHypervisorType hypervisorType;
    int32_t numberOfCpuCores;
    int64_t ramSizeBytes;
} VmConfiguration;

// C interface to expose Qemu implementations of common VM related operations.
typedef struct QAndroidVmOperations {
    bool (*vmStop)(void);
    bool (*vmStart)(void);
    void (*vmReset)(void);
    void (*vmShutdown)(void);

    bool (*vmIsRunning)(void);

    // Snapshot-related VM operations.
    // |outConsuer| and |errConsumer| are used to report output / error
    // respectively. Each line of output is newline terminated and results in
    // one call to the callback. |opaque| is passed to the callbacks as context.
    // Returns true on success, false on failure.
    bool (*snapshotList)(void* opaque,
                         LineConsumerCallback outConsumer,
                         LineConsumerCallback errConsumer);
    bool (*snapshotSave)(const char* name,
                         void* opaque,
                         LineConsumerCallback errConsumer);
    bool (*snapshotLoad)(const char* name,
                         void* opaque,
                         LineConsumerCallback errConsumer);
    bool (*snapshotDelete)(const char* name,
                           void* opaque,
                           LineConsumerCallback errConsumer);

    // Sets a set of callback to listen for snapshot operations.
    void (*setSnapshotCallbacks)(void* opaque,
                                 const SnapshotCallbacks* callbacks);

    // Fills in the supplied |out| with current VM configuration.
    void (*getVmConfiguration)(VmConfiguration* out);

    // Notifies QEMU of failed operations according to our own
    // android::snapshot::FailureReason.
    void (*setFailureReason)(const char* name, int failureReason);

    // Notifies QEMU that the emulator is exiting, can impact how
    // QEMU snapshot save calls work.
    void (*setExiting)(void);

} QAndroidVmOperations;

ANDROID_END_HEADER
