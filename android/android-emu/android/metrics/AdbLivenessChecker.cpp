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

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"
#include "android/emulation/ConfigDirs.h"
#include "android/console.h"

#include "studio_stats.pb.h"

#include <istream>

namespace android {
namespace metrics {

using android::ConfigDirs;
using android::base::PathUtils;
using android::base::System;
using std::shared_ptr;

static const int kMaxAttempts = 3;

// static
AdbLivenessChecker::Ptr AdbLivenessChecker::create(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        MetricsReporter* reporter,
        android::base::StringView emulatorName,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst = Ptr(new AdbLivenessChecker(adb, looper, reporter, emulatorName,
                                           checkIntervalMs));
    return inst;
}

namespace {
struct LivenessStatus {
    bool online;
    bool bootComplete;
};
}  // namespace

static LivenessStatus sLivenessStatus = {};

// static
bool AdbLivenessChecker::isEmulatorOnline() {
    return sLivenessStatus.online;
}

bool AdbLivenessChecker::isEmulatorBooted() {
    return sLivenessStatus.bootComplete;
}

static long bootCheckTaskTimeoutMs(
        android::base::Looper::Duration adbTimeoutMs) {
    if (avdInfo_is_x86ish(getConsoleAgents()->settings->avdInfo())) {
        return std::max<long>(adbTimeoutMs / 100, 250);
    }
    return std::max<long>(adbTimeoutMs / 10, 5000);
}

AdbLivenessChecker::AdbLivenessChecker(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        MetricsReporter* reporter,
        android::base::StringView emulatorName,
        android::base::Looper::Duration checkIntervalMs)
    : mAdb(adb),
      mLooper(looper),
      mReporter(reporter),
      mEmulatorName(emulatorName),
      mCheckIntervalMs(checkIntervalMs),
      // We use raw pointer to |this| instead of a shared_ptr to avoid cicrular
      // ownership. mRecurrentTask promises to cancel any outstanding tasks when
      // it's destructed.
      mRecurrentTask(looper,
                     [this]() { return adbCheckRequest(); },
                     mCheckIntervalMs),
      mBootCheckTask(looper,
                     [this]() { return runBootCheck(); },
                     bootCheckTaskTimeoutMs(mCheckIntervalMs)),
      // TODO: bug 115582308
      // Remove the liveness checker.
      // Until we figure out how to run adb from socket directly.
      //
      // The liveness checker is only meant to measure one extremely narrow
      // thing: the edge trigger for online -> offline.  This one thing, that
      // we don't even look at very much in metrics, and whose rate of going
      // offline is very low already, is a guaranteed onerous burden for users
      // as it is a heavyweight operation that starts a subprocess and involves
      // temp file handling, and easily falls over or overloads the user's
      // machine with adb procesess when the smallest thing is out of whack,
      // such as when there are mismatched ADB servers running on the host.
      mRemainingAttempts(0) {
    // Don't call start() here: start() launches a parallel task that calls
    // shared_from_this(), which needs at least one shared pointer owning
    // |this|. We can't guarantee that until the constructor call returns.
}

AdbLivenessChecker::~AdbLivenessChecker() {
    if (mDevicesCommand) {
        mDevicesCommand->cancel();
    }
    if (mShellExitCommand) {
        mShellExitCommand->cancel();
    }
    if (mBootCompleteCommand) {
        mBootCompleteCommand->cancel();
    }
    stop();
}

void AdbLivenessChecker::start() {
    // TODO: bug 115582308
    // mRecurrentTask.start(true);
    // mBootCheckTask.start(true);
}

void AdbLivenessChecker::stop() {
    // TODO: bug 115582308
    // mRecurrentTask.stopAndWait();
    // mBootCheckTask.stopAndWait();
}

bool AdbLivenessChecker::adbCheckRequest() {
    if (!mRemainingAttempts) {
        LOG(VERBOSE) << "Quitting adb liveness check";
        return false;
    }

    if (mIsCheckRunning) {
        return true;
    }
    mIsCheckRunning = true;

#if 0
    // NOTE: Capture a shared_ptr to |this| to guarantee object lifetime while
    // the parallel task is running.
    auto weakSelf = std::weak_ptr<AdbLivenessChecker>(shared_from_this());
    auto taskFunction = [weakSelf](CheckResult* outResult) {
        if (const auto self = weakSelf.lock()) {
            self->runCheckBlocking(outResult);
        }
    };
    auto taskDoneFunction = [weakSelf](const CheckResult& result) {
        if (const auto self = weakSelf.lock()) {
            self->reportCheckResult(result);
        }
    };
    if (!android::base::runParallelTask<CheckResult>(mLooper, taskFunction,
                                                     taskDoneFunction)) {
        mIsCheckRunning = false;
        return false;
    }
#endif

    runCheckNonBlocking();

    return true;
}

bool AdbLivenessChecker::runBootCheck() {
    if (sLivenessStatus.bootComplete) {
        return false;
    }
    if (mBootCompleteCommand) {
        return true;
    }
    mBootCompleteCommand = mAdb->runAdbCommand(
            {"shell", "getprop", "dev.bootcomplete"},
            [this](const android::emulation::OptionalAdbCommandResult& result) {
                if (!result || result->exit_code || !result->output) {
                    sLivenessStatus.bootComplete = false;
                } else {
                    char out = 0;
                    if (bool(*result->output >> out) && out == '1') {
                        sLivenessStatus.bootComplete = true;
                        mBootCheckTask.stopAsync();
                        dropBootMetrics(BootResult::kBootCompleted);
                    } else {
                        sLivenessStatus.bootComplete = false;
                    }
                }
                mBootCompleteCommand.reset();
            },
            mBootCheckTask.taskIntervalMs(), true);
    return true;
}

void AdbLivenessChecker::runCheckBlocking(CheckResult* outResult) const {
    System::ProcessExitCode exitCode;
    const std::string& adbPath = mAdb->adbPath();
    std::string serial = mAdb->serialString();
    if (serial.empty())
        serial = mEmulatorName;

    const std::vector<std::string> adbServerAliveCmd = {adbPath, "devices"};
    if (!System::get()->runCommand(
                adbServerAliveCmd,
                System::RunOptions::WaitForCompletion |
                        System::RunOptions::TerminateOnTimeout,
                mCheckIntervalMs / 3, &exitCode) ||
        exitCode != 0) {
        *outResult = CheckResult::kFailureAdbServerDead;
        return;
    }

    const std::vector<std::string> emulatorAliveCmd = {adbPath, "-s", serial,
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

void AdbLivenessChecker::runCheckNonBlocking() {
    if (mDevicesCommand || mShellExitCommand || mBootCompleteCommand) {
        // already in progress
        return;
    }

    mDevicesCommand = mAdb->runAdbCommand(
            {"devices"},
            [this](const android::emulation::OptionalAdbCommandResult& result) {
                if (!result || result->exit_code) {
                    reportCheckResult(CheckResult::kFailureAdbServerDead);
                    sLivenessStatus.online = false;
                    sLivenessStatus.bootComplete = false;
                    if (!mBootCheckTask.inFlight()) {
                        mBootCheckTask.start(true);
                    }
                } else {
                    mShellExitCommand = mAdb->runAdbCommand(
                            {"shell", "exit"},
                            [this](const android::emulation::
                                           OptionalAdbCommandResult& result2) {
                                if (!result2 || result2->exit_code) {
                                    reportCheckResult(
                                            CheckResult::kFailureEmulatorDead);
                                    sLivenessStatus.online = false;
                                    sLivenessStatus.bootComplete = false;
                                    if (!mBootCheckTask.inFlight()) {
                                        mBootCheckTask.start(true);
                                    }
                                } else {
                                    reportCheckResult(CheckResult::kOnline);
                                    sLivenessStatus.online = true;
                                }

                                mShellExitCommand.reset();
                            },
                            mCheckIntervalMs / 3, false);
                }

                mDevicesCommand.reset();
            },
            mCheckIntervalMs / 3, false);
}

void AdbLivenessChecker::reportCheckResult(const CheckResult& result) {
    mIsCheckRunning = false;

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
        mRemainingAttempts = 0;
        dropMetrics(result);
    } else if (--mRemainingAttempts == 0) {
        LOG(VERBOSE) << "Reporting error: " << static_cast<int>(result);
        dropMetrics(result);
    } else {
        LOG(VERBOSE) << "Encountered  error. mRemainingAttempts: "
                     << mRemainingAttempts;
    }
}

void AdbLivenessChecker::dropMetrics(const CheckResult& result) {
    // Only report state if it changes. We start with kNoResult, and we only
    // report kNoResult or kFailureNoAdb until emulator starts.
    if (mCurrentState != result) {
        mCurrentState = result;
        mReporter->report([result](android_studio::AndroidStudioEvent* event) {
            event->mutable_emulator_details()->set_adb_liveness(
                    static_cast<android_studio::EmulatorDetails::
                                        EmulatorAdbLiveness>(
                            static_cast<int>(result)));
        });
    }
}

void AdbLivenessChecker::dropBootMetrics(const BootResult& result) {
    // We only report if boot is detected as successful, or if it is detected
    // as failing. In both cases, we want to collect the time taken to reach
    // that conclusion.
    auto timeMs = System::get()->getUnixTimeUs() / 1000;
    auto durationMs = timeMs - mReporter->getStartTimeMs();
    mReporter->report([result, durationMs](android_studio::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_boot_info()->set_boot_status(
                static_cast<android_studio::EmulatorBootInfo::BootStatus>(
                        static_cast<int>(result)));
        event->mutable_emulator_details()->mutable_boot_info()->set_duration_ms(
                durationMs);
    });
}

}  // namespace metrics
}  // namespace android
