// Copyright 2019 The Android Open Source Project
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

#include "android/emulation/address_space_graphics.h"

#include "android/emulation/hostdevices/HostAddressSpace.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/address_space_device.hpp"

#include <gtest/gtest.h>

namespace android {
namespace emulation {

namespace {

constexpr uint64_t BAD_GPA = 0x1234000;
constexpr uint64_t GOOD_GPA_1 = 0x10001000;
constexpr uint64_t GOOD_GPA_2 = 0x20002000;

int empty_add_memory_mapping(uint64_t gpa, void *ptr, uint64_t size) {
    return (gpa == BAD_GPA) ? 0 : 1;
}

int empty_remove_memory_mapping(uint64_t gpa, void *ptr, uint64_t size) { return 1; }

struct address_space_device_control_ops create_address_space_device_control_ops() {
    struct address_space_device_control_ops ops = {};

    ops.add_memory_mapping = &empty_add_memory_mapping;
    ops.remove_memory_mapping = &empty_remove_memory_mapping;

    return ops;
}

AddressSpaceDevicePingInfo createGraphicsContextRequest() {
    AddressSpaceDevicePingInfo req = {};
    req.metadata = static_cast<uint64_t>(
        AddressSpaceDeviceType::Graphics);
    return req;
}

AddressSpaceDevicePingInfo createAllocOrGetOffsetRequest() {
    AddressSpaceDevicePingInfo req = {};
    req.metadata = static_cast<uint64_t>(
        AddressSpaceGraphicsContext::GraphicsCommand::AllocOrGetOffset);
    return req;
}

AddressSpaceDevicePingInfo createGetSizeRequest() {
    AddressSpaceDevicePingInfo req = {};
    req.metadata = static_cast<uint64_t>(
        AddressSpaceGraphicsContext::GraphicsCommand::GetSize);
    return req;
}

AddressSpaceDevicePingInfo createNotifyAvailableRequest() {
    AddressSpaceDevicePingInfo req = {};
    req.metadata = static_cast<uint64_t>(
        AddressSpaceGraphicsContext::GraphicsCommand::NotifyAvailable);
    return req;
}

}  // namespace

class AddressSpaceGraphicsTest : public ::testing::Test {
protected:
    static struct address_space_device_control_ops* sControlOps;

    static void SetUpTestCase() {
        goldfish_address_space_set_vm_operations(gQAndroidVmOperations);
        sControlOps = new struct address_space_device_control_ops;
        *sControlOps = create_address_space_device_control_ops();
    }

    static void TearDownTestCase() {
        delete sControlOps;
    }

    void SetUp() override {
        mDevice = HostAddressSpaceDevice::get();
        AddressSpaceGraphicsContext::init(sControlOps);
    }

    void TearDown() override {
        AddressSpaceGraphicsContext::clear();
        mDevice->clear();
    }

    HostAddressSpaceDevice* mDevice = nullptr;
};

// static
struct address_space_device_control_ops* AddressSpaceGraphicsTest::sControlOps = 0;

TEST_F(AddressSpaceGraphicsTest, Basic) { }

TEST_F(AddressSpaceGraphicsTest, GraphicsContextCreate) {
    uint32_t handle = mDevice->open();

    auto graphicsCreateReq = createGraphicsContextRequest();
    mDevice->ping(handle, &graphicsCreateReq);

    mDevice->close(handle);
}

TEST_F(AddressSpaceGraphicsTest, GraphicsContextGetOffset) {
    uint32_t handle = mDevice->open();

    auto graphicsCreateReq = createGraphicsContextRequest();
    mDevice->ping(handle, &graphicsCreateReq);

    auto graphicsOffsetReq = createAllocOrGetOffsetRequest();
    mDevice->ping(handle, &graphicsOffsetReq);

    EXPECT_NE(0, graphicsOffsetReq.metadata);

    auto offset = graphicsOffsetReq.metadata;
    graphicsOffsetReq = createAllocOrGetOffsetRequest();
    mDevice->ping(handle, &graphicsOffsetReq);
    EXPECT_EQ(offset, graphicsOffsetReq.metadata);

    mDevice->close(handle);
}

TEST_F(AddressSpaceGraphicsTest, GraphicsContextGetOffsetAndSize) {
    uint32_t handle = mDevice->open();

    auto graphicsCreateReq = createGraphicsContextRequest();
    mDevice->ping(handle, &graphicsCreateReq);

    auto graphicsOffsetReq = createAllocOrGetOffsetRequest();
    mDevice->ping(handle, &graphicsOffsetReq);

    EXPECT_NE(0, graphicsOffsetReq.metadata);
    auto offset = graphicsOffsetReq.metadata;

    auto graphicsSizeReq = createGetSizeRequest();
    mDevice->ping(handle, &graphicsSizeReq);

    EXPECT_NE(0, graphicsSizeReq.metadata);

    mDevice->close(handle);
}

TEST_F(AddressSpaceGraphicsTest, NotifyAvailable) {
    uint32_t handle = mDevice->open();

    auto graphicsCreateReq = createGraphicsContextRequest();
    mDevice->ping(handle, &graphicsCreateReq);

    auto graphicsNotifyReq = createNotifyAvailableRequest();
    mDevice->ping(handle, &graphicsNotifyReq);
    EXPECT_EQ(0, graphicsNotifyReq.metadata);

    mDevice->close(handle);
}

}  // namespace emulation
} // namespace android
