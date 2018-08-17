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

#include "android/base/ContiguousRangeMapper.h"
#include "android/base/memory/MemoryHints.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/snapshot/common.h"
#include "android/snapshot/MacSegvHandler.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include <Hypervisor/hv.h>

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
        mAccessCallback(accessCallback),
        mIdleCallback(idleCallback),
        mSegvHandler(MacDoAccessCallback),
        mBackgroundLoadingThread([this]() { bgLoaderWorker(); }) {
    }

    ~Impl() {
        join();
    }

    static void MacDoAccessCallback(void* ptr) {
        sWatch->mImpl->mAccessCallback(ptr);
    }

    static IdleCallbackResult MacIdlePageCallback() {
        return sWatch->mImpl->mIdleCallback();
    }

    bool registerMemoryRange(void* start, size_t length) {
        mRanges.push_back({start, length});

        if (mAccel == CPU_ACCELERATOR_HVF) {
            // The maxium slot number is 32 for HVF.
            uint64_t gpa[32];
            uint64_t size[32];
            int count = hva2gpa_call(start, length, 32, gpa, size);
            for (int i = 0; i < count; ++i) {
                guest_mem_protect_call(gpa[i], size[i], 0);
            }
        }

        mprotect(start, length, PROT_NONE);
        android::base::memoryHint(
            start, length, MemoryHint::Random);
        mSegvHandler.registerMemoryRange(start, length);
        return true;
    }

    void join() {
        for (auto range : mRanges) {
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

    bool fillPage(void* start, size_t length, const void* data,
                  bool isQuickboot) {
        android::base::AutoLock lock(mLock);
        mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (!data) {
            android::base::zeroOutMemory(start, length);
        } else {
            memcpy(start, data, length);
        }
        if (mAccel == CPU_ACCELERATOR_HVF) {
            uint64_t gpa, size;
            int count = hva2gpa_call(start, 1, 1, &gpa, &size);
            if (count) {
                guest_mem_protect_call(gpa, length, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
            }
        }
        return true;
    }

    // Unprotects all pages in the range. Note that the VM must be stopped.
    void initBulkFill(void* startPtr, size_t length) {
        android::base::AutoLock lock(mLock);

        uint64_t gpa[32];
        uint64_t size[32];

        mprotect(startPtr, length, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (mAccel == CPU_ACCELERATOR_HVF) {
            int count = hva2gpa_call(startPtr, length, 32, gpa, size);
            for (int i = 0; i < count; ++i) {
                guest_mem_protect_call(gpa[i], size[i], HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
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
        System::Duration timeoutUs = 0;
        for (;;) {
            switch (mIdleCallback()) {
                case IdleCallbackResult::RunAgain:
                    timeoutUs = 0;
                    break;
                case IdleCallbackResult::Wait:
                    timeoutUs = 500;
                    break;
                case IdleCallbackResult::AllDone:
                    return;
            }
            if (timeoutUs) {
                System::get()->sleepUs(timeoutUs);
            }
        }
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
    MacSegvHandler mSegvHandler;
    base::FunctorThread mBackgroundLoadingThread;
    std::vector<std::pair<void*, size_t> > mRanges;

    android::base::ContiguousRangeMapper mBulkZero = {
        [](uintptr_t start, uintptr_t size) {
            android::base::zeroOutMemory((void*)start, size);
        }, 4096 * 4096};
};

// static
bool MemoryAccessWatch::isSupported() {
    return android::featurecontrol::isEnabled(
            android::featurecontrol::OnDemandSnapshotLoad);
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback) :
    mImpl(isSupported() ? new Impl(std::move(accessCallback),
                                   std::move(idleCallback)) : nullptr) {
    if (isSupported()) {
        sWatch = this;
    }
}

MemoryAccessWatch::~MemoryAccessWatch() {}

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
