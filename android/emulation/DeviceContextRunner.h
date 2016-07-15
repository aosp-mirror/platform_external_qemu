// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Log.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/emulation/VmLock.h"
#include "android/base/synchronization/Lock.h"

#include <memory>
#include <vector>

#pragma once

using android::base::AutoLock;
using android::base::Lock;
using android::base::Looper;
using android::base::ThreadLooper;
using android::VmLock;

typedef void (*device_operation_t)(void*);

// The DeviceContextRunner is a way to handle virtual device operations
// (such as AndroidPipe) in a safe manner: that is, only when
// the main loop holds the VM lock. When we perform virtual device operations
// in this manner, we are safer from threading issues caused by
// VM lock contention by vcpu threads.
class DeviceContextRunner {
public:
    // Initialize threading. This must be called in the main loop
    // thread, so cannot be performed in the constructor which may run in
    // a different one.
    virtual void init(VmLock* vmLock) {
        mVmLock = vmLock;
        // TODO(digit): Find a better event abstraction.
        //
        // Operating on Looper::Timer objects is not supposed to be
        // thread-safe, but it appears that their QEMU1 and QEMU2 specific
        // implementation *is* (see qemu-timer.c:timer_mod()).
        //
        // This means that in practice the code below is safe when used
        // in the context of an Android emulation engine. However, we probably
        // need a better abstraction that also works with the generic
        // Looper implementation (for unit-testing) or any other kind of
        // runtime environment, should we one day link AndroidEmu to a
        // different emulation engine.
        Looper* looper = ThreadLooper::get();
        mTimer.reset(looper->createTimer(
                     [](void* that, Looper::Timer*) {
                        static_cast<DeviceContextRunner*>(that)->onTimerEvent();
                     },
                     this));
        if (!mTimer.get()) {
            LOG(WARNING) << "Failed to create a loop timer, falling back "
                            "to regular locking!";
        }
    }

    bool missingDeviceOperations() const {
        return mDeviceOperation == nullptr || mDeviceOperationPost == nullptr;
    }

    void setDeviceOperations(device_operation_t op,
                             device_operation_t post) {
        mDeviceOperation = op;
        mDeviceOperationPost = post;
    }

    bool inDeviceContext() const { return mVmLock->isLockedBySelf(); }

    void queueDeviceOperation(void* op) {
        if (inDeviceContext()) {
            // Perform the operation correctly since the current thread
            // already holds the lock that protects the global VM state.
            mDeviceOperation(op);
        } else {
            // Queue the operation in the mPendingMap structure, then
            // restart the timer.
            AutoLock lock(mLock);
            mPending.push_back(op);
            lock.unlock();

            // NOTE: See TODO above why this is thread-safe when used with
            // QEMU1 and QEMU2.
            mTimer->startAbsolute(0);
        }
    }

private:
    using Pending = std::vector<void*>;

    void onTimerEvent() {
        // First, clear the current pending set of device commands
        // and operate on each one in turn.
        AutoLock lock(mLock);
        Pending pending;
        pending.swap(mPending);
        lock.unlock();

        for (auto elt : pending) {
            mDeviceOperation(elt);
            mDeviceOperationPost(elt);
        }

        // We need to do swap() above so that we can check if
        // the timer needs to be re-armed.
        // (if someone added an event during processing)
        lock.lock();
        if (mPending.size()) {
            mTimer->startAbsolute(0);
        }
    }

    VmLock* mVmLock = nullptr;

    device_operation_t mDeviceOperation = nullptr;
    device_operation_t mDeviceOperationPost = nullptr;

    Lock mLock;
    Pending mPending;
    std::unique_ptr<Looper::Timer> mTimer;
};

