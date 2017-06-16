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
#include "android/emulation/CpuAccelerator.h"

#include "mac_segv_handler.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include <Hypervisor/hv.h>

namespace android {
namespace snapshot {

static MemoryAccessWatch* sWatch = nullptr;

class MemoryAccessWatch::Impl {
public:
    static void MacDoAccessCallback(void* ptr) {
        sWatch->mImpl->mAccessCallback(ptr);
    }

    static IdleCallbackResult MacIdlePageCallback() {
        return sWatch->mImpl->mIdleCallback();
    }

    Impl(CpuAccelerator accel) : mAccel(accel) {
        setup_mac_segv_handler(MacDoAccessCallback);
    }

    bool registerMemoryRange(void* start, size_t length, const GuestPageInfo& guestPageInfo) {
        if (mAccel == CPU_ACCELERATOR_HVF) {
            if (guestPageInfo.exists) {
                hv_vm_protect(guestPageInfo.addr, length, 0);
            }
        }
        mprotect(start, length, PROT_NONE);
        return true;
    }

    void doneRegistering() { }

    bool fillPage(void* start, size_t length, const void* data, const GuestPageInfo& guestPageInfo) {
        android::base::AutoLock lock(mLock);
        memcpy(start, data, length);
        if (mAccel == CPU_ACCELERATOR_HVF) {
            if (guestPageInfo.exists) {
                hv_vm_protect(guestPageInfo.addr, length,
                              HV_MEMORY_READ |
                              HV_MEMORY_WRITE |
                              HV_MEMORY_EXEC);
            }
        }
        return true;
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
};

// static
bool MemoryAccessWatch::isSupported() {
    return true;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback) :
    mImpl(new Impl(GetCurrentCpuAccelerator())) {
    mImpl->mAccessCallback = std::move(accessCallback);
    mImpl->mIdleCallback = std::move(idleCallback);
    sWatch = this;
}

MemoryAccessWatch::~MemoryAccessWatch() {}

bool MemoryAccessWatch::valid() const {
    if (mImpl) return true;
    return false;
}

bool MemoryAccessWatch::registerMemoryRange(void* start, size_t length, const GuestPageInfo& guestPageInfo) {
    if (mImpl) return mImpl->registerMemoryRange(start, length, guestPageInfo);
    return false;
}

void MemoryAccessWatch::doneRegistering() {
    if (mImpl) mImpl->doneRegistering();
}

bool MemoryAccessWatch::fillPage(void* ptr, size_t length, const void* data, const GuestPageInfo& guestPageInfo) {
    if (!mImpl) return false;

    return mImpl->fillPage(ptr, length, data, guestPageInfo);
}

}  // namespace snapshot
}  // namespace android
