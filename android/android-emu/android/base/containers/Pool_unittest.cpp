// Copyright (C) 2016 The Android Open Source Project
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
#include "android/base/containers/Pool.h"

#include <gtest/gtest.h>

#include <string>

namespace android {
namespace base {

// Tests basic function of pool:
// 1. Can allocate objects
// 2. Can acquire their pointer
// 3. Buffers preserved after deleting other objects
TEST(Pool, Basic) {
    Pool<char, 64> pool;

    const std::string testStr("abcdefg");

    size_t bytes = testStr.size() + 1;

    auto handle1 = pool.gen(bytes);
    auto handle2 = pool.gen(bytes);

    char* ptr1 = pool.acquire(handle1);
    char* ptr2 = pool.acquire(handle2);

    memset(ptr1, 0x0, bytes);
    memcpy(ptr1, testStr.data(), testStr.size());

    memset(ptr2, 0x0, bytes);
    memcpy(ptr2, testStr.data(), testStr.size());

    EXPECT_EQ(testStr, ptr1);
    EXPECT_EQ(testStr, ptr2);

    pool.del(handle1);

    ptr2 = pool.acquire(handle2);
    EXPECT_EQ(testStr, ptr2);

    pool.del(handle2);
}

// Test that nothing crashes if we allocate a
// bunch, and not explicitly delete everything.
// TODO: turn it into a leak test
TEST(Pool, EarlyDtor) {
    Pool<char, 64> pool;

    for (int i = 0; i < 1000; i++) {
        pool.gen(1 + i / 2);
    }
}

} // namespace base
} // namespace android

