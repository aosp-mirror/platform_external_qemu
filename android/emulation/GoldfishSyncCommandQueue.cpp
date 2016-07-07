/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/emulation/GoldfishSyncCommandQueue.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#define WAKE 0

#if WAKE

// Helper class to ensure that commands are sent to the guest
// while holding the VM lock.
class GoldfishSyncWaker final {
public:
    GoldfishSyncWaker(GoldfishSyncCommandQueue* queue) :
        mQueue(queue) { }

    void init(VmLock* vmLock) {
        mVmLock = vmLock;
        Looper* looper = android::base::ThreadLooper::get();
        mTimer.reset(looper->createTimer(
                    [](void* that, Looper::Timer*) {
                    static_cast<GoldfishSyncWaker*>(that)->onTimerEvent();
                    },
                    this));
        if (!mTimer.get()) {
            LOG(WARNING) << "Failed to create a loop timer, falling back "
                "to regular locking!";
        }
    }

    void signal(uint32_t cmd, uint64_t handle, uint32_t time_arg) {
        bool inDeviceContext = mVmLock->isLockedBySelf();
        if (inDeviceContext) {
            // Perform the operation correctly since the current thread
            // already holds the lock that protects the global VM state.
            mQueue->sendCommandToDevice(cmd, handle, time_arg);
        } else {
            // Queue the operation in the mPendingSet structure, then
            // restart the timer.
            base::AutoLock lock(mLock);
            mPendingSet[hwPipe] |= wakeFlags;
            lock.unlock();

            // NOTE: See TODO above why this is thread-safe when used with
            // QEMU1 and QEMU2.
            mTimer->startAbsolute(0);
        }
    }
private:
    // A hash table mapping hwPipe handles to integer flags. Used to implement
    // the set of pending operations when ThreadMode::SingleThreadedWithQueue
    // is used.
    using PendingMap = std::unordered_map<void*, int>;

    // Called whenever the timer is triggered to apply pending operations
    // on the main loop thread.
    void onTimerEvent() {
        base::AutoLock lock(mLock);
        PendingMap pendingMap;
        pendingMap.swap(mPendingSet);
        lock.unlock();

        for (const auto& pair : pendingMap) {
            doPipeOperation(pair.first, pair.second);
        }

        // Check if the timer needs to be re-armed if someone added an event
        // during processing.
        lock.lock();
        if (!mPendingSet.empty()) {
            mTimer->startAbsolute(0);
        }
    }

    // Perform a pipe operation on the device thread.
    void doPipeOperation(void* hwPipe, int flags) {
        CHECK(mVmLock->isLockedBySelf());
        if (flags & PIPE_WAKE_CLOSED) {
            sPipeHwFuncs->closeFromHost(hwPipe);
        } else {
            sPipeHwFuncs->signalWake(hwPipe, flags);
        }
    }

    android::VmLock* mVmLock = nullptr;
    base::Lock mLock;
    PendingSet mPendingSet;
    std::unique_ptr<Looper::Timer> mTimer;
    GoldfishSyncCommandQueue* mQueue;
};

#endif

// Command queue implementation below///////////////////////////////////////////

GoldfishSyncCommandQueue::GoldfishSyncCommandQueue() :
    mCurrentCommand(NULL) {
    this->start();
}

void GoldfishSyncCommandQueue::setQueueCommands
    (queue_device_command_t queueCmd,
     device_command_result_t getCmdResult) {
    mQueueCommand = queueCmd;
    mGetCommandResult = getCmdResult;
}

void GoldfishSyncCommandQueue::sendCommandAndGetResult
    (GoldfishSyncCommand* to_send) {
    DPRINT("call with: "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);

#if DEBUG
    uint32_t orig_cmd = to_send->cmd;
#endif

    mInput.send(to_send);

    AutoLock lock(to_send->lock);

    to_send->done = false;
    while(!to_send->done) {
        DPRINT("wait. cmd=%p", to_send);
        to_send->cvDone.wait(&to_send->lock);
    }

    DPRINT("got from sync thread (orig cmd=%u): "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           orig_cmd,
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);
}

void GoldfishSyncCommandQueue::sendCommand
    (GoldfishSyncCommand* to_send) {
    DPRINT("call with: "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);

#if DEBUG
    uint32_t orig_cmd = to_send->cmd;
#endif

    to_send->async = true;

    mInput.send(to_send);
    AutoLock lock(to_send->lock);

    to_send->done = false;
    while(!to_send->done) {
        DPRINT("wait. cmd=%p", to_send);
        to_send->cvDone.wait(&to_send->lock);
    }
}

void GoldfishSyncCommandQueue::sendCommandToDevice
    (uint32_t cmd, uint64_t handle, uint32_t time_arg, uint64_t hostcmd_handle) {
    mQueueCommand(cmd, handle, time_arg, hostcmd_handle);
}

void GoldfishSyncCommandQueue::receiveCommandResult
    (uint32_t cmd, uint64_t handle, uint32_t time_arg, uint64_t hostcmd_handle) {
    DPRINT("enter");
    GoldfishSyncCommand* recv =
        (GoldfishSyncCommand*)(uintptr_t)(hostcmd_handle);
    assert(recv);
    AutoLock lock(recv->lock);
    DPRINT("got lock");
    recv->cmd = cmd;
    recv->handle = handle;
    recv->time_arg = time_arg;
    recv->done = true;
    DPRINT("set cmds");
    recv->cvDone.signal();
    recv->cvDone.signal();
    recv->cvDone.signal();
    recv->cvDone.signal();
    DPRINT("broadcasted");
    DPRINT("exit");
}

intptr_t GoldfishSyncCommandQueue::main() {
    DPRINT("in sync cmd thread");

    uint32_t num_iter = 0;

    while (true) {
        DPRINT("waiting for sync cmd. this=%p", this);
        mInput.receive(&mCurrentCommand);
        assert(mCurrentCommand);
        DPRINT("run this sync cmd: "
               "cmd=%d "
               "handle=0x%llx "
               "time=%u ",
               mCurrentCommand->cmd,
               mCurrentCommand->handle,
               mCurrentCommand->time_arg);
        num_iter += 1;
        DPRINT("sync cmd thread num iter: %u", num_iter);
        sendCommandToDevice(mCurrentCommand->cmd,
                            mCurrentCommand->handle,
                            mCurrentCommand->time_arg,
                            (uint64_t)(uintptr_t)mCurrentCommand);

        if (mCurrentCommand->async) {
            mCurrentCommand->done = true;
            mCurrentCommand->cvDone.signal();
        }

        // AutoLock lock(mLock);
        // while (!mCurrentCommand->done) {
            // mCurrentCommand->cvDone.wait(&mLock);
        // }
    }

    return 0;
}

void receiveCommandResult(void* queue,
                          uint32_t cmd,
                          uint64_t handle,
                          uint32_t time_arg,
                          uint64_t hostcmd_handle) {
    DPRINT("queue=%p cmd=%u handle=0x%llx time_arg=%u hostcmd_handle=0x%llx",
           queue, cmd, handle, time_arg, hostcmd_handle);
    GoldfishSyncCommandQueue* gsQueue = (GoldfishSyncCommandQueue*)queue;
    gsQueue->receiveCommandResult(cmd, handle, time_arg, hostcmd_handle);
}

