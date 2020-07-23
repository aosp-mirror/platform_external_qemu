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
#include "android/emulation/hostdevices/HostAddressSpace.h"

#include "android/base/AlignedBuf.h"
#include "android/console.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_device.hpp"

#include <gtest/gtest.h>

namespace android {

class HostAddressSpaceTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        emulation::goldfish_address_space_set_vm_operations(getConsoleAgents()->vm);
    }

    static void TearDownTestCase() { }

    void SetUp() override {
        mDevice = HostAddressSpaceDevice::get();
    }

    void TearDown() override {
        if (mDevice) mDevice->clear();
        // mDevice is singleton, no need to tear down
    }
    HostAddressSpaceDevice* mDevice = nullptr;
};

// Tests the constructor.
TEST_F(HostAddressSpaceTest, Basic) { }

// Tests open/close.
TEST_F(HostAddressSpaceTest, OpenClose) {
    uint32_t handle = mDevice->open();
    EXPECT_NE(0, handle);
    mDevice->close(handle);
}

// Tests host interface existing
TEST_F(HostAddressSpaceTest, FunctionPointers) {
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs());
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->allocSharedHostRegion);
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->allocSharedHostRegionLocked);
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->freeSharedHostRegion);
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->freeSharedHostRegionLocked);
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->getPhysAddrStart);
    EXPECT_NE(nullptr,
        get_address_space_device_hw_funcs()->getPhysAddrStartLocked);
}

// Tests allocation and freeing of host shared regions.
TEST_F(HostAddressSpaceTest, HostSharedAllocs) {
    auto hwFuncs = get_address_space_device_hw_funcs();

    // Test that passing nullptr as the offset results in an error.
    int allocRes = hwFuncs->allocSharedHostRegion(4096, nullptr);

    EXPECT_EQ(-EINVAL, allocRes);

    uint64_t offset;
    allocRes = hwFuncs->allocSharedHostRegion(4096, &offset);

    EXPECT_EQ(0, allocRes);

    int freeRes = hwFuncs->freeSharedHostRegion(offset);

    EXPECT_EQ(0, freeRes);

    freeRes = hwFuncs->freeSharedHostRegion(offset);

    EXPECT_EQ(-EINVAL, freeRes);
}

// Tests the interface to get the starting phys addr.
TEST_F(HostAddressSpaceTest, HostGetStartingPhysAddr) {
    auto hwFuncs = get_address_space_device_hw_funcs();
    auto physStart = hwFuncs->getPhysAddrStart();
    EXPECT_NE(0, physStart);
}

// Tests host interface APIs Locked* versions.
TEST_F(HostAddressSpaceTest, HostInterfaceLocked) {
    auto hwFuncs = get_address_space_device_hw_funcs();

    // Test that passing nullptr as the offset results in an error.
    int allocRes = hwFuncs->allocSharedHostRegionLocked(4096, nullptr);

    EXPECT_EQ(-EINVAL, allocRes);

    uint64_t offset;
    allocRes = hwFuncs->allocSharedHostRegionLocked(4096, &offset);

    EXPECT_EQ(0, allocRes);

    int freeRes = hwFuncs->freeSharedHostRegionLocked(offset);

    EXPECT_EQ(0, freeRes);

    freeRes = hwFuncs->freeSharedHostRegionLocked(offset);

    EXPECT_EQ(-EINVAL, freeRes);

    auto physStart = hwFuncs->getPhysAddrStartLocked();
    EXPECT_NE(0, physStart);
}

// Tests claiming/unclaiming shared regions.
// Test that two different handles can share the same offset.
TEST_F(HostAddressSpaceTest, SharedRegionClaiming) {
    auto hwFuncs = get_address_space_device_hw_funcs();
    uint32_t handle = mDevice->open();
    uint32_t handle2 = mDevice->open();

    EXPECT_NE(0, handle);

    uint64_t offset;
    int allocRes = hwFuncs->allocSharedHostRegion(4096, &offset);
    EXPECT_EQ(0, allocRes);

    int claimRes = mDevice->claimShared(handle, offset, 4096);
    EXPECT_EQ(0, claimRes);
    claimRes = mDevice->claimShared(handle2, offset, 4096);
    EXPECT_EQ(0, claimRes);

    int unclaimRes = mDevice->unclaimShared(handle, offset);
    EXPECT_EQ(0, unclaimRes);
    unclaimRes = mDevice->unclaimShared(handle2, offset);
    EXPECT_EQ(0, unclaimRes);

    int freeRes = hwFuncs->freeSharedHostRegion(offset);
    EXPECT_EQ(0, freeRes);

    mDevice->close(handle);
    mDevice->close(handle2);
}

// Tests error cases when claiming/unclaiming shared regions;
// a region must have been created via allocSharedHostRegionLocked,
// and the same offset cannot be claimed or unclaimed twice with the same handle.
TEST_F(HostAddressSpaceTest, SharedRegionClaimingNegative) {
    auto hwFuncs = get_address_space_device_hw_funcs();
    uint32_t handle = mDevice->open();

    EXPECT_NE(0, handle);

    uint64_t offset;
    int allocRes = hwFuncs->allocSharedHostRegion(4096, &offset);
    EXPECT_EQ(0, allocRes);

    int claimRes = mDevice->claimShared(handle, offset - 1, 4096);
    EXPECT_EQ(-EINVAL, claimRes);
    claimRes = mDevice->claimShared(handle, offset, 4097);
    EXPECT_EQ(-EINVAL, claimRes);
    claimRes = mDevice->claimShared(handle, offset, 4096);
    EXPECT_EQ(0, claimRes);
    claimRes = mDevice->claimShared(handle, offset, 4096);
    EXPECT_EQ(-EINVAL, claimRes);

    int unclaimRes = mDevice->unclaimShared(handle, offset + 1);
    EXPECT_EQ(-EINVAL, unclaimRes);
    unclaimRes = mDevice->unclaimShared(handle, offset - 1);
    EXPECT_EQ(-EINVAL, unclaimRes);
    unclaimRes = mDevice->unclaimShared(handle, offset);
    EXPECT_EQ(0, unclaimRes);
    unclaimRes = mDevice->unclaimShared(handle, offset);
    EXPECT_EQ(-EINVAL, unclaimRes);

    int freeRes = hwFuncs->freeSharedHostRegion(offset);
    EXPECT_EQ(0, freeRes);

    mDevice->close(handle);
}

// Tests the HVA association API.
TEST_F(HostAddressSpaceTest, HostAddressMapping) {
    auto hwFuncs = get_address_space_device_hw_funcs();
    auto buf = aligned_buf_alloc(4096, 4096);
    uint32_t handle = mDevice->open();

    EXPECT_NE(0, handle);

    uint64_t offset;
    int allocRes = hwFuncs->allocSharedHostRegion(4096, &offset);
    EXPECT_EQ(0, allocRes);

    uint64_t physAddr = hwFuncs->getPhysAddrStart() + offset;
    mDevice->setHostAddrByPhysAddr(physAddr, buf);

    EXPECT_EQ(buf, mDevice->getHostAddr(physAddr));

    int claimRes = mDevice->claimShared(handle, offset, 4096);
    EXPECT_EQ(0, claimRes);

    int unclaimRes = mDevice->unclaimShared(handle, offset);
    EXPECT_EQ(0, unclaimRes);

    int freeRes = hwFuncs->freeSharedHostRegion(offset);
    EXPECT_EQ(0, freeRes);

    aligned_buf_free(buf);
    mDevice->close(handle);
}

} // namespace android
