/* Copyright (C) 2018 The Android Open Source Project
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

#include "android/emulation/GoldfishHostmemCommandQueue.h"

#include <memory>
#include <string>
#include <vector>

namespace android {

using base::Stream;

base::LazyInstance<GoldfishHostmemCommandQueue>
GoldfishHostmemCommandQueue::sCommandQueue = LAZY_INSTANCE_INIT;

// static
void GoldfishHostmemCommandQueue::initThreading(VmLock* vmLock) {
    sCommandQueue.ptr()->init(vmLock);
}

// static
void GoldfishHostmemCommandQueue::setQueueCommand(hostmem_queue_device_command_t fx) {
    GoldfishHostmemCommandQueue* cmdQueue = sCommandQueue.ptr();
    cmdQueue->tellHostmemDevice = fx;
}

// static
void GoldfishHostmemCommandQueue::hostSignal(uint64_t gpa,
                                             uint64_t id,
                                             void* hostmem,
                                             uint64_t size,
                                             uint64_t status_code,
                                             uint64_t cmd) {

    GoldfishHostmemCommandQueue* queue = sCommandQueue.ptr();

    GoldfishHostmemWakeInfo hostmem_cmd_data;

    hostmem_cmd_data.gpa = gpa;
    hostmem_cmd_data.id = id;
    hostmem_cmd_data.hostmem = hostmem;
    hostmem_cmd_data.size = size;
    hostmem_cmd_data.status_code = status_code;
    hostmem_cmd_data.cmd = cmd;

    queue->queueDeviceOperation(hostmem_cmd_data);
}

// static
void GoldfishHostmemCommandQueue::save(Stream* stream) {
    GoldfishHostmemCommandQueue* queue = sCommandQueue.ptr();
    stream->putBe32(queue->numPending());
    queue->forEachPendingOperation([stream](const GoldfishHostmemWakeInfo& wakeInfo) {
        stream->putBe64(wakeInfo.gpa);
        stream->putBe64(wakeInfo.id);
        stream->putBe64((uint64_t)(uintptr_t)(wakeInfo.hostmem));
        stream->putBe64(wakeInfo.size);
        stream->putBe64(wakeInfo.status_code);
        stream->putBe64(wakeInfo.cmd);
    });
}

// static
void GoldfishHostmemCommandQueue::load(Stream* stream) {
    GoldfishHostmemCommandQueue* queue = sCommandQueue.ptr();
    queue->removeAllPendingOperations(
        [](const GoldfishHostmemWakeInfo&) { return true; });
    // uint32_t pending = stream->getBe32();
    // for (uint32_t i = 0; i < pending; i++) {
    //     GoldfishHostmemWakeInfo cmd = {
    //         stream->getBe64(), // id
    //         stream->getBe64(), // hostmem
    //         stream->getBe64(), // size
    //         stream->getBe64(), // status_code
    //         stream->getBe64(), // cmd
    //     };
    //     queue->queueDeviceOperation(cmd);
    // }
}

void GoldfishHostmemCommandQueue::performDeviceOperation
    (const GoldfishHostmemWakeInfo& wakeInfo) {
    tellHostmemDevice(wakeInfo.gpa,
                      wakeInfo.id,
                      wakeInfo.hostmem,
                      wakeInfo.size,
                      wakeInfo.status_code,
                      wakeInfo.cmd);
}

} // namespace android

