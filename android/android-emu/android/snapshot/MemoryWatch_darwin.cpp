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
        join();
    }

    static void MacDoAccessCallback(void* ptr) {
        sWatch->mImpl->mAccessCallback(
            MemoryAccessWatch::AccessType::Host, ptr);
    }

    static IdleCallbackResult MacIdlePageCallback() {
        return sWatch->mImpl->mIdleCallback();
    }

    bool registerMemoryRange(void* start, size_t length) {
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
        mSegvHandler.registerMemoryRange(start, length);
        return true;
    }

    void join() {
        mBackgroundLoadingThread.wait();
    }

    void doneRegistering() {
        mBackgroundLoadingThread.start();
    }

    bool fillPage(bool loadedAlready,
                  MemoryAccessWatch::AccessType accessType, void* start, size_t length, const void* data,
                  bool isQuickboot) {
        android::base::AutoLock lock(mLock);

        // On Mac, we do not have a global memory access handler as
        // is the case with userfaultfd on Linux. We can easily
        // track RAM accesses from the guest using the HVF API,
        // but host-side dirty tracking is a bit more tricky. But
        // it is worth doing, because otherwise all background loading
        // would be considered host-side dirty accesses while they
        // are not actually dirtying, and there wouldn't be much point
        // to dirty tracking except for very short emulator sessions.
        //
        // Overall scheme of dirty tracking host writes on Mac is that
        // we track whether this is the memory access to load RAM
        // through |loadedAlready|, and distinguish it from further
        // accesses that may imply the need to mark ram dirty
        // Mark memory read-only on the first access,
        // and mark as writeable and dirty on the second and subsequent
        // accesses from host. We distinguish the following access types:
        //
        // 0: Guest (read/write): Accesses to guest RAM directly from the
        // hypervisor running in the VM. This causes a VM exit due to guest EPT
        // fault. Depending on whether the write flag is set for the exit
        // qualification, we can tell immediately whether or not to mark
        // RAM dirty.
        //
        // 1: Host (once only): A special case for systems like macOS where
        // one cannot easily catch all host page faults as is the case with
        // userfaultfd on linux. In particular, the block device on host
        // relies on these lower level faults to function properly. In these
        // situations, we load the block device iov memory early before there
        // is a chance to mess with signals at the kernel level, and assume
        // that it is dirty.
        //
        // 2: Host (read/write): Standard memory r/w that can be caught in
        // the Mach exception handler. Mach does not provide whether an access
        // was a read or write, so we simply unprotect read only first, then
        // the second trip to the exception handler is the write access.
        //
        // Here are the important access patterns and what happens to
        // host / guest memory protection, and whether to mark the page dirty:
        //
        // First , Second (and subsequent) ->
        // host protection, guest protection, mark dirty?
        //
        // * : don't care
        //
        // Guest (write), * -> *, r/w/e, yes
        // Host (write), * -> r/w/e, *, yes
        // Host (once only), * -> r/w/e, *, yes
        //
        // Guest (read), Guest (write) -> r/e, r/w/e, yes
        // Guest (read), Host (read) -> r/e, r/e, no
        // Guest (read), Host (write) -> r/w/e, r/e, yes
        // Guest (read), Host (once only) -> r/w/e, r/e, yes
        // Host (read), Guest (read) -> r/e, r/e, no
        // Host (read), Guest (write) -> r/e, r/w/e, yes
        // Host (read), Host (read) -> r/e, r/e, no
        // Host (read), Host (write) -> r/w/e, r/e, yes
        // Host (read), Host (once only) -> r/w/e, r/e, yes
        //
        // We maintain the invariant that the page is marked dirty
        // on a guest or host write regardless of the access pattern.
        // Background loading (and the first run of the segv handler on
        // any page) counts as a first access of "Host (read)".
        bool remapNeeded = false;
        if (!loadedAlready) {
            mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
            if (!data) {
                // Remapping:
                // Is zero data, so try to use an existing zero page in the OS
                // instead of memset which might cause more memory to be resident.
                if (!isQuickboot) {
                    if(MAP_FAILED ==
                           mmap(start, length,
                                PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0)) {
                        memset(start, 0x0, length);
                    } else {
                        remapNeeded = true;
                    }
                }
            } else {
                memcpy(start, data, length);
            }
            if (accessType == MemoryAccessWatch::AccessType::HostOnceOnly) {
                mDirtyCallback(start);
            } else {
                mprotect(start, length, PROT_READ | PROT_EXEC);
            }
        } else if (accessType == MemoryAccessWatch::AccessType::HostOnceOnly ||
                   accessType == MemoryAccessWatch::AccessType::Host) {
            mprotect(start, length, PROT_READ | PROT_WRITE | PROT_EXEC);
            mDirtyCallback(start);
        }
        if (!loadedAlready && mAccel == CPU_ACCELERATOR_HVF) {
            uint64_t gpa, size;
            int count = hva2gpa_call(start, 1, 1, &gpa, &size);
            if (count) {
                // Restore the mapping because we might have re-mapped above.
                // Don't allow writes just yet; we'd like to track dirty written
                // pages by the guest.
                if (remapNeeded) {
                    guest_mem_remap_call(start, gpa, length, HV_MEMORY_READ | HV_MEMORY_EXEC);
                } else {
                    guest_mem_protect_call(gpa, length, HV_MEMORY_READ | HV_MEMORY_EXEC);
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

    android::base::Lock mLock;
    CpuAccelerator mAccel;
    MemoryAccessWatch::AccessCallback mAccessCallback;
    MemoryAccessWatch::IdleCallback mIdleCallback;
    MemoryAccessWatch::DirtyCallback mDirtyCallback;
    MacSegvHandler mSegvHandler;
    base::FunctorThread mBackgroundLoadingThread;
};

// static
bool MemoryAccessWatch::isSupported() {
    // TODO: HAXM
    return GetCurrentCpuAccelerator() == CPU_ACCELERATOR_HVF;
}

// static
bool MemoryAccessWatch::dirtyTrackingSupported() {
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

bool MemoryAccessWatch::fillPage(bool loadedAlready,
                                 MemoryAccessWatch::AccessType accessType,
                                 void* ptr, size_t length, const void* data,
                                 bool isQuickboot) {
    if (!mImpl) return false;
    return mImpl->fillPage(loadedAlready, accessType, ptr, length, data, isQuickboot);
}

void MemoryAccessWatch::join() {
    if (mImpl) { mImpl->join(); }
}

}  // namespace snapshot
}  // namespace android
