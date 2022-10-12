// Copyright 2019 The Android Open Source Project
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
#include "aemu/base/perflogger/BenchmarkLibrary.h"

#include "aemu/base/perflogger/Benchmark.h"

#include <sstream>
#include <string>
#include <string_view>

namespace android {
namespace perflogger {

// OpenGL benchmarks
void logDrawCallOverheadTest(
        std::string_view glVendor,
        std::string_view glRenderer,
        std::string_view glVersion,

        // |variant|: drawArrays, drawElements, drawSwitchVao, drawSwitchVap
        std::string_view variant,
        // |indirectionLevel|: fullStack, fullStackOverheadOnly, decoder, or
        // host
        std::string_view indirectionLevel,
        int count,
        long rateHz,
        long wallTimeUs,
        long threadCpuTimeUs) {
    std::stringstream ssBenchName;

    ssBenchName << "Draw Call Overhead Test: ";
    ssBenchName << "[" << glVendor << "] ";
    ssBenchName << "[" << glRenderer << "] ";
    ssBenchName << "[" << glVersion << "]";

    std::string benchName = ssBenchName.str();

    Benchmark bench(
        benchName,
        "AndroidEmulator",
        "Tests draw call rate of emulator "
        "graphics driver at various levels",
        {});

    std::stringstream ssMetricName;

    ssMetricName << variant << "_";
    ssMetricName << indirectionLevel << "_";
    ssMetricName << "count_" << count;

    std::string metricBaseName = ssMetricName.str();

    bench.log(metricBaseName + "_rate", rateHz);
    bench.log(metricBaseName + "_wallTimeUs", wallTimeUs);
    bench.log(metricBaseName + "_threadCpuTimeUs", threadCpuTimeUs);
}

} // namespace perflogger
} // namespace android
