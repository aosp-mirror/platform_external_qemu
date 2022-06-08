// Copyright 2021 The Android Open Source Project
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "emulation/VirtioGpuTimelines.h"

#include <memory>

using RingGlobal = VirtioGpuRingGlobal;
using RingContextSpecific = VirtioGpuRingContextSpecific;

TEST(VirtioGpuTimelinesTest, Init) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    virtioGpuTimelines = VirtioGpuTimelines::create(false);
}

TEST(VirtioGpuTimelinesTest, TasksShouldHaveDifferentIds) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    auto taskId1 = virtioGpuTimelines->enqueueTask(RingGlobal{});
    auto taskId2 = virtioGpuTimelines->enqueueTask(RingGlobal{});
    ASSERT_NE(taskId1, taskId2);
}

TEST(VirtioGpuTimelinesTest, CantPollWithAsyncCallbackEnabled) {
    EXPECT_DEATH(
        {
            std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines =
                VirtioGpuTimelines::create(true);
            virtioGpuTimelines->poll();
        },
        ".*");
}

TEST(VirtioGpuTimelinesTest, MultipleTasksAndFencesWithSyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(false);
    using namespace testing;
    MockFunction<void()> check;
    MockFunction<void()> fence1Callback;
    MockFunction<void()> fence2Callback;
    MockFunction<void()> fence3Callback;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;

        EXPECT_CALL(check, Call());
        EXPECT_CALL(fence1Callback, Call());
        EXPECT_CALL(fence2Callback, Call());
        EXPECT_CALL(fence3Callback, Call());
    }

    auto task1Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence1Callback.AsStdFunction());
    auto task2Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence2Callback.AsStdFunction());
    virtioGpuTimelines->notifyTaskCompletion(task1Id);
    auto task3Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence3Callback.AsStdFunction());
    virtioGpuTimelines->notifyTaskCompletion(task2Id);
    virtioGpuTimelines->notifyTaskCompletion(task3Id);
    check.Call();
    virtioGpuTimelines->poll();
}

TEST(VirtioGpuTimelinesTest, MultipleTasksAndFencesWithAsyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    using namespace testing;
    MockFunction<void(int)> check;
    MockFunction<void()> fence1Callback;
    MockFunction<void()> fence2Callback;
    MockFunction<void()> fence3Callback;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;

        EXPECT_CALL(check, Call(1));
        EXPECT_CALL(fence1Callback, Call());
        EXPECT_CALL(check, Call(2));
        EXPECT_CALL(check, Call(3));
        EXPECT_CALL(fence2Callback, Call());
        EXPECT_CALL(check, Call(4));
        EXPECT_CALL(fence3Callback, Call());
    }

    auto task1Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence1Callback.AsStdFunction());
    auto task2Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence2Callback.AsStdFunction());
    check.Call(1);
    virtioGpuTimelines->notifyTaskCompletion(task1Id);
    check.Call(2);
    auto task3Id = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fence3Callback.AsStdFunction());
    check.Call(3);
    virtioGpuTimelines->notifyTaskCompletion(task2Id);
    check.Call(4);
    virtioGpuTimelines->notifyTaskCompletion(task3Id);
}

TEST(VirtioGpuTimelinesTest, FencesWithoutPendingTasksWithAsyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    using namespace testing;
    MockFunction<void()> fenceCallback1;
    MockFunction<void()> fenceCallback2;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;
        EXPECT_CALL(fenceCallback1, Call());
        EXPECT_CALL(fenceCallback2, Call());
    }

    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fenceCallback1.AsStdFunction());
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fenceCallback2.AsStdFunction());
}

TEST(VirtioGpuTimelinesTest, FencesSharingSamePendingTasksWithAsyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    using namespace testing;
    MockFunction<void()> fenceCallback1;
    MockFunction<void()> fenceCallback2;
    MockFunction<void(int)> check;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;
        EXPECT_CALL(check, Call(1));
        EXPECT_CALL(fenceCallback1, Call());
        EXPECT_CALL(fenceCallback2, Call());
    }

    auto taskId = virtioGpuTimelines->enqueueTask(RingGlobal{});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fenceCallback1.AsStdFunction());
    virtioGpuTimelines->enqueueFence(RingGlobal{}, fenceId++, fenceCallback2.AsStdFunction());
    check.Call(1);
    virtioGpuTimelines->notifyTaskCompletion(taskId);
}

TEST(VirtioGpuTimelinesTest, TasksAndFencesOnMultipleContextsWithAsyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    using namespace testing;
    MockFunction<void()> fence1Callback;
    MockFunction<void()> fence2Callback;
    MockFunction<void()> fence3Callback;
    MockFunction<void(int)> check;
    {
        InSequence s;

        EXPECT_CALL(check, Call(1));
        EXPECT_CALL(fence1Callback, Call());
        EXPECT_CALL(check, Call(2));
        EXPECT_CALL(fence2Callback, Call());
        EXPECT_CALL(check, Call(3));
        EXPECT_CALL(fence3Callback, Call());
    }
    auto taskId2 = virtioGpuTimelines->enqueueTask(RingContextSpecific{
        .mCtxId = 2,
        .mRingIdx = 0,
    });
    auto taskId3 = virtioGpuTimelines->enqueueTask(RingContextSpecific{
        .mCtxId = 3,
        .mRingIdx = 0,
    });
    check.Call(1);
    virtioGpuTimelines->enqueueFence(RingGlobal{}, 1, fence1Callback.AsStdFunction());
    check.Call(2);
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 2,
            .mRingIdx = 0,
        },
        2, fence2Callback.AsStdFunction());
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 3,
            .mRingIdx = 0,
        },
        3, fence3Callback.AsStdFunction());
    virtioGpuTimelines->notifyTaskCompletion(taskId2);
    check.Call(3);
    virtioGpuTimelines->notifyTaskCompletion(taskId3);
}

TEST(VirtioGpuTimelinesTest, TasksAndFencesOnMultipleRingsWithAsyncCallback) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(true);
    using namespace testing;
    MockFunction<void()> fence1Callback;
    MockFunction<void()> fence2Callback;
    MockFunction<void()> fence3Callback;
    MockFunction<void(int)> check;
    {
        InSequence s;

        EXPECT_CALL(check, Call(1));
        EXPECT_CALL(fence1Callback, Call());
        EXPECT_CALL(check, Call(2));
        EXPECT_CALL(fence2Callback, Call());
        EXPECT_CALL(check, Call(3));
        EXPECT_CALL(fence3Callback, Call());
    }
    auto taskId2 = virtioGpuTimelines->enqueueTask(RingContextSpecific{
        .mCtxId = 1,
        .mRingIdx = 2,
    });
    auto taskId3 = virtioGpuTimelines->enqueueTask(RingContextSpecific{
        .mCtxId = 1,
        .mRingIdx = 3,
    });
    check.Call(1);
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 1,
            .mRingIdx = 1,
        },
        1, fence1Callback.AsStdFunction());
    check.Call(2);
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 1,
            .mRingIdx = 2,
        },
        2, fence2Callback.AsStdFunction());
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 1,
            .mRingIdx = 3,
        },
        3, fence3Callback.AsStdFunction());
    virtioGpuTimelines->notifyTaskCompletion(taskId2);
    check.Call(3);
    virtioGpuTimelines->notifyTaskCompletion(taskId3);
}

TEST(VirtioGpuTimelinesTest, RecoverFromSnapshot) {
    std::unique_ptr<VirtioGpuTimelines> virtioGpuTimelines = VirtioGpuTimelines::create(false);
    using namespace testing;
    MockFunction<void(VirtioGpuRing, VirtioGpuTimelines::FenceId)> fenceCallback;
    MockFunction<void()> check;
    {
        InSequence s;

        EXPECT_CALL(check, Call());
        EXPECT_CALL(fenceCallback, Call(VirtioGpuRing{RingGlobal{}}, 1));
        EXPECT_CALL(fenceCallback, Call(VirtioGpuRing{RingGlobal{}}, 2));
        EXPECT_CALL(fenceCallback, Call(VirtioGpuRing{RingContextSpecific{
                                            .mCtxId = 1,
                                            .mRingIdx = 3,
                                        }},
                                        3));
    }
    virtioGpuTimelines->enqueueFence(RingGlobal{}, 1, [] {});
    virtioGpuTimelines->enqueueFence(RingGlobal{}, 2, [] {});
    virtioGpuTimelines->enqueueFence(
        RingContextSpecific{
            .mCtxId = 1,
            .mRingIdx = 3,
        },
        3, [] {});
    std::vector<uint8_t> snapshot = virtioGpuTimelines->saveSnapshot();
    virtioGpuTimelines = VirtioGpuTimelines::recreateFromSnapshot(
        std::move(virtioGpuTimelines), snapshot.data(),
        [fenceCallback = fenceCallback.AsStdFunction()](const VirtioGpuRing& ring,
                                                        VirtioGpuTimelines::FenceId fenceId) {
            return [fenceCallback, ring, fenceId] { fenceCallback(ring, fenceId); };
        });
    check.Call();
    virtioGpuTimelines->poll();
}
