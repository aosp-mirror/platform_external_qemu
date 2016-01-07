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

namespace android {
namespace base {

namespace {

// Helper class to enter/leave a critical section on scope enter/exit.
// Equivalent to android::base::AutoLock but do not use it to reduce
// coupling.
class ScopedLocker {
public:
    ScopedLocker(CRITICAL_SECTION* section) : mSection(section) {
        EnterCriticalSection(mSection);
    }

    ~ScopedLocker() {
        LeaveCriticalSection(mSection);
    }
private:
    CRITICAL_SECTION* mSection;
};

}  // namespace

Thread::Thread(ThreadFlags flags) :
    mFlags(flags) {
    InitializeCriticalSection(&mLock);
}

Thread::~Thread() {
    if(mThread != INVALID_HANDLE_VALUE) {
        CloseHandle(mThread);
    }
    DeleteCriticalSection(&mLock);
}

bool Thread::start() {
    if (mStarted) {
        return false;
    }

    ScopedLocker locker(&mLock);
    bool ret = true;
    mStarted = true;
    mThread = CreateThread(NULL, 0, &Thread::thread_main, this, 0, &mThreadId);
    if (!mThread) {
        ret = false;
        // We _do not_ need to guard this access to |mFinished| because we're
        // sure that the launched thread failed, so there can't be parallel
        // access.
        mFinished = false;
    }
    return ret;
}

bool Thread::wait(intptr_t* exitStatus) {
    if (!mStarted) {
        return false;
    }
    {
        ScopedLocker locker(&mLock);
        if (mFinished) {
            // Thread already stopped.
            if (exitStatus) {
                *exitStatus = mExitStatus;
            }
            return true;
        }
    }

    // NOTE: Do not hold lock during wait to aloow thread_main to
    // properly update mIsRunning and mExitStatus on thread exit.
    if (WaitForSingleObject(mThread, INFINITE) == WAIT_FAILED) {
        return false;
    }
    DCHECK(mFinished);

    if (exitStatus) {
        *exitStatus = mExitStatus;
    }
    return true;
}

bool Thread::tryWait(intptr_t* exitStatus) {
    if (!mStarted) {
        return false;
    }

    ScopedLocker locker(&mLock);
    if (!mFinished ||
        WaitForSingleObject(mThread, 0) != WAIT_OBJECT_0) {
        return false;
    }

    if (exitStatus) {
        *exitStatus = mExitStatus;
    }
    return true;
}

// static
DWORD WINAPI Thread::thread_main(void *arg) {
    intptr_t ret;

    {
        // no need to call maskAllSignals() here: we know
        // that on Windows it's a noop
        Thread* self = reinterpret_cast<Thread*>(arg);
        ret = self->main();

        EnterCriticalSection(&self->mLock);
        self->mFinished = true;
        self->mExitStatus = ret;
        LeaveCriticalSection(&self->mLock);

        self->onExit();
        // |self| is not valid beyond this point
    }

    // Ensure all thread-local values are released for this thread.
    ::android::base::ThreadStoreBase::OnThreadExit();

    // This return value is ignored.
    return static_cast<DWORD>(NULL);
}

// static
void Thread::maskAllSignals() {
    // no such thing as signal in Windows
}

}  // namespace base
}  // namespace android
