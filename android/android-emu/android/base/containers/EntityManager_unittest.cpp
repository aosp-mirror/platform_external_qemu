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

// Tests that clearing the component manager makes it forget about
// its current entries.
TEST(ComponentManager, Clear) {
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

    m.clear();

    data1 = m.getByComponent(componentHandle1);
    data2 = m.getByComponent(componentHandle2);

    EXPECT_EQ(nullptr, data1);
    EXPECT_EQ(nullptr, data2);

    int count = 0;
    m.forEachComponent(
        [&count](bool live,
                 BasicComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        if (live) ++count;
    });

    EXPECT_EQ(0, count);
}

// Tests that clearing the unpacked component manager makes it forget about
// its current entries.
TEST(UnpackedComponentManager, Clear) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(1, 1, 2);

    BasicUnpackedComponentManager m;

    m.add((BasicUnpackedComponentManager::EntityHandle)handle1, 3);
    m.add((BasicUnpackedComponentManager::EntityHandle)handle2, 4);

    auto data1 = m.get(handle1);
    auto data2 = m.get(handle2);

    EXPECT_NE(nullptr, data1);
    EXPECT_NE(nullptr, data2);

    EXPECT_EQ(3, *data1);
    EXPECT_EQ(4, *data2);

    m.clear();

    data1 = m.get(handle1);
    data2 = m.get(handle2);

    EXPECT_EQ(nullptr, data1);
    EXPECT_EQ(nullptr, data2);

    int count = 0;

    m.forEachComponent(
        [&count](bool live,
                 BasicComponentManager::ComponentHandle componentHandle,
                 BasicEntityManager::EntityHandle h, int& item) {
        if (live) ++count;
    });

    EXPECT_EQ(0, count);
}

// Tests adding entries to EntityManagers at fixed addresses.
TEST(EntityManager, AddFixed) {
    uint64_t handle1 =
        BasicEntityManager::makeHandle(0, 3, 2);

    BasicEntityManager m;

    auto handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(1, *(m.get(handle1Copy)));

    m.clear();

    handle1 =
        BasicEntityManager::makeHandle(1, 3, 2);

    handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(1, *(m.get(handle1Copy)));

    m.clear();

    handle1 =
        BasicEntityManager::makeHandle(10, 3, 2);

    handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(1, *(m.get(handle1Copy)));
}

// Test adding multiple fixed handle entries in order
// of how they are usually allocated
TEST(EntityManager, AddFixedMultipleForward) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle1Copy = m.addFixed(handle1, 1, 5);
    auto handle2Copy = m.addFixed(handle2, 2, 5);
    auto handle3Copy = m.addFixed(handle3, 3, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle2Copy, handle2);
    EXPECT_EQ(handle1Copy, handle1);
}

// Test adding multiple fixed handle entries in order
// of how they are usually allocated, starting with 0
// which is usually the first free handle
TEST(EntityManager, AddFixedMultipleForwardStartOnZero) {
    BasicEntityManager m;

    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);
    uint64_t handle0 =
        BasicEntityManager::makeHandle(0, 1, 2);

    auto handle0Copy = m.addFixed(handle0, 1, 5);
    auto handle1Copy = m.addFixed(handle1, 2, 5);
    auto handle2Copy = m.addFixed(handle2, 3, 5);

    EXPECT_EQ(handle0Copy, handle0);
    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(handle2Copy, handle2);
}

// Test adding multiple fixed handle entries in reverse order
// of how they are usually allocated
TEST(EntityManager, AddFixedMultipleReverse) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle3Copy = m.addFixed(handle3, 3, 5);
    auto handle2Copy = m.addFixed(handle2, 2, 5);
    auto handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle2Copy, handle2);
    EXPECT_EQ(handle1Copy, handle1);
}

// Test adding multiple fixed handle entries 
// with a turn
TEST(EntityManager, AddFixedMultipleTurn) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle3Copy = m.addFixed(handle3, 3, 5);
    auto handle1Copy = m.addFixed(handle1, 1, 5);
    auto handle2Copy = m.addFixed(handle2, 2, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(handle2Copy, handle2);
}

// Test adding multiple fixed handle entries 
// with a turn on 0 (which is usually the first free index)
TEST(EntityManager, AddFixedMultipleTurnOnZero) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle0 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle3Copy = m.addFixed(handle3, 3, 5);
    auto handle0Copy = m.addFixed(handle0, 2, 5);
    auto handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle0Copy, handle0);
    EXPECT_EQ(handle1Copy, handle1);
}

// Test adding multiple fixed handle entries 
// starting on 0 (which is usually the first free index)
TEST(EntityManager, AddFixedMultipleStartOnZero) {
    BasicEntityManager m;

    uint64_t handle0 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle0Copy = m.addFixed(handle0, 2, 5);
    auto handle3Copy = m.addFixed(handle3, 3, 5);
    auto handle1Copy = m.addFixed(handle1, 1, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle0Copy, handle0);
    EXPECT_EQ(handle1Copy, handle1);
}

// Test adding multiple fixed handle entries while some
// handles are allocated with the standard method.
TEST(EntityManager, AddFixedMixedNormal) {
    BasicEntityManager m;

    uint64_t handle0 =
        BasicEntityManager::makeHandle(0, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);

    auto handle1Copy = m.addFixed(handle1, 2, 5);
    auto handle0Copy = m.addFixed(handle0, 1, 5);
    auto handle2Copy = m.addFixed(handle2, 3, 5);
    auto standardHandle0 = m.add(10, 5);
    auto standardHandle1 = m.add(11, 5);

    EXPECT_EQ(handle0Copy, handle0);
    EXPECT_EQ(handle1Copy, handle1);
    EXPECT_EQ(handle2Copy, handle2);

    EXPECT_NE(nullptr, m.get(handle1Copy));
    EXPECT_NE(nullptr, m.get(handle0Copy));
    EXPECT_NE(nullptr, m.get(handle2Copy));
    EXPECT_NE(nullptr, m.get(standardHandle0));
    EXPECT_NE(nullptr, m.get(standardHandle1));

    EXPECT_EQ(2, *(m.get(handle1Copy)));
    EXPECT_EQ(1, *(m.get(handle0Copy)));
    EXPECT_EQ(3, *(m.get(handle2Copy)));
    EXPECT_EQ(10, *(m.get(standardHandle0)));
    EXPECT_EQ(11, *(m.get(standardHandle1)));
}

// Test adding multiple fixed handle entries in order
// of how they are usually allocated, and also remove
// a fixed handle, but also put it back at the end.
TEST(EntityManager, AddFixedMultipleForwardWithRemove) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle1Copy = m.addFixed(handle1, 1, 5);
    auto handle2Copy = m.addFixed(handle2, 2, 5);
    auto handle3Copy = m.addFixed(handle3, 3, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle2Copy, handle2);
    EXPECT_EQ(handle1Copy, handle1);

    m.remove(handle2Copy);

    EXPECT_EQ(nullptr, m.get(handle2Copy));
    EXPECT_EQ(1, *(m.get(handle1Copy)));
    EXPECT_EQ(3, *(m.get(handle3Copy)));

    handle2Copy = m.addFixed(handle2, 2, 5);
    EXPECT_EQ(handle2Copy, handle2);
    EXPECT_EQ(2, *(m.get(handle2Copy)));
}

// Test adding multiple fixed handle entries in order
// of how they are usually allocated, and also remove
// them in some other order, but then also put them back.
TEST(EntityManager, AddFixedMultipleForwardWithRemoveAll) {
    BasicEntityManager m;

    uint64_t handle3 =
        BasicEntityManager::makeHandle(3, 1, 2);
    uint64_t handle2 =
        BasicEntityManager::makeHandle(2, 1, 2);
    uint64_t handle1 =
        BasicEntityManager::makeHandle(1, 1, 2);

    auto handle1Copy = m.addFixed(handle1, 1, 5);
    auto handle2Copy = m.addFixed(handle2, 2, 5);
    auto handle3Copy = m.addFixed(handle3, 3, 5);

    EXPECT_EQ(handle3Copy, handle3);
    EXPECT_EQ(handle2Copy, handle2);
    EXPECT_EQ(handle1Copy, handle1);

    m.remove(handle2Copy);
    m.remove(handle1Copy);
    m.remove(handle3Copy);

    EXPECT_EQ(nullptr, m.get(handle2Copy));
    EXPECT_EQ(nullptr, m.get(handle1Copy));
    EXPECT_EQ(nullptr, m.get(handle3Copy));

    handle1Copy = m.addFixed(handle1, 1, 5);
    handle2Copy = m.addFixed(handle2, 2, 5);
    handle3Copy = m.addFixed(handle3, 3, 5);

    EXPECT_EQ(1, *(m.get(handle1Copy)));
    EXPECT_EQ(2, *(m.get(handle2Copy)));
    EXPECT_EQ(3, *(m.get(handle3Copy)));
}

}  // namespace base
}  // namespace android
