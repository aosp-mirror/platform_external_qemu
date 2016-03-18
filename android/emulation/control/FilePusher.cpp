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

#include "android/emulation/control/FilePusher.h"

#include "android/base/Compiler.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::ParallelTask;
using android::base::StringView;
using android::base::System;
using std::string;
using std::vector;

FilePusher::Ptr FilePusher::create(Looper* looper,
                                   Looper::Duration progressCheckTimeoutMs) {
    auto inst = new FilePusher(looper, progressCheckTimeoutMs);
    return std::shared_ptr<FilePusher>(inst);
}

FilePusher::FilePusher(Looper* looper, Looper::Duration progressCheckTimeoutMs)
    : mLooper(looper), mProgressCheckTimeoutMs(progressCheckTimeoutMs) {}

FilePusher::~FilePusher() {
    cancel();
}

void FilePusher::setAdbCommandArgs(const vector<string>& adbCommandArgs) {
    mAdbCommandArgs = adbCommandArgs;
    mAdbCommandArgs.push_back("push");
}

bool FilePusher::enqueue(StringView localFilePath, StringView remoteFilePath) {
    LOG(VERBOSE) << "Enqueue new file to push: " << localFilePath << " --> "
                 << remoteFilePath;

    bool triggerPush = mPushQueue.empty() && !mTask;
    mPushQueue.emplace_back(localFilePath, remoteFilePath);

    if (triggerPush) {
        LOG(VERBOSE) << "Starting a new adb push";
        resetProgress();
    }

    ++mNumQueued;
    // Notify progress so that clients get immediate feedback.
    for (auto& subscriber : mSubscribers) {
        if (subscriber.progressCallback) {
            subscriber.progressCallback(
                    static_cast<double>(mNumPushed) / mNumQueued, false);
        }
    }

    if (triggerPush) {
        mSelfRef = shared_from_this();
        pushNextItem();
    }

    return true;
}

void FilePusher::pushNextItem() {
    if (mPushQueue.empty()) {
        return;
    }
    LOG(VERBOSE) << "Pushing next item... [Queue length: " << mPushQueue.size()
                 << "]";

    mCurrentItem = mPushQueue.front();
    mPushQueue.pop_front();

    if (!System::get()->pathCanRead(mCurrentItem.first)) {
        pushDone(Result::FileReadError);
        return;
    }

    auto commandLine = mAdbCommandArgs;
    commandLine.push_back(mCurrentItem.first);
    commandLine.push_back(mCurrentItem.second);

    mTask.reset(new ParallelTask<Result>(
            mLooper,
            [this, commandLine](Result* outResult) {
                this->pushOneFile(commandLine, outResult);
            },
            [this](const Result& result) { this->pushDone(result); },
            mProgressCheckTimeoutMs));
    if (!mTask->start()) {
        // May invalidate |this|.
        pushDone(Result::ProcessStartFailure);
    }
}

// Runs in a worker thread.
// Since this doesn't access any member variables, we don't need any locking.
void FilePusher::pushOneFile(const vector<string>& commandLine,
                             Result* outResult) {
    System::ProcessExitCode exitCode;
    if (!System::get()->runCommand(
                commandLine, System::RunOptions::WaitForCompletion |
                                     System::RunOptions::TerminateOnTimeout,
                System::kInfinite, &exitCode)) {
        *outResult = Result::UnknownError;
        return;
    }
    if (exitCode != 0) {
        *outResult = Result::AdbPushFailure;
        return;
    }

    *outResult = Result::Success;
}

void FilePusher::pushDone(const Result& result) {
    bool allDone = mPushQueue.empty();
    ++mNumPushed;
    for (const auto& subscriber : mSubscribers) {
        if (subscriber.resultCallback) {
            subscriber.resultCallback(mCurrentItem.first, result);
        }
        if (subscriber.progressCallback) {
            subscriber.progressCallback(
                    static_cast<double>(mNumPushed) / mNumQueued, allDone);
        }
    }

    if (!allDone) {
        pushNextItem();
    } else {
        // Tell us that there's no outstanding push atm.
        mTask.reset();
        // May invalidate |this|.
        mSelfRef.reset();
    }
}

void FilePusher::cancel() {
    mPushQueue.clear();
}

void FilePusher::resetProgress() {
    mNumPushed = 0;
    mNumQueued = 0;
}

}  // namespace emulation
}  // namespace android
