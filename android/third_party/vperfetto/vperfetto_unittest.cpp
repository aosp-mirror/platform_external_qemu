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
#include "vperfetto.h"

#include <gtest/gtest.h>

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#include <errno.h>
#include <stdio.h>
#endif

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <cstdio>
#include <filesystem>

namespace vperfetto {

static void runTrace(uint32_t iterations) {
    enableTracing();
    for (uint32_t i = 0; i < iterations; ++i) {
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
    waitSavingDone();
}

TEST(PerfettoTracingOnly, Basic) {
    const bool* tracingDisabledPtr;
    initialize(&tracingDisabledPtr);
    EXPECT_TRUE(*tracingDisabledPtr);

    static char trace1FileName[L_tmpnam];
    static char trace2FileName[L_tmpnam];
    static char combinedFileName[L_tmpnam];

    if (!std::tmpnam(trace1FileName)) {
        FAIL() << "Could not generate trace1 file name";
        return;
    }

    if (!std::tmpnam(trace2FileName)) {
        FAIL() << "Could not generate trace2 file name";
        return;
    }

    if (!std::tmpnam(combinedFileName)) {
        FAIL() << "Could not generate combined file name";
        return;
    }

    fprintf(stderr, "%s: temp names: %s %s %s\n", __func__, trace1FileName, trace2FileName, combinedFileName);
   
    // Generate trace1, which stands in for the guest
    uint64_t startTimeNs = bootTimeNs();
    setGuestTime(startTimeNs);
    setTraceConfig([](VirtualDeviceTraceConfig& config) {
        config.hostFilename = trace1FileName;
        config.guestFilename = nullptr;
    });

    runTrace(400);

    // Generate trace2 and combined. which stands in for the guest
    setTraceConfig([](VirtualDeviceTraceConfig& config) {
        config.hostFilename = trace2FileName;
        config.guestFilename = trace1FileName;
        config.combinedFilename = combinedFileName;
    });

    runTrace(400);

    std::filesystem::remove(std::filesystem::path(combinedFileName));
    std::filesystem::remove(std::filesystem::path(trace1FileName));
    std::filesystem::remove(std::filesystem::path(trace2FileName));
}

} // namespace virtualdeviceperfetto
