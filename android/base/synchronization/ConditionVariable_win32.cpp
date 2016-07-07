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

// Helper class which implements a free list of event handles. Needed in a
// pre-Vista code.
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
            CloseHandle(h);  // don't collect too many open events
        }
    }

private:
    std::vector<Win32Utils::ScopedHandle> mFreeHandles;
    Lock mLock;
};

LazyInstance<WaitEventStorage> sWaitEvents = LAZY_INSTANCE_INIT;

////////////////////////////////////////////////////////////////////////////////
// An interface for the OS-specific condition variable implementation.
class ConditionVariableForOs {
public:
    virtual void initCV(ConditionVariable::Data* cvData) const = 0;
    virtual void waitCV(ConditionVariable::Data* cvData, Lock* lock) const = 0;
    virtual void signalCV(ConditionVariable::Data* cvData) const = 0;
    virtual void broadcastCV(ConditionVariable::Data* cvData) const = 0;
    virtual void destroyCV(ConditionVariable::Data* cvData) const = 0;

protected:
    ~ConditionVariableForOs() noexcept = default;
};

// Fallback code for pre-Vista OS version - use handcrafted code.
class ConditionVariableForXp final : public ConditionVariableForOs {
public:
    virtual void initCV(ConditionVariable::Data* cvData) const override final {
        new (&cvData->xp) ConditionVariable::XpCV();
    }
    virtual void waitCV(ConditionVariable::Data* cvData,
                        Lock* userLock) const override final {
        // Grab new waiter event handle.
        HANDLE handle = sWaitEvents->alloc();
        {
            AutoLock lock(cvData->xp.mLock);
            cvData->xp.mWaiters.emplace_back(handle);
        }

        // Unlock user lock then wait for event.
        userLock->unlock();
        WaitForSingleObject(handle, INFINITE);
        // NOTE: The handle has been removed from mWaiters here,
        // see signal() below. Close/recycle the event.
        sWaitEvents->free(handle);
        userLock->lock();
    }
    virtual void signalCV(
            ConditionVariable::Data* cvData) const override final {
        android::base::AutoLock lock(cvData->xp.mLock);
        if (cvData->xp.mWaiters.empty()) {
            return;
        }
        // NOTE: This wakes up the thread that went to sleep most
        //       recently (LIFO) for performance reason. For better
        //       fairness, using (FIFO) would be appropriate.
        HANDLE handle = cvData->xp.mWaiters.back().release();
        cvData->xp.mWaiters.pop_back();
        lock.unlock();
        SetEvent(handle);
        // NOTE: The handle will be closed/recycled by the waiter.
    }
    virtual void broadcastCV(
            ConditionVariable::Data* cvData) const override final {
        android::base::AutoLock lock(cvData->xp.mLock);
        if (cvData->xp.mWaiters.empty()) {
            return;
        }

        lock.unlock();

        for (auto& waiter : cvData->xp.mWaiters) {
            HANDLE handle = waiter.release();
            SetEvent(handle);
            // NOTE: The handle will be closed/recycled by the waiter.
        }

        cvData->xp.mWaiters.clear();
    }
    virtual void destroyCV(
            ConditionVariable::Data* cvData) const override final {
        cvData->xp.ConditionVariable::XpCV::~XpCV();
    }
};

// A set of Vista+ APIs for condition variables: function types and pointers.
using InitCV = void(WINAPI*)(ConditionVariable::VistaCV*);
using SleepCsCV = BOOL(WINAPI*)(ConditionVariable::VistaCV*,
                                CRITICAL_SECTION*,
                                DWORD);
using SleepSrwLockCV = BOOL(WINAPI*)(ConditionVariable::VistaCV*,
                                     void*,
                                     DWORD,
                                     ULONG);
using WakeCV = void(WINAPI*)(ConditionVariable::VistaCV*);

static InitCV sInitCV = nullptr;
static SleepCsCV sSleepCsCV = nullptr;
static SleepSrwLockCV sSleepSrwLockCV = nullptr;
static WakeCV sWakeAllCV = nullptr;
static WakeCV sWakeOneCV = nullptr;

// Vista+ code for the condition variable, using the raw Windows API.
class ConditionVariableForVista final : public ConditionVariableForOs {
public:
    virtual void initCV(ConditionVariable::Data* cvData) const override final {
        sInitCV(&cvData->vista);
    }
    virtual void waitCV(ConditionVariable::Data* cvData,
                        Lock* userLock) const override final {
        sSleepCsCV(&cvData->vista, &userLock->mLock, INFINITE);
    }
    virtual void signalCV(
            ConditionVariable::Data* cvData) const override final {
        sWakeOneCV(&cvData->vista);
    }
    virtual void broadcastCV(
            ConditionVariable::Data* cvData) const override final {
        sWakeAllCV(&cvData->vista);
    }
    virtual void destroyCV(
            ConditionVariable::Data* cvData) const override final {
        ;  // there's no special function for condition variable destruction
    }
};

// Instances of both implementations and a pointer that will point to the one
// to use on the current OS.
const ConditionVariableForXp sImplXp;
const ConditionVariableForVista sImplVista;
const ConditionVariableForOs* sImpl = nullptr;

// A very simple class with the purpose of initializing the |sImpl| into
// the proper implementation based on a dynamic function loading result.
class ApiVersionSelector final {
public:
    ApiVersionSelector() {
        static const wchar_t kDllName[] = L"kernel32.dll";

        HMODULE kernel32 = ::GetModuleHandleW(kDllName);
        if (!kernel32) {
            kernel32 = ::LoadLibraryW(kDllName);
            if (!kernel32) {
                sImpl = &sImplXp;
                return;  // well, this is a very strange breed of Windows...
            }
        }

        const auto init = (InitCV)::GetProcAddress(
                kernel32, "InitializeConditionVariable");
        const auto sleepCs = (SleepCsCV)::GetProcAddress(
                kernel32, "SleepConditionVariableCS");
        const auto sleepSrw = (SleepSrwLockCV)::GetProcAddress(
                kernel32, "SleepConditionVariableSRW");
        const auto wakeOne =
                (WakeCV)::GetProcAddress(kernel32, "WakeConditionVariable");
        const auto wakeAll =
                (WakeCV)::GetProcAddress(kernel32, "WakeAllConditionVariable");
        if (!init || !sleepCs || !sleepSrw || !wakeOne || !wakeAll) {
            sImpl = &sImplXp;
            return;
        }

        sInitCV = init;
        sSleepCsCV = sleepCs;
        sSleepSrwLockCV = sleepSrw;
        sWakeOneCV = wakeOne;
        sWakeAllCV = wakeAll;
        sImpl = &sImplVista;
    }
};

LazyInstance<ApiVersionSelector> sApiLoader = LAZY_INSTANCE_INIT;

}  // namespace

// Actual ConditionVariable members - these just forward all calls to |sImpl|.

ConditionVariable::ConditionVariable() {
    if (!sImpl) {
        sApiLoader.get();
    }
    assert(sImpl);
    sImpl->initCV(&mData);
}

ConditionVariable::~ConditionVariable() {
    sImpl->destroyCV(&mData);
}

void ConditionVariable::wait(Lock* userLock) {
    sImpl->waitCV(&mData, userLock);
}

void ConditionVariable::signal() {
    sImpl->signalCV(&mData);
}

void ConditionVariable::broadcast() {
    sImpl->broadcastCV(&mData);
}

}  // namespace base
}  // namespace android
