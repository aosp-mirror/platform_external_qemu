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
#include "android/base/StringView.h"
#include "android/base/threads/RecurrentTask.h"

#include <functional>
#include <queue>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class FilePusher {
 public:
    enum class PushResult {
        kSuccess,

        // These errors are file specific, future files will still be tried.
        kUnknownError,

        // These errors stop all future file pushes.
        kUnknownFatalError
    };

    // If provided, this callback reports the result of each individual push
    // operation. It also reports fatal errors -- that render this object
    // unusable in the future.
    using PushResultCallback =
            std::function<void(android::base::StringView filePath,
                               FilePusher::PushResult result,
                               android::base::StringView errorString)>;
    // Progress callback is called with an int value in [0,100].
    // |enqueue|ing more files can cause the progress to decrease.
    using ProgressCallback = std::function<void(unsigned)>;

    // Both *Callback are optional. Pass in nullptr if you don't want to know
    // about the progress of queued items.
    FilePusher(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            ProgressCallback progressCallback,
            PushResultCallback pushResultCallback,
            android::base::Looper::Duration progressCheckIntervalMs = 1000);

    // Enqueue a new file to push.
    // This object DOES NOT guarantee any ordering of the files enqueued.
    bool enqueue(android::base::StringView localFilePath,
                 android::base::StringView remoteFilePath);

    void setAdbCommandArgs(const std::vector<std::string>& adbCommandArgs);

    // Informational
    size_t numFilesPushed() const { return mNumFilesPushed; }
    size_t numFilesFailed() const { return mNumFilesFailed; }
    size_t numFilesInQueue() const { return mPushQueue.size(); }

 protected:
    void pushNextItem();
    bool monitorPush();
    size_t statSize(android::base::StringView localFilePath);

 private:
    typedef struct PushItem {
        std::string localFilePath;
        std::string remoteFilePath;
        size_t fileSize;
    } PushItem;

    android::base::Looper* const mLooper;
    const std::vector<std::string> mAdbCommandArgs;
    const ProgressCallback mProgressCallback;
    const PushResultCallback mPushResultCallback;

    bool mFatalFailureOccured = false;
    bool mIsProgressCheckActive = false;
    size_t mNumFilesPushed = 0;
    size_t mNumFilesFailed = 0;
    // Outstanding items are stored in these.
    std::queue<PushItem> mPushQueue;
    android::base::RecurrentTask mMonitorProgressTask;

    DISALLOW_COPY_AND_ASSIGN(FilePusher);
};

}  // namespace emulation
}  // namespace android
