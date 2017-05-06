// Copyright 2017 The Android Open Source Project
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

#include "android/crashreport/HangDetector.h"

#include "android/base/Optional.h"
#include "android/base/StringFormat.h"
#include "android/utils/debug.h"

#include <utility>

namespace android {
namespace crashreport {

class HangDetector::LooperWatcher {
    DISALLOW_COPY_AND_ASSIGN(LooperWatcher);

public:
    LooperWatcher(base::Looper* looper);
    ~LooperWatcher();

    LooperWatcher(LooperWatcher&&) = default;
    LooperWatcher& operator=(LooperWatcher&&) = default;

    void startHangCheck();
    void cancelHangCheck();
    void process(const HangCallback& hangCallback);

private:
    void startHangCheckLocked();
    void taskComplete();

    base::Looper* const mLooper;
    std::unique_ptr<base::Looper::Task> mTask;
    bool mIsTaskRunning = false;

    base::Optional<base::System::Duration> mLastCheckTimeUs;

    std::unique_ptr<base::Lock> mLock{new base::Lock()};
};

HangDetector::LooperWatcher::LooperWatcher(base::Looper* looper)
    : mLooper(looper) {}

HangDetector::LooperWatcher::~LooperWatcher() {
    if (mTask) {
        mTask->cancel();
    }
}

void HangDetector::LooperWatcher::startHangCheck() {
    base::AutoLock l(*mLock);

    startHangCheckLocked();
}

void HangDetector::LooperWatcher::cancelHangCheck() {
    base::AutoLock l(*mLock);

    mTask->cancel();
    mIsTaskRunning = false;
    mLastCheckTimeUs = base::System::get()->getUnixTimeUs();
}

void HangDetector::LooperWatcher::process(const HangCallback& hangCallback) {
    base::AutoLock l(*mLock);

    const auto now = base::System::get()->getUnixTimeUs();
    if (mIsTaskRunning) {
        if (now > *mLastCheckTimeUs + kTaskProcessingTimeoutMs * 1000) {
            const auto message = base::StringFormat(
                    "detected a hanging thread '%s'. No response for %d ms",
                    mLooper->name(), (int)((now - *mLastCheckTimeUs) / 1000));
            l.unlock();

            derror("%s", message.c_str());
            if (hangCallback) {
                hangCallback(message);
            }
        }
    } else if (now > mLastCheckTimeUs.valueOr(0) + kHangCheckTimeoutMs * 1000) {
        startHangCheckLocked();
    }
}

void HangDetector::LooperWatcher::startHangCheckLocked() {
    if (!mTask) {
        mTask = mLooper->createTask([this]() { taskComplete(); });
    }
    mTask->schedule();
    mIsTaskRunning = true;
    mLastCheckTimeUs = base::System::get()->getUnixTimeUs();
}

void HangDetector::LooperWatcher::taskComplete() {
    base::AutoLock l(*mLock);
    mIsTaskRunning = false;
}

HangDetector::HangDetector(HangCallback&& hangCallback)
    : mHangCallback(std::move(hangCallback)),
      mWorkerThread([this]() { workerThread(); }) {
    mWorkerThread.start();
}

HangDetector::~HangDetector() {
    stop();
}

void HangDetector::addWatchedLooper(base::Looper* looper) {
    base::AutoLock lock(mLock);
    mLoopers.emplace_back(new LooperWatcher(looper));
    mLoopers.back()->startHangCheck();
}

void HangDetector::stop() {
    {
        base::AutoLock lock(mLock);
        for (auto&& lw : mLoopers) {
            lw->cancelHangCheck();
        }
        mStopping = true;
        mWorkerThreadCv.signalAndUnlock(&lock);
    }
    mWorkerThread.wait();
}

void HangDetector::workerThread() {
    base::AutoLock lock(mLock);
    for (;;) {
        const auto waitUntilUs = base::System::get()->getUnixTimeUs() +
                                 kHangLoopIterationTimeoutMs * 1000;
        while (!mStopping &&
               base::System::get()->getUnixTimeUs() < waitUntilUs) {
            mWorkerThreadCv.timedWait(&mLock, waitUntilUs);
        }
        for (auto&& lw : mLoopers) {
            if (mStopping) {
                return;
            }
            lw->process(mHangCallback);
        }
    }
}

}  // namespace crashreport
}  // namespace android
