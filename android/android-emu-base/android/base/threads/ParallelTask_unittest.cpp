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

#include "android/base/testing/TestLooper.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <atomic>
#include <functional>
#include <memory>

namespace {

using android::base::TestLooper;
using android::base::ParallelTask;
using std::unique_ptr;

struct Result {
    bool mWasJoined = false;
    int mExitStatus = 0;
};

class ParallelTaskTest : public testing::Test {
public:
    // set mCheckTimeoutMs to 10 to speed things up.
    ParallelTaskTest()
        : mParallelTask(&mLooper,
                        std::bind(&ParallelTaskTest::taskFunction,
                                  this,
                                  std::placeholders::_1),
                        std::bind(&ParallelTaskTest::taskDoneFunction,
                                  this,
                                  std::placeholders::_1),
                        10) {}

    void taskFunction(Result* outResult) {
        fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
        while (!mShouldRun) {
            android::base::System::get()->sleepMs(10);
        }
        fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
        outResult->mExitStatus = 42;
    }

    void taskDoneFunction(const Result& result) {
        fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
        mResult = result;
        mResult.mWasJoined = true;
        fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    }

    void unblock() {
        fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
        mShouldRun = true;
    }

protected:
    TestLooper mLooper;
    ParallelTask<Result> mParallelTask;
    Result mResult;
    std::atomic_bool mShouldRun = {false};
};

TEST_F(ParallelTaskTest, start) {
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    unblock();
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(mParallelTask.start());
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    mLooper.runWithTimeoutMs(2000);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);

    EXPECT_FALSE(mParallelTask.inFlight());
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(mResult.mWasJoined);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_EQ(42, mResult.mExitStatus);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
}

TEST_F(ParallelTaskTest, inFlight) {
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(mParallelTask.start());
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    // The test will actually block here till timeout. Use a small value.
    mLooper.runWithTimeoutMs(100);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_FALSE(mResult.mWasJoined);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(mParallelTask.inFlight());
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    unblock();
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    mLooper.runWithTimeoutMs(2000);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_FALSE(mParallelTask.inFlight());
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_EQ(42, mResult.mExitStatus);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(mResult.mWasJoined);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
}

static bool parJoined = false;

static void setPar(int* outResult) {
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    *outResult = 42;
}

static void onJoined(const int& outResult) {
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_EQ(42, outResult);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    parJoined = true;
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
}

TEST(ParallelTaskFunctionTest, basic) {
    parJoined = false;

    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    TestLooper looper;
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(android::base::runParallelTask<int>(&looper, &setPar, &onJoined, 10));
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_FALSE(parJoined);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    looper.runWithTimeoutMs(2000);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
    EXPECT_TRUE(parJoined);
    fprintf(stderr, "%s:%s:%d\n", "ParallelTaskTest", __func__, __LINE__);
}

}  // namespace
