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

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "android/base/system/System.h"

static std::unordered_map<void*, int> sInvokeCnt{};

using android::base::CallbackRegistry;
using android::base::System;
using std::chrono::system_clock;

void fake_cb(void* opaque) {
    sInvokeCnt[opaque]++;
}

class CallbackRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { sInvokeCnt.clear(); }
    CallbackRegistry mReg;
};

TEST_F(CallbackRegistryTest, add_increases_count) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    std::string foo = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt[&foo], 1);
}

TEST_F(CallbackRegistryTest, no_events_after_remove) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    std::string foo = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.unregisterCallback(&foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt[&foo], 0);
}

TEST_F(CallbackRegistryTest, no_increase_after_remove) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    std::string foo = "foo";
    mReg.registerCallback(fake_cb, &foo);
    mReg.invokeCallbacks();
    mReg.unregisterCallback(&foo);
    mReg.invokeCallbacks();
    EXPECT_EQ(sInvokeCnt[&foo], 1);
}

void called_by_foo(void* opaque) {
    std::string* incoming = reinterpret_cast<std::string*>(opaque);
    EXPECT_EQ(*incoming, "foo");
}

TEST_F(CallbackRegistryTest, no_calls_after_unregister) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    auto until = system_clock::now() + std::chrono::milliseconds(100);
    std::thread invokeCallbacks([=]() {
        while (system_clock::now() < until)
            mReg.invokeCallbacks();
    });
    std::thread reg_unreg([=]() {
        std::string foo = "foo";
        while (system_clock::now() < until) {
            foo[2] = 'o';
            mReg.registerCallback(called_by_foo, &foo);
            System::get()->yield();
            mReg.unregisterCallback(&foo);

            // Callback should never been called after unregister.
            // if it does, it might see fox instead of foo.
            foo[2] = 'x';
            EXPECT_EQ("fox", foo);
        }
    });

    invokeCallbacks.join();
    reg_unreg.join();
}

TEST_F(CallbackRegistryTest, no_calls_after_unregister_with_many_processors) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    auto until = system_clock::now() + std::chrono::milliseconds(100);
    std::vector<std::thread> processors;

    processors.emplace_back([=]() {
        while (system_clock::now() < until)
            mReg.invokeCallbacks();
    });

    std::thread reg_unreg([=]() {
        std::string foo = "foo";
        while (system_clock::now() < until) {
            foo[2] = 'o';
            mReg.registerCallback(called_by_foo, &foo);
            System::get()->yield();
            mReg.unregisterCallback(&foo);

            // Callback should never been called after unregister.
            // if it does, it might see fox instead of foo.
            foo[2] = 'x';
            EXPECT_EQ("fox", foo);
        }
    });

    reg_unreg.join();
    for (auto& t : processors) {
        t.join();
    }
}

TEST_F(CallbackRegistryTest, safe_exit_when_processing_stops) {
    EXPECT_EQ(sInvokeCnt.size(), 0);
    auto until = system_clock::now() + std::chrono::milliseconds(10);
    std::thread invokeCallbacks([=]() {
        while (system_clock::now() < until)
            mReg.invokeCallbacks();
        mReg.close();
    });

    auto reg_until = system_clock::now() + std::chrono::milliseconds(10);
    ASSERT_LT(until, reg_until);

    std::thread reg_unreg([=]() {
        std::string foo = "foo";
        while (system_clock::now() < reg_until) {
            foo[2] = 'o';
            mReg.registerCallback(called_by_foo, &foo);
            mReg.unregisterCallback(&foo);
        }
    });

    invokeCallbacks.join();
    reg_unreg.join();
}
