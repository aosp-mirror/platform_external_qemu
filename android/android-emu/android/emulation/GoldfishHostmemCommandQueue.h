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

#pragma once

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/DeviceContextRunner.h"
#include "android/emulation/goldfish_hostmem.h"
#include "android/emulation/VmLock.h"
#include "android/utils/stream.h"

#include <vector>

#define DEBUG 0

#if DEBUG
#define DPRINT(fmt,...) do { \
    fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
#else
#define DPRINT(...)
#endif

#define ERR(...) do { \
    derror(__VA_ARGS__); \
} while(0)

namespace android {

////////////////////////////////////////////////////////////////////////////////
// GoldfishHostmemCommandQueue ensures that commands sent to the Goldfish Hostmem
// virtual device take place in "device context"; that is, the commands
// are only executed while the main loop has the VM lock. Like PipeWaker
// of AndroidPipe, this class derives from DeviceContextRunner for this
// functionality.

struct GoldfishHostmemWakeInfo {
    uint64_t gpa;
    uint64_t id;
    void* hostmem;
    uint64_t size;
    uint64_t status_code;
    uint64_t cmd;
};

class GoldfishHostmemCommandQueue final :
    public DeviceContextRunner<GoldfishHostmemWakeInfo> {

public:
    // Like with the PipeWaker for AndroidPipe,
    // we need to process all commands
    // in a context where we hold the VM lock.
    static void initThreading(VmLock* vmLock);

    // Goldfish hostmem virtual device will give out its own
    // callback for queueing commands to it.
    static void setQueueCommand(hostmem_queue_device_command_t fx);

    // Main interface for all Goldfish hostmem device
    // communications.
    static void hostSignal(uint64_t gpa,
                           uint64_t id,
                           void* hostmem,
                           uint64_t size,
                           uint64_t status_code,
                           uint64_t cmd);

    // Save/load pending operations.
    static void save(android::base::Stream* stream);
    static void load(android::base::Stream* stream);

private:

    static base::LazyInstance<GoldfishHostmemCommandQueue> sCommandQueue;
    virtual void performDeviceOperation(const GoldfishHostmemWakeInfo& cmd) override;

    hostmem_queue_device_command_t tellHostmemDevice = nullptr;
};

} // namespace android
