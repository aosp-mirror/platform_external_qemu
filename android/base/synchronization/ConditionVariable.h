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
#include "android/base/system/Win32Utils.h"
#include <vector>
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace android {
namespace base {

// A class that implements a condition variable, which can be used in
// association with a Lock to blocking-wait for specific conditions.
// Useful to implement various synchronization data structures.
class ConditionVariable {
public:
#ifdef _WIN32

    ConditionVariable();
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

    // Like signal(), but wakes all of the waiting threads.
    void broadcast();

    // XP-specific members of the condition variable
    struct XpCV {
        std::vector<Win32Utils::ScopedHandle> mWaiters;
        Lock mLock;
    };

    // Vista-specific members
    struct VistaCV {
        void* data; // this is how CONDITION_VARIABLE is defined in Windows.h
    };

    // A union having either XP or Vista-specific members active
    union Data {
        VistaCV vista;
        XpCV xp;

        Data() {}
        ~Data() {}
    };

private:
    Data mData;

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

}  // namespace base
}  // namespace android
