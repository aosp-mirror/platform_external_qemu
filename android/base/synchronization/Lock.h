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

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <assert.h>

namespace android {
namespace base {

// Simple wrapper class for mutexes.
class Lock {
public:
    // Constructor.
    Lock() {
#ifdef _WIN32
        ::InitializeCriticalSection(&mLock);
#else
        ::pthread_mutex_init(&mLock, NULL);
#endif
    }

    // Destructor.
    ~Lock() {
#ifdef _WIN32
        ::DeleteCriticalSection(&mLock);
#else
        ::pthread_mutex_destroy(&mLock);
#endif
    }

    // Acquire the lock.
    void lock() {
#ifdef _WIN32
      ::EnterCriticalSection(&mLock);
#else
      ::pthread_mutex_lock(&mLock);
#endif
    }

    // Release the lock.
    void unlock() {
#ifdef _WIN32
       ::LeaveCriticalSection(&mLock);
#else
       ::pthread_mutex_unlock(&mLock);
#endif
    }

private:
    friend class ConditionVariable;

#ifdef _WIN32
    CRITICAL_SECTION mLock;
#else
    pthread_mutex_t mLock;
#endif
    DISALLOW_COPY_AND_ASSIGN(Lock);
};

// Helper class to lock / unlock a mutex automatically on scope
// entry and exit.
// NB: not thread-safe (as opposed to the Lock class)
class AutoLock {
public:
    AutoLock(Lock& lock) : mLock(lock) {
        mLock.lock();
    }

    void unlock() {
        assert(mLocked);
        mLock.unlock();
        mLocked = false;
    }

    ~AutoLock() {
        if (mLocked) {
            mLock.unlock();
        }
    }

private:
    Lock& mLock;
    bool mLocked = true;
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

}  // namespace base
}  // namespace android
