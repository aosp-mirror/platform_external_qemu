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
};

PERFETTO_TRACING_ONLY_EXPORT void initialize(const bool** tracingDisabledPtr = nullptr);

PERFETTO_TRACING_ONLY_EXPORT void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)>);
PERFETTO_TRACING_ONLY_EXPORT VirtualDeviceTraceConfig queryTraceConfig();


PERFETTO_TRACING_ONLY_EXPORT void enableTracing();
PERFETTO_TRACING_ONLY_EXPORT void disableTracing();

PERFETTO_TRACING_ONLY_EXPORT void beginTrace(const char* eventName);
PERFETTO_TRACING_ONLY_EXPORT void endTrace();
PERFETTO_TRACING_ONLY_EXPORT void traceCounter(const char* name, int64_t value);

PERFETTO_TRACING_ONLY_EXPORT void setGuestTime(uint64_t guestTime);

} // namespace vperfetto
