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

#include "android/emulation/control/ScreenCapturer.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::String;
using android::base::StringVector;
using android::base::StringView;
using android::base::System;
using std::vector;

// static
const System::Duration ScreenCapturer::kPullTimeoutMs = 5000;
const char ScreenCapturer::kRemoteScreenshotFilePath[] =
        "/data/local/tmp/screen.png";
// If you run screencap while using emulator over remote desktop, this can
// take... a while.
const System::Duration ScreenCapturer::kScreenCaptureTimeoutMs =
        System::kInfinite;

// static
std::shared_ptr<ScreenCapturer> ScreenCapturer::create(
        Looper* looper,
        const StringVector& adbCommandArgs,
        StringView outputDirectoryPath,
        ResultCallback resultCallback,
        unsigned pollingHintMs) {
    auto inst = new ScreenCapturer(looper, adbCommandArgs, outputDirectoryPath,
                                   resultCallback, pollingHintMs);
    return std::shared_ptr<ScreenCapturer>(inst);
}

ScreenCapturer::ScreenCapturer(Looper* looper,
                               const StringVector& adbCommandArgs,
                               StringView outputDirectoryPath,
                               ResultCallback resultCallback,
                               unsigned pollingHintMs)
    : mLooper(looper),
      mResultCallback(resultCallback),
      mOutputDirectoryPath(outputDirectoryPath),
      mNonDefaultCheckTimeoutForTestMs(pollingHintMs) {
    setAdbCommandArgs(adbCommandArgs);
}

bool ScreenCapturer::setAdbCommandArgs(const StringVector& adbCommandArgs) {
    if (inFlight()) {
        return false;
    }

    mCaptureCommand = adbCommandArgs;
    mPullCommandPrefix = adbCommandArgs;

    mCaptureCommand.push_back("shell");
    mCaptureCommand.push_back("screencap");
    mCaptureCommand.push_back("-p");
    mCaptureCommand.push_back(kRemoteScreenshotFilePath);

    mPullCommandPrefix.push_back("pull");
    mPullCommandPrefix.push_back(kRemoteScreenshotFilePath);
    return true;
}

bool ScreenCapturer::setOutputDirectoryPath(StringView path) {
    if (inFlight()) {
        return false;
    }

    mOutputDirectoryPath = path;
    return true;
}

void ScreenCapturer::cancel() {
    mCancelled = true;
}

void ScreenCapturer::start() {
    if (inFlight()) {
        mResultCallback(ScreenCapturer::Result::kOperationInProgress, "",
                        "Another screen capture in progress");
        return;
    }
    mCancelled = false;

    auto shared_this = shared_from_this();
    auto taskFunction = [shared_this](Result* outResult) {
        shared_this->taskFunction(outResult);
    };
    auto taskDoneFunction = [shared_this](const Result& result) {
        shared_this->taskDoneFunction(result);
    };

    if (mNonDefaultCheckTimeoutForTestMs == 0) {
        mParallelTask.reset(new ParallelTask<ScreenCapturer::Result>(
                mLooper, taskFunction, taskDoneFunction));
    } else {
        // Tests-only branch to speed up things a bit.
        mParallelTask.reset(new ParallelTask<ScreenCapturer::Result>(
                mLooper, taskFunction, taskDoneFunction,
                mNonDefaultCheckTimeoutForTestMs));
    }

    mParallelTask->start();

    if (!inFlight()) {
        mResultCallback(ScreenCapturer::Result::kUnknownError, "",
                        "Failed to spawn thread for you!");
    }
}

intptr_t ScreenCapturer::taskFunction(ScreenCapturer::Result* outResult) {
    *outResult = Result::kUnknownError;

    auto sys = System::get();
    System::ProcessExitCode exitCode;
    if (!sys->runCommand(mCaptureCommand,
                         System::RunOptions::WaitForCompletion |
                                 System::RunOptions::HideAllOutput |
                                 System::RunOptions::TerminateOnTimeout,
                         kScreenCaptureTimeoutMs, &exitCode) ||
        exitCode != 0) {
        // TODO(pprabhu): Capture stderr and return an intelligent error string.
        *outResult = Result::kCaptureFailed;
        return 0;
    }

    if (!sys->pathIsDir(mOutputDirectoryPath) ||
        !sys->pathCanWrite(mOutputDirectoryPath)) {
        *outResult = Result::kSaveLocationInvalid;
        return 0;
    }

    auto fileName = android::base::StringFormat(
            "Screenshot_%lu.png",
            static_cast<unsigned long>(sys->getUnixTime()));
    mFilePath = android::base::PathUtils::join(mOutputDirectoryPath, fileName);

    auto command = mPullCommandPrefix;
    command.push_back(mFilePath);
    if (!sys->runCommand(command,
                         System::RunOptions::WaitForCompletion |
                                 System::RunOptions::HideAllOutput |
                                 System::RunOptions::TerminateOnTimeout,
                         kPullTimeoutMs, &exitCode) ||
        exitCode != 0) {
        // TODO(pprabhu): Capture stderr and return an intelligent error string.
        *outResult = Result::kPullFailed;
        return 0;
    }
    *outResult = Result::kSuccess;
    return 0;
}

void ScreenCapturer::taskDoneFunction(const Result& result) {
    if (!mCancelled) {
        mResultCallback(result, mFilePath, "Detailed error not supported yet");
    }
    // Ensure no shared_ptr's linger beyond this in the task.
    // NOTE: May invalidate |this|.
    mParallelTask.reset();
}

}  // namespace emulation
}  // namespace android
