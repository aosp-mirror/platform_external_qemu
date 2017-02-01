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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <assert.h>

namespace android {
namespace base {

// A class that implements a condition variable, which can be used in
// association with a Lock to blocking-wait for specific conditions.
// Useful to implement various synchronization data structures.
class ConditionVariable {
public:
    // A set of functions to efficiently unlock the lock used with
    // the current condition variable and signal or broadcast it.
    //
    // The functions are needed because on some platforms (Posix) it's more
    // efficient to signal the variable before unlocking mutex, while on others
    // (Windows) it's exactly the opposite. Functions implement the best way
    // for each platform and abstract it out from the user.
    void signalAndUnlock(Lock* lock);
    void signalAndUnlock(AutoLock* lock);

    void broadcastAndUnlock(Lock* lock);
    void broadcastAndUnlock(AutoLock* lock);

    void wait(AutoLock* userLock) {
        assert(userLock->mLocked);
        wait(&userLock->mLock);
    }

#ifdef _WIN32

    ConditionVariable() {
        ::InitializeConditionVariable(&mCond);
    }

    // There's no special function to destroy CONDITION_VARIABLE in Windows.
    ~ConditionVariable() = default;

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
    void wait(Lock* userLock) {
        ::SleepConditionVariableSRW(&mCond, &userLock->mLock, INFINITE, 0);
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

private:
    CONDITION_VARIABLE mCond;

#else  // !_WIN32

    // Note: on Posix systems, make it a naive wrapper around pthread_cond_t.

    ConditionVariable() {
        pthread_cond_init(&mCond, NULL);
    }

    ~ConditionVariable() {
        pthread_cond_destroy(&mCond);
    }

    void wait(Lock* userLock) {
        pthread_cond_wait(&mCond, &userLock->mLock);
    }

    void signal() {
        pthread_cond_signal(&mCond);
    }

    void broadcast() {
        pthread_cond_broadcast(&mCond);
    }

private:
    pthread_cond_t mCond;

#endif  // !_WIN32

    DISALLOW_COPY_ASSIGN_AND_MOVE(ConditionVariable);
};

#ifdef _WIN32
inline void ConditionVariable::signalAndUnlock(Lock* lock) {
    lock->unlock();
    signal();
}
inline void ConditionVariable::signalAndUnlock(AutoLock* lock) {
    lock->unlock();
    signal();
}

inline void ConditionVariable::broadcastAndUnlock(Lock* lock) {
    lock->unlock();
    broadcast();
}
inline void ConditionVariable::broadcastAndUnlock(AutoLock* lock) {
    lock->unlock();
    broadcast();
}
#else  // !_WIN32
inline void ConditionVariable::signalAndUnlock(Lock* lock) {
    signal();
    lock->unlock();
}
inline void ConditionVariable::signalAndUnlock(AutoLock* lock) {
    signal();
    lock->unlock();
}
inline void ConditionVariable::broadcastAndUnlock(Lock* lock) {
    broadcast();
    lock->unlock();
}
inline void ConditionVariable::broadcastAndUnlock(AutoLock* lock) {
    broadcast();
    lock->unlock();
}
#endif  // !_WIN32

}  // namespace base
}  // namespace android
