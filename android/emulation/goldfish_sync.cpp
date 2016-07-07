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

#include "android/base/memory/LazyInstance.h"

#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

static GoldfishSyncDeviceInterface* sGoldfishSyncHwFuncs = NULL;

class GoldfishSyncCommandQueueWithDefaultParams :
    public GoldfishSyncCommandQueue {
public:
    GoldfishSyncCommandQueueWithDefaultParams() {
        assert(sGoldfishSyncHwFuncs);

        goldfish_sync_set_cmd_receiver(sGoldfishSyncHwFuncs,
                                       (GoldfishSyncCommandQueue*)this,
                                       ::receiveCommandResult);

        setQueueCommands(sGoldfishSyncHwFuncs->doHostCommand,
                         sGoldfishSyncHwFuncs->receiveCommandResult);
    }
};

static android::base::LazyInstance<GoldfishSyncCommandQueueWithDefaultParams>
sCmdQueue = LAZY_INSTANCE_INIT;

static GoldfishSyncCommandQueue* getCmdQueue() {
    return sCmdQueue.ptr();
}

uint64_t goldfish_sync_create_timeline() {
    GoldfishSyncCommand cmd(CMD_CREATE_SYNC_TIMELINE,
                            0, 0);
    getCmdQueue()->sendCommandAndGetResult(&cmd);
    uint64_t res = cmd.handle;
    DPRINT("create timeline returning 0x%llx", cmd.handle);
    return res;
}

int goldfish_sync_create_fence(uint64_t timeline,
                               uint32_t pt) {
    GoldfishSyncCommand cmd(CMD_CREATE_SYNC_FENCE,
                            timeline,
                            pt);
    getCmdQueue()->sendCommandAndGetResult(&cmd);
    return (int)cmd.handle;
}

void goldfish_sync_timeline_inc(uint64_t timeline,
                                uint32_t howmuch) {
    GoldfishSyncCommand* cmd = new GoldfishSyncCommand(CMD_SYNC_TIMELINE_INC,
                            timeline,
                            howmuch);
    DPRINT("timeline inc iwth cmd=0x%p", cmd);
    // GoldfishSyncCommand cmd(CMD_SYNC_TIMELINE_INC,
    //                         timeline,
    //                         howmuch);
    getCmdQueue()->sendCommandAndGetResult(cmd);
    delete cmd;
}

void goldfish_sync_destroy_timeline(uint64_t timeline) {
    DPRINT("Destroy timeline 0x%llx", timeline);
    GoldfishSyncCommand cmd(CMD_DESTROY_SYNC_TIMELINE,
                            timeline, 0);
    getCmdQueue()->sendCommandAndGetResult(&cmd);
}

void goldfish_sync_register_trigger_wait(trigger_wait_fn_t f) {
    sGoldfishSyncHwFuncs->registerTriggerWait(f);
}

void goldfish_sync_set_hw_funcs(
        GoldfishSyncDeviceInterface* hw_funcs) {
    sGoldfishSyncHwFuncs = hw_funcs;
}

static bool wrapped = false;

void goldfish_sync_set_cmd_receiver(
        GoldfishSyncDeviceInterface* hw_funcs,
        void* context,
        device_command_result_t next_recv) {

    assert(!wrapped);

    if (!context || !next_recv) return;

    hw_funcs->context = context;
    hw_funcs->receiveCommandResult = next_recv;

    return;
}

