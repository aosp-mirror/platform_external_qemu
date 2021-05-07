// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/threads/internal/ParallelTaskBase.h"

namespace android {
namespace base {
namespace internal {

ParallelTaskBase::ParallelTaskBase(Looper* looper, ThreadFlags flags)
    : mLooper(looper),
      mManagedThread(this, flags) {}

bool ParallelTaskBase::start() {
    if (!mManagedThread.start()) {
        return false;
    }

    isRunning = true;
    // A timer is required for a looper to schedule a ParallelTask
    mTimer.reset(mLooper->createTimer(&tryWaitTillJoinedStatic, this));
    mTimer->startRelative(INT32_MAX);
    return true;
}

bool ParallelTaskBase::inFlight() const {
    return isRunning;
}

void ParallelTaskBase::scheduleTaskDone() {
    std::unique_ptr<android::base::Looper::Timer>
        t(mLooper->createTimer(&runTaskDoneStatic, this));

    mTimer->stop();
    mTimer = std::move(t);
    mTimer->startRelative(0);
}

// static
void ParallelTaskBase::tryWaitTillJoinedStatic(void* opaqueThis,
                                               Looper::Timer* timer) {
    static_cast<ParallelTaskBase*>(opaqueThis)->tryWaitTillJoined(timer);
}

void ParallelTaskBase::tryWaitTillJoined(Looper::Timer* timer) {
    if (!mManagedThread.tryWait(nullptr)) {
        timer->startRelative(INT32_MAX);
    }
}

void ParallelTaskBase::runTaskDoneStatic(void* opaqueThis,
                                         Looper::Timer* timer) {
    static_cast<ParallelTaskBase*>(opaqueThis)->runTaskDone(timer);
}

void ParallelTaskBase::runTaskDone(Looper::Timer* timer) {
    isRunning = false;
    taskDoneImpl();
}

}  // namespace internal
}  // namespace base
}  // namespace android
