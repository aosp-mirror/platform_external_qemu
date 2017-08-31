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

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/CpuAccelerator.h"

#include "mac_segv_handler.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include <Hypervisor/hv.h>

using android::base::System;

namespace android {
namespace snapshot {

static MemoryAccessWatch* sWatch = nullptr;

class MemoryAccessWatch::Impl {
public:
    Impl() : mAccel(GetCurrentCpuAccelerator()),
             mBackgroundLoadingThread([this]() { bgLoaderWorker(); }) {
        setup_mac_segv_handler(MacDoAccessCallback);
    }

    ~Impl() {
        mBackgroundLoadingThread.wait();
        teardown_mac_segv_handler();
    }

    static void MacDoAccessCallback(void* ptr) {
        sWatch->mImpl->mAccessCallback(ptr);
    }

    static IdleCallbackResult MacIdlePageCallback() {
        return sWatch->mImpl->mIdleCallback();
    }

    bool registerMemoryRange(void* start, size_t length) {
        if (mAccel == CPU_ACCELERATOR_HVF) {
            bool found = false;
            uint64_t gpa = hva2gpa_call(start, &found);
            if (found) {
                hv_vm_protect(gpa, length, 0);
            }
        }
        mprotect(start, length, PROT_NONE);
        return true;
    }

    void doneRegistering() {
        mBackgroundLoadingThread.start();
    }

    bool fillPage(void* start, size_t length, const void* data) {
        // fprintf(stderr, "%s: start %p len %zu data %p\n", __func__, start, length, data);
        android::base::AutoLock lock(mLock);
        mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
        if (!data) {
            memset(start, 0x0, length);
        } else {
            memcpy(start, data, length);
        }
        if (mAccel == CPU_ACCELERATOR_HVF) {
            bool found = false;
            uint64_t gpa = hva2gpa_call(start, &found);
            if (found) {
                hv_vm_protect(gpa, length, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
            }
        }
        return true;
    }

    void bgLoaderWorker() {
        System::Duration timeoutUs = 0;
        uint32_t i = 0;
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
            System::get()->sleepUs(timeoutUs);
            i++;
        }
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;

    base::FunctorThread mBackgroundLoadingThread;
};

// static
bool MemoryAccessWatch::isSupported() {
    // TODO: HAXM
    return GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HVF;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback) :
    mImpl(new Impl()) {
    mImpl->mAccessCallback = std::move(accessCallback);
    mImpl->mIdleCallback = std::move(idleCallback);
    sWatch = this;
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

bool MemoryAccessWatch::fillPage(void* ptr, size_t length, const void* data) {
    if (!mImpl) return false;

    return mImpl->fillPage(ptr, length, data);
}

}  // namespace snapshot
}  // namespace android
