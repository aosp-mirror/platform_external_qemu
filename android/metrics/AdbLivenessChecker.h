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

#include <functional>
#include <memory>

namespace android {
namespace metrics {

// AdbLivenessChecker is an object that runs repeated background checks to make
// sure that adb can talk to our emulator process.
// In case the link fails at some point, a metric is dropped in the usual way.
class AdbLivenessChecker
        : public std::enable_shared_from_this<AdbLivenessChecker> {
public:
    // Entry point to create an AdbLivenessChecker.
    // Objects of this type are managed via shared_ptr.
    static std::shared_ptr<AdbLivenessChecker> create(
            android::base::Looper* looper,
            android::base::IniFile* metricsFile,
            android::base::StringView emulatorName,
            android::base::Looper::Duration checkIntervalMs);

    void start();
    void stop();

protected:
    // Use |create| to correctly initialize the shared_ptr count.
    AdbLivenessChecker(android::base::Looper* looper,
                       android::base::IniFile* metricsFile,
                       android::base::StringView emulatorName,
                       android::base::Looper::Duration checkIntervalMs);

    enum class CheckResult {
        kNoResult = 0,
        kFailureNoAdb = 1,
        kOnline = 2,
        kFailureAdbServerDead = 3,
        kFailureEmulatorDead = 4
    };

    // Called by the RecurrentTask.
    bool adbCheckRequest();

    // Called by the ParallelTask.
    void runCheckBlocking(CheckResult* result) const;
    void reportCheckResult(const CheckResult& result);

    void dropMetrics(const CheckResult& result);
private:
    android::base::Looper* const mLooper;
    android::base::IniFile* const mMetricsFile;
    const android::base::String mEmulatorName;
    const android::base::Looper::Duration mCheckIntervalMs;
    const android::base::String mAdbPath;
    android::base::RecurrentTask mRecurrentTask;
    int mRemainingAttempts;
    bool mIsOnline = false;
    bool mIsCheckRunning = false;

    DISALLOW_COPY_AND_ASSIGN(AdbLivenessChecker);
};

}  // namespace base
}  // namespace android
