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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android-qemu2-glue/base/async/CpuLooper.h"

#include "aemu/base/StringFormat.h"

#include <atomic>
#include <memory>
#include <string_view>
#include <utility>

// These need to go after all STL headers, as they define some macros that make
// STL go crazy.
extern "C" {
#include "qemu/osdep.h"
#include "qom/cpu.h"
}

namespace android {
namespace qemu {

CpuLooper::CpuLooper(int cpuNo)
    : mCpuNo(cpuNo), mName(base::StringFormat("QEMU2 CPU%d thread", cpuNo)) {}

std::string_view CpuLooper::name() const {
    return mName;
}

base::Looper::Duration CpuLooper::nowMs(base::Looper::ClockType clockType) {
    return 0;
}

base::Looper::DurationNs CpuLooper::nowNs(base::Looper::ClockType clockType) {
    return 0;
}

int CpuLooper::runWithDeadlineMs(base::Looper::Duration deadlineMs) {
    return 0;
}

void CpuLooper::forceQuit() {}

base::Looper::Timer* CpuLooper::createTimer(
        base::Looper::Timer::Callback callback,
        void* opaque,
        base::Looper::ClockType clock) {
    return nullptr;
}

base::Looper::FdWatch* CpuLooper::createFdWatch(
        int fd,
        base::Looper::FdWatch::Callback callback,
        void* opaque) {
    return nullptr;
}

class CpuLooper::TaskImpl final : public base::Looper::Task {
public:
    TaskImpl(base::Looper* looper,
             CpuLooper::TaskCallback&& callback,
             int cpuNo);

    void schedule() override;
    void cancel() override;

private:
    int mCpuNo;

    // A separate dynamically-allocated struct for the scheduled callback
    // to make sure user doesn't delete it in the middle of a call.
    // It has a shared pointer to itself so even if the parent Task object
    // dies CallData lives until the end of the callback execution. Only
    // then CallData clears |self| and gets deleted.
    struct CallData {
        CpuLooper::TaskCallback callback;
        std::shared_ptr<CallData> self;
        std::atomic<bool> canceled;
    };
    std::shared_ptr<CallData> mCallData;
};

base::Looper::TaskPtr CpuLooper::createTask(
        base::Looper::TaskCallback&& callback) {
    return TaskPtr{new TaskImpl(this, std::move(callback), mCpuNo)};
}

void CpuLooper::scheduleCallback(base::Looper::TaskCallback&& callback) {
    TaskImpl(this, std::move(callback), mCpuNo).schedule();
}

CpuLooper::TaskImpl::TaskImpl(base::Looper* looper,
                              base::Looper::TaskCallback&& callback,
                              int cpuNo)
    : Task(looper, {}), mCpuNo(cpuNo), mCallData(std::make_shared<CallData>()) {
    mCallData->callback = std::move(callback);
}

void CpuLooper::TaskImpl::schedule() {
    mCallData->canceled = false;
    // Make sure callData lives while it's scheduled.
    mCallData->self = mCallData;

    auto cpu = qemu_get_cpu(mCpuNo);
    run_on_cpu_data data = {.host_ptr = mCallData.get()};
    async_run_on_cpu(cpu,
                     [](CPUState* cpu, run_on_cpu_data data) {
                         auto self = static_cast<CallData*>(data.host_ptr);
                         if (!self->canceled) {
                             self->callback();
                         }
                         self->self.reset();
                     },
                     data);
}

void CpuLooper::TaskImpl::cancel() {
    mCallData->canceled = true;
}

}  // namespace qemu
}  // namespace android
