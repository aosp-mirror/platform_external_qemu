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

#include "android/base/threads/LooperThread.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

using android::base::Looper;
using android::base::LooperThread;
using std::unique_ptr;

struct Result {
    bool mWasJoined = false;
    bool mWasDeleted = false;
    int mExitStatus = 0;
};

class TestLooperThread : public LooperThread<Result> {
public:
    // set mCheckTimeoutMs to 10 to speed up things.
    TestLooperThread(Looper* looper, Result* result)
        : LooperThread<Result>(looper, 10), mResult(result) {}

    virtual ~TestLooperThread() { mResult->mWasDeleted = true; }

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
    bool mShouldRun = false;
    Result* mResult = nullptr;
};

class LooperThreadTest : public testing::Test {
public:
    LooperThreadTest()
        : mLooper(android::base::ThreadLooper::get()),
          mTestLooperThread(new TestLooperThread(mLooper, &mResult)) {}

protected:
    Result mResult;
    Looper* mLooper;
    unique_ptr<TestLooperThread> mTestLooperThread;
};

TEST_F(LooperThreadTest, start) {
    mTestLooperThread->unblock();
    EXPECT_TRUE(mTestLooperThread->start());
    mLooper->runWithTimeoutMs(2000);

    EXPECT_FALSE(mTestLooperThread->inFlight());
    EXPECT_TRUE(mResult.mWasJoined);
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_FALSE(mResult.mWasDeleted);
}

TEST_F(LooperThreadTest, fireAndForget) {
    mTestLooperThread->unblock();
    EXPECT_TRUE(mTestLooperThread->fireAndForget(std::move(mTestLooperThread)));
    mLooper->runWithTimeoutMs(2000);

    // |mTestLooperThread| is invalid now.
    EXPECT_TRUE(mResult.mWasJoined);
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_TRUE(mResult.mWasDeleted);
}

TEST_F(LooperThreadTest, inFlight) {
    EXPECT_TRUE(mTestLooperThread->start());
    // The test will actually block here till timeout. Use a small value.
    mLooper->runWithTimeoutMs(200);
    EXPECT_FALSE(mResult.mWasJoined);
    EXPECT_TRUE(mTestLooperThread->inFlight());

    // This call races with the check in |mTestLooperThread|. This race is
    // OK though. Worst case, |mTestLooperThread::main| will loop one extra
    // time.
    mTestLooperThread->unblock();
    mLooper->runWithTimeoutMs(2000);
    EXPECT_FALSE(mTestLooperThread->inFlight());
    EXPECT_EQ(42, mResult.mExitStatus);
    EXPECT_TRUE(mResult.mWasJoined);
}

}  // namespace
