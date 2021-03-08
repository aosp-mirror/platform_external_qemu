// Copyright (C) 2020 The Android Open Source Project
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

#include "android/base/async/CallbackRegistry.h"

#include <gtest/gtest.h>  // for Message, TestPartResult, EXP...

#include <chrono>  // for operator<, operator+, system...
#include <string>  // for string
#include <thread>  // for thread
#include <vector>  // for vector

#include "android/base/system/System.h"  // for System

using android::base::AutoLock;
using android::base::CallbackRegistry;
using android::base::ConditionVariable;
using android::base::Lock;
using android::base::System;
using std::chrono::system_clock;
using std::chrono::time_point;

static std::atomic_int sInvokeCnt{0};

void fake_cb(void* opaque) {
    sInvokeCnt++;
}

void called_by_foo(void* opaque) {
    // Copy into a string. Otherwise if you introduce concurrency failures
    // You might see failures, even though the error claims that
    // the string are equivalent.
    std::string incoming = static_cast<char*>(opaque);
    EXPECT_EQ(incoming, "foo");
    sInvokeCnt++;
}

class CallbackRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        sInvokeCnt = 0;
        mDone = false;
    }

    void invokeCallback() {
        mReg.invokeCallbacks();
        mCallbackLock.lock();
        mCvCallback.signalAndUnlock(&mCallbackLock);
    }

    void waitUntilProcessed() {
        AutoLock lock(mCallbackLock);
        // We processed all outstanding requests if the
        // queue is empty and we are not pushing through events.
        mCvCallback.wait(&mCallbackLock, [=]() {
            return mReg.available() == mReg.max_slots() && !mReg.processing();
        });
    }

    void invokeCallback(system_clock::time_point until) {
        // Keep processing requests until the time is over and
        // the other thread has not yet finished.
        while (system_clock::now() < until || !mDone) {
            invokeCallback();
        }
    }

    void noCallAfterUnregister(system_clock::time_point until) {
        char foo[4] = "foo";
        int loop = 0;
        while (system_clock::now() < until) {
            loop++;
            foo[2] = 'o';
            waitUntilProcessed();
            EXPECT_EQ(mReg.available(), mReg.max_slots());
            mReg.registerCallback(called_by_foo, &foo);
            // Make sure we actually got registered.
            waitUntilProcessed();
            mReg.unregisterCallback(&foo);
            // Callback should never been called after unregister.
            // if it does, it might see fox instead of foo.
            foo[2] = 'x';
            EXPECT_STREQ("fox", foo);
        }
        mDone = true;
    }

    CallbackRegistry mReg;
    Lock mCallbackLock;
    ConditionVariable mCvCallback;
    bool mDone;
};

TEST_F(CallbackRegistryTest, add_increases_count) {
    char foo[4] = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt.load(), 1);
}

TEST_F(CallbackRegistryTest, no_events_after_remove) {
    char foo[4] = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.unregisterCallback(&foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt.load(), 0);
}

TEST_F(CallbackRegistryTest, no_increase_after_remove) {
    char foo[4] = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.invokeCallbacks();
    mReg.unregisterCallback(&foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt.load(), 1);
}

TEST_F(CallbackRegistryTest, no_calls_after_unregister) {
    auto until = system_clock::now() + std::chrono::milliseconds(100);
    std::thread invokeCallbacks([=]() { invokeCallback(until); });
    std::thread reg_unreg([=]() { noCallAfterUnregister(until); });
    invokeCallbacks.join();
    reg_unreg.join();
}

TEST_F(CallbackRegistryTest, no_calls_after_unregister_with_many_processors) {
    auto until = system_clock::now() + std::chrono::milliseconds(100);
    std::vector<std::thread> processors;

    for (int i = 0; i < 5; i++) {
        processors.emplace_back([=]() { invokeCallback(until); });
    }
    std::thread reg_unreg([=]() { noCallAfterUnregister(until); });

    reg_unreg.join();
    for (auto& t : processors) {
        t.join();
    }
}

TEST_F(CallbackRegistryTest, safe_exit_when_processing_stops) {
    auto until = system_clock::now() + std::chrono::milliseconds(10);
    std::thread invokeCallbacks([=]() {
        mDone = true;
        invokeCallback(until);
        mReg.close();
    });

    auto reg_until = system_clock::now() + std::chrono::milliseconds(20);
    ASSERT_LT(until, reg_until);

    std::thread reg_unreg([=]() {
        std::string unused = "unused";
        while (system_clock::now() < reg_until) {
            mReg.registerCallback(fake_cb, &unused);
            mReg.unregisterCallback(&unused);
        }
    });

    invokeCallbacks.join();
    reg_unreg.join();
}
