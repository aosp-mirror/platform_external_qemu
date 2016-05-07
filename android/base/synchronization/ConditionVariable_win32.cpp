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

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/Win32Utils.h"

// Technical note: this is loosely based on the Chromium implementation
// of ConditionVariable. This version works on Windows XP and above and
// doesn't try to use Vista's CONDITION_VARIABLE types.

namespace android {
namespace base {

namespace {

// Helper class which implements a free list of event handles.
class WaitEventStorage {
public:
    HANDLE alloc() {
        HANDLE handle;
        AutoLock lock(mLock);
        if (!mFreeHandles.empty()) {
            handle = mFreeHandles.back().release();
            mFreeHandles.pop_back();
        } else {
            lock.unlock();
            handle = CreateEvent(NULL, TRUE, FALSE, NULL);
        }
        return handle;
    }

    void free(HANDLE h) {
        ResetEvent(h);
        AutoLock lock(mLock);
        if (mFreeHandles.size() < 100) {
            mFreeHandles.emplace_back(h);
        } else {
            lock.unlock();
            CloseHandle(h); // don't collect too many open events
        }
    }

private:
    std::vector<Win32Utils::ScopedHandle> mFreeHandles;
    Lock mLock;
};

LazyInstance<WaitEventStorage> sWaitEvents = LAZY_INSTANCE_INIT;

}  // namespace

ConditionVariable::~ConditionVariable() {
    // Don't lock anything here: if there's someone blocked on |this| while
    // we're in a dtor in a different thread, the code is already heavily
    // bugged. Lock here won't save it.
}

void ConditionVariable::wait(Lock* userLock) {
    // Grab new waiter event handle.
    HANDLE handle = sWaitEvents->alloc();
    {
        AutoLock lock(mLock);
        mWaiters.emplace_back(handle);
    }

    // Unlock user lock then wait for event.
    userLock->unlock();
    WaitForSingleObject(handle, INFINITE);
    // NOTE: The handle has been removed from mWaiters here,
    // see signal() below. Close/recycle the event.
    sWaitEvents->free(handle);
    userLock->lock();
}

void ConditionVariable::signal() {
    android::base::AutoLock lock(mLock);
    if (!mWaiters.empty()) {
        // NOTE: This wakes up the thread that went to sleep most
        //       recently (LIFO) for performance reason. For better
        //       fairness, using (FIFO) would be appropriate.
        HANDLE handle = mWaiters.back().release();
        mWaiters.pop_back();
        lock.unlock();
        SetEvent(handle);
        // NOTE: The handle will be closed/recycled by the waiter.
    }
}

}  // namespace base
}  // namespace android
