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

#include "aemu/base/Debug.h"
#include "aemu/base/StringFormat.h"
#include "android/console.h"
#include "android/utils/debug.h"

#include <chrono>
#include <optional>
#include <utility>

namespace android {
namespace crashreport {

class HangDetector::LooperWatcher {
    DISALLOW_COPY_AND_ASSIGN(LooperWatcher);

public:
    LooperWatcher(base::Looper* looper,
                  std::chrono::milliseconds hangTimeout,
                  std::chrono::milliseconds hangCheckTimeout);
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

    std::chrono::high_resolution_clock::time_point mLastCheckTime;
    const std::chrono::milliseconds mTimeout;
    const std::chrono::milliseconds mhangCheckTimeout;
    std::unique_ptr<std::mutex> mLock{new std::mutex()};

    static constexpr int kMaxHangCount = 2;
    int mHangCount = 0;
};

HangDetector::LooperWatcher::LooperWatcher(
        base::Looper* looper,
        std::chrono::milliseconds hangTimeout,
        std::chrono::milliseconds hangCheckTimeout)
    : mLooper(looper),
      mTimeout(hangTimeout),
      mhangCheckTimeout(hangCheckTimeout) {}

HangDetector::LooperWatcher::~LooperWatcher() {
    if (mTask) {
        mTask->cancel();
    }
}

void HangDetector::LooperWatcher::startHangCheck() {
    std::unique_lock<std::mutex> l(*mLock);

    startHangCheckLocked();
}

void HangDetector::LooperWatcher::cancelHangCheck() {
    std::unique_lock<std::mutex> l(*mLock);

    if (mTask) {
        mTask->cancel();
    }
    mIsTaskRunning = false;
    mLastCheckTime = std::chrono::high_resolution_clock::now();
}

void HangDetector::LooperWatcher::process(const HangCallback& hangCallback) {
    std::unique_lock<std::mutex> l(*mLock);

    const auto now = std::chrono::high_resolution_clock::now();
    if (mIsTaskRunning) {
        // Heuristic: If the looper watcher itself took much longer than
        // mTimeout to fire again, it's possible there was a system-wide
        // sleep. In this case, don't count that as hanging.
        // Note that we are using std::chrono to make sure all the durations
        // are scaled appropriately.
        const auto timeSystemLiveAndHanging = mLastCheckTime + mTimeout;
        const auto timeSystemSleepedPastHangTimeout =
                mLastCheckTime + 2 * mTimeout;
        if (now > timeSystemLiveAndHanging &&
            now < timeSystemSleepedPastHangTimeout) {
            std::chrono::milliseconds timePassed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - mLastCheckTime);
            const auto message = base::StringFormat(
                    "detected a hanging thread '%s'. No response for %d ms",
                    mLooper->name().data(), timePassed.count());
            ++mHangCount;
            mLock->unlock();

            derror("%s%s", message,
                   android::base::IsDebuggerAttached()
                           ? ", ignored (debugger attached)"
                           : "");

            if (mHangCount >= kMaxHangCount && hangCallback &&
                !android::base::IsDebuggerAttached()) {
                hangCallback(message);
            } else {
                // Start another hang check in case something happened to
                // this previous event.
                startHangCheckLocked();
            }
        }
    } else if (now > mLastCheckTime + mhangCheckTimeout) {
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
    mLastCheckTime = std::chrono::high_resolution_clock::now();
}

void HangDetector::LooperWatcher::taskComplete() {
    std::unique_lock<std::mutex> l(*mLock);
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

void HangDetector::addWatchedLooper(base::Looper* looper,
                                    std::chrono::milliseconds taskTimeout) {
    std::unique_lock<std::mutex> l(mLock);
    mLoopers.emplace_back(
            new LooperWatcher(looper, taskTimeout, mTiming.hangCheckTimeout));
    if (!mPaused && !mStopping) {
        mLoopers.back()->startHangCheck();
    }
}

void HangDetector::pause(bool paused) {
    std::unique_lock<std::mutex> l(mLock);
    if (paused) {
        mPaused++;
    } else {
        mPaused--;
    }
    // Only the first pause and last resume will actually trigger the
    // pause/resume.
    if (paused && mPaused == 1) {
        for (auto&& lw : mLoopers) {
            lw->cancelHangCheck();
        }
    } else if (mPaused == 0) {
        mWorkerThreadCv.notify_one();
    }
}

void HangDetector::stop() {
    {
        std::unique_lock<std::mutex> l(mLock);
        for (auto&& lw : mLoopers) {
            lw->cancelHangCheck();
        }
        mStopping = true;
        mWorkerThreadCv.notify_one();
    }
    mWorkerThread.wait();
}

void HangDetector::workerThread() {
    auto nextDeadline = [this]() {
        return std::chrono::high_resolution_clock::now() +
               mTiming.hangLoopIterationTimeout;
    };
    std::unique_lock<std::mutex> l(mLock);
    for (;;) {
        auto waitUntil = nextDeadline();

        while (!mStopping &&
               (std::chrono::high_resolution_clock::now() < waitUntil ||
                mPaused)) {
            mWorkerThreadCv.wait_until(l, waitUntil);
            auto nowUs = std::chrono::high_resolution_clock::now();
            auto waitedTooLongUs = waitUntil + mTiming.hangLoopIterationTimeout;
            if (mPaused || nowUs >= waitedTooLongUs) {
                // If paused, or the condition variable wait
                // took much longer than the hang loop iteration
                // timeout, avoid spinning.
                waitUntil = nextDeadline();
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

                derror("%s%s", message,
                       android::base::IsDebuggerAttached()
                               ? ", ignored (debugger attached)"
                               : "");
                if (mHangCallback && !android::base::IsDebuggerAttached()) {
                    mHangCallback(message);
                }
            }
        }
    }
}
HangDetector& HangDetector::addPredicateCheck(HangPredicate&& predicate,
                                              std::string&& msg) {
    std::unique_lock<std::mutex> l(mLock);
    mPredicates.emplace_back(
            std::make_pair(std::move(predicate), std::move(msg)));
    return *this;
}

HangDetector& HangDetector::addPredicateCheck(StatefulHangdetector* detector,
                                              std::string&& msg) {
    {
        std::unique_lock<std::mutex> l(mLock);
        mRegistered.push_back(std::unique_ptr<StatefulHangdetector>(detector));
    }
    HangPredicate pred = [detector] { return detector->check(); };
    return addPredicateCheck([detector] { return detector->check(); },
                             std::move(msg));
}

}  // namespace crashreport
}  // namespace android
