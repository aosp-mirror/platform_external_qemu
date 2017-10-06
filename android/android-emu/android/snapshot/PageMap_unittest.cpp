// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/PageMap.h"

#include <gtest/gtest.h>

#include <vector>

namespace android {
namespace snapshot {

static int testInt = 1;

TEST(PageMap, Basic) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb
    uint64_t pageSize = 4096;

    PageMap p(start, size, pageSize);
}

TEST(PageMap, LookupZeros) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb
    uint64_t pageSize = 4096;
    PageMap p(start, size, pageSize);

    uintptr_t curr = (uintptr_t)start;

    for (int i = 0; i < 1024; i++) {
        EXPECT_FALSE(p.lookup((void*)curr));
        curr += pageSize;
    }
}

TEST(PageMap, LookupOutside) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb
    uint64_t pageSize = 4096;
    PageMap p(start, size, pageSize);

    uintptr_t curr = (uintptr_t)start + size;

    for (int i = 0; i < 1024; i++) {
        EXPECT_FALSE(p.has((void*)curr));
        EXPECT_TRUE(p.lookup((void*)curr));
        curr += pageSize;
    }

    curr = (uintptr_t)start - size;

    for (int i = 0; i < 1024; i++) {
        EXPECT_FALSE(p.has((void*)curr));
        EXPECT_TRUE(p.lookup((void*)curr));
        curr += pageSize;
    }
}

TEST(PageMap, Set) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb
    uint64_t pageSize = 4096;

    PageMap p(start, size, pageSize);

    p.set(start, true);
    EXPECT_TRUE(p.lookup(start));

    p.set(start, false);
    EXPECT_FALSE(p.lookup(start));

    p.set((void*)((uintptr_t)start + (uintptr_t)(pageSize / 2)), true);
    EXPECT_TRUE(p.lookup(start));

    void* next = (void*)((uintptr_t)start + (uintptr_t)(pageSize * 2));
    EXPECT_FALSE(p.lookup(next));
    p.set(next, true);
    EXPECT_TRUE(p.lookup(next));
}

TEST(PageMap, SetPageSizes) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb

    std::vector<uint64_t> pageSizes = {
        4096, 1024, 256
    };

    for (const auto pageSize : pageSizes) {
        PageMap p(start, size, pageSize);

        p.set(start, true);
        EXPECT_TRUE(p.lookup(start));

        p.set(start, false);
        EXPECT_FALSE(p.lookup(start));

        p.set((void*)((uintptr_t)start + (uintptr_t)(pageSize / 2)), true);
        EXPECT_TRUE(p.lookup(start));

        void* next = (void*)((uintptr_t)start + (uintptr_t)(pageSize * 2));
        EXPECT_FALSE(p.lookup(next));
        p.set(next, true);
        EXPECT_TRUE(p.lookup(next));
    }
}

TEST(PageMap, SetPageSizesByIndex) {
    void* start = &testInt;
    uint64_t size = 0x80000000; // 2 gb

    std::vector<uint64_t> pageSizes = {
        4096, 1024, 256
    };

    for (const auto pageSize : pageSizes) {
        PageMap p(start, size, pageSize);

        p.set(start, true);
        EXPECT_TRUE(p.lookup(start));
        EXPECT_TRUE(p.lookupByIndex(0));

        p.set(start, false);
        EXPECT_FALSE(p.lookup(start));
        EXPECT_FALSE(p.lookupByIndex(0));

        p.set((void*)((uintptr_t)start + (uintptr_t)(pageSize / 2)), true);
        EXPECT_TRUE(p.lookup(start));
        EXPECT_TRUE(p.lookupByIndex(0));

        void* next = (void*)((uintptr_t)start + (uintptr_t)(pageSize * 2));
        EXPECT_FALSE(p.lookup(next));
        EXPECT_FALSE(p.lookupByIndex(2));
        p.set(next, true);
        EXPECT_TRUE(p.lookup(next));
        EXPECT_TRUE(p.lookupByIndex(2));
    }
}

}  // namespace base
}  // namespace android
