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

#include "android/metrics/AdbLivenessChecker.h"

#include "android/base/system/System.h"
#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/emulation/ConfigDirs.h"

namespace android {
namespace metrics {

using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::StringVector;
using android::base::String;
using android::base::System;
using android::ConfigDirs;

static const char kAdbExecutableBaseName[] = "adb";
static const char kAdbLivenessKey[] = "adb_liveness";
static const int kMaxAttempts = 3;
static const char kPlatformToolsSubdir[] = "platform-tools";

AdbLivenessChecker::AdbLivenessChecker(
        android::base::Looper* looper,
        android::base::IniFile* metricsFile,
        android::base::StringView emulatorName,
        android::base::Looper::Duration checkIntervalMs)
    : mLooper(looper),
      mMetricsFile(metricsFile),
      mEmulatorName(emulatorName),
      mCheckIntervalMs(checkIntervalMs),
      mRecurrentTask(looper,
                     std::bind(&AdbLivenessChecker::adbCheckRequest, this),
                     checkIntervalMs),
      mRemainingAttempts(kMaxAttempts),
      mAdbPath(PathUtils::join(
              ConfigDirs::getSdkRootDirectory(),
              PathUtils::join(
                      kPlatformToolsSubdir,
                      PathUtils::toExecutableName(kAdbExecutableBaseName)))) {
    dropMetrics(CheckResult::kNoResult);
}

void AdbLivenessChecker::start() {
    mRecurrentTask.start();
}

void AdbLivenessChecker::stop() {
    mRecurrentTask.stop();
}

bool AdbLivenessChecker::adbCheckRequest() {
    if (!mRemainingAttempts) {
        LOG(VERBOSE) << "Quitting adb liveness check";
        return false;
    }

    if (!mCheckTask || !mCheckTask->inFlight()) {
        auto taskFunction = [this] (CheckResult* outResult) {
            this->runCheckBlocking(outResult);
        };
        auto taskDoneFunction = [this] (const CheckResult& result) {
            this->reportCheckResult(result);
        };
        mCheckTask.reset(
                new AdbCheckTask(mLooper, taskFunction, taskDoneFunction));
        mCheckTask->start();
    }
    return true;
}

void AdbLivenessChecker::runCheckBlocking(CheckResult* outResult) const {
    System::ProcessExitCode exitCode;
    const StringVector adbServerAliveCmd = {mAdbPath, "devices"};
    if (!System::get()->runCommand(
                adbServerAliveCmd,
                System::RunOptions::WaitForCompletion |
                System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureAdbServerDead;
        return;
    }

    const StringVector emulatorAliveCmd = {mAdbPath, "-s", mEmulatorName,
        "shell", "exit"};
    if (!System::get()->runCommand(
                emulatorAliveCmd,
                System::RunOptions::WaitForCompletion |
                System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureEmulatorDead;
        return;
    }

    *outResult = AdbLivenessChecker::CheckResult::kOnline;
}

void AdbLivenessChecker::reportCheckResult(const CheckResult& result) {
    // |mIsOnline| starts off set to false. We set it to true if we successfully
    // talk to the emulator at least once. This allows us to distinguish the
    // cases where adb is missing or emulator doesn't boot at all from cases
    // where emulator boots and then disappears from adb.
    if (!mIsOnline) {
        switch (result) {
        case CheckResult::kFailureAdbServerDead:
            dropMetrics(CheckResult::kFailureNoAdb);
            break;
        case CheckResult::kOnline:
            dropMetrics(result);
            mIsOnline = true;
            break;
        default:
            dropMetrics(CheckResult::kNoResult);
        }
        return;
    }

    if (result == CheckResult::kOnline) {
        mRemainingAttempts = kMaxAttempts;
        return;
    }

    if (--mRemainingAttempts == 0) {
        LOG(VERBOSE) << "Reporting error: " << static_cast<int>(result);
        dropMetrics(result);
    }

    LOG(VERBOSE) << "Encountered  error. mRemainingAttempts: "
                 << mRemainingAttempts;
}

void AdbLivenessChecker::dropMetrics(const CheckResult& result) {
    mMetricsFile->setInt(kAdbLivenessKey, static_cast<int>(result));
}

}  // namespace metrics
}  // namespace android
