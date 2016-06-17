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

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/control/test_runner.h"
#include "android/emulation/control/TestRunnerSelfTests.h"
#include "android/utils/debug.h"

#include <memory>
#include <string>
#include <unordered_map>

#include <string.h>

#define ERR(...) do { \
    derror(__VA_ARGS__); \
} while(0)

#define DEBUG 0

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(testrunner)) VERBOSE_ENABLE(testrunner); \
    VERBOSE_TID_FUNCTION_DPRINT(testrunner, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

using android::base::LazyInstance;

namespace android {
namespace emulation {

class TestRunnerState {
public:
    TestRunnerState() {
        DPRINT("ctor");
        addSelfTests();
    }

    std::string query() const {
        DPRINT("enter");
        std::string res;
        for (const auto& item : functions) {
            DPRINT("test:%s", item.first.c_str());
            res += "[";
            res += item.first;
            res += "]\n";
        }
        return res;
    }
    static TestRunnerState& get();

    std::unordered_map<std::string, android_test_function_t> functions;
private:
    void addSelfTests() {
        functions["basicMultiThread"] =
            android::emulation::selfTestRunBasicMultiThread;
    }
};

static LazyInstance<TestRunnerState> sTestRunnerState = LAZY_INSTANCE_INIT;

TestRunnerState& TestRunnerState::get() {
    return sTestRunnerState.get();
}

} // namespace emulation
} // namespace android

using android::emulation::TestRunnerState;

void android_test_runner_add_test(const char* id,
                                  android_test_function_t func) {
    DPRINT(" id=%s", id);
    TestRunnerState& state = TestRunnerState::get();
    DPRINT("got state");
    state.functions[id] = func;
    DPRINT("set the function");
}

bool android_test_runner_run_test(const char* _id) {
    TestRunnerState& state = TestRunnerState::get();

    DPRINT(" id=%s ntest=%d", _id, TestRunnerState::get().functions.size());

    std::string id(_id);
    if (state.functions.find(id) != state.functions.end()) {
        return state.functions[id]();
    } else {
        ERR("test [%s] not found\n", _id);
        return false;
    }
}

void android_test_runner_list_tests(char* buf, unsigned int* sz) {
    DPRINT("getting testrunner");
    TestRunnerState& state = TestRunnerState::get();
    DPRINT("begin query");
    std::string tests = state.query();

    unsigned int nbytes = tests.size() + 1;

    DPRINT("set bytes/buf");
    if (sz) *sz = nbytes;
    if (buf) strncpy(buf, tests.c_str(), nbytes);
    DPRINT("exit");
}

