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

#include "android/base/ContiguousRangeMapper.h"
#include "android/base/memory/MemoryHints.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"

#include <windows.h>

#include <sys/types.h>
#include <sys/mman.h>

#include <cassert>
#include <utility>
#include <vector>

using android::base::MemoryHint;
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
    }

    ~Impl() {
        join();
    }

    bool registerMemoryRange(void* start, size_t length) {
        static constexpr int hax_max_slots = 32;
        uint64_t gpa[hax_max_slots];
        uint64_t size[hax_max_slots];
        int count = hva2gpa_call(start, length, hax_max_slots, gpa, size);
        for (int i = 0; i < count; ++i) {
            guest_mem_protect_call(gpa[i], size[i], 0);
        }
        mRanges.push_back(std::make_pair(start, length));
        return protectHostRange(start, length, PAGE_NOACCESS);
    }

    void join() {
        for (auto range : mRanges) {
            // Could be on exit with interrupted background loader;
            // we must also unprotect on host side here as well.
            protectHostRange(range.first, range.second, PAGE_READWRITE);
            android::base::memoryHint(
                range.first, range.second, MemoryHint::Sequential);
        }
        mBackgroundLoadingThread.wait();
        mBulkZero.finish();
        for (auto range : mRanges) {
            android::base::memoryHint(
                range.first, range.second, MemoryHint::Normal);
        }
        mRanges.clear();
    }

    void doneRegistering() {
        mBackgroundLoadingThread.start();
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
        if (!protectHostRange(start, length, PAGE_READWRITE)) {
            return false;
        }

        if (!data) {
            android::base::zeroOutMemory(start, length);
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

    // Unprotects all pages in the range. Note that the VM must be stopped.
    void initBulkFill(void* startPtr, size_t length) {
        android::base::AutoLock lock(mLock);

        uint64_t gpa[32];
        uint64_t size[32];

        protectHostRange(startPtr, length, PAGE_READWRITE);

        if (mAccel == CPU_ACCELERATOR_HAX) {
            int count = hva2gpa_call(startPtr, length, 32, gpa, size);
            for (int i = 0; i < count; ++i) {
                guest_mem_protect_call(gpa[i], size[i], PROT_READ | PROT_WRITE | PROT_EXEC);
            }
        }
    }

    bool fillPageBulk(void* startPtr, size_t length, const void* data,
                  bool isQuickboot) {
        android::base::AutoLock lock(mLock);
        if (!data) {
            mBulkZero.add((uintptr_t)startPtr, length);
        } else {
            memcpy(startPtr, data, length);
        }
        return true;
    }

    void bgLoaderWorker() {
        System::Duration timeoutMs = 0;
        for (;;) {
            switch (mIdleCallback()) {
                case IdleCallbackResult::RunAgain:
                    timeoutMs = 0;
                    break;
                case IdleCallbackResult::Wait:
                    timeoutMs = 1;
                    break;
                case IdleCallbackResult::AllDone:
                    return;
            }
            if (timeoutMs) {
                System::get()->sleepMs(timeoutMs);
            }
        }
    }

    void* getExceptionHandler() {
        return mExceptionHandler;
    }

    void setExceptionHandler(void* handler) {
        mExceptionHandler = handler;
    }

private:

    bool protectHostRange(void* start, uint64_t size, DWORD protect) {
        DWORD oldProtect;
        if (::VirtualProtect(start, size, protect, &oldProtect) == 0) {
            fprintf(stderr, "Cannot protect range [%p - %p] (protect flags 0x%lx)\n",
                    start, (void *)((size_t)start + size), protect);
            return false;
        }
        return true;
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
    base::FunctorThread mBackgroundLoadingThread;
    void *mExceptionHandler = nullptr;

    android::base::ContiguousRangeMapper mBulkZero = {
        [](uintptr_t start, uintptr_t size) {
            android::base::zeroOutMemory((void*)start, size);
        }, 4096 * 4096};

    std::vector<std::pair<void*, uint64_t> > mRanges;
};

// static
bool MemoryAccessWatch::isSupported() {
    if (!android::featurecontrol::isEnabled(
            android::featurecontrol::OnDemandSnapshotLoad)) {
        return false;
    }

    if (android::featurecontrol::isEnabled(
            android::featurecontrol::QuickbootFileBacked)) {
        return false;
    }

    if (!android::featurecontrol::isEnabled(
            android::featurecontrol::WindowsOnDemandSnapshotLoad)) {
        return false;
    }

    if (GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HAX
        && guest_mem_protect_call)
        return guest_mem_protection_supported_call();

    return GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HAX &&
           guest_mem_protect_call;
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
        mImpl->setExceptionHandler(handler);
    }
}

MemoryAccessWatch::~MemoryAccessWatch() {
    ::RemoveVectoredExceptionHandler(mImpl->getExceptionHandler());

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

void MemoryAccessWatch::initBulkFill(void* ptr, size_t length) {
    if (!mImpl) return;
    mImpl->initBulkFill(ptr, length);
}

bool MemoryAccessWatch::fillPageBulk(void* ptr, size_t length, const void* data, bool isQuickboot) {
    if (!mImpl) return false;
    return mImpl->fillPageBulk(ptr, length, data, isQuickboot);
}

void MemoryAccessWatch::join() {
    if (mImpl) { mImpl->join(); }
}

}  // namespace snapshot
}  // namespace android
