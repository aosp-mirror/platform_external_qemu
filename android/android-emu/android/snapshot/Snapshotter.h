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

#include "android/base/Compiler.h"
#include "android/base/Optional.h"
#include "android/base/system/System.h"
#include "android/snapshot/Loader.h"
#include "android/snapshot/Saver.h"
#include "android/snapshot/common.h"

#include "android/emulation/control/vm_operations.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace android {
namespace snapshot {

class Snapshotter final {
    DISALLOW_COPY_AND_ASSIGN(Snapshotter);

public:
    Snapshotter(const QAndroidVmOperations& vmOperations);
    ~Snapshotter();

    static Snapshotter& get();

    OperationStatus prepareForLoading(const char* name);
    OperationStatus load(const char* name);
    OperationStatus prepareForSaving(const char* name);
    OperationStatus save(const char* name);
    void deleteSnapshot(const char* name);

    base::System::Duration lastLoadUptimeMs() const {
        return mLastLoadUptimeMs;
    }

    Saver& saver() { return *mSaver; }
    Loader& loader() { return *mLoader; }

    const QAndroidVmOperations& vmOperations() const { return mVmOperations; }

private:
    static const SnapshotCallbacks kCallbacks;

    bool onStartSaving(const char* name);
    bool onSavingComplete(const char* name, int res);
    bool onStartLoading(const char* name);
    bool onLoadingComplete(const char* name, int res);
    bool onStartDelete(const char* name);
    bool onDeletingComplete(const char* name, int res);

    QAndroidVmOperations mVmOperations;
    android::base::Optional<Saver> mSaver;
    android::base::Optional<Loader> mLoader;

    base::System::Duration mLastLoadUptimeMs = 0;
};

}  // namespace snapshot
}  // namespace android
