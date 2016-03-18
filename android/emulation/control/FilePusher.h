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
#include "android/base/threads/ParallelTask.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

class FilePusher : public std::enable_shared_from_this<FilePusher> {
 public:
    enum class PushError {
        // These errors are file specific, future files will still be tried.
        kUnknownError,

        // These errors stop all future file pushes.
        kUnknownFatalError
    };

    using PushFailedCallback =
            std::function<void(android::base::StringView filePath,
                               FilePusher::PushError error,
                               android::base::StringView errorString)>;
    // Progress callback is called with a value in [0,1].
    // |enqueue|ing more files can cause the progress to decrease.
    using ProgressCallback = std::function<void(float)>;

    std::shared_ptr<FilePusher> create(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            ProgressCallback progressCallback,
            PushFailedCallback pushFailedCallback);

    // In case of synchronous failures, this will call |resultCallback| with a
    // Result::k*FatalFailure immediately.
    void start();
    // This cancels any future file pushes, and also guarantees that client
    // callbacks are not called in the future.  This DOES NOT cancel an ongoing
    // file push.
    // Once you |start| the FilePusher, you MUST |stop| it to ensure proper
    // cleanup.
    void stop();

    // Enqueue a new file to push.
    // This object DOES NOT guarantee any ordering of the files enqueued.
    bool enqueue(android::base::StringView localFilePath,
                 android::base::StringView remoteFilePath);


    // Informational
    bool inFlight() const;
    size_t numFilesPushed() const { return mNumFilesPushed; }
    size_t numFilesFailed() const { return mNumFilesFailed; }
    size_t numFilesInQueue() const;

 private:
    using std::pair<android::base::StringView, android::base::StringView>
            PushItem;

    class PusherThread : public android::base::Thread {
        intptr_t main() override;
    };

    android::base::Looper* const mLooper;
    const std::vector<std::string> mAdbCommandArgs;
    const ProgressCallback mProgressCallback;
    const PushFailedCallback mPushFailedCallback;

    PusherThread mThread;

    size_t mNumFilesPushed = 0;
    size_t mNumFilesFailed = 0;
    std::unordered_set<android::base::StringView> pushSet;
    std::vector<PushItem> pushQueue;


};

}  // namespace emulation
}  // namespace android
