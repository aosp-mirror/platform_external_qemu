// Copyright 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "aemu/base/threads/FunctorThread.h"
#include "aemu/base/synchronization/MessageChannel.h"

#include <inttypes.h>
#include <functional>

// A thread that regularly processes tasks that need to happen at some vsync
// interval (initialized from FrameBuffer).
//
// Some examples of such tasks include
//
// 1. Signaling guest-side hwc present syncfd's at a regular interval so SF can
// properly schedule presents
// 2.
class VsyncThread {
public:
    using Count = uint64_t;
    using VsyncTask = std::function<void(Count)>;
    VsyncThread(uint64_t vsyncPeriod);
    ~VsyncThread();

    // Runs specificed |task| at the next vsync time as specified by the vsync
    // period. Passes the number of vsyncs so far (Count).
    void schedule(VsyncTask task);

    uint64_t getCount() const {
        return mCount;
    }

    uint64_t getPeriod() const {
        return mPeriodNs;
    }

    // Sets the period dynamically. This is implemented as another scheduled task, so as to not
    // interfere with assumptions of previously scheduled tasks.
    void setPeriod(uint64_t newPeriod);

private:

    enum class CommandType {
        Default = 0,
        Exit = 1,
        ChangePeriod = 2,
    };

    struct VsyncThreadCommand {
        CommandType type;
        VsyncTask task;
        uint64_t newPeriod;
    };

    void exit();
    void threadFunc();

    uint64_t mPeriodNs = 0;
    uint64_t mCount = 0;
    bool mExiting = false;
    android::base::MessageChannel<VsyncThreadCommand, 128> mChannel;
    android::base::FunctorThread mThread;
};
