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

#ifndef ANDROID_BASE_CONDITION_VARIABLE_H
#define ANDROID_BASE_CONDITION_VARIABLE_H

#include "android/base/synchronization/Lock.h"
#include "android/base/containers/PodVector.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace android {
namespace base {

// A class that implements a condition variable, which can be used in
// association with a Lock to blocking-wait for specific conditions.
// Useful to implement various synchronization data structures.
// For an example, see ::emugl::MessageChannel.
class ConditionVariable {
public:
#ifdef _WIN32

    // Default constructor.
    ConditionVariable();

    // Destructor.
    ~ConditionVariable();

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
    void wait(Lock* userLock);

    // Signal that a condition was reached. This will wake at most one
    // waiting thread that is blocked on wait().
    void signal();

private:
    PodVector<HANDLE> mWaiters;
    Lock mLock;

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

private:
    pthread_cond_t mCond;

#endif  // !_WIN32
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_CONDITION_VARIABLE_H
