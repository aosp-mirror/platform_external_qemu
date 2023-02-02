// Copyright 2016 The Android Open Source Project
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

// A small benchmark used to compare the performance of android::base::Lock
// with other mutex implementions.

#include <iostream>  // for operator<<
#include <string>    // for string
#include <utility>   // for pair

#include "aemu/base/streams/RingStreambuf.h"  // for RingStre...
#include "benchmark/benchmark.h"              // for State

using android::base::streams::RingStreambuf;
using namespace std::chrono_literals;
#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->RangeMultiplier(2)->Range(1 << 10, 1 << 14)

void BM_WriteData(benchmark::State& state) {
    std::string src = std::string(state.range_x() / 2, 'a');
    RingStreambuf buf(state.range_x());
    std::ostream stream(&buf);

    while (state.KeepRunning()) {
        stream << src;
    }
}

void BM_WriteAndRead(benchmark::State& state) {
    std::string src = std::string(state.range_x() / 2, 'a');
    src += '\n';
    std::string read;
    RingStreambuf buf(state.range_x());
    std::ostream stream(&buf);
    std::istream in(&buf);

    while (state.KeepRunning()) {
        stream << src;

        // Reading is very slow compared to the logcat scenario, as each
        // character gets read one at a time.
        in >> read;
    }
}

void BM_WriteLogcatScenario(benchmark::State& state) {
    // Mimics what we do in grpc.
    std::string src = std::string(state.range_x() / 2, 'a');
    RingStreambuf buf(state.range_x());
    std::ostream stream(&buf);

    int offset = 0;
    while (state.KeepRunning()) {
        stream << src;
        auto message = buf.bufferAtOffset(offset, 0ms);
        offset += message.second.size();
    }
}

BASIC_BENCHMARK_TEST(BM_WriteData);
BASIC_BENCHMARK_TEST(BM_WriteAndRead);
BASIC_BENCHMARK_TEST(BM_WriteLogcatScenario);
BENCHMARK_MAIN();