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
using SmallEntityManager = EntityManager<16, 16, 32, int>;

using BasicComponentManager = ComponentManager<32, 16, 16, int>;
using BasicUnpackedComponentManager = UnpackedComponentManager<32, 16, 16, int>;

// Tests bitfield operations on handles.
TEST(EntityManager, BasicHandle) {
    const size_t index = 1;
    const size_t generation = 2;
    const size_t type = 7;

    BasicEntityManager::EntityHandle h =
        BasicEntityManager::makeHandle(
            index, generation, type);

    const size_t index2 = 8;
    const size_t generation2 = 9;
    const size_t type2 = 12;

    auto h2 = BasicEntityManager::withIndex(h, index2);
    auto h3 = BasicEntityManager::withGeneration(h, generation2);
    auto h4 = BasicEntityManager::withType(h, type2);

#define CHECK_HANDLE_VALUE(index, generation, type, h) \
    EXPECT_EQ((type << 48ULL) | (generation << 32ULL) | index, h)

    CHECK_HANDLE_VALUE(index, generation, type, h);
    CHECK_HANDLE_VALUE(index2, generation, type, h2);
    CHECK_HANDLE_VALUE(index, generation2, type, h3);
    CHECK_HANDLE_VALUE(index, generation, type2, h4);

    EXPECT_EQ(index, BasicEntityManager::getHandleIndex(h));
    EXPECT_EQ(generation, BasicEntityManager::getHandleGeneration(h));
    EXPECT_EQ(type, BasicEntityManager::getHandleType(h));
}

// Tests a few basic patterns of adding/removing handles.
TEST(EntityManager, AddRemove) {
    BasicEntityManager m;

    const int item = 1;
    const int type = 5;

    auto handle = m.add(item, type);

    int* itemPtr = m.get(handle);

    EXPECT_NE(nullptr, itemPtr);
    EXPECT_EQ(item, *itemPtr);

    m.remove(handle);
    itemPtr = m.get(handle);
    EXPECT_EQ(nullptr, itemPtr);

    // Tests that removing twice doesn't affect anything.
    m.remove(handle);
    itemPtr = m.get(handle);
    EXPECT_EQ(nullptr, itemPtr);

    const int item2 = 2;
    const int item3 = 3;
    const int item4 = 4;

    auto h1 = m.add(item2, type);
    auto h2 = m.add(item3, type);
    auto h3 = m.add(item4, type);

    int* itemPtr2 = m.get(h1);
    int* itemPtr3 = m.get(h2);
    int* itemPtr4 = m.get(h3);

    EXPECT_NE(nullptr, itemPtr2);
    EXPECT_NE(nullptr, itemPtr3);
    EXPECT_NE(nullptr, itemPtr4);

    EXPECT_EQ(item2, *itemPtr2);
    EXPECT_EQ(item3, *itemPtr3);
    EXPECT_EQ(item4, *itemPtr4);

    m.remove(h3);
    m.remove(h1);
    m.remove(h2);
}

// Tests that clear() removes all entries.
TEST(EntityManager, Clear) {
    BasicEntityManager m;
    auto h = m.add(1, 5);
    m.clear();
    EXPECT_EQ(nullptr, m.get(h));
}

// Tests that we can safely overflow the generation field.
TEST(EntityManager, GenerationOverflow) {
    size_t iterCount = 1 << 16;

    BasicEntityManager m;
    for (size_t i = 0; i < iterCount * 2; ++i) {
        auto h = m.add(1, 5);
        EXPECT_NE(nullptr, m.get(h));
        EXPECT_EQ(5, BasicEntityManager::getHandleType(h));
        EXPECT_NE(0, BasicEntityManager::getHandleGeneration(h));
        m.remove(h);
    }
}

// Tests that attempting to allocate more than 1 << indexBits elements fails.
TEST(EntityManager, ElementLimit) {
    size_t iterCount = 1 << 16;

    SmallEntityManager m;
    for (size_t i = 0; i < iterCount + 1; ++i) {
        auto h = m.add(1, 5);
        if (h == INVALID_ENTITY_HANDLE) {
            EXPECT_EQ(iterCount, i);
        }
    }
}

// Tests that we can loop over the entities.
TEST(EntityManager, IterateAll) {
    BasicEntityManager m;
    m.add(1, 5);
    m.add(2, 5);
    m.add(3, 5);

    int count = 0;
    m.forEachEntry([&count](bool live, BasicEntityManager::EntityHandle h, int& item) {
        if (live) ++count;
    });

    EXPECT_EQ(3, count);
}

// Tests that we can loop over just the live entities.
TEST(EntityManager, IterateLive) {
    BasicEntityManager m;
    m.add(1, 5);
    m.add(2, 5);
    m.add(3, 5);
    auto toRemove = m.add(3, 5);
    m.remove(toRemove);

    int count = 0;
    m.forEachLiveEntry([&count](bool live, BasicEntityManager::EntityHandle h, int& item) {
        EXPECT_TRUE(live);
        ++count;
    });

    EXPECT_EQ(3, count);
}

// Tests that component manager works in a basic way
// in isolation.
TEST(ComponentManager, Basic) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicComponentManager m;
    auto componentHandle1 = m.add((BasicComponentManager::EntityHandle)handle1, 3, 1);
    auto componentHandle2 = m.add((BasicComponentManager::EntityHandle)handle2, 4, 1);

    EXPECT_EQ(handle1, m.getEntityHandle(componentHandle1));
    EXPECT_EQ(handle2, m.getEntityHandle(componentHandle2));

    auto data1 = m.getByComponent(componentHandle1);
    auto data2 = m.getByComponent(componentHandle2);

    EXPECT_NE(nullptr, data1);
    EXPECT_NE(nullptr, data2);

    EXPECT_EQ(3, *data1);
    EXPECT_EQ(4, *data2);

    m.removeByComponent(componentHandle1);
    EXPECT_EQ(nullptr, m.getByComponent(componentHandle1));

    EXPECT_EQ(handle2, m.getEntityHandle(componentHandle2));
    EXPECT_NE(nullptr, m.getByComponent(componentHandle2));
    EXPECT_EQ(4, *(m.getByComponent(componentHandle2)));
}

// Tests component manager's iteration over all elements.
TEST(ComponentManager, IterateAll) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicComponentManager m;
    auto componentHandle1 = m.add((BasicComponentManager::EntityHandle)handle1, 3, 1);
    auto componentHandle2 = m.add((BasicComponentManager::EntityHandle)handle2, 4, 1);

    int count = 0;
    m.forEachComponent(
        [&count](bool live,
                 BasicComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        if (live) ++count;
    });

    EXPECT_EQ(2, count);
}

// Tests component manager's iteration over only live elements.
TEST(ComponentManager, IterateLive) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicComponentManager m;
    auto componentHandle1 = m.add((BasicComponentManager::EntityHandle)handle1, 3, 1);
    auto componentHandle2 = m.add((BasicComponentManager::EntityHandle)handle2, 4, 1);

    int count = 0;
    m.forEachLiveComponent(
        [&count](bool live,
                 BasicComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        ++count;
    });

    EXPECT_EQ(2, count);
}

// Tests that reverse mapping of entity handles back to components works.
TEST(ComponentManager, ReverseMapping) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);
    uint64_t handle3 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicComponentManager m;
    auto componentHandle1 =
        m.add((BasicComponentManager::EntityHandle)handle1,
              3, 1, true /* tracked */);
    auto componentHandle2 =
        m.add((BasicComponentManager::EntityHandle)handle2,
              4, 1, true /* tracked */);
    auto componentHandle3 =
        m.add((BasicComponentManager::EntityHandle)handle3,
              5, 1, false /* tracked */);

    EXPECT_EQ(handle1, m.getEntityHandle(componentHandle1));
    EXPECT_EQ(handle2, m.getEntityHandle(componentHandle2));

    EXPECT_EQ(componentHandle1, m.getComponentHandle(handle1));
    EXPECT_EQ(componentHandle2, m.getComponentHandle(handle2));
    EXPECT_NE(componentHandle3, m.getComponentHandle(handle3));

    EXPECT_NE(nullptr, m.getByEntity(handle1));
    EXPECT_EQ(3, *(m.getByEntity(handle1)));
    m.removeByEntity(handle1);
    EXPECT_EQ(nullptr, m.getByEntity(handle1));
    EXPECT_NE(nullptr, m.getByEntity(handle2));
    EXPECT_EQ(4, *(m.getByEntity(handle2)));
}

// Tests unpacked component manager's basic usage.
TEST(UnpackedComponentManager, Basic) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicUnpackedComponentManager m;

    auto handle1Copy = m.add((BasicUnpackedComponentManager::EntityHandle)handle1, 3);
    auto handle2Copy = m.add((BasicUnpackedComponentManager::EntityHandle)handle2, 4);

    EXPECT_EQ(handle1, handle1Copy);
    EXPECT_EQ(handle2, handle2Copy);

    auto data1 = m.get(handle1);
    auto data2 = m.get(handle2);

    EXPECT_NE(nullptr, data1);
    EXPECT_NE(nullptr, data2);

    EXPECT_EQ(3, *data1);
    EXPECT_EQ(4, *data2);

    m.remove(handle1);

    EXPECT_EQ(nullptr, m.get(handle1));

    EXPECT_NE(nullptr, m.get(handle2));
    EXPECT_EQ(4, *(m.get(handle2)));
}

// Tests component manager's iteration over all elements.
TEST(UnpackedComponentManager, IterateAll) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicUnpackedComponentManager m;
    m.add((BasicUnpackedComponentManager::EntityHandle)handle1, 3);
    m.add((BasicUnpackedComponentManager::EntityHandle)handle2, 4);

    int count = 0;
    m.forEachComponent(
        [&count](bool live,
                 BasicUnpackedComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        if (live) ++count;
    });

    EXPECT_EQ(2, count);
}

// Tests component manager's iteration over only live elements.
TEST(UnpackedComponentManager, IterateLive) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicUnpackedComponentManager m;

    m.add((BasicUnpackedComponentManager::EntityHandle)handle1, 3);
    m.add((BasicUnpackedComponentManager::EntityHandle)handle2, 4);

    int count = 0;
    m.forEachLiveComponent(
        [&count](bool live,
                 BasicComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        ++count;
    });

    EXPECT_EQ(2, count);
}

}  // namespace base
}  // namespace android
