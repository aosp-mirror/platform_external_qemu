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
// limitations under the License.// Testing the test runner

#include "android/emulation/control/TestRunnerSelfTests.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/testing/TestThread.h"
#include "android/emulation/control/TestRunner.h"

#include "android/utils/debug.h"

#include <vector>

#define DEBUG 1

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(testrunner)) VERBOSE_ENABLE(testrunner); \
    VERBOSE_TIDFPRINT(testrunner, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

using android::base::TestThread;
using android::base::Lock;
using android::base::AutoLock;

class SelfTestCountState {
public:
    SelfTestCountState() : mLock(), mCount(0) {}
    ~SelfTestCountState() {}

    void increment() {
        AutoLock lock(mLock);
        mCount++;
    }

    int count() const {
        AutoLock lock(mLock);
        return mCount;
    }
private:
    mutable Lock mLock;
    int mCount;
};

static void* selfTestWriteThreadFunction(void* param) {
    SelfTestCountState* p = static_cast<SelfTestCountState*>(param);
    p->increment();
    return NULL;
}

static const size_t kSelfTestNumThreads = 2000;

static void selfTestRunBasicMultiThread() {
    DPRINT("start");

    SelfTestCountState state;

    std::vector<TestThread> threads;
    threads.reserve(kSelfTestNumThreads);

    for (size_t i = 0; i < kSelfTestNumThreads; i++) {
        threads.emplace_back(selfTestWriteThreadFunction, &state);
    }

    for (size_t i = 0; i < kSelfTestNumThreads; i++) {
        threads[i].join();
    }

    DPRINT("counter = %d", state.count());
}

void addSelfTests() {
    DPRINT("enter");
    test_runner_add_test("basicMultiThread", selfTestRunBasicMultiThread);
}
