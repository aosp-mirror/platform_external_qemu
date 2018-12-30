// Copyright (C) 2014 The Android Open Source Project
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
#include "android/base/synchronization/ConditionVariable.h"

#include "android/base/AlignedBuf.h"
#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#ifdef _WIN32
#include "android/base/system/System.h"
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <assert.h>

namespace android {
namespace base {

class ConditionVariable::Impl {
public:
    Impl() {
#ifdef _WIN32
        ::InitializeConditionVariable(&mCond);
#else
        pthread_cond_init(&mCond, NULL);
#endif
    }

    ~Impl() {
#ifndef _WIN32
        // There's no special function to destroy CONDITION_VARIABLE in Windows.
#else
        pthread_cond_destroy(&mCond);
#endif
    }

    void wait(AutoLock* userLock) {
        assert(userLock->mLocked);
        wait(&userLock->mLock);
    }

#ifdef _WIN32
    // Wait until the condition variable is signaled. Note that spurious
    // wakeups are always a possibility, so always check the condition
    // in a loop, i.e. do:
    //
    //    while (!condition) { condVar.wait(&lock); }
    //
    // instead of:
    //
    //    if (!condition) { condVar.wait(&lock); }
    //
    void wait(StaticLock* userLock) {
        ::SleepConditionVariableSRW(&mCond, (SRWLOCK*)userLock->getPlatformLock(), INFINITE, 0);
    }

    bool timedWait(StaticLock *userLock, WaitDeadline waitUntilUs) {
        const auto now = System::get()->getUnixTimeUs();
        const auto timeout =
                std::max<System::Duration>(0, (System::Duration)waitUntilUs  - now) / 1000;
        return ::SleepConditionVariableSRW(
                    &mCond, (SRWLOCK*)userLock->getPlatformLock(), timeout, 0) != 0;
    }

    // Signal that a condition was reached. This will wake at least (and
    // preferrably) one waiting thread that is blocked on wait().
    void signal() {
        ::WakeConditionVariable(&mCond);
    }

    // Like signal(), but wakes all of the waiting threads.
    void broadcast() {
        ::WakeAllConditionVariable(&mCond);
    }

#else  // !_WIN32

    // Note: on Posix systems, make it a naive wrapper around pthread_cond_t.
    void wait(StaticLock* userLock) {
        pthread_cond_wait(&mCond, (pthread_mutex_t*)userLock->getPlatformLock());
    }

    bool timedWait(StaticLock* userLock, WaitDeadline waitUntilUs) {
        timespec abstime;
        abstime.tv_sec = waitUntilUs / 1000000LL;
        abstime.tv_nsec = (waitUntilUs % 1000000LL) * 1000;
        return pthread_cond_timedwait(&mCond, (pthread_mutex_t*)userLock->getPlatformLock(), &abstime) == 0;
    }

    void signal() {
        pthread_cond_signal(&mCond);
    }

    void broadcast() {
        pthread_cond_broadcast(&mCond);
    }
#endif  // !_WIN32

private:

#ifdef _WIN32
    CONDITION_VARIABLE mCond;
#else
    pthread_cond_t mCond;
#endif
};

ConditionVariable::ConditionVariable() {
    void* storage = aligned_buf_alloc(64, sizeof(Impl));
    mImpl = new(storage) Impl;
}

ConditionVariable::~ConditionVariable() {
    mImpl->Impl::~Impl();
    aligned_buf_free(mImpl);
}

void ConditionVariable::wait(AutoLock* userLock) {
    mImpl->wait(userLock);
}

void ConditionVariable::wait(StaticLock* userLock) {
    mImpl->wait(userLock);
}

bool ConditionVariable::timedWait(StaticLock *userLock, WaitDeadline waitUntilUs) {
    return mImpl->timedWait(userLock, waitUntilUs);
}

void ConditionVariable::signal() {
    mImpl->signal();
}

void ConditionVariable::broadcast() {
    mImpl->broadcast();
}

}  // namespace base
}  // namespace android
