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

#pragma once

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/Thread.h"

#include <fstream>
#include <string>
#include <unordered_map>

#include <stdbool.h>

using android::base::Lock;
using android::base::MessageChannel;
using android::base::Thread;

// The Android Console Test Runner's purpose is to allow the user
// to easily run lightweight tests that are too small to justify putting
// them in apps or UI automation, but are too big for unit tests.
// For example, this would apply to any test that would require
// the presence of an active guest, such as virtual devices.
//
// The test runner operates with arbitrary function pointers that
// return true/false on success/failure.
// |android_test_function_t| is the function type
// that represents a Console Test Runner test:
typedef bool (*android_test_function_t)(void);
// These tests can be registered
// on emulator startup by associating them with a name and calling
// |android_test_runner_add_test|.
//
// Any such test can be registered from any source file that includes
// "android/emulation/control/test_runner.h". This is to make it easy
// to register tests from any source file in AndroidEmu and any file
// that can include that header, in general.
//
// Tests themselves are run in the emulator console, whose interface
// is described in more detail below in "Console interface".

namespace android {
namespace emulation {

// One does not simply run tests directly on the same thread as the
// console thread that has been interpreting commands.
// This is because there may be interference with VCPU threads;
// the console itself is on the main loop and any extra processing
// would drastically affect any test that is sensitive to timing.
// Therefore, we put the actual running of tests on a single, separate
// thread, the |TestRunnerThread|.
//
// The |TestRunnerThread| reads |TestRunInfo|s, which just collect
// the function itself along with the name of the test:
typedef std::pair<android_test_function_t, std::string>
        TestRunInfo;
// After the thread finishes executing a test, it then appends the output
// to a file in the user's temporary directory.

static const char* const kTestRunnerResultsFileName =
    "emulator_console_test_runner_results.txt";

class TestRunnerThread final : public Thread {
public:
    TestRunnerThread(const std::string& outFile =
                     kTestRunnerResultsFileName);
    virtual intptr_t main() override;
    void sendTest(const TestRunInfo& info);
    void initTestResultsFile(const std::string& fileName =
                             kTestRunnerResultsFileName);
    void appendTestResult(std::string test_id, bool success);

    // |queryTestResultLocation| is used by the console
    // to tell the user where test results are written.
    std::string queryTestResultLocation() const;

    // We call |cleanup| when the owner of this class
    // calls its destructor. For cleanup, it is necessary
    // to stop receiving any new requests for tests and
    // to wait for the thread itself to exit.
    void cleanup();
private:
    MessageChannel<TestRunInfo, 1024> mInput;
    std::string mTestResultsFileName;
    std::ofstream mTestResultsFileHandle;
};

// |TestRunner| implements the console interface. It:
//     - maintains the set of all registered tests so far,
//       as a map from test ID's to function pointers.
//     - allows concurrent registration of tests through
//       its |lock| member.
//     - has an instance of |TestRunnerThread|, that:
//         - receives incoming test-run requests
//         - is joined upon |TestRunner| destruction
class TestRunner {
public:
    TestRunner(const std::string& outFile =
               kTestRunnerResultsFileName);
    ~TestRunner();

    std::string queryRegisteredTests() const;
    std::string queryTestResultLocation() const;
    bool runTest(const std::string& id);
    void runTestAsync(const TestRunInfo& info);
    void initSelfTests();

    static TestRunner& get();

    Lock lock;
    std::unordered_map<std::string, android_test_function_t> functions;

private:
    TestRunnerThread mTestRunnerThread;
};

} // namespace emulation
} // namespace android
