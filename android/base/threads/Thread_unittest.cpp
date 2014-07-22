// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/threads/Thread.h"

#include "android/base/synchronization/Lock.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

namespace {

// A simple thread instance that does nothing at all and exits immediately.
class EmptyThread : public ::android::base::Thread {
public:
    intptr_t main() { return 42; }
};

class CountingThread : public ::android::base::Thread {
public:
    class State {
    public:
        State() : mLock(), mCount(0) {}
        ~State() {}

        void increment() {
            mLock.lock();
            mCount++;
            mLock.unlock();
        }

        int count() const {
            int ret;
            mLock.lock();
            ret = mCount;
            mLock.unlock();
            return ret;
        }

    private:
        mutable Lock mLock;
        int mCount;
    };

    CountingThread(State* state) : mState(state) {}

    intptr_t main() {
        mState->increment();
        return 0;
    }

private:
    State* mState;
};

}  // namespace

TEST(ThreadTest, SimpleThread) {
    Thread* thread = new EmptyThread();
    EXPECT_TRUE(thread);
    EXPECT_TRUE(thread->start());
    intptr_t status;
    EXPECT_TRUE(thread->wait(&status));
    EXPECT_EQ(42, status);
}

TEST(ThreadTest, MultipleThreads) {
    CountingThread::State state;
    const size_t kMaxThreads = 100;
    Thread* threads[kMaxThreads];

    // Create all threads.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        threads[n] = new CountingThread(&state);
        EXPECT_TRUE(threads[n]) << "thread " << n;
    }

    // Start them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        EXPECT_TRUE(threads[n]->start()) << "thread " << n;
    }

    // Wait for them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        EXPECT_TRUE(threads[n]->wait(NULL)) << "thread " << n;
    }

    // Check state.
    EXPECT_EQ((int)kMaxThreads, state.count());

    // Delete them all.
    for (size_t n = 0; n < kMaxThreads; ++n) {
        delete threads[n];
    }
}

}  // namespace base
}  // namespace android
