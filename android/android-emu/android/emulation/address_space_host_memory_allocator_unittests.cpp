// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/address_space_host_memory_allocator.h"
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

AddressSpaceDevicePingInfo createAllocateRequest(uint64_t phys_addr) {
    AddressSpaceDevicePingInfo req = {};

    req.metadata = static_cast<uint64_t>(
        AddressSpaceHostMemoryAllocatorContext::HostMemoryAllocatorCommand::Allocate);
    req.phys_addr = phys_addr;
    req.size = 1;

    return req;
}

AddressSpaceDevicePingInfo createUnallocateRequest(uint64_t phys_addr) {
    AddressSpaceDevicePingInfo req = {};

    req.metadata = static_cast<uint64_t>(
        AddressSpaceHostMemoryAllocatorContext::HostMemoryAllocatorCommand::Unallocate);
    req.phys_addr = phys_addr;

    return req;
}
}  // namespace

TEST(AddressSpaceHostMemoryAllocatorContext, getDeviceType) {
    struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    AddressSpaceHostMemoryAllocatorContext ctx(&ops);

    EXPECT_EQ(ctx.getDeviceType(), AddressSpaceDeviceType::HostMemoryAllocator);
}

TEST(AddressSpaceHostMemoryAllocatorContext, AllocateDeallocate) {
    struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    AddressSpaceHostMemoryAllocatorContext ctx(&ops);

    AddressSpaceDevicePingInfo req;

    req = createAllocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);
}

TEST(AddressSpaceHostMemoryAllocatorContext, AllocateSamePhysAddr) {
    struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    AddressSpaceHostMemoryAllocatorContext ctx(&ops);

    AddressSpaceDevicePingInfo req;

    req = createAllocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createAllocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_NE(req.metadata, 0);

    req = createAllocateRequest(GOOD_GPA_2);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_2);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createAllocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);
}

TEST(AddressSpaceHostMemoryAllocatorContext, AllocateMappingFail) {
    struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    AddressSpaceHostMemoryAllocatorContext ctx(&ops);

    AddressSpaceDevicePingInfo req;

    req = createAllocateRequest(BAD_GPA);
    ctx.perform(&req);
    EXPECT_NE(req.metadata, 0);
}

TEST(AddressSpaceHostMemoryAllocatorContext, UnallocateTwice) {
    struct address_space_device_control_ops ops =
        create_address_space_device_control_ops();

    AddressSpaceHostMemoryAllocatorContext ctx(&ops);

    AddressSpaceDevicePingInfo req;

    req = createAllocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_EQ(req.metadata, 0);

    req = createUnallocateRequest(GOOD_GPA_1);
    ctx.perform(&req);
    EXPECT_NE(req.metadata, 0);
}

}  // namespace emulation
} // namespace android
