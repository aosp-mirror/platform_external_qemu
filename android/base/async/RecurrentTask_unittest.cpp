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

#include "android/base/async/RecurrentTask.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"

#include <gtest/gtest.h>

using android::base::Looper;
using android::base::RecurrentTask;
using android::base::ThreadLooper;

class CountUpToN {
public:
    CountUpToN(Looper* looper, int maxCount)
        : mMaxCount(maxCount),
          mRecurrentTask(looper,
                         std::bind(&CountUpToN::countOne, this),
                         0),
          mPtrMayBeNull(new int) {}

    ~CountUpToN() {
        delete mPtrMayBeNull;
        mPtrMayBeNull = nullptr;
    }

    RecurrentTask& task() {
        return mRecurrentTask;
    }

    void start() { mRecurrentTask.start(); }

    bool countOne() {

        blowUpIfDeleted();

        if (mCurCount >= mMaxCount) {
            return false;
        }
        ++mCurCount;
        return true;
    }

    void blowUpIfDeleted() {
        // Make sure this has a side effect.
        *mPtrMayBeNull = sideEffectVar;
    }

    int curCount() const { return mCurCount; }

private:
    int mMaxCount;
    RecurrentTask mRecurrentTask;
    int mCurCount = 0;

    // Designed to blow up if |countOne| is called after we've been deleted.
    int* mPtrMayBeNull;
    volatile int sideEffectVar;
};

TEST(RecurrentTaskTest, zeroTimes) {
    Looper* looper = ThreadLooper::get();
    CountUpToN tasker(looper, 0);
    tasker.start();

    EXPECT_TRUE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
    looper->runWithTimeoutMs(500);
    EXPECT_FALSE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
}

TEST(RecurrentTaskTest, fiveTimes) {
    Looper* looper = ThreadLooper::get();
    CountUpToN tasker(looper, 5);
    tasker.start();

    EXPECT_TRUE(tasker.task().inFlight());
    EXPECT_EQ(0, tasker.curCount());
    looper->runWithTimeoutMs(500);
    EXPECT_FALSE(tasker.task().inFlight());
    EXPECT_EQ(5, tasker.curCount());
}

TEST(RecurrentTaskTest, deleteStartedObject) {
    Looper* looper = ThreadLooper::get();
    auto tasker = new CountUpToN(looper, 5);
    tasker->start();
    delete tasker;

    // If |RecurrentTask| does not gracefully handle being deleted while active,
    // this will blow up.
    looper->runWithTimeoutMs(500);
}
