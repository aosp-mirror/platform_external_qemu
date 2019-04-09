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

#include "goldfish_vk_baseprotodefs.pb.h"

#include "android/base/Pool.h"
#include "common/goldfish_vk_baseprotoconversion.h"
#include "common/goldfish_vk_testing.h"

#include "VulkanHandleMapping.h"

#include <gtest/gtest.h>

#include <vector>

#include <string.h>
#include <vulkan.h>

using android::base::Pool;

namespace goldfish_vk {

// Tests that a simple Vulkan struct works through protobuf API.
TEST(VulkanProtobuf, Basic) {

    VkExtent2D testStruct = {
        1, 2,
    };

    goldfish_vk_proto::VkExtent2D testProto;

    testProto.set_width(testStruct.width);
    testProto.set_height(testStruct.height);

    EXPECT_EQ(testStruct.width, testProto.width());
    EXPECT_EQ(testStruct.height, testProto.height());
}

// Tests that a Vulkan struct with strings works through protobuf API.
TEST(VulkanProtobuf, String) {
    constexpr char testAppName[] = "testAppName";
    constexpr char testEngineName[] = "testEngineName";

    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
        testAppName, 1,
        testEngineName, 2,
        3,
    };

    goldfish_vk_proto::VkApplicationInfo appInfoProto;
    appInfoProto.set_papplicationname(appInfo.pApplicationName);
    appInfoProto.set_penginename(appInfo.pEngineName);

    EXPECT_STREQ(appInfoProto.papplicationname().c_str(), appInfo.pApplicationName);
    EXPECT_STREQ(appInfoProto.penginename().c_str(), appInfo.pEngineName);
}

// Tests that a Vulkan struct with string arrays works through protobuf API.
TEST(VulkanProtobuf, StringArray) {
    const char* const testExtensionNames[] = {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
    };

    VkInstanceCreateInfo instCi = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        2, testExtensionNames,
    };

    goldfish_vk_proto::VkInstanceCreateInfo instCiProto;
    for (uint32_t i = 0; i < instCi.enabledExtensionCount; ++i) {
        *(instCiProto.add_ppenabledextensionnames()) =
            instCi.ppEnabledExtensionNames[i];
    }

    for (uint32_t i = 0; i < instCi.enabledExtensionCount; ++i) {
        EXPECT_STREQ(instCiProto.ppenabledextensionnames(i).c_str(),
                     instCi.ppEnabledExtensionNames[i]);
    }
}

// Tests that a Vulkan struct containing another struct works through protobuf API.
TEST(VulkanProtobuf, NestedStruct) {

    constexpr char testAppName[] = "testAppName";
    constexpr char testEngineName[] = "testEngineName";

    const char* const testExtensionNames[] = {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
    };

    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
        testAppName, 1,
        testEngineName, 2,
        3,
    };

    VkInstanceCreateInfo instCi = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        &appInfo,
        0, nullptr,
        2, testExtensionNames,
    };

    goldfish_vk_proto::VkInstanceCreateInfo instCiProto;

    for (uint32_t i = 0; i < instCi.enabledExtensionCount; ++i) {
        *(instCiProto.add_ppenabledextensionnames()) =
            instCi.ppEnabledExtensionNames[i];
    }

    for (uint32_t i = 0; i < instCi.enabledExtensionCount; ++i) {
        EXPECT_STREQ(instCiProto.ppenabledextensionnames(i).c_str(),
                     instCi.ppEnabledExtensionNames[i]);
    }

    EXPECT_FALSE(instCiProto.has_papplicationinfo());

    auto appInfoProtoPtr =
        instCiProto.mutable_papplicationinfo();

    EXPECT_TRUE(instCiProto.has_papplicationinfo());

    appInfoProtoPtr->set_papplicationname(appInfo.pApplicationName);
    appInfoProtoPtr->set_penginename(appInfo.pEngineName);

    EXPECT_STREQ(appInfoProtoPtr->papplicationname().c_str(), appInfo.pApplicationName);
    EXPECT_STREQ(appInfoProtoPtr->penginename().c_str(), appInfo.pEngineName);
}

// Tests that a Vulkan struct containing another repeated struct works through protobuf API.
TEST(VulkanProtobuf, NestedRepeatedStruct) {

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
    };

    VkDescriptorPoolCreateInfo poolCi = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1,
        (uint32_t)poolSizes.size(),
        poolSizes.data(),
    };

    goldfish_vk_proto::VkDescriptorPoolCreateInfo poolCiProto;

    for (uint32_t i = 0; i < poolCi.poolSizeCount; ++i) {
        auto size = poolCiProto.add_ppoolsizes();
        size->set_type(poolCi.pPoolSizes[i].type);
        size->set_descriptorcount(poolCi.pPoolSizes[i].descriptorCount);
    }

    for (uint32_t i = 0; i < poolCi.poolSizeCount; ++i) {
        EXPECT_EQ(poolCi.pPoolSizes[i].type,
                  poolCiProto.ppoolsizes(i).type());
        EXPECT_EQ(poolCi.pPoolSizes[i].descriptorCount,
                  poolCiProto.ppoolsizes(i).descriptorcount());
    }
}

// Tests that static string structs work via the protobuf api.
TEST(VulkanProtobuf, StaticString) {
    VkExtensionProperties extProps = {
        "VK_KHR_dedicated_allocation", 1,
    };

    goldfish_vk_proto::VkExtensionProperties extPropsProto;
    extPropsProto.set_extensionname(extProps.extensionName);
    extPropsProto.set_specversion(extProps.specVersion);

    EXPECT_STREQ(
        extProps.extensionName,
        extPropsProto.extensionname().c_str());
}

// Tests that extension structs work via the protobuf api.
TEST(VulkanProtobuf, ExtensionStruct) {
    VkMemoryDedicatedAllocateInfo dedicatedAi = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, 0,
        (VkImage)0,
        (VkBuffer)1,
    };

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        &dedicatedAi,
        4096,
        1,
    };

    goldfish_vk_proto::VkMemoryAllocateInfo allocInfoProto;
    auto ext = allocInfoProto.mutable_pnext();

    EXPECT_FALSE(ext->has_extension_vkmemorydedicatedallocateinfo());
    EXPECT_FALSE(ext->has_extension_vkexportmemoryallocateinfo());

    auto dedicatedAiProto =
        ext->mutable_extension_vkmemorydedicatedallocateinfo();

    // Check we don't step on and set any other extension.
    EXPECT_TRUE(ext->has_extension_vkmemorydedicatedallocateinfo());
    EXPECT_FALSE(ext->has_extension_vkexportmemoryallocateinfo());

    dedicatedAiProto->set_stype(dedicatedAi.sType);
    dedicatedAiProto->set_image((uint64_t)dedicatedAi.image);
    dedicatedAiProto->set_buffer((uint64_t)dedicatedAi.buffer);

    EXPECT_EQ(dedicatedAi.sType,
              allocInfoProto.pnext()
                  .extension_vkmemorydedicatedallocateinfo()
                  .stype());
}

static void onFailCompareFunc(const char* errMsg) {
    fprintf(stderr, "Failed to match: [%s]\n", errMsg);
    FAIL();
}

// Tests the protobuf conversion API at a basic level.
TEST(VulkanProtobuf, ConversionSimple) {
    Pool pool;
    DefaultHandleMapping mapping;

    VkExtent2D extent = { 1, 2, };
    VkExtent2D extentReturn;

    goldfish_vk_proto::VkExtent2D extentProto;

    to_proto_VkExtent2D(
        &mapping, &extent, &extentProto);

    from_proto_VkExtent2D(
        &pool, &mapping, &extentProto, &extentReturn);

    EXPECT_EQ(extent.width, extentReturn.width);
    EXPECT_EQ(extent.height, extentReturn.height);

    // Test the testing API as well.
    checkEqual_VkExtent2D(&extent, &extentReturn, onFailCompareFunc);
}

// Tests the protobuf conversion API when strings are involved.
TEST(VulkanProtobuf, ConversionStrings) {
    Pool pool;
    DefaultHandleMapping mapping;

    constexpr char testAppName[] = "testAppName";
    constexpr char testEngineName[] = "testEngineName";

    VkApplicationInfo testStruct = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
        testAppName, 1,
        testEngineName, 2,
        3,
    };

    VkApplicationInfo testStructReturn;

    goldfish_vk_proto::VkApplicationInfo testProto;

    to_proto_VkApplicationInfo(
        &mapping, &testStruct, &testProto);

    from_proto_VkApplicationInfo(
        &pool, &mapping, &testProto, &testStructReturn);

    EXPECT_STREQ(
        testStruct.pApplicationName,
        testStructReturn.pApplicationName);

    checkEqual_VkApplicationInfo(
        &testStruct, &testStructReturn, onFailCompareFunc);
}

// Tests the protobuf conversion API with arrays of strings.
TEST(VulkanProtobuf, ConversionStringArrays) {
    Pool pool;
    DefaultHandleMapping mapping;

    const char* const testExtensionNames[] = {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
    };

    VkInstanceCreateInfo testStruct = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        2, testExtensionNames,
    };

    VkInstanceCreateInfo testStructReturn;

    goldfish_vk_proto::VkInstanceCreateInfo testProto;

    to_proto_VkInstanceCreateInfo(
        &mapping, &testStruct, &testProto);

    EXPECT_FALSE(testProto.has_papplicationinfo());

    from_proto_VkInstanceCreateInfo(
        &pool, &mapping, &testProto, &testStructReturn);

    EXPECT_EQ(testStruct.enabledExtensionCount,
              testStructReturn.enabledExtensionCount);

    for (uint32_t i = 0; i < testStruct.enabledExtensionCount; ++i) {
        EXPECT_STREQ(
            testStruct.ppEnabledExtensionNames[i],
            testStructReturn.ppEnabledExtensionNames[i]);
    }

    checkEqual_VkInstanceCreateInfo(
        &testStruct, &testStructReturn, onFailCompareFunc);
}

// Tests the protobuf conversion API for nested structs.
TEST(VulkanProtobuf, ConversionNestedStruct) {
    Pool pool;
    DefaultHandleMapping mapping;

    constexpr char testAppName[] = "testAppName";
    constexpr char testEngineName[] = "testEngineName";

    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
        testAppName, 1,
        testEngineName, 2,
        3,
    };

    const char* const testExtensionNames[] = {
        "VK_KHR_get_physical_device_properties2",
        "VK_KHR_external_memory_capabilities",
    };

    VkInstanceCreateInfo testStruct = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        &appInfo,
        0, nullptr,
        2, testExtensionNames,
    };

    VkInstanceCreateInfo testStructReturn;

    goldfish_vk_proto::VkInstanceCreateInfo testProto;

    to_proto_VkInstanceCreateInfo(
        &mapping, &testStruct, &testProto);

    EXPECT_TRUE(testProto.has_papplicationinfo());

    from_proto_VkInstanceCreateInfo(
        &pool, &mapping, &testProto, &testStructReturn);

    EXPECT_EQ(testStruct.enabledExtensionCount,
              testStructReturn.enabledExtensionCount);
    EXPECT_NE(nullptr, testStructReturn.pApplicationInfo);

    checkEqual_VkInstanceCreateInfo(
        &testStruct, &testStructReturn, onFailCompareFunc);
}

// Tests that protobuf conversion API can work with nested repeated structs.
TEST(VulkanProtobuf, ConversionNestedRepeatedStruct) {
    Pool pool;
    DefaultHandleMapping mapping;

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
    };

    VkDescriptorPoolCreateInfo testStruct = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1,
        (uint32_t)poolSizes.size(),
        poolSizes.data(),
    };
    VkDescriptorPoolCreateInfo testStructReturn;

    goldfish_vk_proto::VkDescriptorPoolCreateInfo testProto;

    to_proto_VkDescriptorPoolCreateInfo(
        &mapping, &testStruct, &testProto);
    from_proto_VkDescriptorPoolCreateInfo(
        &pool, &mapping, &testProto, &testStructReturn);

    EXPECT_EQ(testStruct.poolSizeCount, testStructReturn.poolSizeCount);

    checkEqual_VkDescriptorPoolCreateInfo(
        &testStruct, &testStructReturn, onFailCompareFunc);
}

// Tests the protobuf conversion API for structs that have static strings.
TEST(VulkanProtobuf, ConversionStaticString) {
    Pool pool;
    DefaultHandleMapping mapping;

    VkExtensionProperties extProps = {
        "VK_KHR_dedicated_allocation", 1,
    };
    VkExtensionProperties extPropsReturn;

    goldfish_vk_proto::VkExtensionProperties extPropsProto;

    to_proto_VkExtensionProperties(&mapping, &extProps, &extPropsProto);
    from_proto_VkExtensionProperties(&pool, &mapping, &extPropsProto, &extPropsReturn);

    EXPECT_STREQ(extPropsReturn.extensionName, extProps.extensionName);

    checkEqual_VkExtensionProperties(&extProps, &extPropsReturn, onFailCompareFunc);
}

// Tests that protobuf conversion API can work with extension structs.
TEST(VulkanProtobuf, ConversionExtensionStruct) {
    Pool pool;
    DefaultHandleMapping mapping;

    VkMemoryDedicatedAllocateInfo dedicatedAi = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, 0,
        (VkImage)0,
        (VkBuffer)1,
    };

    VkMemoryAllocateInfo testStruct = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        &dedicatedAi,
        4096,
        1,
    };
    VkMemoryAllocateInfo testStructReturn;

    goldfish_vk_proto::VkMemoryAllocateInfo testProto;

    to_proto_VkMemoryAllocateInfo(&mapping, &testStruct, &testProto);
    from_proto_VkMemoryAllocateInfo(&pool, &mapping, &testProto, &testStructReturn);

    EXPECT_TRUE(testProto.has_pnext());

    checkEqual_VkMemoryAllocateInfo(
        &testStruct, &testStructReturn, onFailCompareFunc);
}

} // namespace goldfish_vk
