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
#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/base/async/Looper.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include <memory>
#include <vector>

namespace android {
namespace crashreport {

class HangDetector {
    DISALLOW_COPY_AND_ASSIGN(HangDetector);

public:
    HangDetector(bool crashOnHangs);
    ~HangDetector();

    void addWatchedLooper(base::Looper* looper);
    void stop();

private:
    void workerThread();

private:
    // Timeout between hang loop iterations.
    static constexpr base::System::Duration kHangLoopIterationTimeoutMs =
            5 * 1000;
    // How long is it OK to process a task before we consider it hanging.
    static constexpr base::System::Duration kTaskProcessingTimeoutMs =
            10 * 1000;
    // Timeout between hang checks.
    static constexpr base::System::Duration kHangCheckTimeoutMs = 5 * 1000;

    // A class that watches a single looper.
    class LooperWatcher {
        DISALLOW_COPY_AND_ASSIGN(LooperWatcher);

    public:
        LooperWatcher(base::Looper* looper);
        ~LooperWatcher();

        LooperWatcher(LooperWatcher&&) = default;
        LooperWatcher& operator=(LooperWatcher&&) = default;

        void startHangCheck();
        void cancelHangCheck();
        void process(bool crashOnHang);

    private:
        void startHangCheckLocked();
        void taskComplete();

        base::Looper* const mLooper;
        std::unique_ptr<base::Looper::Task> mTask;
        bool mIsTaskRunning = false;

        base::Optional<base::System::Duration> mLastCheckTimeUs;

        std::unique_ptr<base::Lock> mLock{new base::Lock()};
    };

    std::vector<std::shared_ptr<LooperWatcher>> mLoopers;
    const bool mCrashOnHangs;

    // A separate worker thread so it's not affected if anything hangs.
    base::FunctorThread mWorkerThread;

    bool mStopping = false;
    base::Lock mLock;
    base::ConditionVariable mWorkerThreadCv;
};

}  // namespace crashreport
}  // namespace android
