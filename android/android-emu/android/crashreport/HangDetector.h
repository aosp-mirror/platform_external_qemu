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

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/StringView.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include <functional>
#include <memory>
#include <vector>

namespace android {
namespace crashreport {

// HangDetector - a class that monitors a set of Loopers and checks if any of
// those is hanging. It calls a user-supplied callback in that case.
//
// HangDetector uses Looper::createTask() to get a task object for each looper
// it watches. Separate thread wakes every couple of seconds to check if
// it needs to schedule a new task on a looper, or, if a task was scheduled for
// a while an didn't finish in time, to call the |hangCallback|.
//
class HangDetector {
    DISALLOW_COPY_AND_ASSIGN(HangDetector);

public:
    using HangCallback = std::function<void(base::StringView message)>;

    HangDetector(HangCallback&& hangCallback);
    ~HangDetector();

    void addWatchedLooper(base::Looper* looper);
    void stop();

private:
    void workerThread();

private:
    // Timeout between worker thread's loop iterations.
    static constexpr base::System::Duration kHangLoopIterationTimeoutMs =
            5 * 1000;
    // How long is it OK to process a task before we consider it hanging.
    static constexpr base::System::Duration kTaskProcessingTimeoutMs =
            15 * 1000;
    // Timeout between hang checks.
    static constexpr base::System::Duration kHangCheckTimeoutMs = 15 * 1000;

    // A class that watches a single looper.
    class LooperWatcher;

    const HangCallback mHangCallback;

    std::vector<std::unique_ptr<LooperWatcher>> mLoopers;
    bool mStopping = false;
    base::Lock mLock;
    base::ConditionVariable mWorkerThreadCv;

    // A separate worker thread so it's not affected if anything hangs.
    base::FunctorThread mWorkerThread;
};

}  // namespace crashreport
}  // namespace android
