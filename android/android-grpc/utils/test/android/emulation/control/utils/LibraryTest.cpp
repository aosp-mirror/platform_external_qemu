// Copyright (C) 2023 The Android Open Source Project
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
#include <gtest/gtest.h>
#include <thread>
#include "android/emulation/control/utils/Library.h"

// Global variable to keep track of destructor calls
int globalDestructorCalls = 0;

using android::emulation::control::Library;

// Custom object that increments the global variable when destructed
struct CustomObject {
    CustomObject() = default;
    ~CustomObject() { globalDestructorCalls++; }

    void nothing() { /* do nothing.. */
    }
};

TEST(LibraryTest, BorrowFromEmptyLibrary) {
    Library<int> myLibrary;
    auto obj = myLibrary.acquire();

    // We only have one use count, and that's us
    EXPECT_EQ(obj.use_count(), 1);
}

TEST(LibraryTest, ReturnObjectMultipleTimes) {
    Library<int> myLibrary;
    auto obj = myLibrary.acquire();

    obj.reset();
    EXPECT_EQ(obj.use_count(), 0);

    obj.reset();  // Repeated reset should not cause issues
    EXPECT_EQ(obj.use_count(), 0);
}

TEST(LibraryTest, CustomObjectDestruction) {
    Library<CustomObject> myLibrary;

    globalDestructorCalls = 0;
    // Borrow a CustomObject
    auto obj1 = myLibrary.acquire();

    // Ensure the globalDestructorCalls is not incremented yet
    EXPECT_EQ(globalDestructorCalls, 0);

    // Return the CustomObject by letting obj1 go out of scope
    // This should increment globalDestructorCalls
    obj1.reset();

    // Ensure the globalDestructorCalls is incremented after returning
    EXPECT_EQ(globalDestructorCalls, 1);
}

TEST(LibraryTest, ForeachInvocation) {
    Library<CustomObject> myLibrary;

    auto obj1 = myLibrary.acquire();
    {
        // We now have 2 borrowed objects..
        auto obj2 = myLibrary.acquire();

        // So we should iterate over two objects!
        int count = 0;
        myLibrary.forEach([&count](auto o) {
            o->nothing();
            count++;
        });

        EXPECT_EQ(count, 2);
    }

    // We returned an object, so iterating over them should result in only one
    // callback function being invoked.
    int count = 0;
    myLibrary.forEach([&count](auto o) {
        o->nothing();
        count++;
    });

    EXPECT_EQ(count, 1);
}

TEST(LibraryTest, ConcurrencyTest) {
    constexpr int numThreads = 4;
    constexpr int numIterationsPerThread = 1000;
    Library<int> myLibrary;

    std::vector<std::thread> threads;
    std::atomic<int> counter(0);

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < numIterationsPerThread; ++j) {
                auto obj = myLibrary.acquire();
                // Do some work with the borrowed object
                counter.fetch_add(1, std::memory_order_relaxed);

                // Check that the library is not empty.
                int count = 0;
                myLibrary.forEach([&count](auto o) { count++; });
                EXPECT_GE(count, 1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Ensure the counter matches the expected total number of borrows
    EXPECT_EQ(counter.load(std::memory_order_relaxed),
              numThreads * numIterationsPerThread);

    // Check that the library is empty.
    int count = 0;
    myLibrary.forEach([&count](auto o) { count++; });
    EXPECT_EQ(count, 0);
}

TEST(LibraryTest, WaitForEmptyLibraryTestTimesOut) {
    Library<int> myLibrary;
    auto item = myLibrary.acquire(5);
    EXPECT_FALSE(
            myLibrary.waitUntilLibraryIsClear(std::chrono::milliseconds(10)));
}

TEST(LibraryTest, WaitForEmptyLibraryTestWaitsUntilFinished) {
    using namespace std::chrono_literals;
    Library<int> myLibrary;
    bool started = false;
    std::mutex mtx;
    std::condition_variable cv;

    std::thread t([&]() {
        auto item = myLibrary.acquire();
        {
            std::lock_guard<std::mutex> lock(mtx);
            started = true;
        }
        cv.notify_one();
        std::this_thread::sleep_for(10ms);
    });

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return started; });
    }
    auto now = std::chrono::system_clock::now();
    EXPECT_TRUE(myLibrary.waitUntilLibraryIsClear(500ms));
    auto time_passed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - now);
    EXPECT_GE(time_passed.count(), (10ms).count());

    t.join();
}