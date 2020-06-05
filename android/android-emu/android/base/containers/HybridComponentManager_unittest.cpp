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
#include "android/base/containers/HybridComponentManager.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

static constexpr uint32_t kTestMaxIndex = 256;
using TestHCM = HybridComponentManager<kTestMaxIndex, uint64_t, int>;

// Tests basic usage of the hybrid component manager.
// High indices are chosen to make sure that the fallback to map works.
TEST(HybridComponentManager, Basic) {
    TestHCM m;

    for (uint32_t i = 0; i < 2 * kTestMaxIndex; ++i) {
        m.add(i, i);

        EXPECT_NE(nullptr, m.get(i));
        EXPECT_NE(nullptr, m.get_const(i));

        EXPECT_EQ(i, *m.get_const(i));
        EXPECT_EQ(i, *m.get(i));

        m.remove(i);

        EXPECT_EQ(nullptr, m.get(i));
        EXPECT_EQ(nullptr, m.get_const(i));
    }

    for (uint32_t i = 0; i < 2 * kTestMaxIndex; ++i) {
        EXPECT_EQ(nullptr, m.get(i));
        EXPECT_EQ(nullptr, m.get_const(i));
    }

    for (uint32_t i = 0; i < 2 * kTestMaxIndex; ++i) {
        if (i % 2 == 0) {
            m.add(i, i);
        }
    }

    uint32_t count = 0;
    TestHCM::IterFunc func = [&count](
        bool, uint64_t, uint64_t, int&) {
        ++count;
    };

    m.forEachLive(func);
    EXPECT_EQ(kTestMaxIndex, count);
}

// Tests a super high index value to make sure it doesn't allocate too much.
TEST(HybridComponentManager, SuperHighIndex) {
    TestHCM m;

    constexpr uint64_t highIndex = ~0ULL;

    m.add(highIndex, 1);

    EXPECT_EQ(1, *m.get(highIndex));
    EXPECT_EQ(1, *m.get_const(highIndex));
}

// Tests getExceptZero.
TEST(HybridComponentManager, GetExceptZero) {
    TestHCM m;

    constexpr uint64_t highIndex = ~0ULL;

    m.add(0, 1);
    m.add(2, 0);
    m.add(highIndex, 1);
    m.add(highIndex - 1, 0);

    EXPECT_NE(nullptr, m.get(0));
    EXPECT_NE(nullptr, m.get_const(0));
    EXPECT_EQ(1, *m.get(0));
    EXPECT_EQ(1, *m.get_const(0));
    EXPECT_NE(nullptr, m.getExceptZero(0));
    EXPECT_EQ(1, *m.getExceptZero_const(0));

    EXPECT_NE(nullptr, m.get(2));
    EXPECT_NE(nullptr, m.get_const(2));
    EXPECT_EQ(0, *m.get(2));
    EXPECT_EQ(0, *m.get_const(2));
    EXPECT_EQ(nullptr, m.getExceptZero(2));
    EXPECT_EQ(nullptr, m.getExceptZero_const(2));

    EXPECT_EQ(1, *m.get(highIndex));
    EXPECT_EQ(0, *m.get_const(highIndex - 1));
    EXPECT_EQ(nullptr, m.getExceptZero_const(highIndex - 1));
    EXPECT_NE(nullptr, m.getExceptZero_const(highIndex));
    EXPECT_EQ(nullptr, m.getExceptZero_const(2));
}

} // namespace android
} // namespace base
