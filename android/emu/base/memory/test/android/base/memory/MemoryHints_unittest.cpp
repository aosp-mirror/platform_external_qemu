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

#include "aemu/base/memory/MemoryHints.h"

#include <gtest/gtest.h>

#include <vector>

namespace android {
namespace base {

#ifdef __APPLE__
#ifdef __arm64__
    static const uintptr_t kPageSize = 16384;
#else
    static const uintptr_t kPageSize = 4096;
#endif
#else
    static const uintptr_t kPageSize = 4096;
#endif

// A basic check that hints succeed for page-aligned memory,
// and that no crashes happen for other alignments.
TEST(MemoryHints, Basic) {
    const int numIter = 100;

    // Do many iterations with reallocation
    // to get around flaky lucky alignments and so forth.
    std::vector<char*> toDealloc;
    for (int i = 0; i < numIter; i++) {
        char* unalignedPtr = new char[kPageSize];

        // Don't delete those ptrs right away either, because
        // chances are they will just reuse the same addresses
        // due to compiler opt / OS.
        toDealloc.push_back(unalignedPtr);
        // Not necessarily page-aligned; don't expect success
        memoryHint(unalignedPtr, kPageSize, MemoryHint::DontNeed);
        memoryHint(unalignedPtr, kPageSize, MemoryHint::Normal);
        memoryHint(unalignedPtr, kPageSize, MemoryHint::Random);
        memoryHint(unalignedPtr, kPageSize, MemoryHint::Sequential);

        char* forAlignedPtr = new char[kPageSize * 2];
        toDealloc.push_back(forAlignedPtr);
        char* pagePtr =
            (char*)(kPageSize + (uintptr_t)forAlignedPtr & ~(kPageSize - 1));

        EXPECT_TRUE(memoryHint(pagePtr, kPageSize, MemoryHint::DontNeed));
        EXPECT_TRUE(memoryHint(pagePtr, kPageSize, MemoryHint::Normal));
        EXPECT_TRUE(memoryHint(pagePtr, kPageSize, MemoryHint::Random));
        EXPECT_TRUE(memoryHint(pagePtr, kPageSize, MemoryHint::Sequential));

        // Check that zeroOutMemory works.
        EXPECT_TRUE(zeroOutMemory(pagePtr, kPageSize));
        
    }

    for (auto ptr : toDealloc) {
        delete [] ptr;
    }
}

// A basic check that invalid addresses should not result in a
// crash. All bets are off as to the return result, as some hint
// settings are not supported on some platforms and are no-ops.
TEST(MemoryHints, Negative) {
    memoryHint(0, kPageSize, MemoryHint::DontNeed);
    memoryHint(0, kPageSize, MemoryHint::Normal);
    memoryHint(0, kPageSize, MemoryHint::Random);
    memoryHint(0, kPageSize, MemoryHint::Sequential);
}

}  // namespace base
}  // namespace android

