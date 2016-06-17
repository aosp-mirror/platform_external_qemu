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

#include "android/emulation/goldfish_sync.h"
#include "android/emulation/goldfish_sync_tests.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/testing/TestThread.h"
#include "android/emulation/control/test_runner.h"

#include "android/utils/debug.h"

#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>

#define DEBUG 1

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(testrunner)) VERBOSE_ENABLE(testrunner); \
    VERBOSE_TID_FUNCTION_DPRINT(testrunner, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

using android::base::TestThread;
using android::base::Lock;
using android::base::AutoLock;

class GoldfishSyncState {
public:
    GoldfishSyncState() : mLock() {}
    ~GoldfishSyncState() {}

    void add_timeline(uint64_t timeline) {
        AutoLock lock(mLock);
        mTimelines.push_back(timeline);
    }

    void remove_timeline(uint64_t timeline) {
        AutoLock lock(mLock);
        mTimelines.erase(std::remove(mTimelines.begin(),
                                     mTimelines.end(),
                                     timeline),
                         mTimelines.end());
    }

    void add_fence_fd(int fd) {
        AutoLock lock(mLock);
        mFenceFds.push_back(fd);
    }

    void remove_fence_fd(int fd) {
        AutoLock lock(mLock);
        mFenceFds.erase(std::remove(mFenceFds.begin(),
                                    mFenceFds.end(),
                                    fd),
                        mFenceFds.end());
    }

    // Reads should be safe insofar as
    // they don't overflow the number of
    // items.
    size_t get_num_timelines() const {
        return mTimelines.size();
    }

    size_t get_num_fence_fds() const {
        return mFenceFds.size();
    }

    mutable Lock mLock;
    std::vector<uint64_t> mTimelines;
    std::vector<int> mFenceFds;

    // These are populated randomly.
    std::vector<uint32_t> random_cmd;
    std::vector<uint32_t> random_time;
};

static void* selfTestWriteThreadFunction(void* param) {
    // GoldfishSyncState* p = static_cast<GoldfishSyncState*>(param);

    DPRINT("create a timeline...");
    uint64_t my_timeline = goldfish_sync_create_timeline();
    DPRINT("timeline: 0x%llx", my_timeline);
    return NULL;
}

static const size_t kSyncDeviceTestNumThreads = 24;
static const size_t kSyncDeviceTestStackSize = 512 * 1024;

static bool goldfishSyncMultiThread() {
    DPRINT("start");

    GoldfishSyncState state;

    std::vector<TestThread> threads;
    threads.reserve(kSyncDeviceTestNumThreads);

    for (size_t i = 0; i < kSyncDeviceTestNumThreads; i++) {
        threads.emplace_back(selfTestWriteThreadFunction, &state,
                             kSyncDeviceTestStackSize);
    }

    return true;
}

void add_goldfish_sync_tests() {
    DPRINT("enter");
    android_test_runner_add_test
        ("goldfishsyncmulti", goldfishSyncMultiThread);
}
