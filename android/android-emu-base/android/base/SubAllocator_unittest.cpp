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
#include "android/base/SubAllocator.h"

#include "android/base/ArraySize.h"
#include "android/base/FunctionView.h"

#include <gtest/gtest.h>

#include <random>
#include <string>

namespace android {
namespace base {

// Test: can allocate/free memory of various sizes,
// and if the allocation reasonably cannot be satisfied,
// the allocation fails.
TEST(SubAllocator, Basic) {
    const size_t pageSizesToTest[] = {
        1, 2, 4, 8, 16, 32, 64, 128,
        256, 512, 1024, 2048, 4096,
        8192, 16384, 32768, 65536,
    };

    const size_t sizesToTest[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 32, 33, 64, 65,
        127, 129, 1023, 1024,
        2047, 2048, 2049,
        4096, 16384, 65535
    };

    const size_t allocCounts[] = {
        1, 2, 3, 4,
    };

    const size_t bufferSize = 65536;

    std::vector<uint8_t> buffer(bufferSize);

    for (size_t pageSize : pageSizesToTest) {
        SubAllocator subAlloc(
            buffer.data(),
            (uint64_t)bufferSize,
            pageSize);

        for (size_t allocCount : allocCounts) {
            std::vector<void*> ptrs;

            for (size_t allocSize : sizesToTest) {

                size_t trySize = allocSize / allocCount;

                size_t atomSize =
                     pageSize *
                     ((trySize + pageSize - 1) / pageSize);

                size_t total = 0;

                for (size_t i = 0; i < allocCount; ++i) {

                    void* ptr = subAlloc.alloc(trySize);

                    if (ptr) {
                        total += atomSize;
                    } else {
                        EXPECT_TRUE(
                            (trySize == 0) ||
                            (total + atomSize > bufferSize)) <<
                        "pageSize " << pageSize <<
                        "allocCount " << allocCount <<
                        "trySize " << trySize <<
                        "try# " << i <<
                        "allocedSoFar " << total;
                    }

                    ptrs.push_back(ptr);
                }

                for (auto ptr : ptrs) {
                    subAlloc.free(ptr);
                }
            }
        }
    }
}

// Test: freeAll resets the state and allows more allocation
// without individual freeing.
TEST(SubAllocator, FreeAll) {
    const size_t pageSize = 64;
    const size_t bufferSize = 65536;

    std::vector<uint8_t> buffer(bufferSize);

    SubAllocator subAlloc(
        buffer.data(),
        (uint64_t)bufferSize,
        (uint64_t)pageSize);

    const size_t fillCount = bufferSize / pageSize;
    const size_t numTrials = 10;

    for (size_t i = 0; i < numTrials; ++i) {
        for (size_t j = 0; j < fillCount; ++j) {
            void* ptr = subAlloc.alloc(pageSize);
            EXPECT_NE(nullptr, ptr);
        }
        subAlloc.freeAll();
    }
}

// Test: Random testing
TEST(SubAllocator, Random) {
    // Given the buffer size, any combination of
    // page size, alloc size, and count should work
    // over a single run.
    const size_t pageSizesToTest[] = {
        1, 2, 4, 8, 16, 32, 64, 128,
        256, 512, 1024, 2048, 4096,
    };

    const size_t sizesToTest[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 32, 33, 64, 65,
        127, 129, 1023, 1024,
        2047, 2048, 2049,
        4096,
    };

    const size_t allocCounts[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    };

    const size_t numPageSizes = arraySize(pageSizesToTest);
    const size_t numSizes = arraySize(sizesToTest);
    const size_t numCounts = arraySize(allocCounts);

    const size_t bufferSize = 65536;
    std::vector<uint8_t> buffer(bufferSize);

    const size_t numTrials = 1000;

    std::default_random_engine generator;
    generator.seed(0);

    std::uniform_int_distribution<int> sizeDistribution(0, numSizes - 1);
    std::uniform_int_distribution<int> countDistribution(0, numCounts - 1);

    for (auto pageSize : pageSizesToTest) {
        SubAllocator subAlloc(
            buffer.data(),
            (uint64_t)bufferSize,
            pageSize);

        for (size_t i = 0; i < numTrials; ++i) {
            size_t count =
                allocCounts[countDistribution(generator)];

            std::vector<void*> ptrs;

            size_t total = 0;

            for (size_t j = 0; j < count; ++j) {
                size_t allocSize =
                    sizesToTest[sizeDistribution(generator)];
                void* ptr = subAlloc.alloc(allocSize);

                if (ptr) {
                    total += allocSize;
                }

                ptrs.push_back(ptr);
                EXPECT_NE(nullptr, ptr) <<
                    "pageSize " << pageSize <<
                    "runCount " << count <<
                    "alloc# " << j <<
                    "size " << allocSize <<
                    "total " << total;
            }

            for (auto ptr : ptrs) {
                subAlloc.free(ptr);
            }
        }
    }
}

} // namespace base
} // namespace android
