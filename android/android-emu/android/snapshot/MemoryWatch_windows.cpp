// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/MemoryWatch.h"
#include "android/snapshot/Snapshotter.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/CpuAccelerator.h"

#include <windows.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <cassert>
#include <utility>

using android::base::System;

namespace android {
namespace snapshot {

static MemoryAccessWatch* sWatch = nullptr;

class MemoryAccessWatch::Impl {
public:
    Impl(MemoryAccessWatch::AccessCallback accessCallback,
         MemoryAccessWatch::IdleCallback idleCallback) :
        mAccel(GetCurrentCpuAccelerator()),
        mAccessCallback(std::move(accessCallback)),
        mIdleCallback(std::move(idleCallback)),
        mBackgroundLoadingThread([this]() { bgLoaderWorker(); }) {
        Snapshotter::get().addOperationCallback(
            [this](Snapshotter::Operation op, Snapshotter::Stage stage) {
                if (stage == Snapshotter::Stage::End &&
                    op == Snapshotter::Operation::Load)
                        mBackgroundLoadingThread.start();
            });
    }

    ~Impl() {
        join();
    }

    bool registerMemoryRange(void* start, size_t length) {
        DWORD oldProtect;

        static constexpr int hax_max_slots = 32;
        uint64_t gpa[hax_max_slots];
        uint64_t size[hax_max_slots];
        int count = hva2gpa_call(start, length, hax_max_slots, gpa, size);
        for (int i = 0; i < count; ++i) {
            guest_mem_protect_call(gpa[i], size[i], 0);
        }
        return ::VirtualProtect(start, length, PAGE_NOACCESS, &oldProtect) != 0;
    }

    void join() {
        mBackgroundLoadingThread.wait();
    }

    void doneRegistering() {
    }

    static LONG WINAPI handleException(EXCEPTION_POINTERS* exceptionInfo) {
        if (exceptionInfo->ExceptionRecord->ExceptionCode !=
            EXCEPTION_ACCESS_VIOLATION) {
            return (LONG)EXCEPTION_CONTINUE_SEARCH;
        }
        auto ptr =
                (void*)exceptionInfo->ExceptionRecord->ExceptionInformation[1];
        sWatch->mImpl->mAccessCallback(ptr);
        return (LONG)EXCEPTION_CONTINUE_EXECUTION;
    }

    bool fillPage(void* start, size_t length, const void* data,
                  bool isQuickboot) {
        android::base::AutoLock lock(mLock);
        DWORD oldProtect;
        if (::VirtualProtect(start, length, PAGE_READWRITE, &oldProtect) == 0) {
            fprintf(stderr, "Cannot unprotect range [%p - %p]\n",
                    start, (void *)((size_t)start + length));
            return false;
        }

        if (!data) {
            memset(start, 0, length);
        } else {
            memcpy(start, data, length);
        }
        if (mAccel == CPU_ACCELERATOR_HAX) {
            uint64_t gpa, size;
            int count = hva2gpa_call(start, 1, 1, &gpa, &size);
            if (count) {
                guest_mem_protect_call(gpa, length, PROT_READ | PROT_WRITE | PROT_EXEC);
            }
        }
        return true;
    }

    void bgLoaderWorker() {
        return;

        // System::Duration timeoutMs = 0;
        // for (;;) {
        //     switch (mIdleCallback()) {
        //         case IdleCallbackResult::RunAgain:
        //             timeoutMs = 0;
        //             break;
        //         case IdleCallbackResult::Wait:
        //             timeoutMs = 1;
        //             break;
        //         case IdleCallbackResult::AllDone:
        //             return;
        //     }
        //     if (timeoutMs) {
        //         System::get()->sleepMs(timeoutMs);
        //     }
        // }
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
    base::FunctorThread mBackgroundLoadingThread;
    void *mExceptionHandler = nullptr;
};

// static
bool MemoryAccessWatch::isSupported() {
    if (GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HAX
        && guest_mem_protect_call)
        return guest_mem_protection_supported_call();
    return false;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback) :
    mImpl(isSupported() ? new Impl(std::move(accessCallback),
                                   std::move(idleCallback)) : nullptr) {
    if (isSupported()) {
        sWatch = this;
    }

    PVOID handler = ::AddVectoredExceptionHandler(true, &Impl::handleException);

    if (!handler) {
        mImpl.reset();
    } else {
        mImpl->mExceptionHandler = handler;
    }
}

MemoryAccessWatch::~MemoryAccessWatch() {
    ::RemoveVectoredExceptionHandler(mImpl->mExceptionHandler);

    assert(sWatch == this);
    sWatch = nullptr;
}

bool MemoryAccessWatch::valid() const {
    if (mImpl) return true;
    return false;
}

bool MemoryAccessWatch::registerMemoryRange(void* start, size_t length) {
    if (mImpl) return mImpl->registerMemoryRange(start, length);
    return false;
}

void MemoryAccessWatch::doneRegistering() {
    if (mImpl) mImpl->doneRegistering();
}

bool MemoryAccessWatch::fillPage(void* ptr, size_t length, const void* data,
                                 bool isQuickboot) {
    if (!mImpl) return false;
    return mImpl->fillPage(ptr, length, data, isQuickboot);
}

void MemoryAccessWatch::join() {
    if (mImpl) { mImpl->join(); }
}

}  // namespace snapshot
}  // namespace android
