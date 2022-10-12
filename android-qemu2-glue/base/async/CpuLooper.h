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

#include "aemu/base/async/Looper.h"

#include <string>
#include <string_view>

namespace android {
namespace qemu {

// CpuLooper - an implementation of android::base::Looper for a vCPU thread.
//
// CpuLooper uses QEMU functions for vCPU controls to run tasks inside the
// dedicated threads assigned to emulated CPUs.
// As of now the only implemented Looper's API are Looper::createTask(), which
// returns a Task object that runs a callback asynchronously on the CPU thread,
// and Looper::scheduleCallback(), to do the same thing without a separate Task
// object.
//
// Running a function inside the vCPU thread means that CPU needs to stop
// running guest commands, exit the hypervisor and run host's scheduled
// function. That's why:
//  - Try to not do any real work in the callback. For the guest it has the
//    same effects as having an interrupt handler or signal handler running;
//    treat your callback the same way.
//
//  - Don't run it more often than you need. This kind of interrupt flushes
//    CPU cache and will page out at least some of the guest's memory - so it's
//    a noticeable performance hit even for a noop callback.
//
// The rest of Looper members are just stubs returning 0 or nullptr. Add real
// code there only if there's a specific use case.
//
class CpuLooper : public android::base::Looper {
    class TaskImpl;

public:
    CpuLooper(int cpuNo);

    std::string_view name() const override;

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

    TaskPtr createTask(TaskCallback&& callback) override;
    void scheduleCallback(TaskCallback&& callback) override;

private:
    const int mCpuNo;
    const std::string mName;
};

}  // namespace qemu
}  // namespace android
