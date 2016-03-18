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
#include "android/base/system/Process.h"
#include "android/base/async/RecurrentTask.h"

#include <functional>
#include <queue>
#include <memory>
#include <string>
#include <vector>

namespace android {
namespace emulation {

namespace filepusher {

class IProgressCalculator {
public:
    virtual ~IProgressCalculator() {}
    virtual void reset() = 0;
    virtual void enqueued(const std::string& filePath) = 0;
    virtual void startedPush(const std::string& filePath) = 0;
    virtual void finishedPush() = 0;
    virtual void notifyCurrentFileProgress(unsigned percent) = 0;

    virtual unsigned calculateProgress() = 0;
};

}  // namespace filepusher

class FilePusher {
 public:
    enum class Result {
        Success,

        // These errors are file specific, future files will still be tried.
        UnknownError,
        ProcessStartFailure,

        // These errors stop all future file pushes.
        UnknownFatalError
    };

    // If provided, this callback reports the result of each individual push
    // operation. It also reports fatal errors -- that render this object
    // unusable in the future.
    using PushResultCallback =
            std::function<void(android::base::StringView filePath,
                               FilePusher::Result result,
                               android::base::StringView errorString)>;
    // First argument has an int value in [0,100], reporting progress.
    // Second argument reports whether current batch of file pushing has been
    // completed.
    // |enqueue|ing more files can cause the progress to decrease.
    using ProgressCallback = std::function<void(unsigned, bool)>;

    // Both *Callback are optional. Pass in nullptr if you don't want to know
    // about the progress of queued items.
    FilePusher(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            PushResultCallback pushResultCallback = nullptr,
            ProgressCallback progressCallback = nullptr);
    // For testing, set custom objects for external interfaces.
    FilePusher(
            android::base::Looper* looper,
            const std::vector<std::string>& adbCommandArgs,
            PushResultCallback pushResultCallback,
            ProgressCallback progressCallback,
            android::base::Looper::Duration progressCheckIntervalMs,
            filepusher::IProgressCalculator* progressCalculator);
    ~FilePusher();

    // Enqueue a new file to push.
    // This object DOES NOT guarantee any ordering of the files enqueued.
    bool enqueue(android::base::StringView localFilePath,
                 android::base::StringView remoteFilePath);

    // Cancel current push and drop all subsequent pushes.
    void cancel();

    void setAdbCommandArgs(const std::vector<std::string>& adbCommandArgs);

    // Informational
    size_t numFilesPushed() const { return mNumFilesPushed; }
    size_t numFilesFailed() const { return mNumFilesFailed; }
    size_t numFilesInQueue() const { return mPushQueue.size(); }

 protected:
    typedef struct PushItem {
        std::string localFilePath;
        std::string remoteFilePath;

        PushItem() {}
        PushItem(const std::string& lfp, const std::string& rfp) :
            localFilePath(lfp), remoteFilePath(rfp) {}
    } PushItem;

    static unsigned kDefaultProgressCheckIntervalMs;
    static const char kFilePusherOutputFileBase[];

    void pushNextItem();
    bool monitorTaskCallback();
    unsigned getCurrentPushPercent();
    void finishedPush(Result result);

 private:
    std::vector<std::string> mAdbCommandArgs;
    const PushResultCallback mPushResultCallback;
    const ProgressCallback mProgressCallback;
    std::unique_ptr<filepusher::IProgressCalculator> mProgressCalculator;
    const std::string mOutputFilePath;
    android::base::RecurrentTask mMonitorProgressTask;

    PushItem mCurrentItem;
    std::queue<PushItem> mPushQueue;
    std::unique_ptr<android::base::Process> mPushProcess;

    bool mFatalFailureOccured = false;
    size_t mNumFilesPushed = 0;
    size_t mNumFilesFailed = 0;

    DISALLOW_COPY_AND_ASSIGN(FilePusher);
};

}  // namespace emulation
}  // namespace android
