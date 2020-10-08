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
#include "perfetto-tracing-only.h"

#include <gtest/gtest.h>

namespace virtualdeviceperfetto {

TEST(PerfettoTracingOnly, Basic) {
    setTraceConfig([](VirtualDeviceTraceConfig& config) {
        config.hostFilename = "test1.trace";
        config.guestFilename = nullptr;
    });
    enableTracing();
    for (uint32_t i = 0; i < 4000; ++i) {
        traceCounter("counter1", 1);
        beginTrace("test trace 1");
        traceCounter("counter1", 2);
          beginTrace("test trace 1.1");
        traceCounter("counter1", 3);
          endTrace();
        traceCounter("counter1", 4);
        endTrace();
        beginTrace("test trace 2");
        endTrace();
    }
    disableTracing();

    setTraceConfig([](VirtualDeviceTraceConfig& config) {
        config.hostFilename = "test2.trace";
        config.guestFilename = nullptr;
    });

    enableTracing();
    for (uint32_t i = 0; i < 40; ++i) {
        traceCounter("counter1", 1);
        beginTrace("test trace 1");
        traceCounter("counter1", 2);
          beginTrace("test trace 1.1");
        traceCounter("counter1", 3);
          endTrace();
        traceCounter("counter1", 4);
        endTrace();
        beginTrace("test trace 2");
        endTrace();
    }
    disableTracing();
}

} // namespace virtualdeviceperfetto
