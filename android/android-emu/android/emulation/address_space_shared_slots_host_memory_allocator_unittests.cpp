// Copyright 2020 The Android Open Source Project
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

#include "android/emulation/address_space_shared_slots_host_memory_allocator.h"
#include <gtest/gtest.h>

namespace android {
namespace emulation {
namespace {
typedef AddressSpaceSharedSlotsHostMemoryAllocatorContext ASSSHMAC;
typedef ASSSHMAC::MemBlock MemBlock;
typedef MemBlock::FreeSubblocks_t FreeSubblocks_t;

int add_memory_mapping(uint64_t gpa, void *ptr, uint64_t size) {
    return 1;
}

int remove_memory_mapping(uint64_t gpa, void *ptr, uint64_t size) { return 1; }

struct address_space_device_control_ops create_address_space_device_control_ops() {
    struct address_space_device_control_ops ops = {};

    ops.add_memory_mapping = &add_memory_mapping;
    ops.remove_memory_mapping = &remove_memory_mapping;

    return ops;
}

uint64_t getPhysAddrStartLocked(void) {
    return 2020;
}

int allocSharedHostRegionLocked(uint64_t page_aligned_size, uint64_t* offset) {
    *offset = page_aligned_size * 10;
    return 0;
}

int freeSharedHostRegionLocked(uint64_t offset) {
    return 0;
}

AddressSpaceHwFuncs create_AddressSpaceHwFuncs() {
    AddressSpaceHwFuncs hw = {};

    hw.allocSharedHostRegionLocked = &allocSharedHostRegionLocked;
    hw.freeSharedHostRegionLocked = &freeSharedHostRegionLocked;
    hw.getPhysAddrStartLocked = &getPhysAddrStartLocked;

    return hw;
}
}

TEST(MemBlock_findFreeSubblock, Simple) {
    FreeSubblocks_t fsb;
    EXPECT_TRUE(MemBlock::findFreeSubblock(&fsb, 11) == fsb.end());

    fsb[100] = 10;
    EXPECT_TRUE(MemBlock::findFreeSubblock(&fsb, 11) == fsb.end());

    FreeSubblocks_t::const_iterator i;

    i = MemBlock::findFreeSubblock(&fsb, 7);
    ASSERT_TRUE(i != fsb.end());
    EXPECT_EQ(i->first, 100);
    EXPECT_EQ(i->second, 10);

    fsb[200] = 6;
    i = MemBlock::findFreeSubblock(&fsb, 7);
    ASSERT_TRUE(i != fsb.end());
    EXPECT_EQ(i->first, 100);
    EXPECT_EQ(i->second, 10);

    fsb[300] = 8;
    i = MemBlock::findFreeSubblock(&fsb, 7);
    ASSERT_TRUE(i != fsb.end());
    EXPECT_EQ(i->first, 300);
    EXPECT_EQ(i->second, 8);
}

TEST(MemBlock_tryMergeSubblocks, NoMerge) {
    FreeSubblocks_t fsb;

    auto i = fsb.insert({10, 5}).first;
    auto j = fsb.insert({20, 5}).first;

    auto r = MemBlock::tryMergeSubblocks(&fsb, i, i, j);

    EXPECT_EQ(fsb.size(), 2);
    EXPECT_EQ(fsb[10], 5);
    EXPECT_EQ(fsb[20], 5);
    EXPECT_TRUE(r == i);
}

TEST(MemBlock_tryMergeSubblocks, Merge) {
    FreeSubblocks_t fsb;

    auto i = fsb.insert({10, 10}).first;
    auto j = fsb.insert({20, 5}).first;

    auto r = MemBlock::tryMergeSubblocks(&fsb, i, i, j);

    EXPECT_EQ(fsb.size(), 1);
    EXPECT_EQ(fsb[10], 15);
    ASSERT_TRUE(r != fsb.end());
    EXPECT_EQ(r->first, 10);
    EXPECT_EQ(r->second, 15);
}

TEST(MemBlock, allocate) {
    const struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    const AddressSpaceHwFuncs hw = create_AddressSpaceHwFuncs();

    MemBlock block(&ops, &hw, 100);
    EXPECT_TRUE(block.isAllFree());
    EXPECT_EQ(block.physBase, 2020 + 100 * 10);

    EXPECT_EQ(block.allocate(110), 0);  // too large

    uint32_t off;

    off = block.allocate(50);
    EXPECT_GE(off, 2020 + 100 * 10);

    off = block.allocate(47);
    EXPECT_GE(off, 2020 + 100 * 10);

    off = block.allocate(2);
    EXPECT_GE(off, 2020 + 100 * 10);

    off = block.allocate(2);
    EXPECT_EQ(off, 0);

    off = block.allocate(1);
    EXPECT_GE(off, 2020 + 100 * 10);

    off = block.allocate(1);
    EXPECT_EQ(off, 0);
}

TEST(MemBlock, unallocate) {
    const struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    const AddressSpaceHwFuncs hw = create_AddressSpaceHwFuncs();

    MemBlock block(&ops, &hw, 100);
    EXPECT_TRUE(block.isAllFree());
    EXPECT_EQ(block.physBase, 2020 + 100 * 10);

    uint32_t off60 = block.allocate(60);
    EXPECT_GE(off60, 2020 + 100 * 10);

    uint32_t off20 = block.allocate(20);
    EXPECT_GE(off20, 2020 + 100 * 10);

    uint32_t off30 = block.allocate(30);
    EXPECT_EQ(off30, 0);

    block.unallocate(off20, 20);

    off30 = block.allocate(30);
    EXPECT_GE(off30, 2020 + 100 * 10);

    block.unallocate(off60, 60);
    block.unallocate(off30, 30);

    EXPECT_TRUE(block.isAllFree());
}

}  // namespace emulation
} // namespace android
