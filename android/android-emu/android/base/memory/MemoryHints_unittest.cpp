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

#include "android/base/memory/MemoryHints.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// A basic check that no crashes happen for either stack or heap
// memory, and the operation should succeed.
TEST(MemoryHints, Basic) {
    uint64_t small = 16;
    uint64_t large = 1024;

    char memory[small];
    EXPECT_TRUE(MemoryHints::set(memory, small, MemoryHints::Hint::DontNeed));
    EXPECT_TRUE(MemoryHints::set(memory, small, MemoryHints::Hint::Normal));
    EXPECT_TRUE(MemoryHints::set(memory, small, MemoryHints::Hint::Random));
    EXPECT_TRUE(MemoryHints::set(memory, small, MemoryHints::Hint::Sequential));

    char* heap = new char[large];
    EXPECT_TRUE(MemoryHints::set(heap, large, MemoryHints::Hint::DontNeed));
    EXPECT_TRUE(MemoryHints::set(heap, large, MemoryHints::Hint::Normal));
    EXPECT_TRUE(MemoryHints::set(heap, large, MemoryHints::Hint::Random));
    EXPECT_TRUE(MemoryHints::set(heap, large, MemoryHints::Hint::Sequential));
    delete [] heap;
}

// A basic check that invalid addresses should not result in a
// crash. All bets are off as to the return result, as some hint
// settings are not supported on some platforms and are no-ops.
TEST(MemoryHints, Negative) {
    MemoryHints::set(0, 4096, MemoryHints::Hint::DontNeed);
    MemoryHints::set(0, 4096, MemoryHints::Hint::Normal);
    MemoryHints::set(0, 4096, MemoryHints::Hint::Random);
    MemoryHints::set(0, 4096, MemoryHints::Hint::Sequential);
}

}  // namespace base
}  // namespace android

