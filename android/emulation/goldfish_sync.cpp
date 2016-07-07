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

#include "android/emulation/goldfish_sync.h"
#include "android/emulation/GoldfishSyncCommandQueue.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <unordered_map>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::Lock;

// Tag each command with unique id's.
static uint64_t sUniqueId = 0;
static Lock sUniqueIdLock;

uint64_t next_unique_id() {
    AutoLock lock(sUniqueIdLock);
    uint64_t res = sUniqueId;
    sUniqueId += 1;
    return res;
}

struct CommandWaitInfo {
    CommandWaitInfo() : lock(), done(false), cvDone() { }
    Lock lock;
    bool done;
    ConditionVariable cvDone;
    uint64_t return_value;
};

static std::unordered_map<uint64_t, CommandWaitInfo*> wait_map;

static CommandWaitInfo* allocWait(uint64_t id) {
    AutoLock lock(sUniqueIdLock);
    CommandWaitInfo* res = new CommandWaitInfo;
    wait_map[id] = res;
    return res;
}

static void freeWait(uint64_t id) {
    AutoLock lock(sUniqueIdLock);
    if (auto elt = android::base::find(wait_map, id)) {
        delete *elt;
        wait_map.erase(id);
    }
}

static GoldfishSyncDeviceInterface* sGoldfishSyncHwFuncs = NULL;


static void sendCommand(uint32_t cmd,
                        uint64_t handle,
                        uint32_t time_arg,
                        uint64_t hostcmd_handle) {
    GoldfishSyncCommandQueue::hostSignal(cmd, handle, time_arg, hostcmd_handle);
}

static void receiveCommandResult(void* data,
                                 uint32_t cmd,
                                 uint64_t handle,
                                 uint32_t time_arg,
                                 uint64_t hostcmd_handle) {
    if (auto elt = android::base::find(wait_map, hostcmd_handle)) {
        CommandWaitInfo* wait_info = *elt;
        AutoLock lock(wait_info->lock);
        wait_info->return_value = handle;
        wait_info->done = true;
        wait_info->cvDone.broadcast();
    }
}

static uint64_t sendCommandAndGetResult(uint64_t cmd,
                                    uint64_t handle,
                                    uint64_t time_arg,
                                    uint64_t hostcmd_handle) {
    // Set the "received command" callback if not set already
    goldfish_sync_set_cmd_receiver(sGoldfishSyncHwFuncs, receiveCommandResult);

    // Do the usual thing when sending commands
    sendCommand(cmd, handle, time_arg, hostcmd_handle);

    CommandWaitInfo* waitInfo = allocWait(hostcmd_handle);

    AutoLock lock(waitInfo->lock);
    while (!waitInfo->done) {
        waitInfo->cvDone.wait(&waitInfo->lock);
    }

    uint64_t res = waitInfo->return_value;

    lock.unlock();
    freeWait(hostcmd_handle);

    return res;
}

uint64_t goldfish_sync_create_timeline() {
    return sendCommandAndGetResult(CMD_CREATE_SYNC_TIMELINE,
                                   0, 0, next_unique_id());
}

int goldfish_sync_create_fence(uint64_t timeline, uint32_t pt) {
    return (int)sendCommandAndGetResult(CMD_CREATE_SYNC_FENCE,
                                        timeline, pt, next_unique_id());
}

void goldfish_sync_timeline_inc(uint64_t timeline, uint32_t howmuch) {
    sendCommand(CMD_SYNC_TIMELINE_INC, timeline, howmuch, next_unique_id());
}

void goldfish_sync_destroy_timeline(uint64_t timeline) {
    sendCommand(CMD_DESTROY_SYNC_TIMELINE, timeline, 0, next_unique_id());
}

void goldfish_sync_register_trigger_wait(trigger_wait_fn_t f) {
    sGoldfishSyncHwFuncs->registerTriggerWait(f);
}

void goldfish_sync_set_hw_funcs(GoldfishSyncDeviceInterface* hw_funcs) {
    sGoldfishSyncHwFuncs = hw_funcs;
    GoldfishSyncCommandQueue::setQueueCommand(sGoldfishSyncHwFuncs->doHostCommand);
}

static bool wrapped = false;

void goldfish_sync_set_cmd_receiver(
        GoldfishSyncDeviceInterface* hw_funcs,
        device_command_result_t next_recv) {

    if (wrapped || !next_recv) return;

    hw_funcs->receiveCommandResult = next_recv;

    wrapped = true;

    return;
}

