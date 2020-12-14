// Copyright 2018 The Android Open Source Project
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

#pragma once

#include "android/base/Compiler.h"

#include <condition_variable>
#include <chrono>
#include <mutex>

#include <gtest/gtest.h>

// Helper for multithreaded tests to wait for an event to occur before
// continuing test execution. Usage:
//
// TestEvent event;
// setCallback([&event]() {
//     event.signal();
// });
//
// asyncCallCallback();
// event.wait();
//
// By default, the timeout is 10 seconds but it can be changed by overriding
// the default parameter of wait().
//
// TestEvent is counted, so calling signal() more than once will result in
// multiple wait() events being triggered.  Call reset() to reset the current
// count.

class TestEvent {
    DISALLOW_COPY_AND_ASSIGN(TestEvent);
public:
    static constexpr int64_t kDefaultTimeoutMs = 10000;  // 10 seconds.

    TestEvent() = default;

    void signal() {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            ++mSignaledCount;
        }
        mCv.notify_one();
    }

    bool isSignaled() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSignaledCount > 0;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mMutex);
        mSignaledCount = 0;
    }

    void wait(int64_t timeoutMs = kDefaultTimeoutMs) {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mSignaledCount > 0 ||
            mCv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                         [this] { return mSignaledCount > 0; })) {
            ASSERT_GT(mSignaledCount, 0);
            --mSignaledCount;
        } else {
            FAIL() << "TestEvent::wait() timed out.";
        }
    }

private:
    std::condition_variable mCv;
    std::mutex mMutex;
    size_t mSignaledCount = 0;
};
