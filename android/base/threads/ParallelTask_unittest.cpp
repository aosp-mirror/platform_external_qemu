// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/threads/ParallelTask.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>

namespace {

using android::base::Looper;
using android::base::ParallelTask;
using std::unique_ptr;

struct Result {
    bool mWasJoined = false;
    bool mWasDeleted = false;
    int mExitStatus = 0;
};

class TestParallelTask : public ParallelTask<Result> {
public:
    // set mCheckTimeoutMs to 10 to speed up things.
    TestParallelTask(Looper* looper, Result* result)
        : ParallelTask<Result>(looper, 10), mResult(result) {}

    virtual ~TestParallelTask() { mResult->mWasDeleted = true; }

    void main(Result* outResult) override {
        while (!mShouldRun) {
            android::base::System::get()->sleepMs(10);
        }
        outResult->mExitStatus = 42;
    }

    void onJoined(const Result& result) override {
        ASSERT_NE(nullptr, mResult);
        mResult->mExitStatus = result.mExitStatus;
        mResult->mWasJoined = true;
    }

    void unblock() { mShouldRun = true; }

private:
    std::atomic_bool mShouldRun = {false};
    Result* mResult = nullptr;
};

class ParallelTaskTest : public testing::Test {
public:
    ParallelTaskTest()
        : mLooper(android::base::ThreadLooper::get()),
          mTestParallelTask(new TestParallelTask(mLooper, &mResult)) {}

protected:
    Result mResult;
    Looper* mLooper;
    unique_ptr<TestParallelTask> mTestParallelTask;
};

TEST_F(ParallelTaskTest, start) {
    mTestParallelTask->unblock();
    EXPECT_TRUE(mTestParallelTask->start());
    mLooper->runWithTimeoutMs(2000);

    EXPECT_FALSE(mTestParallelTask->inFlight());
    EXPECT_TRUE(mResult.mWasJoined);
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_FALSE(mResult.mWasDeleted);
}

TEST_F(ParallelTaskTest, fireAndForget) {
    mTestParallelTask->unblock();
    EXPECT_TRUE(mTestParallelTask->fireAndForget(std::move(mTestParallelTask)));
    mLooper->runWithTimeoutMs(2000);

    // |mTestParallelTask| is invalid now.
    EXPECT_TRUE(mResult.mWasJoined);
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_TRUE(mResult.mWasDeleted);
}

TEST_F(ParallelTaskTest, inFlight) {
    EXPECT_TRUE(mTestParallelTask->start());
    // The test will actually block here till timeout. Use a small value.
    mLooper->runWithTimeoutMs(200);
    EXPECT_FALSE(mResult.mWasJoined);
    EXPECT_TRUE(mTestParallelTask->inFlight());

    // This call races with the check in |mTestParallelTask|. This race is
    // OK though. Worst case, |mTestParallelTask::main| will loop one extra
    // time.
    mTestParallelTask->unblock();
    mLooper->runWithTimeoutMs(2000);
    EXPECT_FALSE(mTestParallelTask->inFlight());
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_TRUE(mResult.mWasJoined);
}

}  // namespace
