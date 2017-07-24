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

#include "android/snapshot/Snapshot.h"

#include "android/snapshot/PathUtils.h"

namespace android {
namespace snapshot {

Snapshot::Snapshot(const char* name)
    : mName(name), mDataDir(getSnapshotDir(name)) {}

bool Snapshot::save() {
    return true;
}

bool Snapshot::load() {
    // TODO: verify the snapshot data and check if it's compatible with the
    // current AVD configuration, host and emulator parameters.
    return true;
}

}  // namespace snapshot
}  // namespace android
