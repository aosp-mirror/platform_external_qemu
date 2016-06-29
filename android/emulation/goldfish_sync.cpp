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

#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/MessageChannel.h"

#include "android/emulation/goldfish_sync.h"

#include "android/utils/debug.h"
#include "android/utils/system.h"

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

static const GoldfishSyncHwFuncs* sGoldfishSyncHwFuncs = NULL;

using android::base::Lock;
using android::base::AutoLock;
using android::base::ConditionVariable;

using android::base::LazyInstance;
using android::base::Thread;
using android::base::MessageChannel;

class GoldfishSyncCommand {
public:
    GoldfishSyncCommand(uint32_t cmd,
                        uint64_t handle,
                        uint32_t time_arg) :
        cmd(cmd),
        handle(handle),
        time_arg(time_arg) { }

    uint32_t cmd;
    uint64_t handle;
    uint32_t time_arg;
    Lock lock;
    bool done;
    ConditionVariable cvDone;
};

class SyncCmdThread : public Thread {
public:
    SyncCmdThread();
    static SyncCmdThread* get();
    void sendCommandAndGetResult(GoldfishSyncCommand* cmd);
private:
    virtual intptr_t main() override;
    MessageChannel<GoldfishSyncCommand*, 4U> mInput;
};

static LazyInstance<SyncCmdThread> sSyncCmdThread = LAZY_INSTANCE_INIT;

SyncCmdThread::SyncCmdThread() {
    this->start();
}

SyncCmdThread* SyncCmdThread::get() {
    return sSyncCmdThread.ptr();
}

void SyncCmdThread::sendCommandAndGetResult(GoldfishSyncCommand* to_send) {
    DPRINT("call with: "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);

    uint32_t orig_cmd = to_send->cmd;
    mInput.send(to_send);

    AutoLock(to_send->lock);

    to_send->done = false;
    while(!to_send->done) {
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

intptr_t SyncCmdThread::main() {
    DPRINT("in sync cmd thread");

    GoldfishSyncCommand* currentCommand;
    uint32_t cmd_out;
    uint64_t handle_out;
    uint32_t time_arg_out;

    while (true) {
        DPRINT("waiting for sync cmd");
        mInput.receive(&currentCommand);
        uint32_t origcmd= currentCommand->cmd;
        DPRINT("run this sync cmd: "
               "cmd=%d "
               "handle=0x%llx "
               "time=%u ",
               currentCommand->cmd,
               currentCommand->handle,
               currentCommand->time_arg);
        sGoldfishSyncHwFuncs->doHostCommand(
                currentCommand->cmd,
                currentCommand->handle,
                currentCommand->time_arg,
                &cmd_out, &handle_out, &time_arg_out);
        DPRINT("done with cmd. guest wrote: "
               "origcmd=%u "
               "cmd=%d "
               "handle=0x%llx "
               "time=%u ",
               origcmd,
               cmd_out, handle_out, time_arg_out);
        AutoLock(currentCommand->lock);
        currentCommand->cmd = cmd_out;
        currentCommand->handle = handle_out;
        currentCommand->time_arg = time_arg_out;
        currentCommand->done = true;
        currentCommand->cvDone.signal();
    }

    return 0;
}

const GoldfishSyncHwFuncs* goldfish_sync_set_hw_funcs(
        const GoldfishSyncHwFuncs* hw_funcs) {
        const GoldfishSyncHwFuncs* result = sGoldfishSyncHwFuncs;
        sGoldfishSyncHwFuncs = hw_funcs;
        return result;
}

uint64_t goldfish_sync_create_timeline() {
    GoldfishSyncCommand cmd(CMD_CREATE_SYNC_TIMELINE,
                            0, 0);
    SyncCmdThread::get()->sendCommandAndGetResult(&cmd);
    uint64_t res = cmd.handle;
    DPRINT("create timeline returning 0x%llx", cmd.handle);
    return res;
}

int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt) {
    GoldfishSyncCommand cmd(CMD_CREATE_SYNC_FENCE,
                            timeline,
                            pt);
    SyncCmdThread::get()->sendCommandAndGetResult(&cmd);
    return (int)cmd.handle;
}

void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch) {
    GoldfishSyncCommand cmd(CMD_SYNC_TIMELINE_INC,
                            timeline,
                            howmuch);
    SyncCmdThread::get()->sendCommandAndGetResult(&cmd);
}

void goldfish_sync_destroy_timeline(uint64_t timeline) {
    DPRINT("Destroy timeline 0x%llx", timeline);
    GoldfishSyncCommand cmd(CMD_DESTROY_SYNC_TIMELINE,
                            timeline, 0);
    SyncCmdThread::get()->sendCommandAndGetResult(&cmd);
}

void goldfish_sync_register_trigger_wait(trigger_wait_fn_t f) {
    sGoldfishSyncHwFuncs->registerTriggerWait(f);
}

#if TEST_GOLDFISH_SYNC

#include <stdio.h>
#include <pthread.h>

void goldfish_sync_run_test() {
    fprintf(stderr, "%s: testing!\n", __FUNCTION__);
}

#endif


