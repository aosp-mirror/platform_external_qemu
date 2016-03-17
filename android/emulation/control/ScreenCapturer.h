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

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"

#include <functional>
#include <string>
#include <memory>
#include <vector>

namespace android {
namespace emulation {

class ScreenCapturer : public std::enable_shared_from_this<ScreenCapturer> {
public:
    enum class Result {
        kSuccess,
        kOperationInProgress,
        kCaptureFailed,
        kSaveLocationInvalid,
        kPullFailed,
        kUnknownError
    };

    using ResultCallback = std::function<
            void(Result result,
                 android::base::StringView outputFilePath,
                 android::base::StringView errorString)>;

    // Entry point to ScreenCapturer.
    // Objects of this type are managed via std::shared_ptr
    static std::shared_ptr<ScreenCapturer> create(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            android::base::StringView outputDirectoryPath,
            ResultCallback resultCallback);

    // In case of synchronous failures, this can call |resultCallback| before
    // returning.
    void start();
    void cancel();

    bool inFlight() const { return mParallelTask && mParallelTask->inFlight(); }

    // Fails if a screen capture is in progress, indicated by the return value.
    bool setAdbCommandArgs(const std::vector<std::string>& adbCommandArgs);


    static const android::base::System::Duration kPullTimeoutMs;
    static const char kRemoteScreenshotFilePath[];
    static const android::base::System::Duration kScreenCaptureTimeoutMs;

private:
    ScreenCapturer(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            android::base::StringView outputDirectoryPath,
            ResultCallback resultCallback);

    intptr_t taskFunction(Result* outResult);
    void taskDoneFunction(const Result& result);


private:
    android::base::Looper* const mLooper;
    const ResultCallback mResultCallback;
    const std::string mOutputDirectoryPath;
    std::vector<std::string> mCaptureCommand;
    std::vector<std::string> mPullCommandPrefix;

    bool mCancelled = false;
    std::unique_ptr<android::base::ParallelTask<ScreenCapturer::Result>>
            mParallelTask;
    std::string mFilePath;

    DISALLOW_COPY_AND_ASSIGN(ScreenCapturer);
};

}  // namespace emulation
}  // namespace android
