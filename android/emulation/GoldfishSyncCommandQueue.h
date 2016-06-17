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

#include "android/emulation/DeviceContextRunner.h"

#include "android/emulation/goldfish_sync.h"
#include "android/emulation/VmLock.h"

#pragma once

#include <vector>

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

namespace android {

////////////////////////////////////////////////////////////////////////////////
// GoldfishSyncCommandQueue ensures that commands sent to the Goldfish Sync
// virtual device take place in "device context"; that is, the commands
// are only executed while the main loop has the VM lock. Like PipeWaker
// of AndroidPipe, this class derives from DeviceContextRunner for this
// functionality.

struct GoldfishSyncWakeInfo {
    uint32_t cmd;
    uint64_t handle;
    uint32_t time_arg;
    uint64_t hostcmd_handle;
};

class GoldfishSyncCommandQueue final :
    public DeviceContextRunner<GoldfishSyncWakeInfo> {

public:
    // Like with the PipeWaker for AndroidPipe,
    // we need to process all commands
    // in a context where we hold the VM lock.
    static void initThreading(VmLock* vmLock);

    // Goldfish sync virtual device will give out its own
    // callback for queueing commands to it.
    static void setQueueCommand(queue_device_command_t fx);

    // Main interface for all Goldfish sync device
    // communications.
    static void hostSignal(uint32_t cmd,
                           uint64_t handle,
                           uint32_t time_arg,
                           uint64_t hostcmd_handle);

private:
    virtual void performDeviceOperation(const GoldfishSyncWakeInfo& cmd);

    queue_device_command_t tellSyncDevice = nullptr;
};

} // namespace android
