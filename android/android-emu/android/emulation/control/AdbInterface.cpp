// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/AdbInterface.h"

#include "android/base/StringView.h"
#include "android/base/Uuid.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/ComponentVersion.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <utility>

using android::base::AutoLock;
using android::base::Looper;
using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::RunOptions;
using android::base::System;
using android::base::Uuid;
using android::base::Version;
using android::base::makeOptional;

namespace android {
namespace emulation {

class AdbInterfaceImpl final : public AdbInterface {
public:
    explicit AdbInterfaceImpl(Looper* looper);

    bool isAdbVersionCurrent() const final { return mAdbVersionCurrent; }

    void setCustomAdbPath(const std::string& path) final {
        mCustomAdbPath = path;
    }

    const std::string& detectedAdbPath() const final { return mAutoAdbPath; }

    const std::string& adbPath() const final {
        return mCustomAdbPath.empty() ? mAutoAdbPath : mCustomAdbPath;
    }

    virtual void setSerialNumberPort(int port) final {
        mSerialString = std::string("emulator-") + std::to_string(port);
    }

    const std::string& serialString() const final { return mSerialString; }

    AdbCommandPtr runAdbCommand(const std::vector<std::string>& args,
                                ResultCallback&& result_callback,
                                System::Duration timeout_ms,
                                bool want_output = true) final;

private:
    Looper* mLooper;
    std::string mAutoAdbPath;
    std::string mCustomAdbPath;
    std::string mSerialString;
    bool mAdbVersionCurrent = false;
};

std::unique_ptr<AdbInterface> AdbInterface::create(Looper* looper) {
    return std::unique_ptr<AdbInterface>{new AdbInterfaceImpl(looper)};
}

// Helper function, checks if the version of adb in the given SDK is
// fresh enough.
static bool checkAdbVersion(const std::string& sdk_root_directory,
                            const std::string& adb_path) {
    static const int kMinAdbVersionMajor = 23;
    static const int kMinAdbVersionMinor = 1;
    static const Version kMinAdbVersion(kMinAdbVersionMajor,
                                        kMinAdbVersionMinor, 0);
    if (sdk_root_directory.empty()) {
        return false;
    }

    if (!System::get()->pathCanExec(adb_path)) {
        return false;
    }

    Version version = android::getCurrentSdkVersion(
            sdk_root_directory, android::SdkComponentType::PlatformTools);

    if (version.isValid()) {
        return !(version < kMinAdbVersion);

    } else {
        // If the version is invalid, assume the tools directory is broken in
        // some way, and updating should fix the problem.
        return false;
    }
}

AdbInterfaceImpl::AdbInterfaceImpl(Looper* looper)
    : mLooper(looper) {
    // First try finding ADB by the environment variable.
    auto sdk_root_by_env = android::ConfigDirs::getSdkRootDirectoryByEnv();
    if (!sdk_root_by_env.empty()) {
        // If ANDROID_SDK_ROOT is defined, the user most likely wanted to use
        // that ADB. Store it for later - if the second potential ADB path
        // also fails, we'll warn the user about this one.
        auto adb_path = PathUtils::join(sdk_root_by_env, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        if (checkAdbVersion(sdk_root_by_env, adb_path)) {
            mAutoAdbPath = adb_path;
            mAdbVersionCurrent = true;
            return;
        }
    }

    // If the first path was non-existent or a bad version, try to infer the
    // path based on the emulator executable location.
    auto sdk_root_by_path = android::ConfigDirs::getSdkRootDirectoryByPath();
    if (sdk_root_by_path != sdk_root_by_env && !sdk_root_by_path.empty()) {
        auto adb_path = PathUtils::join(sdk_root_by_path, "platform-tools",
                                        PathUtils::toExecutableName("adb"));
        if (checkAdbVersion(sdk_root_by_path, adb_path)) {
            mAutoAdbPath = adb_path;
            mAdbVersionCurrent = true;
            return;
        }
    }

    // TODO(zyy): check if there's an adb binary on %PATH% and use that as a
    //  last line of defense.

    // If no ADB has been found at this point, an error message will warn the
    // user and direct them to the custom adb path setting.
}

AdbCommandPtr AdbInterfaceImpl::runAdbCommand(
        const std::vector<std::string>& args,
        ResultCallback&& result_callback,
        System::Duration timeout_ms,
        bool want_output) {
    auto command = std::shared_ptr<AdbCommand>(new AdbCommand(
            mLooper, mCustomAdbPath.empty() ? mAutoAdbPath : mCustomAdbPath,
            mSerialString, args, want_output, timeout_ms,
            std::move(result_callback)));
    command->start();
    return command;
}

AdbCommand::AdbCommand(Looper* looper,
                       const std::string& adb_path,
                       const std::string& serial_string,
                       const std::vector<std::string>& command,
                       bool want_output,
                       System::Duration timeoutMs,
                       ResultCallback&& callback)
    : mLooper(looper),
      mResultCallback(std::move(callback)),
      mOutputFilePath(PathUtils::join(
              System::get()->getTempDir(),
              std::string("adbcommand").append(Uuid::generate().toString()))),
      mWantOutput(want_output),
      mTimeoutMs(timeoutMs) {
    mCommand.push_back(adb_path);

    // TODO: when run headless, the serial string won't be properly
    // initialized, so make a best attempt by using -e. This should be updated
    // when the headless emulator is given an AdbInterface reference.
    if (serial_string.empty()) {
        mCommand.push_back("-e");
    } else {
        mCommand.push_back("-s");
        mCommand.push_back(serial_string);
    }

    mCommand.insert(mCommand.end(), command.begin(), command.end());
}

void AdbCommand::start(int checkTimeoutMs) {
    if (!mTask && !mFinished) {
        auto shared = shared_from_this();
        mTask.reset(new ParallelTask<OptionalAdbCommandResult>(
                mLooper,
                [shared](OptionalAdbCommandResult* result) {
                    shared->taskFunction(result);
                },
                [shared](const OptionalAdbCommandResult& result) {
                    shared->taskDoneFunction(result);
                },
                checkTimeoutMs));
        mTask->start();
    }
}

void AdbCommand::taskDoneFunction(const OptionalAdbCommandResult& result) {
    if (!mCancelled) {
        mResultCallback(result);
    }
    AutoLock lock(mLock);
    mFinished = true;
    mCv.broadcastAndUnlock(&lock);
    // This may invalidate this object and clean it up.
    // DO NOT reference any internal state from this class after this
    // point.
    mTask.reset();
}

bool AdbCommand::wait(System::Duration timeoutMs) {
    if (!mTask) {
        return true;
    }

    AutoLock lock(mLock);
    if (timeoutMs < 0) {
        mCv.wait(&lock, [this]() { return mFinished; });
        return true;
    }

    const auto deadlineUs = System::get()->getUnixTimeUs() + timeoutMs * 1000;
    while (!mFinished && System::get()->getUnixTimeUs() < deadlineUs) {
        mCv.timedWait(&mLock, deadlineUs);
    }

    return mFinished;
}

void AdbCommand::taskFunction(OptionalAdbCommandResult* result) {
    if (mCommand.empty() || mCommand.front().empty()) {
        result->clear();
        return;
    }

    RunOptions output_flag = mWantOutput ? RunOptions::DumpOutputToFile
                                         : RunOptions::HideAllOutput;
    RunOptions run_flags = RunOptions::WaitForCompletion |
                           RunOptions::TerminateOnTimeout | output_flag;
    System::Pid pid;
    System::ProcessExitCode exit_code;

    bool command_ran = System::get()->runCommand(
            mCommand, run_flags, mTimeoutMs, &exit_code, &pid, mOutputFilePath);

    if (command_ran) {
        result->emplace(exit_code,
                        mWantOutput ? mOutputFilePath : std::string());
    }
}

AdbCommandResult::AdbCommandResult(System::ProcessExitCode exitCode,
                                   const std::string& outputName)
    : exit_code(exitCode),
      output(outputName.empty() ? nullptr
                                : new std::ifstream(outputName.c_str())),
      output_name(outputName) {}

AdbCommandResult::~AdbCommandResult() {
    output.reset();
    if (!output_name.empty()) {
        path_delete_file(output_name.c_str());
    }
}

AdbCommandResult::AdbCommandResult(AdbCommandResult&& other)
    : exit_code(other.exit_code),
      output(std::move(other.output)),
      output_name(std::move(other.output_name)) {
    other.output_name.clear();  // make sure |other| won't delete it
}

AdbCommandResult& AdbCommandResult::operator=(AdbCommandResult&& other) {
    exit_code = other.exit_code;
    output = std::move(other.output);
    output_name = std::move(other.output_name);
    other.output_name.clear();
    return *this;
}

}  // namespace emulation
}  // namespace android
