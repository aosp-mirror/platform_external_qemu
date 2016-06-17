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

#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/control/test_runner.h"
#include "android/emulation/control/TestRunner.h"
#include "android/emulation/control/TestRunnerSelfTests.h"
#include "android/utils/debug.h"

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

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::System;

namespace android {
namespace emulation {

static LazyInstance<TestRunner> sTestRunner =
    LAZY_INSTANCE_INIT;

TestRunner& TestRunner::get() {
    return sTestRunner.get();
}

TestRunner::TestRunner(const std::string& outFile) :
    mTestRunnerThread(outFile) {
    mTestRunnerThread.start();
}

TestRunner::~TestRunner() {
    mTestRunnerThread.cleanup();
}

std::string TestRunner::queryRegisteredTests() const {
    DPRINT("enter");
    std::string res;
    for (const auto& item : functions) {
        DPRINT("test:%s", item.first.c_str());
        res += "["; res += item.first; res += "]\n";
    }
    return res;
}

std::string TestRunner::queryTestResultLocation() const {
    return mTestRunnerThread.queryTestResultLocation();
}

bool TestRunner::runTest(const std::string& id) {
    if (auto runFunc = android::base::find(functions, id)) {
        runTestAsync(std::make_pair(*runFunc, id));
        return true;
    } else {
        ERR("test [%s] not found\n", id.c_str());
        return false;
    }
}

void TestRunner::runTestAsync(const TestRunInfo& info) {
    mTestRunnerThread.sendTest(info);
}

void TestRunner::initSelfTests() {
    functions["basicMultiThread"] =
        android::emulation::selfTestRunBasicMultiThread;
}

// TestRunnerThread implementation//////////////////////////////////////////////
TestRunnerThread::TestRunnerThread(const std::string& outFile) {
    DPRINT("ctor");
    initTestResultsFile(outFile);
}

intptr_t TestRunnerThread::main() {
    DPRINT("main");
    TestRunInfo currentTest;
    android_test_function_t currentTestFunction;
    std::string currentTestName;
    bool result;
    while (mInput.receive(&currentTest)) {
        std::tie(currentTestFunction, currentTestName) = currentTest;
        DPRINT("got a test:%s", currentTestName.c_str());
        result = currentTestFunction();
        appendTestResult(currentTestName, result);
    }
    return 0;
}

void TestRunnerThread::sendTest(const TestRunInfo& info) {
    mInput.send(info);
}

void TestRunnerThread::initTestResultsFile(const std::string& fileName) {
    System* sys = System::get();
    std::string tempDir = sys->getTempDir();
    mTestResultsFileName =
        PathUtils::join(tempDir, fileName);

    // Clear out the file
    mTestResultsFileHandle.open(mTestResultsFileName, std::ios::out);
    mTestResultsFileHandle.close();
}

void TestRunnerThread::appendTestResult(std::string test_id, bool success) {
    std::string result = success ? "success" : "failure";
    mTestResultsFileHandle.open(mTestResultsFileName, std::ios::app);
    mTestResultsFileHandle << test_id;
    mTestResultsFileHandle << ":";
    mTestResultsFileHandle << result;
    mTestResultsFileHandle << std::endl;
    mTestResultsFileHandle.close();
}

std::string TestRunnerThread::queryTestResultLocation() const {
    return mTestResultsFileName;
}

void TestRunnerThread::cleanup() {
    // stop getting new tests
    mInput.stop();

    // wait for the thread to exit
    intptr_t exitStatus;
    wait(&exitStatus);
}

} // namespace emulation
} // namespace android

using android::emulation::TestRunner;

void android_test_runner_add_test(const char* id,
                                  android_test_function_t func) {
    TestRunner& testRunner = TestRunner::get();
    AutoLock lock(testRunner.lock);
    DPRINT(" id=%s", id);
    DPRINT("got testRunner");
    testRunner.functions[id] = func;
    DPRINT("set the function");
}

bool android_test_runner_run_test(const char* id) {
    TestRunner& testRunner = TestRunner::get();

    DPRINT(" id=%s ntest=%d", id, TestRunner::get().functions.size());

    return testRunner.runTest(id);
}

void android_test_runner_list_tests(char* buf, unsigned int* sz) {
    DPRINT("getting testrunner");
    TestRunner& testRunner = TestRunner::get();
    DPRINT("begin query of registered tests");
    std::string tests = testRunner.queryRegisteredTests();

    unsigned int nbytes = tests.size() + 1;

    DPRINT("set bytes/buf");
    if (sz) *sz = nbytes;
    if (buf) strncpy(buf, tests.c_str(), nbytes);
    DPRINT("exit");
}

void android_test_runner_get_result_location(char* buf, unsigned int* sz) {
    DPRINT("getting testrunner");
    TestRunner& testRunner = TestRunner::get();
    DPRINT("begin query of results location");
    std::string tests = testRunner.queryTestResultLocation();

    unsigned int nbytes = tests.size() + 1;

    DPRINT("set bytes/buf");
    if (sz) *sz = nbytes;
    if (buf) strncpy(buf, tests.c_str(), nbytes);
    DPRINT("exit");
}

