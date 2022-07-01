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

#include <memory>
#include <string>
#include <vector>

namespace android {

using base::Stream;

base::LazyInstance<GoldfishSyncCommandQueue>
GoldfishSyncCommandQueue::sCommandQueue = LAZY_INSTANCE_INIT;

// static
void GoldfishSyncCommandQueue::initThreading(VmLock* vmLock) {
    sCommandQueue.ptr()->init(vmLock);
}

// static
void GoldfishSyncCommandQueue::setQueueCommand(queue_device_command_t fx) {
    GoldfishSyncCommandQueue* cmdQueue = sCommandQueue.ptr();
    cmdQueue->tellSyncDevice = fx;
}

// static
void GoldfishSyncCommandQueue::hostSignal(uint32_t cmd,
                                          uint64_t handle,
                                          uint32_t time_arg,
                                          uint64_t hostcmd_handle) {

    GoldfishSyncCommandQueue* queue = sCommandQueue.ptr();

    GoldfishSyncWakeInfo sync_data;
    sync_data.cmd = cmd;
    sync_data.handle = handle;
    sync_data.time_arg = time_arg;
    sync_data.hostcmd_handle = hostcmd_handle;

    queue->queueDeviceOperation(sync_data);
}

// static
void GoldfishSyncCommandQueue::save(Stream* stream) {
    GoldfishSyncCommandQueue* queue = sCommandQueue.ptr();
    stream->putBe32(queue->numPending());
    queue->forEachPendingOperation([stream](const GoldfishSyncWakeInfo& wakeInfo) {
            stream->putBe64(wakeInfo.handle);
            stream->putBe64(wakeInfo.hostcmd_handle);
            stream->putBe32(wakeInfo.cmd);
            stream->putBe32(wakeInfo.time_arg);
    });
}

// static
void GoldfishSyncCommandQueue::load(Stream* stream) {
    GoldfishSyncCommandQueue* queue = sCommandQueue.ptr();
    queue->removeAllPendingOperations(
        [](const GoldfishSyncWakeInfo&) { return true; });
    uint32_t pending = stream->getBe32();
    for (uint32_t i = 0; i < pending; i++) {
        GoldfishSyncWakeInfo cmd = {
            stream->getBe64(), // handle
            stream->getBe64(), // hostcmd_handle
            stream->getBe32(), // cmd
            stream->getBe32(), // time_arg
        };
        queue->queueDeviceOperation(cmd);
    }
}

void GoldfishSyncCommandQueue::performDeviceOperation
    (const GoldfishSyncWakeInfo& wakeInfo) {
    tellSyncDevice(wakeInfo.cmd,
                   wakeInfo.handle,
                   wakeInfo.time_arg,
                   wakeInfo.hostcmd_handle);
}

} // namespace android
