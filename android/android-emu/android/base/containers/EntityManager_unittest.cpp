// Copyright (C) 2019 The Android Open Source Project
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
#include "android/base/containers/EntityManager.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

using BasicEntityManager = EntityManager<32, 16, 16, int>;

TEST(EntityManager, BasicHandle) {
    size_t index = 1;
    size_t generation = 2;
    size_t type = 7;

    BasicEntityManager::EntityHandle h =
        BasicEntityManager::makeHandle(
            index, generation, type);

#define CHECK_HANDLE_VALUE(index, generation, type, h) \
    EXPECT_EQ((type << 48ULL) | (generation << 32ULL) | index, h)

    CHECK_HANDLE_VALUE(index, generation, type, h);
}

TEST(EntityManager, AddRemove) {
    BasicEntityManager m;

    const int item = 1;

    auto handle = m.add(item, 5);

    int* itemPtr = m.get(handle);

    EXPECT_NE(nullptr, itemPtr);
    EXPECT_EQ(item, *itemPtr);

    m.remove(handle);

    itemPtr = m.get(handle);

    EXPECT_EQ(nullptr, itemPtr);

    auto h1 = m.add(1, 5);
    auto h2 = m.add(1, 5);
    auto h3 = m.add(1, 5);

    m.remove(h3);
    m.remove(h1);
    m.remove(h2);
}

}  // namespace base
}  // namespace android
