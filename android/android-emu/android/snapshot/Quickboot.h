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
#include "android/base/StringView.h"
#include "android/base/async/Looper.h"
#include "android/base/system/System.h"
#include "android/emulation/control/vm_operations.h"
#include "android/snapshot/common.h"

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

    bool load(base::StringView name);
    bool save(base::StringView name);
    void invalidate(base::StringView name);

private:
    void decideFailureReport(base::StringView name,
                             const base::Optional<FailureReason>& failureReason);
    void reportSuccessfulLoad(base::StringView name,
                              base::System::WallDuration startTimeMs);
    void reportSuccessfulSave(base::StringView name,
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
};

}  // namespace snapshot
}  // namespace android
