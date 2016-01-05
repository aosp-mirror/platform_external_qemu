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
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <iostream>
#include <memory>

namespace android {
namespace base {

namespace {

using ::android::base::Thread;
using std::cin;
using std::cout;
using std::endl;

// A simple thread instance that does nothing at all and exits immediately.
class EmptyThread : public Thread {
public:
    intptr_t main() { return 42; }
};

// A thread that checks if onExit is called
class OnExitThread : public ::android::base::Thread {
public:
    static bool onExitCalled;
    static bool dtorCalled;

    OnExitThread() { onExitCalled = dtorCalled = false; }

    ~OnExitThread() { dtorCalled = true; }

    virtual intptr_t main() { return 42; }

    virtual void onExit() {
        onExitCalled = true;
        delete this;
    }
};

bool OnExitThread::onExitCalled = false;
bool OnExitThread::dtorCalled = false;

class CountingThread : public Thread {
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

    CountingThread(State* state) : Thread(), mState(state) {}

    intptr_t main() {
        cout << "OK, waiting by the mount of doom" << endl;
        ::android::base::System::get()->sleepMs(2000);
        cout << "About to access the taboo memory... falllllling off the ed" << endl;
        mState->increment();
        cout << "I'm still here ;)" << endl;
        return 0;
    }

private:
    State* mState;
};

// A thread that blocks till it's instructed to continue.
class BlockingThread : public Thread {
 public:
    BlockingThread() : Thread(), mLock(), mSharedContinueFlag(new bool) {
        *mSharedContinueFlag = false;
    }

    void unblock() {
        mLock.lock();
        *mSharedContinueFlag = true;
        mLock.unlock();
    }

    intptr_t main() {
        bool continueFlag;
        do {
            mLock.lock();
            continueFlag = *mSharedContinueFlag;
            mLock.unlock();

        } while (!continueFlag);
        return 42;
    }

 private:
    mutable Lock mLock;
    std::unique_ptr<bool> mSharedContinueFlag;
};

}  // namespace

TEST(ThreadTest, SimpleThread) {
    Thread* thread = new EmptyThread();
    intptr_t status;
    // Can't wait on threads before starting them.
    EXPECT_FALSE(thread->wait(&status));
    EXPECT_FALSE(thread->tryWait(&status));

    EXPECT_TRUE(thread);
    EXPECT_TRUE(thread->start());
    status = 0;
    EXPECT_TRUE(thread->wait(&status));
    EXPECT_EQ(42, status);

    // Thread objects are single use.
    EXPECT_FALSE(thread->start());
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

TEST(ThreadTest, OnExit) {
    OnExitThread* thread = new OnExitThread();
    EXPECT_FALSE(OnExitThread::onExitCalled);
    EXPECT_FALSE(OnExitThread::dtorCalled);
    EXPECT_TRUE(thread->start());
    EXPECT_TRUE(thread->wait(NULL));
    EXPECT_TRUE(OnExitThread::onExitCalled);
    EXPECT_TRUE(OnExitThread::dtorCalled);
}

TEST(ThreadTest, tryWait) {
    BlockingThread thread;
    intptr_t result = 0;
    EXPECT_FALSE(thread.tryWait(&result));
    EXPECT_TRUE(thread.start());
    EXPECT_FALSE(thread.tryWait(&result));
    thread.unblock();

    result = 0;
    EXPECT_TRUE(thread.wait(&result));
    EXPECT_EQ(42, result);
    result = 0;
    EXPECT_TRUE(thread.tryWait(&result));
    EXPECT_EQ(42, result);
}


void launcher() {
    int i;
    cout << "Enter 0 ";
    cin >> i;
    if (i != 0) {
        launcher();
        return;
    }

    CountingThread::State state;
    CountingThread thread(&state);
    thread.start();
    cout << "Started thread" << endl;
}

void stomper() {
    cout << "Stomp. Stomp. Stomp." << endl;
    int i;
    cout << "Enter 0 ";
    cin >> i;
    if (i != 0) {
        stomper();
        return;
    }

    intptr_t x[10] = {0,0,0,0,0,0,0,0,0,0};
    for (int j =0; j < 10; ++j) {
        cout << "Enter x[" << j << "] ";
        cin >> x[j];
    }
    for (int j = 0; j <10; j+=2) {
        cout << x[j] << endl;
    }
}

TEST(ThreadTest, onStack) {
    launcher();
    stomper();
    cout << "Sleeping for 5..." << endl;
    android::base::System::get()->sleepMs(5000);
    cout << "Done" << endl;
}

}  // namespace base
}  // namespace android
