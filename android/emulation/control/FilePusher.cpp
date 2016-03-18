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

FilePusher::Ptr FilePusher::create(Looper* looper) {
    auto inst = new FilePusher(looper);
    return std::shared_ptr<FilePusher>(inst);
}

FilePusher::FilePusher(Looper* looper) : mLooper(looper) {}

FilePusher::~FilePusher() {
    cancel();
}

void FilePusher::setAdbCommandArgs(const vector<string>& adbCommandArgs) {
    mAdbCommandArgs = adbCommandArgs;
    mAdbCommandArgs.push_back("push");
}

bool FilePusher::enqueue(StringView localFilePath,
                         StringView remoteFilePath) {
    if (mFatalFailureOccured) {
        return false;
    }

    bool triggerPush = mPushQueue.empty();
    mPushQueue.emplace(localFilePath, remoteFilePath);

    if (triggerPush) {
        LOG(VERBOSE) << "Starting a new adb push";
        resetProgress();
        for (auto& subscriber : mSubscribers) {
            if (subscriber.progressCallback) {
                subscriber.progressCallback(0, false);
            }
        }
        triggerPush = true;
    }

    ++mNumQueued;
    if (triggerPush) {
        pushNextItem();
    }

    return true;
}

void FilePusher::pushNextItem() {
    if (mPushQueue.empty()) {
        return;
    }

    LOG(VERBOSE) << "Pushing next item... [Queue length: "
                 << mPushQueue.size() << "]";

    // Don't pop the item yet. We'll pop it when we're done pushing it.
    auto currentItem = mPushQueue.front();

    static auto sys = System::get();
    if (!sys->pathCanRead(currentItem.first)) {
        pushDone(Result::FileReadError);
        return;
    }

    auto commandLine = mAdbCommandArgs;
    commandLine.push_back(currentItem.first);
    commandLine.push_back(currentItem.second);

    auto shared_this = shared_from_this();
    mTask.reset(new ParallelTask<Result>(
            mLooper,
            [shared_this, commandLine] (Result* outResult) {
                shared_this->pushOneFile(commandLine, outResult);
            },
            [shared_this] (const Result& result) {
                shared_this->pushDone(result);
            }));
    if (!mTask->start()) {
        // invalidates mTask.
        pushDone(Result::ProcessStartFailure);
    }
}

// Runs in a worker thread.
// Since this doesn't access any member variables, we don't need any locking.
void FilePusher::pushOneFile(vector<string> commandLine,
                             Result* outResult) {
    static auto sys = System::get();

    System::ProcessExitCode exitCode;
    if(!sys->runCommand(
            commandLine,
            System::RunOptions::WaitForCompletion |
            System::RunOptions::TerminateOnTimeout,
            System::kInfinite, &exitCode) ||
       exitCode != 0) {
        *outResult = Result::UnknownError;
        return;
    }

    *outResult = Result::Success;
}

void FilePusher::pushDone(const Result& result) {
    auto fromFile  = mPushQueue.front().first;
    mPushQueue.pop();
    bool allDone = !mPushQueue.empty();
    ++mNumPushed;
    for (const auto& subscriber : mSubscribers) {
        if (subscriber.resultCallback) {
            subscriber.resultCallback(fromFile, result);
        }
        if (subscriber.progressCallback) {
            subscriber.progressCallback(
                    static_cast<double>(mNumPushed) / mNumQueued,
                    allDone);
        }
    }
    // Release shared pointer held in the task.
    mTask.reset();

    if (!allDone) {
        pushNextItem();
    }
}

void FilePusher::cancel() {
    while (!mPushQueue.empty()) {
        mPushQueue.pop();
    }
}

void FilePusher::resetProgress() {
    mNumPushed = 0;
    mNumQueued = 0;
}

}  // namespace emulation
}  // namespace android
