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

static android::base::LazyInstance<GoldfishSyncCommandQueue> sSyncWaker = LAZY_INSTANCE_INIT;

struct GoldfishSyncWakerData {
    uint32_t cmd;
    uint64_t handle;
    uint32_t time_arg;
    uint64_t hostcmd_handle;
};

static void syncWakerDeviceOp(void* data) {
    GoldfishSyncWakerData* sync_data = (GoldfishSyncWakerData*)data;
    sSyncWaker.ptr()->tellSyncDevice(sync_data->cmd,
                                     sync_data->handle,
                                     sync_data->time_arg,
                                     sync_data->hostcmd_handle);
}

static void syncWakerDeviceOpPost(void* data) {
    GoldfishSyncWakerData* sync_data = (GoldfishSyncWakerData*)data;
    delete sync_data;
}

void GoldfishSyncCommandQueue::initThreading(VmLock* lock) {
    sSyncWaker.ptr()->init(lock);
}

void GoldfishSyncCommandQueue::setQueueCommand(queue_device_command_t fx) {
    GoldfishSyncCommandQueue* syncWaker = sSyncWaker.ptr();
    if (syncWaker->missingDeviceOperations()) {
        fprintf(stderr, "set goldfish sync waker\n");
        assert(fx);
        syncWaker->setDeviceOperations(syncWakerDeviceOp, syncWakerDeviceOpPost);
        syncWaker->tellSyncDevice = fx;
    }
}

void GoldfishSyncCommandQueue::hostSignal(uint32_t cmd,
                                          uint64_t handle,
                                          uint32_t time_arg,
                                          uint64_t hostcmd_handle) {
    GoldfishSyncCommandQueue* waker = sSyncWaker.ptr();

    GoldfishSyncWakerData* sync_data = new GoldfishSyncWakerData;
    sync_data->cmd = cmd;
    sync_data->handle = handle;
    sync_data->time_arg = time_arg;
    sync_data->hostcmd_handle = hostcmd_handle;

    waker->queueDeviceOperation((void*)sync_data);
}
