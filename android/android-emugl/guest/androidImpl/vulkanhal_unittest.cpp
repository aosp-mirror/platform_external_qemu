// Copyright (C) 2018 The Android Open Source Project
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
#include <gtest/gtest.h>

#include "GoldfishOpenglTestEnv.h"
#include "GrallocDispatch.h"
#include "GrallocUsageConversion.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_android_native_buffer.h>

#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/opengles.h"

#include <atomic>
#include <memory>
#include <vector>

using android::base::pj;
using android::base::System;

namespace aemu {

static constexpr int kWindowSize = 256;

class VulkanHalTest : public ::testing::Test {
protected:

    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        testEnv = new GoldfishOpenglTestEnv;
    }

    static void TearDownTestCase() {
        delete testEnv;
        testEnv = nullptr;
    }

    void SetUp() override {
        setupGralloc();
    }

    void TearDown() override {
        teardownGralloc();
        // Cancel all host threads as well
        android_finishOpenglesRenderer();
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);

        EXPECT_NE(nullptr, mGralloc.fb_dev);
        EXPECT_NE(nullptr, mGralloc.alloc_dev);
        EXPECT_NE(nullptr, mGralloc.fb_module);
        EXPECT_NE(nullptr, mGralloc.alloc_module);
    }

    void teardownGralloc() { unload_gralloc_module(&mGralloc); }

    buffer_handle_t createTestGrallocBuffer(
            int usage, int format,
            int width, int height, int* stride_out) {
        buffer_handle_t buffer;

        mGralloc.alloc(width, height, format, usage, &buffer, stride_out);
        mGralloc.registerBuffer(buffer);

        return buffer;
    }

    void destroyTestGrallocBuffer(buffer_handle_t buffer) {
        mGralloc.unregisterBuffer(buffer);
        mGralloc.free(buffer);
    }

    struct gralloc_implementation mGralloc;
};

// static
GoldfishOpenglTestEnv* VulkanHalTest::testEnv = nullptr;

// A basic test of Vulkan HAL:
// - Touch the Android loader at global, instance, and device level.
// - Allocate, map, flush, invalidate some host visible memory.
TEST_F(VulkanHalTest, Basic) {
    uint32_t extCount = 0;
    std::vector<VkExtensionProperties> exts;
    EXPECT_EQ(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                  nullptr, &extCount, nullptr));
    exts.resize(extCount);
    EXPECT_EQ(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                  nullptr, &extCount, exts.data()));

    for (uint32_t i = 0; i < extCount; ++i) {
        printf("Available extension: %s\n", exts[i].extensionName);
    }

    VkInstanceCreateInfo instCi = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        0, nullptr,
    };

    VkInstance inst;
    EXPECT_EQ(VK_SUCCESS, vkCreateInstance(&instCi, nullptr, &inst));

    uint32_t physdevCount = 0;
    std::vector<VkPhysicalDevice> physdevs;
    EXPECT_EQ(VK_SUCCESS, vkEnumeratePhysicalDevices(inst, &physdevCount, nullptr));
    physdevs.resize(physdevCount);
    EXPECT_EQ(VK_SUCCESS,
              vkEnumeratePhysicalDevices(inst, &physdevCount, physdevs.data()));

    uint32_t bestPhysicalDevice = 0;
    uint32_t bestQueueFamily = 0;
    for (uint32_t i = 0; i < physdevCount; ++i) {
        uint32_t queueFamilyCount = 0;
        std::vector<VkQueueFamilyProperties> queueFamilyProps;
        vkGetPhysicalDeviceQueueFamilyProperties(physdevs[i], &queueFamilyCount,
                                                 nullptr);
        queueFamilyProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physdevs[i], &queueFamilyCount,
                                                 queueFamilyProps.data());
        bool queuesGood = false;

        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
            auto count = queueFamilyProps[j].queueCount;
            auto flags = queueFamilyProps[j].queueFlags;
            if (count > 0 && (flags & VK_QUEUE_GRAPHICS_BIT)) {
                bestPhysicalDevice = i;
                bestQueueFamily = j;
                queuesGood = true;
                break;
            }
        }

        if (queuesGood) {
            break;
        }
    }

    VkPhysicalDevice physdev = physdevs[bestPhysicalDevice];

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physdev, &memProps);

    uint32_t hostVisibleMemoryTypeIndex = 0;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if (memProps.memoryTypes[i].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            hostVisibleMemoryTypeIndex = i;
            break;
        }
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo dqCi = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
        bestQueueFamily, 1, &priority,
    };

    VkDeviceCreateInfo dCi = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
        1, &dqCi,
        0, nullptr, // no layers
        0, nullptr, // no extensions
        nullptr, // no features
    };

    VkDevice device;
    EXPECT_EQ(VK_SUCCESS, vkCreateDevice(physdevs[bestPhysicalDevice], &dCi, nullptr, &device));

    static constexpr VkDeviceSize kTestAlloc = 16 * 1024;
    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
        kTestAlloc,
        hostVisibleMemoryTypeIndex,
    };
    VkDeviceMemory mem;
    EXPECT_EQ(VK_SUCCESS, vkAllocateMemory(device, &allocInfo, nullptr, &mem));

    void* hostPtr;
    EXPECT_EQ(VK_SUCCESS, vkMapMemory(device, mem, 0, VK_WHOLE_SIZE, 0, &hostPtr));

    memset(hostPtr, 0xff, kTestAlloc);

    VkMappedMemoryRange toFlush = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
        mem, 0, kTestAlloc,
    };

    EXPECT_EQ(VK_SUCCESS, vkFlushMappedMemoryRanges(device, 1, &toFlush));
    EXPECT_EQ(VK_SUCCESS, vkInvalidateMappedMemoryRanges(device, 1, &toFlush));

    for (uint32_t i = 0; i < kTestAlloc; ++i) {
        EXPECT_EQ(0xff, *((uint8_t*)hostPtr + i));
    }

    int usage = GRALLOC_USAGE_HW_RENDER;
    int format = HAL_PIXEL_FORMAT_RGBA_8888;
    int stride;
    buffer_handle_t buffer =
        createTestGrallocBuffer(
            usage, format, kWindowSize, kWindowSize, &stride);

    uint64_t producerUsage, consumerUsage;
    android_convertGralloc0To1Usage(usage, &producerUsage, &consumerUsage);

    VkNativeBufferANDROID nativeBufferInfo = {
        VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID, nullptr,
        buffer, stride,
        format,
        usage,
        {
            consumerUsage,
            producerUsage,
        },
    };

    VkImageCreateInfo testImageCi = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, (const void*)&nativeBufferInfo,
        0,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        { kWindowSize, kWindowSize, 1, },
        1, 1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr /* shared queue families */,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage testAndroidNativeImage;
    EXPECT_EQ(VK_SUCCESS, vkCreateImage(device, &testImageCi, nullptr,
                                        &testAndroidNativeImage));
    vkDestroyImage(device, testAndroidNativeImage, nullptr);
    destroyTestGrallocBuffer(buffer);

    vkUnmapMemory(device, mem);
    vkFreeMemory(device, mem, nullptr);

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(inst, nullptr);
}

}  // namespace aemu
