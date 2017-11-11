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

#include "android/snapshot/interface.h"

#include "android/snapshot/Snapshotter.h"
#include "android/utils/debug.h"

using android::snapshot::FailureReason;
using android::snapshot::OperationStatus;
using android::snapshot::Snapshotter;

AndroidSnapshotStatus androidSnapshot_prepareForLoading(const char* name) {
    auto res = Snapshotter::get().prepareForLoading(name);
    if (res == OperationStatus::Error) {
        return SNAPSHOT_STATUS_ERROR_NOT_CHANGED;
    } else {
        return AndroidSnapshotStatus(res);
    }
}

AndroidSnapshotStatus androidSnapshot_load(const char* name) {
    auto res = Snapshotter::get().load(false /* is not Quickboot */, name);
    if (res != OperationStatus::Ok) {
        // Check if the error is about something done as just a check or
        // we've started actually loading the VM data
        if (auto failureReason =
                    Snapshotter::get().loader().snapshot().failureReason()) {
            if (*failureReason != FailureReason::Empty &&
                *failureReason < FailureReason::ValidationErrorLimit) {
                dwarning(
                        "Snapshot '%s' can not be loaded (%d), state is "
                        "unchanged.",
                        name, int(*failureReason));
                return SNAPSHOT_STATUS_ERROR_NOT_CHANGED;
            } else {
                derror("Snapshot '%s' can not be loaded (%d), emulator "
                       "stopped.",
                       name, int(*failureReason));
            }
        } else {
            derror("Snapshot '%s' can not be loaded (reason not set), emulator "
                   "stopped.",
                   name);
        }
    }
    return AndroidSnapshotStatus(res);
}

const char* androidSnapshot_loadedSnapshotFile() {
    return Snapshotter::get().loadedSnapshotFile().c_str();
}

AndroidSnapshotStatus androidSnapshot_prepareForSaving(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().prepareForSaving(name));
}

AndroidSnapshotStatus androidSnapshot_save(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().save(name));
}

void androidSnapshot_delete(const char* name) {
    Snapshotter::get().deleteSnapshot(name);
}

int64_t androidSnapshot_lastLoadUptimeMs() {
    return Snapshotter::get().lastLoadUptimeMs();
}
