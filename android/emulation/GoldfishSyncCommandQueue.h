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

#pragma once

#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/MessageChannel.h"

#include "android/emulation/goldfish_sync.h"
#include "android/emulation/VmLock.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(goldfishsync)) VERBOSE_ENABLE(goldfishsync); \
    VERBOSE_TID_FUNCTION_DPRINT(goldfishsync, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

#define ERR(...) do { \
    derror(__VA_ARGS__); \
} while(0)

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::Lock;
using android::base::MessageChannel;
using android::base::Thread;
using android::VmLock;

class GoldfishSyncCommand {
public:
    GoldfishSyncCommand(uint32_t cmd,
                        uint64_t handle,
                        uint32_t time_arg) :
        cmd(cmd),
        handle(handle),
        time_arg(time_arg),
        async(false) { }

    GoldfishSyncCommand(uint32_t cmd,
                        uint64_t handle,
                        uint32_t time_arg,
                        uint32_t id) :
        cmd(cmd),
        handle(handle),
        time_arg(time_arg),
        async(false), id(id) { }

    uint32_t cmd;
    uint64_t handle;
    uint32_t time_arg;
    bool async;
    uint32_t id;

    Lock lock;
    bool done;
    ConditionVariable cvDone;
};

class GoldfishSyncWaker;
void GoldfishSyncWakerInitThreading(VmLock* vmLock);

class GoldfishSyncCommandQueue : public Thread {
public:
    GoldfishSyncCommandQueue();
    void initThreading();
    void setQueueCommands(queue_device_command_t queueCmd,
                          device_command_result_t getCmdResult);

    // For users of goldfish sync command queue.
    void sendCommandAndGetResult(GoldfishSyncCommand* cmd);
    void sendCommand(GoldfishSyncCommand* cmd);

    // Queue uses this to talk to waker.
    // void wakeCommand(
    //         uint32_t cmd,
    //         uint64_t handle,
    //         uint32_t time_arg,
    //         uint32_t* cmd_out,
    //         uint64_t* handle_out,
    //         uint32_t* time_arg_out);
    
    // Waker uses this to actually communicate with device.
    void sendCommandToDevice(
            uint32_t cmd,
            uint64_t handle,
            uint32_t time_arg,
            uint64_t hostcmd_handle);

    void receiveCommandResult(uint32_t cmd,
                              uint64_t handle,
                              uint32_t time_arg,
                              uint64_t hostcmd_handle);
private:
    virtual intptr_t main() override;

    GoldfishSyncWaker* mSyncWaker;

    GoldfishSyncCommand* mCurrentCommand;
    Lock mLock;

    MessageChannel<GoldfishSyncCommand*, 512U> mInput;
    queue_device_command_t mQueueCommand;
    device_command_result_t mGetCommandResult;
};

void receiveCommandResult(void* queue,
                          uint32_t cmd,
                          uint64_t handle,
                          uint32_t time_arg,
                          uint64_t hostcmd_handle);
