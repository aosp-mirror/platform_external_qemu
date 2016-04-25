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
#include "android/base/containers/PodVector.h"

// Technical note: this is loosely based on the Chromium implementation
// of ConditionVariable. This version works on Windows XP and above and
// doesn't try to use Vista's CONDITION_VARIABLE types.

namespace android {
namespace base {

namespace {
/*
// Helper class which implements a free list of event handles.
class WaitEventStorage {
public:
    WaitEventStorage() : mFreeHandles(), mLock() {}

    ~WaitEventStorage() {
        for (size_t n = 0; n < mFreeHandles.size(); ++n) {
            CloseHandle(mFreeHandles[n]);
        }
    }

    HANDLE alloc() {
        HANDLE handle;
        mLock.lock();
        size_t size = mFreeHandles.size();
        if (size > 0) {
            handle = mFreeHandles[size - 1U];
            mFreeHandles.remove(size - 1U);
        } else {
            handle = CreateEvent(NULL, TRUE, FALSE, NULL);
        }
        mLock.unlock();
        return handle;
    }

    void free(HANDLE h) {
        mLock.lock();
        ResetEvent(h);
        mFreeHandles.push_back(h);
        mLock.unlock();
    }

private:
    PodVector<HANDLE> mFreeHandles;
    Lock mLock;
};

LazyInstance<WaitEventStorage> sWaitEvents = LAZY_INSTANCE_INIT;
*/
}  // namespace

ConditionVariable::ConditionVariable() {
    InitializeConditionVariable(&mCv);
}

ConditionVariable::~ConditionVariable() {}

void ConditionVariable::wait(Lock* userLock) {
    SleepConditionVariableCS(&mCv, &userLock->mLock, INFINITE);
}

void ConditionVariable::signal() {
    WakeConditionVariable(&mCv);
}

}  // namespace base
}  // namespace android
