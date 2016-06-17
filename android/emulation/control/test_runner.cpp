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

#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/control/test_runner.h"
#include "android/emulation/control/TestRunnerSelfTests.h"
#include "android/utils/debug.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include <string.h>

#define ENABLE_INFO 0

#if ENABLE_INFO
#define INFO(fmt,...) do { \
    dprint("test_runner: " fmt, ##__VA_ARGS__); \
} while(0)
#else
#define INFO(...)
#endif

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
using android::base::MessageChannel;
using android::base::PathUtils;
using android::base::System;
using android::base::Thread;

static const char* const kTestRunnerResultsFileName =
    "emulator_test_runner_results.txt";

namespace android {
namespace emulation {

class TestRunnerThread : public Thread {
public:
    TestRunnerThread();
    virtual intptr_t main() override;
    void sendTest(const std::string& id);
private:
    // Holds the name of the test to run
    MessageChannel<std::string, 4U> mInput;

    // Notifies success/failure
    MessageChannel<bool, 4U> mOutput;
};

class TestRunnerState {
public:
    TestRunnerState() : mTestRunnerThread() {
        INFO("Initializing Android Console Test Runner");
        initTestResultsFile();
        addSelfTests();
        mTestRunnerThread.start();
    }

    std::string queryRegisteredTests() const {
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

    std::string queryTestResultLocation() const {
        return mTestResultsFileName;
    }

    bool runTest(const std::string& id) {
        if (functions.find(id) != functions.end()) {
            return functions[id]();
        } else {
            ERR("test [%s] not found\n", id.c_str());
            return false;
        }
    }

    void runTestAsync(const std::string& id) {
        mTestRunnerThread.sendTest(id);
    }

    void appendTestResult(std::string test_id, bool success) {
        std::string result = success ? "success" : "failure";
        INFO("Test result: %s: %s", test_id.c_str(), result.c_str());
        mTestResultsFileHandle.open(mTestResultsFileName, std::ios::app);
        mTestResultsFileHandle << test_id;
        mTestResultsFileHandle << ":";
        mTestResultsFileHandle << result;
        mTestResultsFileHandle << std::endl;
        mTestResultsFileHandle.close();
    }

    static TestRunnerState& get();

    std::unordered_map<std::string, android_test_function_t> functions;
private:
    void initTestResultsFile() {
        System* sys = System::get();
        std::string tempDir = sys->getTempDir();
        mTestResultsFileName =
            PathUtils::join(tempDir, kTestRunnerResultsFileName);

        INFO("Using test results file at %s", mTestResultsFileName.c_str());

        // Clear out the file
        mTestResultsFileHandle.open(mTestResultsFileName, std::ios::out);
        mTestResultsFileHandle.close();
    }


    void addSelfTests() {
        functions["basicMultiThread"] =
            android::emulation::selfTestRunBasicMultiThread;
    }

    TestRunnerThread mTestRunnerThread;
    std::string mTestResultsFileName;
    std::ofstream mTestResultsFileHandle;
};

static LazyInstance<TestRunnerState> sTestRunnerState = LAZY_INSTANCE_INIT;

TestRunnerState& TestRunnerState::get() {
    return sTestRunnerState.get();
}


TestRunnerThread::TestRunnerThread() {
    DPRINT("ctor");
}

intptr_t TestRunnerThread::main() {
    DPRINT("main");
    std::string testname;
    bool result;
    while (true) {
        mInput.receive(&testname);
        DPRINT("got a test:%s", testname.c_str());
        result = TestRunnerState::get().runTest(testname);
        TestRunnerState::get().appendTestResult(testname, result);
    }
    return 0;
}

void TestRunnerThread::sendTest(const std::string& id) {
    mInput.send(id);
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
        state.runTestAsync(id);
        return true;
    } else {
        ERR("test [%s] not found\n", _id);
        return false;
    }
}

void android_test_runner_list_tests(char* buf, unsigned int* sz) {
    DPRINT("getting testrunner");
    TestRunnerState& state = TestRunnerState::get();
    DPRINT("begin query of registered tests");
    std::string tests = state.queryRegisteredTests();

    unsigned int nbytes = tests.size() + 1;

    DPRINT("set bytes/buf");
    if (sz) *sz = nbytes;
    if (buf) strncpy(buf, tests.c_str(), nbytes);
    DPRINT("exit");
}

void android_test_runner_get_result_location(char* buf, unsigned int* sz) {
    DPRINT("getting testrunner");
    TestRunnerState& state = TestRunnerState::get();
    DPRINT("begin query of results location");
    std::string tests = state.queryTestResultLocation();

    unsigned int nbytes = tests.size() + 1;

    DPRINT("set bytes/buf");
    if (sz) *sz = nbytes;
    if (buf) strncpy(buf, tests.c_str(), nbytes);
    DPRINT("exit");
}

