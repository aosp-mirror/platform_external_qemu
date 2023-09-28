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

#pragma once

#include "aemu/base/Compiler.h"

#include "aemu/base/async/Looper.h"
#include "aemu/base/threads/FunctorThread.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace android {
namespace crashreport {

// Use this interface if your hangdetector needs to
// keep track of state.
class StatefulHangdetector {
public:
    virtual ~StatefulHangdetector(){};
    virtual bool check() = 0;
};

// HangDetector - a class that monitors a set of Loopers and checks if any of
// those is hanging. It calls a user-supplied callback in that case.
//
// HangDetector uses Looper::createTask() to get a task object for each looper
// it watches. Separate thread wakes every couple of seconds to check if
// it needs to schedule a new task on a looper, or, if a task was scheduled for
// a while an didn't finish in time, to call the |hangCallback|.
//
// Note: Be careful with the timing.hangLoopIterationTimeout. Setting it too
// aggressively can prevent the hang detector from functioning properly.
class HangDetector {
    DISALLOW_COPY_AND_ASSIGN(HangDetector);

public:
    struct Timing {
        // Timeout between worker thread's loop iterations.
        const std::chrono::milliseconds hangLoopIterationTimeout;
        // Timeout between hang checks.
        const std::chrono::milliseconds hangCheckTimeout;
    };

    using HangCallback = std::function<void(std::string_view message)>;
    using HangPredicate = std::function<bool()>;

    HangDetector(HangCallback&& hangCallback, Timing timing = defaultTiming());
    ~HangDetector();

    void addWatchedLooper(
            base::Looper* looper,
            std::chrono::milliseconds taskTimeout = std::chrono::seconds(15));

    // We implicitly assume:
    //    predicate() -> []predicate() (if a predicate becomes true, it will
    //    always return true, we only need to infer a system hangs once)
    HangDetector& addPredicateCheck(HangPredicate&& predicate,
                                    std::string&& msg = "");

    // Registers a stateful hangdetector. This class will take ownership of the
    // object
    HangDetector& addPredicateCheck(StatefulHangdetector* detector,
                                    std::string&& msg = "");
    void pause(bool paused);
    void stop();

private:
    void workerThread();

private:
    static constexpr Timing defaultTiming() {
        return {.hangLoopIterationTimeout = std::chrono::seconds(5),
                .hangCheckTimeout = std::chrono::seconds(15)};
    }

    std::chrono::milliseconds hangTimeoutMs();

    // A class that watches a single looper.
    class LooperWatcher;

    const HangCallback mHangCallback;

    const Timing mTiming;
    std::vector<std::unique_ptr<LooperWatcher>> mLoopers;
    std::vector<std::pair<HangPredicate, std::string>> mPredicates;
    std::vector<std::unique_ptr<StatefulHangdetector>> mRegistered;

    int mPaused = 0;
    bool mStopping = false;
    std::mutex mLock;
    std::condition_variable mWorkerThreadCv;

    // A separate worker thread so it's not affected if anything hangs.
    base::FunctorThread mWorkerThread;
};

}  // namespace crashreport
}  // namespace android
