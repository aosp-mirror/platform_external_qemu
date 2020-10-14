// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <functional>
#include <cstdint>

// Convenient declspec dllexport macro for android-emu-shared on Windows
#ifndef PERFETTO_TRACING_ONLY_EXPORT
    #ifdef _MSC_VER
        #define PERFETTO_TRACING_ONLY_EXPORT __declspec(dllexport)
    #else // _MSC_VER
        #define PERFETTO_TRACING_ONLY_EXPORT __attribute__((visibility("default")))
    #endif // !_MSC_VER
#endif // !PERFETTO_TRACING_ONLY_EXPORT

namespace vperfetto {

struct VirtualDeviceTraceConfig {
    bool initialized;
    bool tracingDisabled;
    uint32_t packetsWritten;
    bool sequenceIdWritten;
    uint32_t currentInterningId;
    uint32_t currentThreadId;
    const char* hostFilename;
    const char* guestFilename;
    const char* combinedFilename;
    uint64_t hostStartTime;
    uint64_t guestStartTime;
    uint64_t guestTimeDiff;
    uint32_t perThreadStorageMb;
    bool saving;
};


// Workflow:
//
// 0. Call initialize first.
//
// This needs to be called for Perfetto's static state to be set up (when using the full tracing sdk)
// |tracingDisabledPtr| is a convenience for users who want to set up more CC_LIKELY macros elsewhere.
PERFETTO_TRACING_ONLY_EXPORT void initialize(const bool** tracingDisabledPtr = nullptr);

// 1. The environment variables
//
// VPERFETTO_HOST_FILE
// VPERFETTO_GUEST_FILE
// VPERFETTO_COMBINED_FILE
//
// will override the VirtualDeviceTraceConfig's hostFilename, guestFilename, and combinedFilename.
// Or we can use setTraceConfig to set those fields before tracing:
PERFETTO_TRACING_ONLY_EXPORT void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)>);

// queryTraceConfig() can return a copy of those those fields, along with whether tracing is disabled.
PERFETTO_TRACING_ONLY_EXPORT VirtualDeviceTraceConfig queryTraceConfig();

// Right before tracing starts, guest sends a message with its current time (|guestTime|). Use your favorite transport,
// virtio-gpu, pipe, shared memory, socket, etc, etc. Just somehow wire it up :)
PERFETTO_TRACING_ONLY_EXPORT void setGuestTime(uint64_t guestTime);

// Start tracing. This is meant to be triggered when tracing starts in the guest. Use your favorite transport,
// virtio-gpu, pipe, virtual perfetto, etc etc. Just somehow wire it up :)
PERFETTO_TRACING_ONLY_EXPORT void enableTracing();

// End tracing. This is meant to be triggerd when tracing ends in the guest. Again, use your favorite transport.
// This will also trigger trace saving. It is assumed that at around roughly this time, the host/guest also send over the finished trace from the guest to the host to the path specified in VPERFETTO_GUEST_FILE or traceconfig.guestFilename, such as via `adb pull /data/local/traces/guestfile.trace`.
// After waiting for a while, the guest/host traces are post processed and catted together into VPERFETTO_COMBINED_FILE.
PERFETTO_TRACING_ONLY_EXPORT void disableTracing();

// Start/end a particular track event on the host. By default, every such event is in the 'gfx' category.
PERFETTO_TRACING_ONLY_EXPORT void beginTrace(const char* eventName);
PERFETTO_TRACING_ONLY_EXPORT void endTrace();

// Record a counter. This currently doesn't work with the -sdk.cpp implementation. TODO(lfy): add it
PERFETTO_TRACING_ONLY_EXPORT void traceCounter(const char* name, int64_t value);

// Miscellanous APIs that are useful but fall outside the standard workflow
// Obtains BOOTTIME nanoseconds the way perfetto sdk calculates it.
PERFETTO_TRACING_ONLY_EXPORT uint64_t bootTimeNs();
// Microsecond sleep according to perfetto sdk.
PERFETTO_TRACING_ONLY_EXPORT void sleepUs(unsigned);
PERFETTO_TRACING_ONLY_EXPORT void waitSavingDone();

} // namespace vperfetto
