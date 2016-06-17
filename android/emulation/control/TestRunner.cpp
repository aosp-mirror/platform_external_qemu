// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/TestRunner.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <string>
#include <unordered_map>

#define DEBUG 1

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(testrunner)) VERBOSE_ENABLE(testrunner); \
    VERBOSE_TIDFPRINT(testrunner, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

static bool initialized = false;
static std::unordered_map<std::string, test_function_t>
sTestFunctionMap;

void test_runner_init() {
    DPRINT("enter");
    initialized = true;
}

void test_runner_add_test(const char* id, test_function_t func) {
    DPRINT(" id=%s", id);
    if (!initialized) return;
    sTestFunctionMap[id] = func;
}

bool test_runner_run_test(const char* _id) {
    DPRINT(" id=%s", _id);
    if (!initialized) return false;
    std::string id(_id);
    if (sTestFunctionMap.find(id) != sTestFunctionMap.end()) {
        sTestFunctionMap[id]();
        return true;
    } else {
        return false;
    }
}
