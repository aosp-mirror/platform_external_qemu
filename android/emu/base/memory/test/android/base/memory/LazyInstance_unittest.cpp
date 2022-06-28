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

#include "android/base/memory/LazyInstance.h"

// #include "android/base/synchronization/Lock.h"
// #include "android/base/testing/TestThread.h"

#include <gtest/gtest.h>
#include <mutex>
#include <thread>

namespace android {
namespace base {

namespace {

class Foo {
public:
    Foo() : mValue(42) {}
    int get() const { return mValue; }
    void set(int value) { mValue = value; }
    ~Foo() { mValue = 13; }
private:
    int mValue;
};

class StaticCounter {
public:
    StaticCounter() {
        std::lock_guard lock(mLock);
        mCounter++;
    }

    int getValue() const {
        std::lock_guard lock(mLock);
        return mCounter;
    }

    static void reset() {
        std::lock_guard lock(mLock);
        mCounter = 0;
    }

private:
    static std::mutex mLock;
    static int mCounter;
};

// NOTE: This introduces a static C++ constructor for this object file,
//       but that's ok because a LazyInstance<Lock> should not be used to
//       test the behaviour of LazyInstance :-)
std::mutex StaticCounter::mLock;
int StaticCounter::mCounter = 0;

}  // namespace

TEST(LazyInstance, HasInstance) {
    LazyInstance<Foo> foo_instance = LAZY_INSTANCE_INIT;
    EXPECT_FALSE(foo_instance.hasInstance());
    EXPECT_FALSE(foo_instance.hasInstance());
    foo_instance.ptr();
    EXPECT_TRUE(foo_instance.hasInstance());
}

TEST(LazyInstance, Simple) {
    LazyInstance<Foo> foo_instance = LAZY_INSTANCE_INIT;
    Foo* foo1 = foo_instance.ptr();
    EXPECT_TRUE(foo1);
    EXPECT_EQ(42, foo_instance->get());
    foo1->set(10);
    EXPECT_EQ(10, foo_instance->get());
    EXPECT_EQ(foo1, foo_instance.ptr());
}

TEST(LazyInstance, Clear) {
    LazyInstance<Foo> foo_instance = LAZY_INSTANCE_INIT;
    EXPECT_EQ(42, foo_instance->get());
    foo_instance->set(500);
    foo_instance.clear();
    EXPECT_EQ(42, foo_instance->get());
}

// For the following test, launch 1000 threads that each try to get
// the instance pointer of a lazy instance. Then verify that they're all
// the same value.
//
// The lazy instance has a special constructor that will increment a
// global counter. This allows us to ensure that it is only called once.
//

namespace {

// The following is the shared structure between all threads.
struct MultiState {
    MultiState(LazyInstance<StaticCounter>* staticCounter) :
            mStaticCounter(staticCounter), mCount(0) {}

    enum {
        kMaxThreads = 1000,
    };

    std::mutex  mLock;
    LazyInstance<StaticCounter>* mStaticCounter;
    size_t mCount;
    void* mValues[kMaxThreads];
    std::thread* mThreads[kMaxThreads];
};

// The thread function for the test below.
static void* threadFunc(void* param) {
    MultiState* state = static_cast<MultiState*>(param);
    std::lock_guard lock(state->mLock);
    if (state->mCount < MultiState::kMaxThreads) {
        state->mValues[state->mCount++] = state->mStaticCounter->ptr();
    }
    return NULL;
}

}  // namespace

TEST(LazyInstance, MultipleThreads) {
    StaticCounter::reset();

    LazyInstance<StaticCounter> counter_instance = LAZY_INSTANCE_INIT;
    MultiState state(&counter_instance);
    const size_t kNumThreads = MultiState::kMaxThreads;

    // Create all threads.
    for (size_t n = 0; n < kNumThreads; ++n) {
        state.mThreads[n] = new std::thread([this, &state]{threadFunc(&state);});
    }

    // Wait for their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        state.mThreads[n]->join();
    }

    // Now check that the constructor was only called once.
    EXPECT_EQ(1, counter_instance->getValue());

    // Now compare all the store values, they should be the same.
    StaticCounter* expectedValue = counter_instance.ptr();
    for (size_t n = 0; n < kNumThreads; ++n) {
        EXPECT_EQ(expectedValue, state.mValues[n]) << "For thread " << n;
    }

    for (size_t n = 0; n < kNumThreads; ++n) {
        delete state.mThreads[n];
    }
}

}  // namespace base
}  // namespace android
