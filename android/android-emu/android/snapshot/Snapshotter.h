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
#include "android/snapshot/common.h"

#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

// Fwd-declare the protobuf class
namespace android_studio {
class AndroidStudioEvent;
} // namespace android_studio

namespace android {
namespace snapshot {

class Saver;
class Loader;

class Snapshotter final {
    DISALLOW_COPY_AND_ASSIGN(Snapshotter);

public:
    Snapshotter();
    ~Snapshotter();
    static Snapshotter& get();

    struct SnapshotOperationStats {
        bool forSave;
        std::string name;
        base::System::Duration durationMs;
        bool onDemandRamEnabled;
        bool incrementallySaved;
        bool compressedRam;
        bool compressedTextures;
        base::System::MemUsage memUsage;
        bool usingHDD;
        int64_t diskSize;
        int64_t ramSize;
        int64_t texturesSize;
        base::System::Duration ramDurationMs;
        base::System::Duration texturesDurationMs;
    };

    static void fillSnapshotMetrics(
        android_studio::AndroidStudioEvent* event,
        const SnapshotOperationStats& stats);
    SnapshotOperationStats getLoadStats(const char* name, base::System::Duration durationMs);
    SnapshotOperationStats getSaveStats(const char* name, base::System::Duration durationMs);

    void initialize(const QAndroidVmOperations& vmOperations,
                    const QAndroidEmulatorWindowAgent& windowAgent);

    OperationStatus prepareForLoading(const char* name);
    OperationStatus load(bool isQuickboot, const char* name);
    OperationStatus prepareForSaving(const char* name);
    OperationStatus save(bool isOnExit, const char* name);

    void setRamFile(const char* path, bool shared);
    void setRamFileShared(bool shared);

    // Cancels the current save operation, and queries
    // whether saving was canceled.
    void cancelSave();
    bool isSavingCanceled(const char* name) const;

    // Generic snapshot functions. These differ from normal
    // save() / load() in that they perform validation and
    // metrics reporting, and are meant to be used as part of
    // the interface androidSnapshot_save/load (and any
    // generic snapshots UI).
    OperationStatus saveGeneric(const char* name);
    OperationStatus loadGeneric(const char* name);

    void deleteSnapshot(const char* name);
    void invalidateSnapshot(const char* name);
    bool areSavesSlow(const char* name);
    void listSnapshots(void* opaque,
                       int (*cbOut)(void* opaque, const char* buf, int strlen),
                       int (*cbErr)(void* opaque, const char* buf, int strlen));

    void onCrashedSnapshot(const char* name);

    // Returns an empty string if the AVD was Cold Booted
    const std::string& loadedSnapshotFile() { return mLoadedSnapshotFile; }

    base::System::Duration lastLoadUptimeMs() const {
        return mLastLoadUptimeMs;
    }

    base::System::Duration lastSaveUptimeMs() const {
        return mLastSaveUptimeMs;
    }

    Saver& saver() { return *mSaver; }
    Loader& loader() { return *mLoader; }
    bool hasLoader() const { return mLoader ? true : false ; }

    const QAndroidVmOperations& vmOperations() const { return mVmOperations; }
    const QAndroidEmulatorWindowAgent& windowAgent() const {
        return mWindowAgent;
    }

    // Add a callback that's called on the specific stages of the snapshot
    // operations.
    enum class Operation { Save, Load };
    enum class Stage { Start, End };
    using Callback = std::function<void(Operation, Stage)>;
    void addOperationCallback(Callback&& cb);

    bool isQuickboot() const { return mIsQuickboot; }
    bool hasRamFile() const { return !mRamFile.empty(); }
    bool isRamFileShared() const { return !mRamFile.empty() && mRamFileShared; }
    void setRemapping(bool remapping) { mIsRemapping = remapping; }

    void setUsingHdd(bool usingHdd) { mUsingHdd = usingHdd; }
    bool isUsingHdd() const { return mUsingHdd; }

private:
    bool onStartSaving(const char* name);
    bool onSavingComplete(const char* name, int res);
    void onSavingFailed(const char* name, int res);
    bool onStartLoading(const char* name);
    bool onLoadingComplete(const char* name, int res);
    void onLoadingFailed(const char* name, int res);
    bool onStartDelete(const char* name);
    bool onDeletingComplete(const char* name, int res);

    void finishLoading();

    void prepareLoaderForSaving(const char* name);
    void callCallbacks(Operation op, Stage stage);

    void appendSuccessfulSave(const char* name,
                              base::System::Duration durationMs);
    void appendSuccessfulLoad(const char* name,
                              base::System::Duration durationMs);
    void showError(const std::string& msg);
    bool checkSafeToSave(const char* name, bool reportMetrics = true);
    void handleGenericSave(const char* name, OperationStatus saveStatus, bool reportMetrics = true);
    bool checkSafeToLoad(const char* name, bool reportMetrics = true);
    void handleGenericLoad(const char* name, OperationStatus loadStatus, bool reportMetrics = true);

    QAndroidVmOperations mVmOperations;
    QAndroidEmulatorWindowAgent mWindowAgent;
    std::unique_ptr<Saver> mSaver;
    std::unique_ptr<Loader> mLoader;
    std::vector<Callback> mCallbacks;
    std::string mLoadedSnapshotFile;

    base::System::Duration mLastSaveUptimeMs = 0;
    base::System::Duration mLastLoadUptimeMs = 0;
    android::base::Optional<base::System::Duration> mLastSaveDuration = 0;
    android::base::Optional<base::System::Duration> mLastLoadDuration = 0;

    bool mIsQuickboot = false;
    bool mIsOnExit = false;
    bool mIsInvalidating = false;
    bool mIsRemapping = false;

    std::string mRamFile;
    bool mRamFileShared = false;
    bool mUsingHdd = false;
};

}  // namespace snapshot
}  // namespace android
