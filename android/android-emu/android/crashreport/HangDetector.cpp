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
#include "android/globals.h"
#include "android/utils/debug.h"

#include <utility>

namespace android {
namespace crashreport {

class HangDetector::LooperWatcher {
    DISALLOW_COPY_AND_ASSIGN(LooperWatcher);

public:
    LooperWatcher(base::Looper* looper,
                  base::System::Duration hangTimeoutMs,
                  base::System::Duration hangCheckTimeoutMs);
    ~LooperWatcher();

    LooperWatcher(LooperWatcher&&) = default;

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
    const base::System::Duration mTimeoutMs;
    const base::System::Duration mHangCheckTimeoutMs;
    std::unique_ptr<base::Lock> mLock{new base::Lock()};

    static constexpr int kMaxHangCount = 2;
    int mHangCount = 0;
};

HangDetector::LooperWatcher::LooperWatcher(
        base::Looper* looper,
        base::System::Duration hangTimeoutMs,
        base::System::Duration hangCheckTimeoutMs)
    : mLooper(looper),
      mTimeoutMs(hangTimeoutMs),
      mHangCheckTimeoutMs(hangCheckTimeoutMs) {}

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

    if (mTask) {
        mTask->cancel();
    }
    mIsTaskRunning = false;
    mLastCheckTimeUs = base::System::get()->getUnixTimeUs();
}

void HangDetector::LooperWatcher::process(const HangCallback& hangCallback) {
    base::AutoLock l(*mLock);

    const auto now = base::System::get()->getUnixTimeUs();
    if (mIsTaskRunning) {
        // Heuristic: If the looper watcher itself took much longer than
        // mTimeoutMs to fire again, it's possible there was a system-wide
        // sleep. In this case, don't count that as hanging.
        const auto timeSystemLiveAndHanging = *mLastCheckTimeUs + mTimeoutMs * 1000;
        const auto timeSystemSleepedPastHangTimeout = *mLastCheckTimeUs + 2 * mTimeoutMs * 1000;
        if (now > timeSystemLiveAndHanging &&
            now < timeSystemSleepedPastHangTimeout) {
            const auto message = base::StringFormat(
                    "detected a hanging thread '%s'. No response for %d ms",
                    mLooper->name(), (int)((now - *mLastCheckTimeUs) / 1000));
            ++mHangCount;
            l.unlock();

            derror("%s%s", message.c_str(),
                  android::base::IsDebuggerAttached() ? ", ignored (debugger attached)" : "");

            if (mHangCount >= kMaxHangCount &&
                hangCallback && !android::base::IsDebuggerAttached()) {
                hangCallback(message);
            } else {
                // Start another hang check in case something happened to
                // this previous event.
                startHangCheckLocked();
            }
        }
    } else if (now > mLastCheckTimeUs.valueOr(0) + mHangCheckTimeoutMs * 1000) {
        mHangCount = 0;
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

HangDetector::HangDetector(HangCallback&& hangCallback, Timing timing)
    : mHangCallback(std::move(hangCallback)),
      mTiming(timing),
      mWorkerThread([this]() { workerThread(); }) {
    mWorkerThread.start();
}

HangDetector::~HangDetector() {
    stop();
}

void HangDetector::addWatchedLooper(base::Looper* looper) {
    base::AutoLock lock(mLock);
    mLoopers.emplace_back(new LooperWatcher(looper, hangTimeoutMs(),
                                            mTiming.hangCheckTimeoutMs));
    if (!mPaused && !mStopping) {
        mLoopers.back()->startHangCheck();
    }
}

void HangDetector::pause(bool paused) {
    base::AutoLock lock(mLock);
    mPaused = paused;
    if (paused) {
        for (auto&& lw : mLoopers) {
            lw->cancelHangCheck();
        }
    } else {
        mWorkerThreadCv.signalAndUnlock(&lock);
    }
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
    auto nextDeadline = [this]() {
        return base::System::get()->getUnixTimeUs() +
               mTiming.hangLoopIterationTimeoutMs * 1000;
    };
    base::AutoLock lock(mLock);
    for (;;) {
        auto waitUntilUs = nextDeadline();

        while (!mStopping &&
               (base::System::get()->getUnixTimeUs() < waitUntilUs ||
                mPaused)) {
            mWorkerThreadCv.timedWait(&mLock, waitUntilUs);
            auto nowUs = base::System::get()->getUnixTimeUs();
            const int64_t waitedTooLongUs =
                waitUntilUs + mTiming.hangLoopIterationTimeoutMs * 1000;
            if (mPaused ||
                nowUs >= waitedTooLongUs) {
                // If paused, or the condition variable wait
                // took much longer than the hang loop iteration
                // timeout, avoid spinning.
                waitUntilUs = nextDeadline();
            }
        }
        if (mStopping) {
            break;
        }
        if (mPaused) {
            continue;
        }
        for (auto&& lw : mLoopers) {
            lw->process(mHangCallback);
        }

        // Check to see if any of the predicates evaluate to true.
        for (const auto& predicate : mPredicates) {
            if (predicate.first()) {
                const auto message = base::StringFormat(
                        "Failed hang detection predicate: '%s'",
                        predicate.second);

                derror("%s%s", message.c_str(),
                  android::base::IsDebuggerAttached() ? ", ignored (debugger attached)" : "");
                if (mHangCallback && !android::base::IsDebuggerAttached()) {
                    mHangCallback(message);
                }
            }
        }
    }
}
HangDetector& HangDetector::addPredicateCheck(HangPredicate&& predicate,
                                     std::string&& msg) {
    base::AutoLock lock(mLock);
    mPredicates.emplace_back(
            std::make_pair(std::move(predicate), std::move(msg)));
    return *this;
}

 HangDetector& HangDetector::addPredicateCheck(StatefulHangdetector* detector, std::string&& msg) {
     {
        base::AutoLock lock(mLock);
        mRegistered.push_back(std::unique_ptr<StatefulHangdetector>(detector));
     }
     HangPredicate pred = [detector] { return detector->check();};
     return addPredicateCheck([detector] { return detector->check();}, std::move(msg));
 }

base::System::Duration HangDetector::hangTimeoutMs() {
    // x86 and x64 run pretty fast, but other types of images could be really
    // slow - so let's have a longer timeout for those.
    // Note that android_avdInfo is not set in unit tests.
    if (android_avdInfo && avdInfo_is_x86ish(android_avdInfo)) {
        return mTiming.taskProcessingTimeoutMs;
    }
    // something around 100 seconds should be fine.
    return mTiming.taskProcessingTimeoutMs * 7;
}

}  // namespace crashreport
}  // namespace android
