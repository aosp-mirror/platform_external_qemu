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

#include "aemu/base/Compiler.h"

#include "aemu/base/async/Looper.h"
#include "android/base/system/System.h"
#include "host-common/vm_operations.h"
#include "snapshot/common.h"

#include <string_view>
namespace android {
namespace snapshot {

class Quickboot {
    DISALLOW_COPY_AND_ASSIGN(Quickboot);

public:
    static Quickboot& get();
    static void initialize(const QAndroidVmOperations& vmOps,
                           const QAndroidEmulatorWindowAgent& window);
    static void finalize();

    ~Quickboot();

    static constexpr const char* kDefaultBootSnapshot = "default_boot";

    bool load(const char* name);
    bool save(std::string_view name);
    void invalidate(std::string_view name);

    bool saveAvdToSystemImageSnapshotsLocalDir();
    void setShortRunCheck(bool enable);

private:
    void decideFailureReport(
            std::string_view name,
            const base::Optional<FailureReason>& failureReason);
    void reportSuccessfulLoad(std::string_view name,
                              base::System::WallDuration startTimeMs);
    void reportSuccessfulSave(std::string_view name,
                              base::System::WallDuration durationMs,
                              base::System::WallDuration sessionUptimeMs);

    void startLivenessMonitor();
    void onLivenessTimer();

    Quickboot(const QAndroidVmOperations& vmOps,
              const QAndroidEmulatorWindowAgent& window);

    const QAndroidVmOperations mVmOps;
    const QAndroidEmulatorWindowAgent mWindow;

    int mAdbConnectionRetries = 0;
    base::System::WallDuration mLoadTimeMs = 0;
    base::System::WallDuration mStartTimeMs =
            base::System::get()->getHighResTimeUs() / 1000;
    bool mLoaded = false;
    std::string mLoadedSnapshotName;
    OperationStatus mLoadStatus = OperationStatus::NotStarted;

    std::unique_ptr<base::Looper::Timer> mLivenessTimer;

    bool mShortRunCheck = true;
};

}  // namespace snapshot
}  // namespace android
