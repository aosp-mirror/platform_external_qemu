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

#include "android/base/memory/LazyInstance.h"

#include <memory>
#include <string>
#include <vector>

using android::base::LazyInstance;

static android::base::LazyInstance<GoldfishSyncCommandQueue>
sCommandQueue = LAZY_INSTANCE_INIT;

static void commandQueueDeviceOp(const GoldfishSyncWakeInfo& wakeInfo) {
    sCommandQueue.ptr()->tellSyncDevice(wakeInfo.cmd,
                                     wakeInfo.handle,
                                     wakeInfo.time_arg,
                                     wakeInfo.hostcmd_handle);
}

void GoldfishSyncCommandQueue::initThreading(VmLock* lock) {
    sCommandQueue.ptr()->init(lock);
}

void GoldfishSyncCommandQueue::setQueueCommand(queue_device_command_t fx) {
    GoldfishSyncCommandQueue* cmdQueue = sCommandQueue.ptr();
    if (cmdQueue->missingDeviceOperation()) {
        fprintf(stderr, "set goldfish sync command queue\n");
        assert(fx);
        cmdQueue->setDeviceOperation(commandQueueDeviceOp);
        cmdQueue->tellSyncDevice = fx;
    }
}

void GoldfishSyncCommandQueue::hostSignal(uint32_t cmd,
                                          uint64_t handle,
                                          uint32_t time_arg,
                                          uint64_t hostcmd_handle) {
    GoldfishSyncCommandQueue* queue = sCommandQueue.ptr();

    GoldfishSyncWakeInfo sync_data = {
        .cmd = cmd,
        .handle = handle,
        .time_arg = time_arg,
        .hostcmd_handle = hostcmd_handle,
    };

    queue->queueDeviceOperation(sync_data);
}
