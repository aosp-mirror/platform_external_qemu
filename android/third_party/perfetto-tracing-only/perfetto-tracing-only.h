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

namespace virtualdeviceperfetto {

struct TraceWriterOperations {
    std::function<void()> openSink;
    std::function<void()> closeSink;
    std::function<void(uint8_t*, size_t)> writeToSink;
};

struct VirtualDeviceTraceConfig {
    bool initialized;
    bool tracingDisabled;
    uint32_t packetsWritten;
    bool sequenceIdWritten;
    uint32_t currentInterningId;
    uint32_t currentThreadId;
    const char* filename;
    uint64_t guestStartTime;
    uint64_t hostStartTime;
    bool useGuestStartTime;
    TraceWriterOperations ops;
};

void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)>);
VirtualDeviceTraceConfig queryTraceConfig();

// An optimization to have faster queries of whether tracing is enabled.
void initialize(const bool** tracingDisabledPtr);

void enableTracing();
void disableTracing();

void beginTrace(const char* eventName);
void endTrace();
void traceCounter(const char* name, int64_t value);

void basic_test();

} // namespace virtualdeviceperfetto
