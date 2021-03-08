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

#include <fstream>
#include <iostream>  // for operator<<
#include <string>    // for string
#include <utility>   // for pair

#include "android/base/files/PathUtils.h"
#include "android/base/files/TarStream.h"
#include "android/base/system/System.h"
#include "android/utils/Random.h"
#include "benchmark/benchmark_api.h"  // for State

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->RangeMultiplier(2)->Range(1 << 10, 1 << 20)

using android::base::System;
using android::base::TarWriter;

void SetUp() {
    auto tmpdir = System::get()->getTempDir();
    auto tstfile = android::base::pj(tmpdir, "randomdata.txt");

    if (!System::get()->pathExists(tstfile)) {
        std::cout << "Test file in: " << tstfile << std::endl;

        std::ofstream rnd(tstfile, std::ios::binary);
        char sz[4096];
        for (int i = 0; i < 25600; i++) {
            android::generateRandomBytes(sz, sizeof(sz));
            rnd.write(sz, sizeof(sz));
        }
    }
}

std::ostream* nullstream() {
    static std::ofstream os;
    if (!os.is_open())
        os.open("/dev/null", std::ofstream::out | std::ofstream::app);
    return &os;
}

void BM_TarStreamTest(benchmark::State& st) {
    SetUp();
    auto null = nullstream();
    while (st.KeepRunning()) {
        TarWriter tw(System::get()->getTempDir(), *null, st.range_x());
        tw.addFileEntry("randomdata.txt");
    }
}

BASIC_BENCHMARK_TEST(BM_TarStreamTest);
