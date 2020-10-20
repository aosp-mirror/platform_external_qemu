// Copyright (C) 2018 The Android Open Source Project
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
#include "android/base/Pool.h"

#include "android/base/FunctionView.h"

#include <gtest/gtest.h>

#include <string>

namespace android {
namespace base {

// Tests basic function of pool:
// 1. Can allocate/free memory of various sizes
// 2. Can deal with allocations above the maxSize
//    and that overrun the internally set chunk counts.
TEST(Pool, Basic) {
    const size_t minSize = 8;
    const size_t maxSize = 4096;

    const size_t numTrials = 512;
    const size_t chunksPerSize = numTrials - 35;

    const size_t sizesToTest[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16,
        127, 129, 1023, 1024,
        2047, 2048, 2049,
        4096, 16384, 65535
    };

    Pool pool(minSize, maxSize, chunksPerSize);

    for (size_t allocSize : sizesToTest) {
        for (size_t i = 0; i < numTrials; i++) {
            uint8_t* ptr1 = (uint8_t*)pool.alloc(allocSize);
            uint8_t* ptr2 = (uint8_t*)pool.alloc(allocSize);
            uint8_t* ptr3 = (uint8_t*)pool.alloc(allocSize);

            *ptr1 = 1;
            *ptr2 = 2;
            *ptr3 = 3;

            EXPECT_EQ(1, *ptr1);
            EXPECT_EQ(2, *ptr2);
            EXPECT_EQ(3, *ptr3);

            pool.free(ptr1);

            EXPECT_EQ(2, *ptr2);
            EXPECT_EQ(3, *ptr3);

            pool.free(ptr3);

            EXPECT_EQ(2, *ptr2);

            pool.free(ptr2);

            uint8_t* ptr4 = (uint8_t*)pool.alloc(allocSize);

            *ptr4 = 4;

            EXPECT_EQ(4, *ptr4);

            // always leave a bit more allocated at the end of this loop, so
            // that we can test what happens when the internal blocks run out
            // of chunks.
        }
    }
}

TEST(Pool, Buffer) {
    const size_t minSize = 8;
    const size_t maxSize = 4096;

    const char* testBuffer = "Hello World!";

    const size_t bytes = strlen(testBuffer) + 1;

    Pool pool(minSize, maxSize);

    char* buf = pool.allocArray<char>(bytes);
    memset(buf, 0x0, bytes);
    memcpy(buf, testBuffer, bytes);

    EXPECT_STREQ(testBuffer, buf);
}

TEST(Pool, FreeAll) {
    const size_t minSize = 8;
    const size_t maxSize = 4096;

    Pool pool(minSize, maxSize);

    pool.alloc(8);
    pool.alloc(8);
    pool.alloc(8);

    pool.freeAll();
}

} // namespace base
} // namespace android

