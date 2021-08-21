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

class VirtioGpuTimelinesTest : public ::testing::Test {
   protected:
    std::unique_ptr<VirtioGpuTimelines> mVirtioGpuTimelines;
    void SetUp() override {
        mVirtioGpuTimelines = std::make_unique<VirtioGpuTimelines>();
    }
};

TEST_F(VirtioGpuTimelinesTest, Init) {}

TEST_F(VirtioGpuTimelinesTest, TasksShouldHaveDifferentIds) {
    auto taskId1 = mVirtioGpuTimelines->enqueueTask(0);
    auto taskId2 = mVirtioGpuTimelines->enqueueTask(0);
    ASSERT_NE(taskId1, taskId2);
}

TEST_F(VirtioGpuTimelinesTest, MultipleTasksAndFences) {
    using namespace testing;
    MockFunction<void(int)> check;
    MockFunction<void()> fence1Callback;
    MockFunction<void()> fence2Callback;
    MockFunction<void()> fence3Callback;
    VirtioGpuTimelines::CtxId ctxId = 0;
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

    auto task1Id = mVirtioGpuTimelines->enqueueTask(ctxId);
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fence1Callback.AsStdFunction());
    auto task2Id = mVirtioGpuTimelines->enqueueTask(ctxId);
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fence2Callback.AsStdFunction());
    check.Call(1);
    mVirtioGpuTimelines->notifyTaskCompletion(task1Id);
    check.Call(2);
    auto task3Id = mVirtioGpuTimelines->enqueueTask(ctxId);
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fence3Callback.AsStdFunction());
    check.Call(3);
    mVirtioGpuTimelines->notifyTaskCompletion(task2Id);
    check.Call(4);
    mVirtioGpuTimelines->notifyTaskCompletion(task3Id);
}

TEST_F(VirtioGpuTimelinesTest, FencesWithoutPendingTasks) {
    using namespace testing;
    MockFunction<void()> fenceCallback1;
    MockFunction<void()> fenceCallback2;
    VirtioGpuTimelines::CtxId ctxId = 0;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;
        EXPECT_CALL(fenceCallback1, Call());
        EXPECT_CALL(fenceCallback2, Call());
    }

    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fenceCallback1.AsStdFunction());
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fenceCallback2.AsStdFunction());
}

TEST_F(VirtioGpuTimelinesTest, FencesSharingSamePendingTasks) {
    using namespace testing;
    MockFunction<void()> fenceCallback1;
    MockFunction<void()> fenceCallback2;
    MockFunction<void(int)> check;
    VirtioGpuTimelines::CtxId ctxId = 0;
    VirtioGpuTimelines::FenceId fenceId = 0;
    {
        InSequence s;
        EXPECT_CALL(check, Call(1));
        EXPECT_CALL(fenceCallback1, Call());
        EXPECT_CALL(fenceCallback2, Call());
    }

    auto taskId = mVirtioGpuTimelines->enqueueTask(ctxId);
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fenceCallback1.AsStdFunction());
    mVirtioGpuTimelines->enqueueFence(ctxId, fenceId++,
                                      fenceCallback2.AsStdFunction());
    check.Call(1);
    mVirtioGpuTimelines->notifyTaskCompletion(taskId);
}

TEST_F(VirtioGpuTimelinesTest, TasksAndFencesOnMultipleContexts) {
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
    auto taskId2 = mVirtioGpuTimelines->enqueueTask(2);
    auto taskId3 = mVirtioGpuTimelines->enqueueTask(3);
    check.Call(1);
    mVirtioGpuTimelines->enqueueFence(1, 1, fence1Callback.AsStdFunction());
    check.Call(2);
    mVirtioGpuTimelines->enqueueFence(2, 2, fence2Callback.AsStdFunction());
    mVirtioGpuTimelines->enqueueFence(3, 3, fence3Callback.AsStdFunction());
    mVirtioGpuTimelines->notifyTaskCompletion(taskId2);
    check.Call(3);
    mVirtioGpuTimelines->notifyTaskCompletion(taskId3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}