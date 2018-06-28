#include "HostGoldfishSync.h"

#include "android/emulation/GoldfishSyncCommandQueue.h"
#include "android/emulation/goldfish_sync.h"
#include "android/emulation/testing/TestVmLock.h"

#include <functional>

#include <string.h>

namespace emugl {

static trigger_wait_fn_t sTriggerWaitFn = nullptr;

// These callbacks are called from the host sync service to operate
// on the virtual device (.doHostCommand) or register a trigger-waiting
// callback that will be invoked later from the device
// (.registerTriggerWait).
static GoldfishSyncDeviceInterface kSyncDeviceInterface = {
    .doHostCommand = [](uint32_t cmd, uint64_t handle, uint32_t time_arg, uint64_t hostcmd_handle) {
        // TODO
    },
    .registerTriggerWait = [](trigger_wait_fn_t fn) {
        sTriggerWaitFn = fn;
    },
};

bool host_goldfish_sync_init(android::VmLock* vmLock) {
    goldfish_sync_set_hw_funcs(&kSyncDeviceInterface);
    android::GoldfishSyncCommandQueue::initThreading(vmLock);
}

} // namespace emugl
