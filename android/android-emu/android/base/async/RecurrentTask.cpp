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
#include "android/base/async/RecurrentTask.h"

#include "android/base/async/Looper.h"

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/memory/ScopedPtr.h"

#include <memory>

namespace android {
namespace base {

class RecurrentTask::Impl {
public:

    Impl(Looper* looper,
            TaskFunction function,
            RecurrentTask::Duration taskIntervalMs)
        : mLooper(looper),
        mFunction(function),
        mTaskIntervalMs(int(taskIntervalMs)),
        mTimer(mLooper->createTimer(&Impl::taskCallback, this)) {}

    ~Impl() { stopAndWait(); }

    void start(bool runImmediately = false) {
        {
            AutoLock lock(mLock);
            mInFlight = true;
        }
        mTimer->startRelative(runImmediately ? 0 : mTaskIntervalMs);
    }

    void stopAsync() {
        mTimer->stop();
    
        AutoLock lock(mLock);
        mInFlight = false;
    }
    
    void stopAndWait() {
        mTimer->stop();
    
        AutoLock lock(mLock);
        mInFlight = false;
    
        // Make sure we wait for the pending task to complete if it was running.
        while (mInTimerCallback) {
            mInTimerCondition.wait(&lock);
        }
    }

    bool inFlight() const {
        AutoLock lock(mLock);
        return mInFlight;
    }

    void waitUntilRunning() {
        AutoLock lock(mLock);
        while (mInFlight && !mInTimerCallback) {
            mInTimerCondition.wait(&lock);
        }
    }

    Duration taskIntervalMs() const { return mTaskIntervalMs; }

    static void taskCallback(void* opaqueThis, Looper::Timer* timer) {
        const auto self = static_cast<Impl*>(opaqueThis);
    
        AutoLock lock(self->mLock);
        self->mInTimerCallback = true;
        const bool inFlight = self->mInFlight;
        self->mInTimerCondition.broadcastAndUnlock(&lock);
    
        const auto undoInTimerCallback =
                makeCustomScopedPtr(self, [&lock](Impl* self) {
                    if (!lock.isLocked()) {
                        lock.lock();
                    }
                    self->mInTimerCallback = false;
                    self->mInTimerCondition.broadcastAndUnlock(&lock);
                });
    
        if (!inFlight) {
            return;
        }
    
        const bool callbackResult = self->mFunction();
    
        lock.lock();
        if (!callbackResult) {
            self->mInFlight = false;
            return;
        }
        // It is possible that the client code in |mFunction| calls |stop|, so
        // we must double check before reposting the task.
        if (!self->mInFlight) {
            return;
        }
        lock.unlock();
        self->mTimer->startRelative(self->mTaskIntervalMs);
    }


    Looper* const mLooper;
    const TaskFunction mFunction;
    const int mTaskIntervalMs;
    bool mInTimerCallback = false;
    bool mInFlight = false;
    const std::unique_ptr<Looper::Timer> mTimer;

    mutable Lock mLock;
    ConditionVariable mInTimerCondition;
};

RecurrentTask::RecurrentTask(Looper* looper,
              TaskFunction function,
              RecurrentTask::Duration taskIntervalMs)
    : mImpl(new RecurrentTask::Impl(looper, function, taskIntervalMs)) { }

RecurrentTask::~RecurrentTask() {
    delete mImpl;
}

void RecurrentTask::start(bool runImmediately) {
    mImpl->start(runImmediately);
}

void RecurrentTask::stopAsync() {
    mImpl->stopAsync();
}

void RecurrentTask::stopAndWait() {
    mImpl->stopAndWait();
}

bool RecurrentTask::inFlight() const {
    return mImpl->inFlight();
}

void RecurrentTask::waitUntilRunning() {
    return mImpl->waitUntilRunning();
}

RecurrentTask::Duration
RecurrentTask::taskIntervalMs() const {
    return mImpl->taskIntervalMs();;
}

}  // namespace base
}  // namespace android
