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
#include "android/base/async/SubscriberList.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"

#include <functional>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class FilePusher : public std::enable_shared_from_this<FilePusher> {
public:
    using Ptr = std::shared_ptr<FilePusher>;
    using SubscriptionToken = android::base::SubscriptionToken;
    using PushItem = std::pair<std::string, std::string>;

    enum class Result {
        Success,

        UnknownError,
        ProcessStartFailure,
        // Count not read file to push.
        FileReadError,
        AdbPushFailure,
    };

    // Instances of FilePusher are managed via shared_ptr. Internally,
    // FilePusher uses worker threads to 'adb push' files. By using shared_ptr,
    // the instance guarantees that it stays alive while a worker thread is
    // active, ensuring proper cleanup.
    // |progressCheckTimeoutMs| is used mostly by tests to speed up execution.
    static Ptr create(
            android::base::Looper* looper,
            android::base::Looper::Duration progressCheckTimeoutMs = 10);
    ~FilePusher();

    void setAdbCommandArgs(const std::vector<std::string>& adbCommandArgs);

    // Enqueue a new file to push.
    bool enqueue(android::base::StringView localFilePath,
                 android::base::StringView remoteFilePath);
    bool enqueue(const std::vector<PushItem>& items) {
        bool success = true;
        for (const auto& item : items) {
            success &= enqueue(item.first, item.second);
        }
        return success;
    }

    // Cancel all outstanding pushes. Note that we do not cancel currently
    // active push.
    void cancel();

    // This callback reports the result of each individual push operation.
    using ResultCallback =
            std::function<void(android::base::StringView filePath,
                               FilePusher::Result result)>;
    // First argument has an double value in [0,1], reporting progress.
    // Second argument reports whether current batch of file pushing has been
    // completed.
    // |enqueue|ing more files can cause the progress to decrease.
    using ProgressCallback = std::function<void(double, bool)>;

    // Subscribe to per-file-push and overall-progress updates.
    // Both callbacks are optional. Pass in nullptr to ignore.
    SubscriptionToken subscribeToUpdates(ResultCallback resultCallback,
                                         ProgressCallback progressCallback) {
        return mSubscribers.emplace(resultCallback, progressCallback);
    }

private:
    struct SubscriptionInfo {
        ResultCallback resultCallback;
        ProgressCallback progressCallback;
        SubscriptionInfo(ResultCallback rc, ProgressCallback pc)
            : resultCallback(rc), progressCallback(pc) {}
    };

    FilePusher(android::base::Looper* looper,
               android::base::Looper::Duration progressCheckTimeoutMs);

    void pushNextItem();
    void resetProgress();

    // ParallelTask callbacks.
    void pushOneFile(const std::vector<std::string>& commandLine,
                     Result* outResult);
    void pushDone(const Result& result);

    android::base::Looper* const mLooper;
    android::base::Looper::Duration mProgressCheckTimeoutMs;
    std::vector<std::string> mAdbCommandArgs;

    android::base::SubscriberList<SubscriptionInfo> mSubscribers;

    std::deque<PushItem> mPushQueue;
    std::unique_ptr<android::base::ParallelTask<Result>> mTask;
    PushItem mCurrentItem;
    size_t mNumQueued = 0;
    size_t mNumPushed = 0;

    // We hold a reference to ourselves as long as there is more work to do.
    // This ensures that the object stays alive even if the external references
    // are dropped, and we finish our work successfully.
    Ptr mSelfRef;

    DISALLOW_COPY_AND_ASSIGN(FilePusher);
};

}  // namespace emulation
}  // namespace android
