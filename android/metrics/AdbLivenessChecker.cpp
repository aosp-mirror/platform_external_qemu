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

AdbLivenessChecker::AdbLivenessChecker(android::base::Looper* looper,
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
        mMaxAttempts(3),
        mRemainingAttempts(mMaxAttempts) {
    static const char kPlatformToolsSubdir[] = "platform-tools";
#ifdef _WIN32
    static const char kAdbExecutable[] = "adb.exe";
#else
    static const char kAdbExecutable[] = "adb";
#endif
    mAdbPath = PathUtils::join(
            ConfigDirs::getSdkRootDirectory(),
            PathUtils::join(kPlatformToolsSubdir, kAdbExecutable));
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

    static auto taskFunction = std::bind(&AdbLivenessChecker::runCheckBlocking,
                                         this, std::placeholders::_1);
    static auto taskDoneFunction =
            std::bind(&AdbLivenessChecker::reportCheckResult, this,
                      std::placeholders::_1);
    if (!mCheckTask || !mCheckTask->inFlight()) {
        mCheckTask.reset(
                new AdbCheckTask(mLooper, taskFunction, taskDoneFunction));
        mCheckTask->start();
    }
    return true;
}

void AdbLivenessChecker::runCheckBlocking(CheckResult* outResult) {
    System::ProcessExitCode exitCode;
    static const StringVector adbServerAliveCmd = {mAdbPath, "devices"};
    if (!System::get()->runCommand(
                adbServerAliveCmd,
                System::RunOptions::WaitForCompletion |
                System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureAdbServerDead;
        return;
    }

    static const StringVector emulatorAliveCmd = {mAdbPath, "-s",
        mEmulatorName.c_str(), "shell", "exit"};
    if (!System::get()->runCommand(
                emulatorAliveCmd,
                System::RunOptions::WaitForCompletion |
                System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureEmulatorDead;
        return;
    }

    *outResult = AdbLivenessChecker::CheckResult::kSuccess;
}

static const char kAdbOnlineKey[] = "adb_online";
static const char kAdbErrorKey[] = "adb_error";

void AdbLivenessChecker::reportCheckResult(const CheckResult& result) {
    // TODO(pprabhu): This can fail if platform-tools is not installed. Perhaps
    // just report an enum of results?
    if (result == CheckResult::kSuccess) {
        if (!mIsOnline) {
            mIsOnline = true;
            mMetricsFile->setInt(kAdbOnlineKey, 1);
        }
        mRemainingAttempts = mMaxAttempts;
        return;
    }

    if (!mIsOnline && result != CheckResult::kFailureAdbServerDead) {
        // |CheckResult::kFailureAdbServerDead| is the only real failure before
        // we ever go online. Ignore other initial failures.
        return;
    }

    if (--mRemainingAttempts == 0) {
        LOG(VERBOSE) << "Reporting error: " << static_cast<int>(result);
        mMetricsFile->setInt(kAdbErrorKey, static_cast<int>(result));
    }

    LOG(VERBOSE) << "Encountered  error. mRemainingAttempts: "
                 << mRemainingAttempts;
}

}  // namespace metrics
}  // namespace android
