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

// Enumeration of various causes for shutdown. Please keep this in sync
// with the similar enum in include/sysem/sysemu.h.
typedef enum QemuShutdownCause {
    QEMU_SHUTDOWN_CAUSE_NONE,              /* No shutdown request pending */
    QEMU_SHUTDOWN_CAUSE_HOST_ERROR,        /* An error prevents further use of guest */
    QEMU_SHUTDOWN_CAUSE_HOST_QMP,          /* Reaction to a QMP command, like 'quit' */
    QEMU_SHUTDOWN_CAUSE_HOST_SIGNAL,       /* Reaction to a signal, such as SIGINT */
    QEMU_SHUTDOWN_CAUSE_HOST_UI,           /* Reaction to UI event, like window close */
    QEMU_SHUTDOWN_CAUSE_GUEST_SHUTDOWN,    /* Guest shutdown/suspend request, via
                                              ACPI or other hardware-specific means */
    QEMU_SHUTDOWN_CAUSE_GUEST_RESET,       /* Guest reset request, and command line
                                              turns that into a shutdown */
    QEMU_SHUTDOWN_CAUSE_GUEST_PANIC,       /* Guest panicked, and command line turns
                                              that into a shutdown */
    QEMU_SHUTDOWN_CAUSE__MAX,
} QemuShutdownCause;

#define SHUTDOWN_CAUSE_STATIC_ASSERT_ERROR_MESSAGE "QEMU shutdown cause values differ from AndroidEmu's!" \

#define SHUTDOWN_CAUSE_STATIC_MATCH(origCause) \
    static_assert((int)(QEMU_##origCause) == (int)(origCause), SHUTDOWN_CAUSE_STATIC_ASSERT_ERROR_MESSAGE);

#define STATIC_ASSERT_SHUTDOWN_CAUSE_MATCHES \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_NONE) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_HOST_ERROR) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_HOST_QMP) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_HOST_SIGNAL) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_HOST_UI) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_GUEST_SHUTDOWN) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_GUEST_RESET) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE_GUEST_PANIC) \
    SHUTDOWN_CAUSE_STATIC_MATCH(SHUTDOWN_CAUSE__MAX) \

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
    bool (*vmPause)(void);
    bool (*vmResume)(void);

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
    bool (*snapshotRemap)(bool shared,
                          void* opaque,
                          LineConsumerCallback errConsumer);

    // Sets a set of callback to listen for snapshot operations.
    void (*setSnapshotCallbacks)(void* opaque,
                                 const SnapshotCallbacks* callbacks);

    // callbacks to "plug" and "unplug" memory into the provided address range
    // on fly.
    void (*mapUserBackedRam)(uint64_t gpa, void* hva, uint64_t size);
    void (*unmapUserBackedRam)(uint64_t gpa, uint64_t size);

    // Fills in the supplied |out| with current VM configuration.
    void (*getVmConfiguration)(VmConfiguration* out);

    // Notifies QEMU of failed operations according to our own
    // android::snapshot::FailureReason.
    void (*setFailureReason)(const char* name, int failureReason);

    // Notifies QEMU that the emulator is exiting, can impact how
    // QEMU snapshot save calls work.
    void (*setExiting)(void);

    // Allow actual audio on host through to the guest.
    void (*allowRealAudio)(bool allow);

    // Get the host address of a guest physical address, if any.
    void* (*physicalMemoryGetAddr)(uint64_t gpa);

    // Query whether host audio is allowed.
    bool (*isRealAudioAllowed)(void);

    // Set whether to skip snapshotting on exit.
    void (*setSkipSnapshotSave)(bool used);

    // Retrieve the state of whether snapshotting is skipped.
    bool (*isSnapshotSaveSkipped)(void);

} QAndroidVmOperations;

// gQAndroidVmOperations is defined in .cpp depending on the target it used for,
// e.g. QEMU, a host or tests.
extern const QAndroidVmOperations* const gQAndroidVmOperations;

ANDROID_END_HEADER
