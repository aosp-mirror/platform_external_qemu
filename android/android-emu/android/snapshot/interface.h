// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"

ANDROID_BEGIN_HEADER

typedef enum {
    SNAPSHOT_STATUS_NOT_STARTED,
    SNAPSHOT_STATUS_OK,
    SNAPSHOT_STATUS_ERROR,
    SNAPSHOT_STATUS_ERROR_NOT_CHANGED,
} AndroidSnapshotStatus;

void androidSnapshot_initialize(const QAndroidVmOperations* vmOperations,
                                const QAndroidEmulatorWindowAgent* windowAgent);
void androidSnapshot_finalize();

AndroidSnapshotStatus androidSnapshot_prepareForLoading(const char* name);
AndroidSnapshotStatus androidSnapshot_load(const char* name);

int64_t androidSnapshot_lastLoadUptimeMs();

AndroidSnapshotStatus androidSnapshot_prepareForSaving(const char* name);
AndroidSnapshotStatus androidSnapshot_save(const char* name);

void androidSnapshot_delete(const char* name);

// Returns the name of the snapshot file that was loaded to start
// the current image.
// Returns an empty string if the AVD was cold-booted.
const char* androidSnapshot_loadedSnapshotFile();

// Some hypervisors do not support generic AND quickboot save.
void androidSnapshot_setQuickbootSaveNoGood();
bool androidSnapshot_isQuickbootSaveNoGood();

// These two functions implement a quickboot feature: load() tries to load from
// the |name| snapshot (or default one if it is null or empty) and save() saves
// the current state into it.
// Return value is |true| if the feature is enabled,
// and the function has at least tried to do something, |false| otherwise.
bool androidSnapshot_quickbootLoad(const char* name);
bool androidSnapshot_quickbootSave(const char* name);

// For when we want to skip quickboot AND skip loading it next time, such as
// in the case of a power-off and restart.
void androidSnapshot_quickbootInvalidate(const char* name);

// List snapshots to stdout.
void androidSnapshot_listStdout();

// List snapshots with a custom callback for consuming the lines.
void androidSnapshot_list(void* opaque,
                          int (*cbOut)(void* opaque, const char* buf, int strlen),
                          int (*cbErr)(void* opaque, const char* buf, int strlen));

ANDROID_END_HEADER
