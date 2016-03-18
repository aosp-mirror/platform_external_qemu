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
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/system/Process.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"

#include <unordered_map>

#include <assert.h>

namespace android {
namespace emulation {

using android::base::Looper;
using android::base::PathUtils;
using android::base::Process;
using android::base::StringView;
using android::base::System;
using android::base::Uuid;
using filepusher::IProgressCalculator;
using std::string;
using std::unordered_map;
using std::vector;

namespace filepusher {

class FileCounter : public IProgressCalculator {
public:
    FileCounter() {
        reset();
    }

    void reset() {
        mTotal = 0;
        mDone = 0;
    }

    void enqueued(const string&) override {
        ++mTotal;
    }

    void finishedPush() override {
        ++mDone;
    }

    unsigned calculateProgress() override {
        if (mTotal == 0) {
            // Noting to do.
            return 100;
        }
        return static_cast<unsigned>((mDone * 100) / mTotal);
    }

    void startedPush(const string&) override {}
    void notifyCurrentFileProgress(unsigned percent) {}

private:
    unsigned mTotal;
    unsigned mDone;

    DISALLOW_COPY_AND_ASSIGN(FileCounter);
};

class FileSizeCalculator : public IProgressCalculator {
public:
    FileSizeCalculator() {
        reset();
    }

    void reset() override {
        mTotal = 0;
        mDone = 0;
        mCurrent = 0;
        mCurrentProgress = 0;
        mSizeForFile.clear();
    }

    void enqueued(const string& filePath) override {
        System::FileSize size;
        if (!System::get()->pathFileSize(filePath, &size)) {
            size = kDefaultSize;
        }

        mTotal += size;
        // For each file, we want to make sure that we use the same value of
        // size in future calculations, even if it's wrong.
        mSizeForFile[filePath] = size;
    }

    void startedPush(const string& filePath) override {
        auto cit = mSizeForFile.find(filePath);
        if (cit == mSizeForFile.end()) {
            // Unknown file. Better not show any progress at all.
            return;
        }
        mCurrent = cit->second;
    }

    void finishedPush() override {
        mDone += mCurrent;
        mCurrent = 0;
    }

    void notifyCurrentFileProgress(unsigned percent) override {
        assert(0 <= percent);
        assert(percent <= 100);
        mCurrentProgress = percent;
    }

    unsigned calculateProgress() override {
        if (mTotal == 0) {
            // Nothing to do.
            return 100;
        }
        return static_cast<unsigned>(
                (mDone + (mCurrent * mCurrentProgress) / (100.0)) / mTotal);
    }

private:
    using SizePerFile = unordered_map<string, System::FileSize>;

    static const System::FileSize kDefaultSize;

    System::FileSize mTotal;
    System::FileSize mDone;
    System::FileSize mCurrent;
    unsigned mCurrentProgress;
    SizePerFile mSizeForFile;


    DISALLOW_COPY_AND_ASSIGN(FileSizeCalculator);
};

// static
// If we can't read the file size, assume some small constant size
// instead of 0 so that we can still show progress. How about 1KB?
const System::FileSize FileSizeCalculator::kDefaultSize = 1024;

}  // namespace filepusher

// static
unsigned FilePusher::kDefaultProgressCheckIntervalMs = 1000;
const char FilePusher::kFilePusherOutputFileBase[] = "adbPush-";

FilePusher::FilePusher(Looper* looper,
                       const vector<string>& adbCommandArgs,
                       PushResultCallback pushResultCallback,
                       ProgressCallback progressCallback):
    FilePusher(looper, adbCommandArgs, pushResultCallback, progressCallback,
               kDefaultProgressCheckIntervalMs,
               new filepusher::FileCounter()) {}

FilePusher::FilePusher(Looper* looper,
                       const vector<string>& adbCommandArgs,
                       PushResultCallback pushResultCallback,
                       ProgressCallback progressCallback,
                       Looper::Duration progressCheckIntervalMs,
                       IProgressCalculator* progressCalculator) :
    mPushResultCallback(pushResultCallback),
    mProgressCallback(progressCallback),
    mProgressCalculator(progressCalculator),
    mOutputFilePath(PathUtils::join(System::get()->getTempDir(),
                string(kFilePusherOutputFileBase).append(
                    Uuid::generate().toString()))),
    mMonitorProgressTask(looper,
                         [this] () { return this->monitorTaskCallback(); },
                         progressCheckIntervalMs) {
        setAdbCommandArgs(adbCommandArgs);
    }

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

    bool triggerPush = false;
    if (mPushQueue.empty()) {
        LOG(VERBOSE) << "Starting a new adb push";
        // Starting a new batch of files.
        mProgressCalculator->reset();
        // Notify a progress of 0, so that clients get immediate feedback about
        // the progress.
        if (mProgressCallback) {
            mProgressCallback(0, false);
        }
        triggerPush = true;
    }

    mPushQueue.emplace(localFilePath, remoteFilePath);
    mProgressCalculator->enqueued(localFilePath);
    if (triggerPush) {
        pushNextItem();
    }

    return true;
}

void FilePusher::pushNextItem() {
    LOG(VERBOSE) << "Pushing next item... [Queue length: "
                 << mPushQueue.size() << "]";
    if (mPushQueue.empty()) {
        mMonitorProgressTask.stop();
        return;
    }

    // Don't pop the item yet. We'll pop it when we're done pushing it.
    mCurrentItem = mPushQueue.front();
    mProgressCalculator->startedPush(mCurrentItem.localFilePath);
    auto commandLine = mAdbCommandArgs;
    commandLine.push_back(mCurrentItem.localFilePath);
    commandLine.push_back(mCurrentItem.remoteFilePath);

    mPushProcess.reset(new Process(
                commandLine, Process::Options::DumpOutputToFile,
                mOutputFilePath));

    if (!mPushProcess->start()) {
        finishedPush(Result::ProcessStartFailure);
    }

    if (!mMonitorProgressTask.inFlight()) {
        mMonitorProgressTask.start();
    }
}

void FilePusher::finishedPush(Result result) {
    mPushQueue.pop();
    mPushProcess.reset();
    mProgressCalculator->finishedPush();
    if (mPushResultCallback) {
        mPushResultCallback(mCurrentItem.localFilePath, result, "");;
    }
}

bool FilePusher::monitorTaskCallback() {
    if (mPushProcess) {
        auto currentFileProgress = getCurrentPushPercent();
        mProgressCalculator->notifyCurrentFileProgress(currentFileProgress);

        Process::ExitCode exitCode;
        Process::WaitResult result = mPushProcess->wait(&exitCode);

        switch (result) {
        case Process::WaitResult::TimedOut:
            break;
        case Process::WaitResult::Success:
            finishedPush(Result::Success);
            break;
        default:
            mPushProcess->terminate();
            finishedPush(Result::UnknownError);
        }
    }

    if (mProgressCallback) {
        auto progress = mProgressCalculator->calculateProgress();
        assert(0 <= progress);
        assert(progress <= 100);
        mProgressCallback(progress, mPushQueue.empty() && !mPushProcess);
    }

    if (!mPushProcess) {
        pushNextItem();
    }
    return true;
}

unsigned FilePusher::getCurrentPushPercent() {
    return 50;
}

void FilePusher::cancel() {
    if (mPushProcess) {
        mPushProcess->terminate();
        mPushProcess->wait();
        mPushProcess.reset();
    }
    // std::queue doesn't have a clear.
    while (!mPushQueue.empty()) {
        mPushQueue.pop();
    }
    mMonitorProgressTask.stop();
    if (mProgressCallback) {
        auto progress = mProgressCalculator->calculateProgress();
        assert(0 <= progress);
        assert(progress <= 100);
        mProgressCallback(progress, true);
    }
}

}  // namespace emulation
}  // namespace android
