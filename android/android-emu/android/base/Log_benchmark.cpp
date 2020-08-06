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

// A small benchmark used to compare the performance of android::base::Lock
// with other mutex implementions.

#include "android/base/Log.h"
#include "android/utils/debug.h"
#include "benchmark/benchmark_api.h"

#define BASIC_BENCHMARK_TEST(x) BENCHMARK(x)->Arg(8)->Arg(512)->Arg(8192)

void BM_LG_Log_simple_string(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);
    while (state.KeepRunning()) {
        LOG(INFO) << "Hello world!";
    }
}

void BM_LG_Log_simple_string_logging_off(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_FATAL);
    while (state.KeepRunning()) {
        LOG(INFO) << "Hello world!";
    }
}

void BM_LG_QLog_simple_string(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);
    while (state.KeepRunning()) {
        QLOG(INFO) << "Hello world!";
    }
}

void BM_LG_VLog_simple_string_off(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);
    android_verbose = 0;
    while (state.KeepRunning()) {
        VLOG(virtualscene) << "Hello virtual world!";
    }
}

void BM_LG_VLog_simple_string_on(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);
    VERBOSE_ENABLE(virtualscene);
    while (state.KeepRunning()) {
        VLOG(virtualscene) << "Hello virtual world!";
    }
}

void BM_LG_Log_simple_int(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);

    int i = 0;
    while (state.KeepRunning()) {
        i++;
        LOG(INFO) << i;
    }
}

void BM_LG_Log_complex_msg(benchmark::State& state) {
    setMinLogLevel(android::base::LogSeverity::LOG_INFO);

    int i = 0;
    int j = 0;
    while (state.KeepRunning()) {
        LOG(INFO) << "Hello, i: " << i++ << ", with: " << j << ", and that's all!";
        j = 2 * i;
    }
}

BENCHMARK(BM_LG_Log_simple_string);
BENCHMARK(BM_LG_Log_simple_string_logging_off);
BENCHMARK(BM_LG_QLog_simple_string);
BENCHMARK(BM_LG_VLog_simple_string_off);
BENCHMARK(BM_LG_VLog_simple_string_on);
BENCHMARK(BM_LG_Log_simple_int);
BENCHMARK(BM_LG_Log_complex_msg);
