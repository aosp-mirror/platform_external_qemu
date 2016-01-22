// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/Compiler.h"
#include "android/base/files/IniFile.h"
#include "android/base/String.h"
#include "android/base/StringView.h"
#include "android/base/threads/ParallelTask.h"

#include <functional>

namespace android {
namespace metrics {

// AdbLivenessChecker is an object that runs repeated background checks to make
// sure that adb can talk to our emulator process.
// In case the link fails at some point, a metric is dropped in the usual way.
class AdbLivenessChecker {
public:
    AdbLivenessChecker(android::base::Looper* looper,
                       android::base::IniFile* metricsFile,
                       android::base::StringView emulatorName,
                       android::base::Looper::Duration checkIntervalMs);

    void start();
    void stop();

protected:
    enum class CheckResult {
        kSuccess,
        kFailureAdbServerDead = 1,
        kFailureEmulatorDead = 2
    };
    using AdbCheckTask =
            android::base::ParallelTask<AdbLivenessChecker::CheckResult>;

    // Called by the RecurrentTask.
    bool adbCheckRequest();

    // Called by the ParallelTask.
    void runCheckBlocking(CheckResult* result);
    void reportCheckResult(const CheckResult& result);

private:
    android::base::String mAdbPath;
    android::base::Looper* mLooper;
    android::base::IniFile* mMetricsFile;
    const android::base::String mEmulatorName;
    android::base::Looper::Duration mCheckIntervalMs;
    android::base::RecurrentTask mRecurrentTask;
    std::unique_ptr<AdbCheckTask> mCheckTask;
    int mMaxAttempts;
    int mRemainingAttempts;
    bool mIsOnline = false;

    DISALLOW_COPY_AND_ASSIGN(AdbLivenessChecker);
};

}  // namespace base
}  // namespace android
