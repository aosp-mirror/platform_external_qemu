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

#include "android/emulation/control/ApkInstaller.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/base/Uuid.h"

#include <fstream>
#include <iostream>
#include <string>

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::PathUtils;
using android::base::String;
using android::base::StringVector;
using android::base::StringView;
using android::base::System;
using android::base::Uuid;

// static
// TODO(birenbaum): what's an appropriate number here? We don't want to go
// forever but some apps are going to be super big and take a long time.
// This is especially troubling since we don't have a reliable way of killing
// the new process besides using the out PID argument and killing that process
// when the user presses cancel (yikes).
const System::Duration ApkInstaller::kInstallTimeoutMs = System::kInfinite;
const char ApkInstaller::kInstallOutputFileBase[] = "install-";
const char ApkInstaller::kDefaultErrorString[] = "Could not parse error string";

// static
std::shared_ptr<ApkInstaller> ApkInstaller::create(
        Looper* looper,
        const StringVector& adbCommandArgs,
        StringView apkFilePath,
        ResultCallback resultCallback) {
    auto inst = new ApkInstaller(looper, adbCommandArgs, apkFilePath,
                                 resultCallback);
    return std::shared_ptr<ApkInstaller>(inst);
}

ApkInstaller::ApkInstaller(Looper* looper,
                           const StringVector& adbCommandArgs,
                           StringView apkFilePath,
                           ResultCallback resultCallback)
    : mLooper(looper),
      mResultCallback(resultCallback),
      mApkFilePath(apkFilePath),
      mOutputFilePath(
              PathUtils::join(System::get()->getTempDir(),
                              String(kInstallOutputFileBase)
                                      .append(Uuid::generate().toString()))) {
    setAdbCommandArgs(adbCommandArgs);
}

bool ApkInstaller::setAdbCommandArgs(const StringVector& adbCommandArgs) {
    if (inFlight()) {
        return false;
    }

    mInstallCommand = adbCommandArgs;

    mInstallCommand.push_back("install");
    mInstallCommand.push_back("-r");

    return true;
}

bool ApkInstaller::setApkFilePath(base::StringView apkFilePath) {
    if (inFlight()) {
        return false;
    }

    mApkFilePath = apkFilePath;
    return true;
}

void ApkInstaller::cancel() {
    mCancelled = true;
}

void ApkInstaller::start() {
    if (inFlight()) {
        mResultCallback(ApkInstaller::Result::kOperationInProgress, "");
        return;
    }
    mCancelled = false;

    auto shared_this = shared_from_this();
    mParallelTask.reset(new ParallelTask<ApkInstaller::Result>(
            mLooper,
            [shared_this](Result* outResult) {
                shared_this->taskFunction(outResult);
            },
            [shared_this](const Result& result) {
                shared_this->taskDoneFunction(result);
            }));
    mParallelTask->start();

    if (!mParallelTask->inFlight()) {
        mResultCallback(ApkInstaller::Result::kUnknownError, "");
    }
}

intptr_t ApkInstaller::taskFunction(ApkInstaller::Result* outResult) {
    *outResult = Result::kUnknownError;

    auto sys = System::get();

    if (!sys->pathCanRead(mApkFilePath)) {
        *outResult = Result::kApkPermissionsError;
        return 0;
    }

    System::ProcessExitCode exitCode;
    System::Pid pid;

    auto command = mInstallCommand;
    command.push_back(mApkFilePath);

    if (!sys->runCommand(command,
                         System::RunOptions::WaitForCompletion |
                                 System::RunOptions::DumpOutputToFile |
                                 System::RunOptions::TerminateOnTimeout,
                         kInstallTimeoutMs, &exitCode, &pid, mOutputFilePath) ||
        exitCode != 0) {
        // If the device is full, we may be able to get some information from
        // the output (if it exists).
        String errorString;
        if (!parseOutputForFailure(mOutputFilePath, &errorString)) {
            *outResult = Result::kDeviceStorageFull;
        } else {
            *outResult = Result::kAdbConnectionFailed;
        }
        return 0;
    }

    String errorCode;
    if (!parseOutputForFailure(mOutputFilePath, &errorCode)) {
        mErrorCode = errorCode;
        *outResult = Result::kInstallFailed;
        return 0;
    }

    *outResult = Result::kSuccess;
    return 0;
}

// static
bool ApkInstaller::parseOutputForFailure(const String& outputFilePath,
                                         String* outErrorString) {
    // "adb install" does not return a helpful exit status, so instead we parse
    // the output of the process looking for:
    // - "adb: error: failed to copy" in the case that the apk could not be
    //   copied because the device is full
    // - "Failure [<some error code>]", in the case that the apk could not be
    //   installed because of a specific error
    std::ifstream file(outputFilePath.c_str());
    if (!file.is_open()) {
        *outErrorString = kDefaultErrorString;
        return false;
    }

    std::string line;
    while (getline(file, line)) {
        if (!line.compare(0, 7, "Failure")) {
            auto openBrace = line.find("[");
            auto closeBrace = line.find("]", openBrace + 1);
            if (openBrace != std::string::npos && closeBrace != std::string::npos) {
                *outErrorString =
                        line.substr(openBrace + 1, closeBrace - openBrace - 1);
            } else {
                *outErrorString = kDefaultErrorString;
            }
            return false;
        } else if (!line.compare(0, 26, "adb: error: failed to copy")) {
            return false;
        }
    }

    return true;
}

void ApkInstaller::taskDoneFunction(const Result& result) {
    if (!mCancelled) {
        mResultCallback(result, mErrorCode);
    }
    // Ensure no shared_ptr's linger beyond this in the task.
    // NOTE: May invalidate |this|.
    mParallelTask.reset();
}

}  // namespace emulation
}  // namespace android
