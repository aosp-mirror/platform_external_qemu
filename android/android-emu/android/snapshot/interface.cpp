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

using android::snapshot::Snapshotter;

void androidSnapshot_prepareForLoading(const char* name) {
    Snapshotter::get().prepareForLoading(name);
}

AndroidSnapshotStatus androidSnapshot_load(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().load(name));
}

void androidSnapshot_prepareForSaving(const char* name) {
    Snapshotter::get().prepareForSaving(name);
}

AndroidSnapshotStatus androidSnapshot_save(const char* name) {
    return AndroidSnapshotStatus(Snapshotter::get().save(name));
}
