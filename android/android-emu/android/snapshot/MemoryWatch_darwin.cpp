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
#include "android/crashreport/crash-handler.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/snapshot/MacSegvHandler.h"

#include <unordered_map>

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
    Impl(MemoryAccessWatch::AccessCallback accessCallback,
         MemoryAccessWatch::IdleCallback idleCallback,
         MemoryAccessWatch::DirtyCallback dirtyCallback) :
        mAccel(GetCurrentCpuAccelerator()),
        mAccessCallback(accessCallback),
        mIdleCallback(idleCallback),
        mDirtyCallback(dirtyCallback),
        mSegvHandler(MacDoAccessCallback),
        mBackgroundLoadingThread([this]() { bgLoaderWorker(); }) {
    }

    ~Impl() {
        mBackgroundLoadingThread.wait();
    }

    static void MacDoAccessCallback(void* ptr) {
        // Assume all host->guest RAM accesses are dirtying.
        sWatch->mImpl->mAccessCallback(ptr);
        sWatch->mImpl->mDirtyCallback(ptr);
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
                mRegisteredGpas[gpa] = length;
            }
        }
        mprotect(start, length, PROT_NONE);
        mSegvHandler.registerMemoryRange(start, length);
        mRegisteredHvas[start] = length;
        return true;
    }

    void doneRegistering() {
        mBackgroundLoadingThread.start();
    }

    bool fillPage(void* start, size_t length, const void* data,
                  bool isQuickboot) {
        android::base::AutoLock lock(mLock);

        if (!mJoining) {
            mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
        }
        bool remapNeeded = false;
        if (!data) {
            // Remapping:
            // Is zero data, so try to use an existing zero page in the OS
            // instead of memset which might cause more memory to be resident.
            if (!isQuickboot &&
                (MAP_FAILED == mmap(start, length,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0))) {
                memset(start, 0x0, length);
                remapNeeded = false;
            } else {
                remapNeeded = true;
            }
        } else {
            memcpy(start, data, length);
        }
        if (mAccel == CPU_ACCELERATOR_HVF) {
            bool found = false;
            uint64_t gpa = hva2gpa_call(start, &found);
            if (found && !mJoining) {
                // Restore the mapping because we might have re-mapped above.
                if (remapNeeded) {
                    hv_vm_unmap(gpa, length);
                    hv_vm_map(start, gpa, length, HV_MEMORY_READ | HV_MEMORY_EXEC);
                } else {
                    hv_vm_protect(gpa, length, HV_MEMORY_READ | HV_MEMORY_EXEC);
                }
            }
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

    void onJoin() {
        android::base::AutoLock lock(mLock);
        mJoining = true;
        for (const auto& it : mRegisteredGpas) {
            hv_vm_protect(it.first, it.second, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
        }
        for (const auto& it : mRegisteredHvas) {
            mprotect(it.first, it.second, PROT_READ | PROT_WRITE | PROT_EXEC);
        }
        mRegisteredHvas.clear();
        mRegisteredGpas.clear();
    }

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
    MemoryAccessWatch::DirtyCallback mDirtyCallback;
    MacSegvHandler mSegvHandler;
    base::FunctorThread mBackgroundLoadingThread;

    bool mJoining = false;
    std::unordered_map<void*, uint64_t> mRegisteredHvas;
    std::unordered_map<uint64_t, uint64_t> mRegisteredGpas;
};

// static
bool MemoryAccessWatch::isSupported() {
    // TODO: HAXM
    return GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HVF;
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& accessCallback,
                                     IdleCallback&& idleCallback,
                                     DirtyCallback&& dirtyCallback) :
    mImpl(isSupported() ? new Impl(std::move(accessCallback),
                                   std::move(idleCallback),
                                   std::move(dirtyCallback)) : nullptr) {
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

void MemoryAccessWatch::onJoin() {
    if (mImpl) mImpl->onJoin();
}

}  // namespace snapshot
}  // namespace android
