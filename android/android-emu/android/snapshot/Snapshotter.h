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
#include "android/emulation/control/window_agent.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace android {
namespace snapshot {

class Snapshotter final {
    DISALLOW_COPY_AND_ASSIGN(Snapshotter);

public:
    Snapshotter();
    ~Snapshotter();
    static Snapshotter& get();

    void initialize(const QAndroidVmOperations& vmOperations,
                    const QAndroidEmulatorWindowAgent& windowAgent);

    OperationStatus prepareForLoading(const char* name);
    OperationStatus load(bool isQuickboot, const char* name);
    OperationStatus prepareForSaving(const char* name);
    OperationStatus save(const char* name);
    void deleteSnapshot(const char* name);
    void onCrashedSnapshot(const char* name);

    // Returns an empty string if the AVD was Cold Booted
    const std::string& loadedSnapshotFile() { return mLoadedSnapshotFile; }

    base::System::Duration lastLoadUptimeMs() const {
        return mLastLoadUptimeMs;
    }

    Saver& saver() { return *mSaver; }
    Loader& loader() { return *mLoader; }

    const QAndroidVmOperations& vmOperations() const { return mVmOperations; }
    const QAndroidEmulatorWindowAgent& windowAgent() const { return mWindowAgent; }

    // Add a callback that's called on the specific stages of the snapshot
    // operations.
    enum class Operation { Save, Load };
    enum class Stage { Start, End };
    using Callback = std::function<void(Operation, Stage)>;
    void addOperationCallback(Callback&& cb);

    bool isQuickboot() const {
        return mIsQuickboot;
    }

private:
    bool onStartSaving(const char* name);
    bool onSavingComplete(const char* name, int res);
    void onSavingFailed(const char* name, int res);
    bool onStartLoading(const char* name);
    bool onLoadingComplete(const char* name, int res);
    void onLoadingFailed(const char* name, int res);
    bool onStartDelete(const char* name);
    bool onDeletingComplete(const char* name, int res);

    void callCallbacks(Operation op, Stage stage);

    QAndroidVmOperations mVmOperations;
    QAndroidEmulatorWindowAgent mWindowAgent;
    android::base::Optional<Saver> mSaver;
    android::base::Optional<Loader> mLoader;
    std::vector<Callback> mCallbacks;
    std::string mLoadedSnapshotFile;

    base::System::Duration mLastLoadUptimeMs = 0;

    bool mIsQuickboot = false;
};

}  // namespace snapshot
}  // namespace android
