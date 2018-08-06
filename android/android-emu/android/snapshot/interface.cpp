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

#include "android/base/files/PathUtils.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/snapshot/common.h"
#include "android/snapshot/Loader.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshotter.h"

#include "android/utils/debug.h"
#include "android/utils/path.h"

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
    return AndroidSnapshotStatus(Snapshotter::get().loadGeneric(name));
}

const char* androidSnapshot_loadedSnapshotFile() {
    return Snapshotter::get().loadedSnapshotFile().c_str();
}

AndroidSnapshotStatus androidSnapshot_prepareForSaving(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().prepareForSaving(name));
}

AndroidSnapshotStatus androidSnapshot_save(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().saveGeneric(name));
}

void androidSnapshot_cancelSave() {
    Snapshotter::get().cancelSave();
}

void androidSnapshot_delete(const char* name) {
    Snapshotter::get().deleteSnapshot(name);
}

void androidSnapshot_invalidate(const char* name) {
    Snapshotter::get().invalidateSnapshot(name);
}

bool androidSnapshot_areSavesSlow(const char* name) {
    return Snapshotter::get().areSavesSlow(name);
}

int64_t androidSnapshot_lastLoadUptimeMs() {
    return Snapshotter::get().lastLoadUptimeMs();
}

static int sStdoutLineConsumer(void* opaque, const char* buf, int strlen) {
    (void)opaque;
    printf("%s", buf);
    return strlen;
}

static int sStderrLineConsumer(void* opaque, const char* buf, int strlen) {
    (void)opaque;
    fprintf(stderr, "%s", buf);
    return strlen;
}

void androidSnapshot_listStdout() {
    androidSnapshot_list(nullptr,
                         sStdoutLineConsumer,
                         sStderrLineConsumer);
}

void androidSnapshot_list(void* opaque,
                          int (*cbOut)(void*, const char*, int),
                          int (*cbErr)(void*, const char*, int)) {
    Snapshotter::get().listSnapshots(opaque, cbOut, cbErr);
}

void androidSnapshot_setRamFile(const char* path, int shared) {
    Snapshotter::get().setRamFile(path, shared);
}

const char* androidSnapshot_getRamFilePath(const char* _name) {
    const char* name =
        _name ? _name : android::snapshot::kDefaultBootSnapshot;

    std::string dir = android::snapshot::getSnapshotDir(name);
    path_mkdir_if_needed(dir.c_str(), 0744);

    auto mapPath = android::base::PathUtils::join(dir, "ram.img");
    return strdup(mapPath.c_str());
}

AndroidSnapshotRamFileMode androidSnapshot_getRamFileInfo() {
    if (Snapshotter::get().hasRamFile()) {
        if (Snapshotter::get().isRamFileShared()) {
            return SNAPSHOT_RAM_FILE_SHARED;
        } else {
            return SNAPSHOT_RAM_FILE_PRIVATE;
        }
    } else {
        return SNAPSHOT_RAM_FILE_NONE;
    }
}
