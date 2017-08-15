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

ANDROID_BEGIN_HEADER

typedef enum {
    SNAPSHOT_STATUS_NOT_STARTED,
    SNAPSHOT_STATUS_OK,
    SNAPSHOT_STATUS_ERROR,
    SNAPSHOT_STATUS_ERROR_NOT_CHANGED,
} AndroidSnapshotStatus;

void androidSnapshot_initialize(const QAndroidVmOperations* vmOperations);
void androidSnapshot_finalize();

AndroidSnapshotStatus androidSnapshot_prepareForLoading(const char* name);
AndroidSnapshotStatus androidSnapshot_load(const char* name);

int64_t androidSnapshot_lastLoadUptimeMs();

AndroidSnapshotStatus androidSnapshot_prepareForSaving(const char* name);
AndroidSnapshotStatus androidSnapshot_save(const char* name);

void androidSnapshot_delete(const char* name);

// Makes sure the emulator is in a state when snapshot can be saved, e.g.
// all internals are initialized etc.
bool androidSnapshot_canSave();

ANDROID_END_HEADER
