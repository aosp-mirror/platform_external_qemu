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

#include "android/metrics/AdbLivenessChecker.h"
#include "android/snapshot/Snapshotter.h"

using android::metrics::AdbLivenessChecker;
using android::snapshot::Snapshotter;

AndroidSnapshotStatus androidSnapshot_prepareForLoading(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().prepareForLoading(name));
}

AndroidSnapshotStatus androidSnapshot_load(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().load(name));
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

bool androidSnapshot_canSave() {
    return AdbLivenessChecker::isEmulatorBooted();
}

static bool sWantLoadBootSnapshot = true;
static bool sWantSaveBootSnapshot = true;

void androidSnapshot_setWantLoadBootSnapshot(bool enable) {
    sWantLoadBootSnapshot = enable;
}

void androidSnapshot_setWantSaveBootSnapshot(bool enable) {
    sWantSaveBootSnapshot = enable;
}

bool androidSnapshot_wantLoadBootSnapshot() {
    return sWantLoadBootSnapshot;
}
bool androidSnapshot_wantSaveBootSnapshot() {
    return sWantSaveBootSnapshot;
}
