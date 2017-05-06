// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/async/Looper.h"

#include <atomic>
#include <memory>
#include <string>

// An implementation of android::base::Looper for vCPU loop.
namespace android {
namespace qemu {

class CpuLooper : public android::base::Looper {
public:
    CpuLooper(int cpuNo);

    base::StringView name() const override;

    Duration nowMs(ClockType clockType) override;
    DurationNs nowNs(ClockType clockType) override;
    int runWithDeadlineMs(Duration deadlineMs) override;
    void forceQuit() override;
    Timer* createTimer(Timer::Callback callback,
                       void* opaque,
                       ClockType clock) override;
    FdWatch* createFdWatch(int fd,
                           FdWatch::Callback callback,
                           void* opaque) override;

    class TaskImpl : public Looper::Task {
    public:
        TaskImpl(Looper* looper,
                 TaskCallback&& callback,
                 int cpuNo);

        void schedule() override;
        void cancel() override;

    private:
        int mCpuNo;

        // A separate dynamically-allocated struct for the scheduled callback
        // to make sure user doesn't delete it in the middle of a call.
        struct CallData {
            TaskCallback callback;
            std::shared_ptr<CallData> self;
            std::atomic<bool> canceled;
        };
        std::shared_ptr<CallData> mCallData;
    };

    TaskPtr createTask(TaskCallback&& callback) override;
    void scheduleCallback(TaskCallback&& callback) override;

private:
    const int mCpuNo;
    const std::string mName;
};

}
}
