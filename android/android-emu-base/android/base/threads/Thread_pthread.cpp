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

#include "android/base/Log.h"
#include "android/base/threads/ThreadStore.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace android {
namespace base {

Thread::Thread(ThreadFlags flags, int stackSize)
    : mThread((pthread_t)NULL), mStackSize(stackSize), mFlags(flags) {}

Thread::~Thread() {
    assert(!mStarted || mFinished);
    if ((mFlags & ThreadFlags::Detach) == ThreadFlags::NoFlags && mStarted &&
        !mJoined) {
        // Make sure we reclaim the OS resources.
        pthread_join(mThread, nullptr);
    }
}

bool Thread::start() {
    if (mStarted) {
        return false;
    }

    bool ret = true;
    mStarted = true;

    const auto useAttributes = mStackSize != 0;

    pthread_attr_t attr;
    if (useAttributes) {
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, mStackSize);
    }

    if (pthread_create(&mThread, mStackSize ? &attr : nullptr, thread_main,
                       this)) {
        LOG(ERROR) << "Thread: failed to create a thread, errno " << errno;
        ret = false;
        // We _do not_ need to guard this access to |mFinished| because we're
        // sure that the launched thread failed, so there can't be parallel
        // access.
        mFinished = true;
        mExitStatus = -errno;
        // Nothing to join, so technically it's joined.
        mJoined = true;
    }

    if (useAttributes) {
        pthread_attr_destroy(&attr);
    }

    return ret;
}

bool Thread::wait(intptr_t* exitStatus) {
    if (!mStarted || (mFlags & ThreadFlags::Detach) != ThreadFlags::NoFlags) {
        return false;
    }

    // NOTE: Do not hold the lock when waiting for the thread to ensure
    // it can update mFinished and mExitStatus properly in thread_main
    // without blocking.
    if (!mJoined && pthread_join(mThread, NULL)) {
        return false;
    }
    mJoined = true;

    if (exitStatus) {
        *exitStatus = mExitStatus;
    }
    return true;
}

bool Thread::tryWait(intptr_t* exitStatus) {
    if (!mStarted || (mFlags & ThreadFlags::Detach) != ThreadFlags::NoFlags) {
        return false;
    }

    {
        AutoLock locker(mLock);
        if (!mFinished) {
            return false;
        }
    }

    if (!mJoined) {
        if (pthread_join(mThread, NULL)) {
            LOG(WARNING) << "Thread: failed to join a finished thread, errno "
                         << errno;
        }
        mJoined = true;
    }

    if (exitStatus) {
        *exitStatus = mExitStatus;
    }
    return true;
}

// static
void* Thread::thread_main(void* arg) {
    intptr_t ret;

    {
        Thread* self = reinterpret_cast<Thread*>(arg);
        if ((self->mFlags & ThreadFlags::MaskSignals) != ThreadFlags::NoFlags) {
            Thread::maskAllSignals();
        }

        if ((self->mFlags & ThreadFlags::Detach) != ThreadFlags::NoFlags) {
            if (pthread_detach(pthread_self())) {
                // This only means a slow memory leak, so use VERBOSE.
                LOG(VERBOSE) << "Failed to set thread to detach mode";
            }
        }

        ret = self->main();

        {
            AutoLock lock(self->mLock);
            self->mFinished = true;
            self->mExitStatus = ret;
        }

        self->onExit();
        // |self| is not valid beyond this point
    }

    ::android::base::ThreadStoreBase::OnThreadExit();

    // This return value is ignored.
    return NULL;
}

// static
void Thread::maskAllSignals() {
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, nullptr);
}

// static
void Thread::sleepMs(unsigned n) {
    usleep(n * 1000);
}

// static
void Thread::sleepUs(unsigned n) {
    usleep(n);
}

// static
void Thread::yield() {
    sched_yield();
}

unsigned long getCurrentThreadId() {
    pthread_t tid = pthread_self();
    // POSIX doesn't require pthread_t to be a numeric type.
    // Instead, just pick up the first sizeof(long) bytes as the "id".
    static_assert(sizeof(tid) >= sizeof(long),
                  "Expected pthread_t to be at least sizeof(long) wide");
    return *reinterpret_cast<unsigned long*>(&tid);
}

}  // namespace base
}  // namespace android
