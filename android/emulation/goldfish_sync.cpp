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

static const GoldfishSyncHwFuncs* sGoldfishSyncHwFuncs = NULL;

class GoldfishSyncCommandQueueWithDefaultParams :
    public GoldfishSyncCommandQueue {
public:
    GoldfishSyncCommandQueueWithDefaultParams() {
        assert(sGoldfishSyncHwFuncs);
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
    GoldfishSyncCommand cmd(CMD_SYNC_TIMELINE_INC,
                            timeline,
                            howmuch);
    getCmdQueue()->sendCommandAndGetResult(&cmd);
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

const GoldfishSyncHwFuncs* goldfish_sync_set_hw_funcs(
        const GoldfishSyncHwFuncs* hw_funcs) {
        const GoldfishSyncHwFuncs* result = sGoldfishSyncHwFuncs;
        sGoldfishSyncHwFuncs = hw_funcs;
        return result;
}

