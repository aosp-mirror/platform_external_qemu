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

#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>

using android::base::TestThread;
using android::base::Lock;
using android::base::AutoLock;

class GoldfishSyncTestCommand {
public:
    GoldfishSyncTestCommand() {
        testCmd = -1;
        in_timeline_threadid = -1;
        in_fencefd_threadid = -1;
        in_timearg_immediate = -1;
        in_timearg_threadid = -1;
    }

    uint32_t testCmd;
    uint32_t in_timeline_threadid;
    uint32_t in_fencefd_threadid;
    uint32_t in_timearg_immediate;
    uint32_t in_timearg_threadid;
};

class GoldfishSyncTestProgram {
public:
    GoldfishSyncTestProgram() { }

    void createTimeline() {
        GoldfishSyncTestCommand res;
        res.testCmd = CMD_CREATE_SYNC_TIMELINE;
        statements.push_back(res);
    }

    void createFence(uint32_t threadid_of_timeline,
                     uint32_t time_arg) {
        GoldfishSyncTestCommand res;
        res.testCmd = CMD_CREATE_SYNC_FENCE;
        res.in_timeline_threadid = threadid_of_timeline;
        res.in_timearg_immediate = time_arg;
        statements.push_back(res);
    }

    void incTimeline(uint32_t threadid_of_timeline,
                     uint32_t time_arg) {
        GoldfishSyncTestCommand res;
        res.testCmd = CMD_SYNC_TIMELINE_INC;
        res.in_timeline_threadid = threadid_of_timeline;
        res.in_timearg_immediate = time_arg;
        statements.push_back(res);
    }

    void destroyTimeline(uint32_t threadid_of_timeline) {
        GoldfishSyncTestCommand res;
        res.testCmd = CMD_DESTROY_SYNC_TIMELINE;
        res.in_timeline_threadid = threadid_of_timeline;
        statements.push_back(res);
    }

    std::vector<GoldfishSyncTestCommand> statements;
};

class GoldfishSyncTestState {
public:
    GoldfishSyncTestState() {}
    GoldfishSyncTestState(uint32_t num_threads) {
        timelines.resize(num_threads, 0);
        fenceFds.resize(num_threads, -1);
        progs.resize(num_threads);
        timeargs.resize(num_threads, 0);
    }

    void runProg(uint32_t thread_id) {

        uint32_t time_arg;

        for (auto& stmt : progs[thread_id].statements) {
            time_arg = (stmt.in_timearg_immediate ==
                        0xFFFFFFFFULL) ?
                        stmt.in_timearg_threadid :
                        stmt.in_timearg_immediate;
            switch (stmt.testCmd) {
            case CMD_CREATE_SYNC_TIMELINE:
                timelines[thread_id] = goldfish_sync_create_timeline();
                break;
            case CMD_CREATE_SYNC_FENCE:
                fenceFds[thread_id] =
                    goldfish_sync_create_fence(timelines[stmt.in_timeline_threadid],
                                               time_arg);
                break;
            case CMD_SYNC_TIMELINE_INC:
                goldfish_sync_timeline_inc(timelines[stmt.in_timeline_threadid],
                                           time_arg);
                break;
            case CMD_DESTROY_SYNC_TIMELINE:
                goldfish_sync_destroy_timeline(timelines[stmt.in_timeline_threadid]);
                break;
            }
        }
    }

    ~GoldfishSyncTestState() {}

    std::vector<uint64_t> timelines;
    std::vector<int> fenceFds;
    std::vector<GoldfishSyncTestProgram> progs;
    std::vector<uint32_t> timeargs;
};

class IndexedState {
public:
    IndexedState(uint32_t _index, GoldfishSyncTestState* _state) :
        index(_index), state(_state) { }
    uint32_t index;
    GoldfishSyncTestState* state;
};


static void* goldfishSyncTestThreadFunction(void* param) {
    IndexedState* indexedState = static_cast<IndexedState*>(param);
    uint32_t thread_id = indexedState->index;
    GoldfishSyncTestState* test_state = indexedState->state;

    test_state->runProg(thread_id);

    return NULL;
}

static const size_t kSyncDeviceTestNumThreads = 4;
static const size_t kSyncDeviceTestStackSize = 512 * 1024;

static bool goldfishSyncCreateTimeline() {
    uint64_t timeline_handle = 0;
    timeline_handle = goldfish_sync_create_timeline();
    goldfish_sync_destroy_timeline(timeline_handle);
    return timeline_handle != 0;
}

static bool goldfishSyncCreateTimelineAndFence() {
    uint64_t timeline_handle = 0;
    int fence_fd = -1;
    timeline_handle = goldfish_sync_create_timeline();
    fence_fd = goldfish_sync_create_fence(timeline_handle, 5);
    goldfish_sync_destroy_timeline(timeline_handle);
    return timeline_handle != 0 && fence_fd >= 0;
}

static bool goldfishSyncCreateTimelineAndFenceWithInc() {
    uint64_t timeline_handle = 0;
    int fence_fd = -1;
    timeline_handle = goldfish_sync_create_timeline();
    fence_fd = goldfish_sync_create_fence(timeline_handle, 5);
    goldfish_sync_timeline_inc(timeline_handle, 1);
    goldfish_sync_timeline_inc(timeline_handle, 1);
    goldfish_sync_timeline_inc(timeline_handle, 1);
    goldfish_sync_timeline_inc(timeline_handle, 1);
    goldfish_sync_timeline_inc(timeline_handle, 1);
    goldfish_sync_destroy_timeline(timeline_handle);
    return timeline_handle != 0 && fence_fd >= 0;
}

static bool goldfishSyncCreateTimelineAndFenceWithIncRepeated() {
    uint64_t timeline_handle = 0;
    int fence_fd = -1;
    timeline_handle = goldfish_sync_create_timeline();
    for (size_t i = 0; i < 100; i++) {
        fence_fd = goldfish_sync_create_fence(timeline_handle, 5 + i * 5);
        goldfish_sync_timeline_inc(timeline_handle, 1);
        goldfish_sync_timeline_inc(timeline_handle, 1);
        goldfish_sync_timeline_inc(timeline_handle, 1);
        goldfish_sync_timeline_inc(timeline_handle, 1);
        goldfish_sync_timeline_inc(timeline_handle, 1);
    }
    goldfish_sync_destroy_timeline(timeline_handle);
    return timeline_handle != 0 && fence_fd >= 0;
}

bool check_timelines_notnull(const GoldfishSyncTestState& state) {
    bool res = true;
    for (auto item: state.timelines) {
        if (!item) res = false;
    }
    return res;
}

bool check_fences_notnull(const GoldfishSyncTestState& state) {
    bool res = true;
    for (auto item: state.fenceFds) {
        if (item < 0) res = false;
    }
    return res;
}

static void goldfishSyncRunMultiThreadTest(
        GoldfishSyncTestState& state) {
    size_t num_threads = state.progs.size();

    std::vector<IndexedState> indexed_states;
    for (size_t i = 0 ; i < num_threads; i++) {
        indexed_states.emplace_back(i, &state);
    }

    std::vector<TestThread> threads;
    threads.reserve(num_threads);

    for (size_t i = 0; i < num_threads; i++) {
        threads.emplace_back(goldfishSyncTestThreadFunction,
                             &indexed_states[i],
                             kSyncDeviceTestStackSize);
    }

    for (size_t i = 0; i < num_threads; i++) {
        threads[i].join();
    }
}

static bool goldfishSyncCreateTimelineMultiThread() {
    GoldfishSyncTestState state(kSyncDeviceTestNumThreads);

    for (size_t i = 0 ; i < kSyncDeviceTestNumThreads; i++) {
        state.progs[i].createTimeline();
        state.progs[i].destroyTimeline(i);
    }

    goldfishSyncRunMultiThreadTest(state);

    bool test_result =
        check_timelines_notnull(state);
    return test_result;
}

static bool goldfishSyncCreateTimelineAndFenceMultiThread() {
    GoldfishSyncTestState state(kSyncDeviceTestNumThreads);
    for (size_t i = 0 ; i < kSyncDeviceTestNumThreads; i++) {
        state.progs[i].createTimeline();
        state.progs[i].createFence(i, 5);
        state.progs[i].destroyTimeline(i);
    }

    goldfishSyncRunMultiThreadTest(state);

    bool test_result =
        check_timelines_notnull(state) &&
        check_fences_notnull(state);
    return test_result;
}

static bool goldfishSyncCreateTimelineAndFenceWithIncMultiThread() {
    GoldfishSyncTestState state(kSyncDeviceTestNumThreads);
    for (size_t i = 0 ; i < kSyncDeviceTestNumThreads; i++) {
        state.progs[i].createTimeline();
        state.progs[i].createFence(i, 5);
        state.progs[i].incTimeline(i, 1);
        state.progs[i].incTimeline(i, 1);
        state.progs[i].incTimeline(i, 1);
        state.progs[i].incTimeline(i, 1);
        state.progs[i].incTimeline(i, 1);
        state.progs[i].destroyTimeline(i);
    }

    goldfishSyncRunMultiThreadTest(state);

    bool test_result =
        check_timelines_notnull(state) &&
        check_fences_notnull(state);
    return test_result;
}

static bool goldfishSyncCreateTimelineAndFenceWithIncRepeatedMultiThread() {
    GoldfishSyncTestState state(kSyncDeviceTestNumThreads);
    for (size_t i = 0 ; i < kSyncDeviceTestNumThreads; i++) {
        state.progs[i].createTimeline();
        for (size_t k = 0; k < 100; k++) {
            state.progs[i].createFence(i, 5 + k * 5);
            state.progs[i].incTimeline(i, 1);
            state.progs[i].incTimeline(i, 1);
            state.progs[i].incTimeline(i, 1);
            state.progs[i].incTimeline(i, 1);
            state.progs[i].incTimeline(i, 1);
        }
        state.progs[i].destroyTimeline(i);
    }

    goldfishSyncRunMultiThreadTest(state);

    bool test_result =
        check_timelines_notnull(state) &&
        check_fences_notnull(state);
    return test_result;
}

#define ADD_TEST(name) \
    android_test_runner_add_test(#name, name)

void add_goldfish_sync_tests() {
    ADD_TEST(goldfishSyncCreateTimeline);
    ADD_TEST(goldfishSyncCreateTimelineAndFence);
    ADD_TEST(goldfishSyncCreateTimelineAndFenceWithInc);
    ADD_TEST(goldfishSyncCreateTimelineAndFenceWithIncRepeated);
    ADD_TEST(goldfishSyncCreateTimelineMultiThread);
    ADD_TEST(goldfishSyncCreateTimelineAndFenceMultiThread);
    ADD_TEST(goldfishSyncCreateTimelineAndFenceWithIncMultiThread);
    ADD_TEST(goldfishSyncCreateTimelineAndFenceWithIncRepeatedMultiThread);
}
