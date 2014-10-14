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

#include "android/base/threads/Thread.h"

#include "android/base/threads/ThreadStore.h"

#include <stdio.h>

namespace android {
namespace base {

namespace {

// Helper class to automatically lock / unlock a mutex on scope enter/exit.
// Equivalent to android::base::AutoLock, but avoid using it to reduce
// coupling.
class ScopedLocker {
public:
    ScopedLocker(pthread_mutex_t* mutex) : mLock(mutex) {
        pthread_mutex_lock(mLock);
    }

    ~ScopedLocker() {
        pthread_mutex_unlock(mLock);
    }
private:
    pthread_mutex_t* mLock;
};

}  // namespace

Thread::Thread() :
    mThread((pthread_t)NULL),
    mExitStatus(0),
    mIsRunning(false) {
    pthread_mutex_init(&mLock, NULL);
}

Thread::~Thread() {
    pthread_mutex_destroy(&mLock);
}

bool Thread::start() {
    bool ret = true;
    pthread_mutex_lock(&mLock);
    mIsRunning = true;
    if (pthread_create(&mThread, NULL, thread_main, this)) {
        ret = false;
        mIsRunning = false;
    }
    pthread_mutex_unlock(&mLock);
    return ret;
}

bool Thread::wait(intptr_t *exitStatus) {
    {
        ScopedLocker locker(&mLock);
        if (!mIsRunning) {
            // Thread already stopped.
            if (exitStatus) {
                *exitStatus = mExitStatus;
            }
            return true;
        }
    }

    // NOTE: Do not hold the lock when waiting for the thread to ensure
    // it can update mIsRunning and mExitStatus properly in thread_main
    // without blocking.
    void *retval;
    if (pthread_join(mThread, &retval)) {
        return false;
    }
    if (exitStatus) {
        *exitStatus = (intptr_t)retval;
    }
    return true;
}

bool Thread::tryWait(intptr_t *exitStatus) {
    ScopedLocker locker(&mLock);
    if (!mIsRunning) {
        return false;
    }
    if (exitStatus) {
        *exitStatus = mExitStatus;
    }
    return true;
}

// static
void* Thread::thread_main(void *arg) {
    Thread* self = reinterpret_cast<Thread*>(arg);
    intptr_t ret = self->main();

    pthread_mutex_lock(&self->mLock);
    self->mIsRunning = false;
    self->mExitStatus = ret;
    pthread_mutex_unlock(&self->mLock);

    ::android::base::ThreadStoreBase::OnThreadExit();

    return (void*)ret;
}

}  // namespace base
}  // namespace android
