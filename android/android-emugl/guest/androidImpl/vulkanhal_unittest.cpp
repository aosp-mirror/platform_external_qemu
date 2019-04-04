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
#include "AndroidVulkanDispatch.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <vulkan/vk_android_native_buffer.h>

#include "android/base/ArraySize.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/opengles.h"

#include <android/hardware_buffer.h>

#include <atomic>
#include <random>
#include <memory>
#include <vector>

using android::base::pj;
using android::base::System;

namespace aemu {

static constexpr int kWindowSize = 256;

static android_vulkan_dispatch* vk = nullptr;

class VulkanHalTest : public ::testing::Test {
protected:

    static GoldfishOpenglTestEnv* testEnv;

    static void SetUpTestCase() {
        testEnv = new GoldfishOpenglTestEnv;
#ifdef _WIN32
        const char* libFilename = "vulkan_android.dll";
#elif defined(__APPLE__)
        const char* libFilename = "libvulkan_android.dylib";
#else
        const char* libFilename = "libvulkan_android.so";
#endif
        auto path =
            pj(System::get()->getProgramDirectory(),
                "lib64", libFilename);
        vk = load_android_vulkan_dispatch(path.c_str());
    }

    static void TearDownTestCase() {
        // Cancel all host threads as well
        android_finishOpenglesRenderer();

        delete testEnv;
        testEnv = nullptr;

        delete vk;
    }

    void SetUp() override {
        setupGralloc();
        setupVulkan();
    }

    void TearDown() override {
        teardownVulkan();
        teardownGralloc();
    }

    void setupGralloc() {
        auto grallocPath = pj(System::get()->getProgramDirectory(), "lib64",
                              "gralloc.ranchu" LIBSUFFIX);

        load_gralloc_module(grallocPath.c_str(), &mGralloc);
        set_global_gralloc_module(&mGralloc);

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

    void setupVulkan() {
        uint32_t extCount = 0;
        std::vector<VkExtensionProperties> exts;
        EXPECT_EQ(VK_SUCCESS, vk->vkEnumerateInstanceExtensionProperties(
                                      nullptr, &extCount, nullptr));
        exts.resize(extCount);
        EXPECT_EQ(VK_SUCCESS, vk->vkEnumerateInstanceExtensionProperties(
                                      nullptr, &extCount, exts.data()));

        bool hasGetPhysicalDeviceProperties2 = false;
        bool hasExternalMemoryCapabilities = false;

        for (const auto& prop : exts) {
            if (!strcmp("VK_KHR_get_physical_device_properties2", prop.extensionName)) {
                hasGetPhysicalDeviceProperties2 = true;
            }
            if (!strcmp("VK_KHR_external_memory_capabilities", prop.extensionName)) {
                hasExternalMemoryCapabilities = true;
            }
        }

        std::vector<const char*> enabledExtensions;

        if (hasGetPhysicalDeviceProperties2) {
            enabledExtensions.push_back("VK_KHR_get_physical_device_properties2");
            mInstanceHasGetPhysicalDeviceProperties2Support = true;
        }

        if (hasExternalMemoryCapabilities) {
            enabledExtensions.push_back("VK_KHR_external_memory_capabilities");
            mInstanceHasExternalMemorySupport = true;
        }

        const char* const* enabledExtensionNames =
                enabledExtensions.size() > 0 ? enabledExtensions.data()
                                             : nullptr;

        VkInstanceCreateInfo instCi = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            0, 0, nullptr,
            0, nullptr,
            (uint32_t)enabledExtensions.size(),
            enabledExtensionNames,
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateInstance(&instCi, nullptr, &mInstance));

        uint32_t physdevCount = 0;
        std::vector<VkPhysicalDevice> physdevs;
        EXPECT_EQ(VK_SUCCESS,
                  vk->vkEnumeratePhysicalDevices(mInstance, &physdevCount, nullptr));
        physdevs.resize(physdevCount);
        EXPECT_EQ(VK_SUCCESS,
                  vk->vkEnumeratePhysicalDevices(mInstance, &physdevCount,
                                                 physdevs.data()));
        std::vector<VkPhysicalDevice> physdevsSecond(physdevCount);
        EXPECT_EQ(VK_SUCCESS,
                  vk->vkEnumeratePhysicalDevices(mInstance, &physdevCount,
                                                 physdevsSecond.data()));
        // Check that a second call to vkEnumeratePhysicalDevices
        // retrieves the same physical device handles.
        EXPECT_EQ(physdevs, physdevsSecond);

        uint32_t bestPhysicalDevice = 0;
        bool queuesGood = false;
        bool hasAndroidNativeBuffer = false;
        bool hasExternalMemorySupport = false;

        for (uint32_t i = 0; i < physdevCount; ++i) {

            queuesGood = false;
            hasAndroidNativeBuffer = false;
            hasExternalMemorySupport = false;

            bool hasGetMemoryRequirements2 = false;
            bool hasDedicatedAllocation = false;
            bool hasExternalMemoryBaseExtension = false;
            bool hasExternalMemoryPlatformExtension = false;

            uint32_t queueFamilyCount = 0;
            std::vector<VkQueueFamilyProperties> queueFamilyProps;
            vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    physdevs[i], &queueFamilyCount, nullptr);
            queueFamilyProps.resize(queueFamilyCount);
            vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    physdevs[i], &queueFamilyCount, queueFamilyProps.data());

            for (uint32_t j = 0; j < queueFamilyCount; ++j) {
                auto count = queueFamilyProps[j].queueCount;
                auto flags = queueFamilyProps[j].queueFlags;
                if (count > 0 && (flags & VK_QUEUE_GRAPHICS_BIT)) {
                    bestPhysicalDevice = i;
                    mGraphicsQueueFamily = j;
                    queuesGood = true;
                    break;
                }
            }

            uint32_t devExtCount = 0;
            std::vector<VkExtensionProperties> availableDeviceExtensions;
            vk->vkEnumerateDeviceExtensionProperties(physdevs[i], nullptr,
                                                     &devExtCount, nullptr);
            availableDeviceExtensions.resize(devExtCount);
            vk->vkEnumerateDeviceExtensionProperties(
                    physdevs[i], nullptr, &devExtCount, availableDeviceExtensions.data());
            for (uint32_t j = 0; j < devExtCount; ++j) {
                if (!strcmp("VK_KHR_swapchain",
                            availableDeviceExtensions[j].extensionName)) {
                    hasAndroidNativeBuffer = true;
                }
                if (!strcmp("VK_KHR_get_memory_requirements2",
                            availableDeviceExtensions[j].extensionName)) {
                    hasGetMemoryRequirements2 = true;
                }
                if (!strcmp("VK_KHR_dedicated_allocation",
                            availableDeviceExtensions[j].extensionName)) {
                    hasDedicatedAllocation = true;
                }
                if (!strcmp("VK_KHR_external_memory",
                            availableDeviceExtensions[j].extensionName)) {
                    hasExternalMemoryBaseExtension = true;
                }
                static const char* externalMemoryPlatformExtension =
                    "VK_ANDROID_external_memory_android_hardware_buffer";

                if (!strcmp(externalMemoryPlatformExtension,
                            availableDeviceExtensions[j].extensionName)) {
                    hasExternalMemoryPlatformExtension = true;
                }
            }

            hasExternalMemorySupport =
                (hasGetMemoryRequirements2 &&
                 hasDedicatedAllocation &&
                 hasExternalMemoryBaseExtension &&
                 hasExternalMemoryPlatformExtension);

            if (queuesGood && hasExternalMemorySupport) {
                bestPhysicalDevice = i;
                break;
            }
        }

        EXPECT_TRUE(queuesGood);
        EXPECT_TRUE(hasAndroidNativeBuffer);

        mDeviceHasExternalMemorySupport =
            hasExternalMemorySupport;

        mPhysicalDevice = physdevs[bestPhysicalDevice];

        VkPhysicalDeviceMemoryProperties memProps;
        vk->vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProps);

        bool foundHostVisibleMemoryTypeIndex = false;
        bool foundDeviceLocalMemoryTypeIndex = false;

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if (memProps.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                mHostVisibleMemoryTypeIndex = i;
                foundHostVisibleMemoryTypeIndex = true;
            }

            if (memProps.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                mDeviceLocalMemoryTypeIndex = i;
                foundDeviceLocalMemoryTypeIndex = true;
            }

            if (foundHostVisibleMemoryTypeIndex &&
                foundDeviceLocalMemoryTypeIndex) {
                break;
            }
        }

        EXPECT_TRUE(
            foundHostVisibleMemoryTypeIndex &&
            foundDeviceLocalMemoryTypeIndex);

        EXPECT_TRUE(foundHostVisibleMemoryTypeIndex);

        float priority = 1.0f;
        VkDeviceQueueCreateInfo dqCi = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            0, 0,
            mGraphicsQueueFamily, 1,
            &priority,
        };

        VkDeviceCreateInfo dCi = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
            1, &dqCi,
            0, nullptr,  // no layers
            0, nullptr,  // no extensions
            nullptr,  // no features
        };

        std::vector<const char*> externalMemoryExtensions = {
            "VK_KHR_get_memory_requirements2",
            "VK_KHR_dedicated_allocation",
            "VK_KHR_external_memory",
            "VK_ANDROID_external_memory_android_hardware_buffer",
        };

        if (mDeviceHasExternalMemorySupport) {
            dCi.enabledExtensionCount =
                (uint32_t)externalMemoryExtensions.size();
            dCi.ppEnabledExtensionNames =
            externalMemoryExtensions.data();
        }

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateDevice(physdevs[bestPhysicalDevice], &dCi,
                                             nullptr, &mDevice));
        vk->vkGetDeviceQueue(mDevice, mGraphicsQueueFamily, 0, &mQueue);
    }

    void teardownVulkan() {
        vk->vkDestroyDevice(mDevice, nullptr);
        vk->vkDestroyInstance(mInstance, nullptr);
    }

    void createAndroidNativeImage(buffer_handle_t* buffer_out, VkImage* image_out) {

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
        EXPECT_EQ(VK_SUCCESS, vk->vkCreateImage(mDevice, &testImageCi, nullptr,
                                            &testAndroidNativeImage));

        *buffer_out = buffer;
        *image_out = testAndroidNativeImage;
    }

    void destroyAndroidNativeImage(buffer_handle_t buffer, VkImage image) {
        vk->vkDestroyImage(mDevice, image, nullptr);
        destroyTestGrallocBuffer(buffer);
    }

    AHardwareBuffer* allocateAndroidHardwareBuffer(
        int width = kWindowSize,
        int height = kWindowSize,
        AHardwareBuffer_Format format =
            AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
        uint64_t usage =
            AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
            AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
            AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN) {
    
        AHardwareBuffer_Desc desc = {
            kWindowSize, kWindowSize, 1,
            AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            usage,
            4, // stride ignored for allocate; don't check this
        };
    
        AHardwareBuffer* buf = nullptr;
        AHardwareBuffer_allocate(&desc, &buf);

        EXPECT_NE(nullptr, buf);

        return buf;
    }

    void exportAllocateAndroidHardwareBuffer(
        VkMemoryDedicatedAllocateInfo* dedicated,
        VkDeviceSize allocSize,
        uint32_t memoryTypeIndex,
        VkDeviceMemory* pMemory,
        AHardwareBuffer** ahw) {

        EXPECT_TRUE(mDeviceHasExternalMemorySupport);

        VkExportMemoryAllocateInfo exportAi = {
            VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, dedicated,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
        };

        VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &exportAi,
            allocSize, memoryTypeIndex,
        };

        VkResult res = vk->vkAllocateMemory(mDevice, &allocInfo, nullptr, pMemory);
        EXPECT_EQ(VK_SUCCESS, res);

        if (ahw) {
            VkMemoryGetAndroidHardwareBufferInfoANDROID getAhbInfo = {
                VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, 0, *pMemory,
            };

            EXPECT_EQ(VK_SUCCESS,
                vk->vkGetMemoryAndroidHardwareBufferANDROID(
                    mDevice, &getAhbInfo, ahw));
        }
    }

    void importAllocateAndroidHardwareBuffer(
        VkMemoryDedicatedAllocateInfo* dedicated,
        VkDeviceSize allocSize,
        uint32_t memoryTypeIndex,
        AHardwareBuffer* ahw,
        VkDeviceMemory* pMemory) {

        VkImportAndroidHardwareBufferInfoANDROID importInfo = {
            VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
            dedicated, ahw,
        };
    
        VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &importInfo,
            allocSize, memoryTypeIndex,
        };
    
        VkDeviceMemory memory;
        VkResult res = vk->vkAllocateMemory(mDevice, &allocInfo, nullptr, pMemory);
    
        EXPECT_EQ(VK_SUCCESS, res);
    }

    void createExternalImage(
        VkImage* pImage,
        uint32_t width = kWindowSize,
        uint32_t height = kWindowSize,
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) {

        VkExternalMemoryImageCreateInfo extMemImgCi = {
            VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, 0,
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
        };
    
        VkImageCreateInfo testImageCi = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            (const void*)&extMemImgCi, 0,
            VK_IMAGE_TYPE_2D, format,
            { width, height, 1, }, 1, 1,
            VK_SAMPLE_COUNT_1_BIT,
            tiling,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0, nullptr /* shared queue families */,
            VK_IMAGE_LAYOUT_UNDEFINED,
        };
    
        EXPECT_EQ(VK_SUCCESS,
            vk->vkCreateImage(
                mDevice, &testImageCi, nullptr, pImage));
    }

    uint32_t getFirstMemoryTypeIndexForImage(VkImage image) {
        VkMemoryRequirements memReqs;
        vk->vkGetImageMemoryRequirements(
            mDevice, image, &memReqs);

        uint32_t memoryTypeIndex = 0;
        EXPECT_NE(0, memReqs.memoryTypeBits);

        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
            if (memReqs.memoryTypeBits & (1 << i)) {
                memoryTypeIndex = i;
                break;
            }
        }
        return memoryTypeIndex;
    }

    VkDeviceSize getNeededMemorySizeForImage(VkImage image) {
        VkMemoryRequirements memReqs;
        vk->vkGetImageMemoryRequirements(
            mDevice, image, &memReqs);
        return memReqs.size;
    }

    struct gralloc_implementation mGralloc;

    bool mInstanceHasGetPhysicalDeviceProperties2Support = false;
    bool mInstanceHasExternalMemorySupport = false;
    bool mDeviceHasExternalMemorySupport = false;

    VkInstance mInstance;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
    VkQueue mQueue;
    uint32_t mHostVisibleMemoryTypeIndex;
    uint32_t mDeviceLocalMemoryTypeIndex;
    uint32_t mGraphicsQueueFamily;
};

// static
GoldfishOpenglTestEnv* VulkanHalTest::testEnv = nullptr;

// A basic test of Vulkan HAL:
// - Touch the Android loader at global, instance, and device level.
TEST_F(VulkanHalTest, Basic) { }

// Test: Allocate, map, flush, invalidate some host visible memory.
TEST_F(VulkanHalTest, MemoryMapping) {
    static constexpr VkDeviceSize kTestAlloc = 16 * 1024;
    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
        kTestAlloc,
        mHostVisibleMemoryTypeIndex,
    };
    VkDeviceMemory mem;
    EXPECT_EQ(VK_SUCCESS, vk->vkAllocateMemory(mDevice, &allocInfo, nullptr, &mem));

    void* hostPtr;
    EXPECT_EQ(VK_SUCCESS, vk->vkMapMemory(mDevice, mem, 0, VK_WHOLE_SIZE, 0, &hostPtr));

    memset(hostPtr, 0xff, kTestAlloc);

    VkMappedMemoryRange toFlush = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
        mem, 0, kTestAlloc,
    };

    EXPECT_EQ(VK_SUCCESS, vk->vkFlushMappedMemoryRanges(mDevice, 1, &toFlush));
    EXPECT_EQ(VK_SUCCESS, vk->vkInvalidateMappedMemoryRanges(mDevice, 1, &toFlush));

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
    EXPECT_EQ(VK_SUCCESS, vk->vkCreateImage(mDevice, &testImageCi, nullptr,
                                        &testAndroidNativeImage));
    vk->vkDestroyImage(mDevice, testAndroidNativeImage, nullptr);
    destroyTestGrallocBuffer(buffer);

    vk->vkUnmapMemory(mDevice, mem);
    vk->vkFreeMemory(mDevice, mem, nullptr);
}

// Tests creation of VkImages backed by gralloc buffers.
TEST_F(VulkanHalTest, AndroidNativeImageCreation) {
    VkImage image;
    buffer_handle_t buffer;
    createAndroidNativeImage(&buffer, &image);
    destroyAndroidNativeImage(buffer, image);
}

// Tests the path to sync Android native buffers with Gralloc buffers.
TEST_F(VulkanHalTest, AndroidNativeImageQueueSignal) {
    VkImage image;
    buffer_handle_t buffer;
    int fenceFd;

    createAndroidNativeImage(&buffer, &image);

    PFN_vkQueueSignalReleaseImageANDROID func =
        (PFN_vkQueueSignalReleaseImageANDROID)
        vk->vkGetDeviceProcAddr(mDevice, "vkQueueSignalReleaseImageANDROID");

    if (func) {
        fprintf(stderr, "%s: qsig\n", __func__);
        func(mQueue, 0, nullptr, image, &fenceFd);
    }

    destroyAndroidNativeImage(buffer, image);
}

// Tests VK_KHR_get_physical_device_properties2:
// new API: vkGetPhysicalDeviceProperties2KHR
TEST_F(VulkanHalTest, GetPhysicalDeviceProperties2) {
    if (!mInstanceHasGetPhysicalDeviceProperties2Support) {
        printf("Warning: Not testing VK_KHR_physical_device_properties2, not "
               "supported\n");
        return;
    }

    PFN_vkGetPhysicalDeviceProperties2KHR physProps2KHRFunc =
            (PFN_vkGetPhysicalDeviceProperties2KHR)vk->vkGetInstanceProcAddr(
                    mInstance, "vkGetPhysicalDeviceProperties2KHR");

    EXPECT_NE(nullptr, physProps2KHRFunc);

    VkPhysicalDeviceProperties2KHR props2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, 0,
    };

    physProps2KHRFunc(mPhysicalDevice, &props2);

    VkPhysicalDeviceProperties props;
    vk->vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

    EXPECT_EQ(props.vendorID, props2.properties.vendorID);
    EXPECT_EQ(props.deviceID, props2.properties.deviceID);
}

// Tests VK_KHR_get_physical_device_properties2:
// new API: vkGetPhysicalDeviceFeatures2KHR
TEST_F(VulkanHalTest, GetPhysicalDeviceFeatures2KHR) {
    if (!mInstanceHasGetPhysicalDeviceProperties2Support) {
        printf("Warning: Not testing VK_KHR_physical_device_properties2, not "
               "supported\n");
        return;
    }

    PFN_vkGetPhysicalDeviceFeatures2KHR physDeviceFeatures =
            (PFN_vkGetPhysicalDeviceFeatures2KHR)vk->vkGetInstanceProcAddr(
                    mInstance, "vkGetPhysicalDeviceFeatures2KHR");

    EXPECT_NE(nullptr, physDeviceFeatures);

    VkPhysicalDeviceFeatures2 features2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, 0,
    };

    physDeviceFeatures(mPhysicalDevice, &features2);
}

// Tests VK_KHR_get_physical_device_properties2:
// new API: vkGetPhysicalDeviceImageFormatProperties2KHR
TEST_F(VulkanHalTest, GetPhysicalDeviceImageFormatProperties2KHR) {
    if (!mInstanceHasGetPhysicalDeviceProperties2Support) {
        printf("Warning: Not testing VK_KHR_physical_device_properties2, not "
               "supported\n");
        return;
    }

    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR
            physDeviceImageFormatPropertiesFunc =
                    (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)
                            vk->vkGetInstanceProcAddr(mInstance,
                                                  "vkGetPhysicalDeviceImageForm"
                                                  "atProperties2KHR");

    EXPECT_NE(nullptr, physDeviceImageFormatPropertiesFunc);

    VkPhysicalDeviceImageFormatInfo2 imageFormatInfo = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, 0,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        0,
    };

    VkImageFormatProperties2 res = {
        VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, 0,
    };

    EXPECT_EQ(VK_SUCCESS, physDeviceImageFormatPropertiesFunc(
                                  mPhysicalDevice, &imageFormatInfo, &res));
}

// Tests that if we create an instance and the API version is less than 1.1,
// we return null for 1.1 core API calls.
TEST_F(VulkanHalTest, Hide1_1FunctionPointers) {
    VkPhysicalDeviceProperties props;

    vk->vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

    if (props.apiVersion < VK_API_VERSION_1_1) {
        EXPECT_EQ(nullptr,
                  vk->vkGetDeviceProcAddr(mDevice, "vkTrimCommandPool"));
    } else {
        EXPECT_NE(nullptr,
                  vk->vkGetDeviceProcAddr(mDevice, "vkTrimCommandPool"));
    }
}

// Tests VK_ANDROID_external_memory_android_hardware_buffer's allocation API.
// The simplest: export allocate device local memory.
TEST_F(VulkanHalTest, AndroidHardwareBufferAllocate_ExportDeviceLocal) {
    if (!mDeviceHasExternalMemorySupport) return;

    VkDeviceMemory memory;
    AHardwareBuffer* ahw;
    exportAllocateAndroidHardwareBuffer(
        nullptr, 4096, mDeviceLocalMemoryTypeIndex,
        &memory, &ahw);

    vk->vkFreeMemory(mDevice, memory, nullptr);
}

// Test AHB allocation via import.
TEST_F(VulkanHalTest, AndroidHardwareBufferAllocate_ImportDeviceLocal) {
    if (!mDeviceHasExternalMemorySupport) return;

    AHardwareBuffer* testBuf = allocateAndroidHardwareBuffer();

    VkDeviceMemory memory;

    importAllocateAndroidHardwareBuffer(
        nullptr,
        4096, // also checks that the top-level allocation size is ignored
        mDeviceLocalMemoryTypeIndex,
        testBuf,
        &memory);

    vk->vkFreeMemory(mDevice, memory, nullptr);

    AHardwareBuffer_release(testBuf);
}

// Test AHB allocation via export, but with a dedicated allocation (image).
TEST_F(VulkanHalTest, AndroidHardwareBufferAllocate_Dedicated_Export) {
    if (!mDeviceHasExternalMemorySupport) return;

    VkImage testAhbImage;
    createExternalImage(&testAhbImage);

    VkMemoryDedicatedAllocateInfo dedicatedAi = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, 0,
        testAhbImage, VK_NULL_HANDLE,
    };

    VkDeviceMemory memory;
    AHardwareBuffer* buffer;
    exportAllocateAndroidHardwareBuffer(
        &dedicatedAi,
        4096,
        getFirstMemoryTypeIndexForImage(testAhbImage),
        &memory, &buffer);

    EXPECT_EQ(VK_SUCCESS, vk->vkBindImageMemory(mDevice, testAhbImage, memory, 0));

    vk->vkFreeMemory(mDevice, memory, nullptr);
    vk->vkDestroyImage(mDevice, testAhbImage, nullptr);
}

// Test AHB allocation via import, but with a dedicated allocation (image).
TEST_F(VulkanHalTest, AndroidHardwareBufferAllocate_Dedicated_Import) {
    if (!mDeviceHasExternalMemorySupport) return;

    AHardwareBuffer* testBuf =
        allocateAndroidHardwareBuffer();

    VkImage testAhbImage;
    createExternalImage(&testAhbImage);

    VkMemoryDedicatedAllocateInfo dedicatedAi = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, 0,
        testAhbImage, VK_NULL_HANDLE,
    };

    VkDeviceMemory memory;
    importAllocateAndroidHardwareBuffer(
        &dedicatedAi,
        4096, // also checks that the top-level allocation size is ignored
        getFirstMemoryTypeIndexForImage(testAhbImage),
        testBuf,
        &memory);

    EXPECT_EQ(VK_SUCCESS,
        vk->vkBindImageMemory(mDevice, testAhbImage, memory, 0));

    vk->vkFreeMemory(mDevice, memory, nullptr);
    vk->vkDestroyImage(mDevice, testAhbImage, nullptr);

    AHardwareBuffer_release(testBuf);
}

// Test many host visible allocations.
TEST_F(VulkanHalTest, HostVisibleAllocations) {
    static constexpr VkDeviceSize kTestAllocSizesSmall[] =
        { 4, 5, 6, 16, 32, 37, 64, 255, 256, 267,
          1024, 1023, 1025, 4095, 4096,
          4097, 16333, };

    static constexpr size_t kNumSmallAllocSizes = android::base::arraySize(kTestAllocSizesSmall);
    static constexpr size_t kNumTrialsSmall = 1000;

    static constexpr VkDeviceSize kTestAllocSizesLarge[] =
        { 1048576, 1048577, 1048575 };

    static constexpr size_t kNumLargeAllocSizes = android::base::arraySize(kTestAllocSizesLarge);
    static constexpr size_t kNumTrialsLarge = 20;

    static constexpr float kLargeAllocChance = 0.05;

    std::default_random_engine generator;
    // Use a consistent seed value to avoid flakes
    generator.seed(0);

    std::uniform_int_distribution<size_t>
        smallAllocIndexDistribution(0, kNumSmallAllocSizes - 1);
    std::uniform_int_distribution<size_t>
        largeAllocIndexDistribution(0, kNumLargeAllocSizes - 1);
    std::bernoulli_distribution largeAllocDistribution(kLargeAllocChance);

    size_t smallAllocCount = 0;
    size_t largeAllocCount = 0;

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
        0,
        mHostVisibleMemoryTypeIndex,
    };

    std::vector<VkDeviceMemory> allocs;

    while (smallAllocCount < kNumTrialsSmall ||
           largeAllocCount < kNumTrialsLarge) {
        
        VkDeviceMemory mem = VK_NULL_HANDLE;
        void* hostPtr = nullptr;

        if (largeAllocDistribution(generator)) {
            if (largeAllocCount < kNumTrialsLarge) {
                fprintf(stderr, "%s: large alloc\n", __func__);
                allocInfo.allocationSize =
                    kTestAllocSizesLarge[
                        largeAllocIndexDistribution(generator)];
                ++largeAllocCount;
            }
        } else {
            if (smallAllocCount < kNumTrialsSmall) {
                allocInfo.allocationSize =
                    kTestAllocSizesSmall[
                        smallAllocIndexDistribution(generator)];
                ++smallAllocCount;
            }
        }

        EXPECT_EQ(VK_SUCCESS,
            vk->vkAllocateMemory(mDevice, &allocInfo, nullptr, &mem));

        if (!mem) continue;

        allocs.push_back(mem);

        EXPECT_EQ(VK_SUCCESS,
        vk->vkMapMemory(mDevice, mem, 0, VK_WHOLE_SIZE, 0, &hostPtr));

        memset(hostPtr, 0xff, 4);

        VkMappedMemoryRange toFlush = {
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
            mem, 0, 4,
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkFlushMappedMemoryRanges(mDevice, 1, &toFlush));
        EXPECT_EQ(VK_SUCCESS, vk->vkInvalidateMappedMemoryRanges(mDevice, 1, &toFlush));

        for (uint32_t i = 0; i < 4; ++i) {
            EXPECT_EQ(0xff, *((uint8_t*)hostPtr + i));
        }
    }

    for (auto mem : allocs) {
        vk->vkUnmapMemory(mDevice, mem);
        vk->vkFreeMemory(mDevice, mem, nullptr);
    }
}

TEST_F(VulkanHalTest, BufferCreate) {
    VkBufferCreateInfo bufCi = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
        4096,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
    };

    VkBuffer buffer;
    vk->vkCreateBuffer(mDevice, &bufCi, nullptr, &buffer);

    VkMemoryRequirements memReqs;
    vk->vkGetBufferMemoryRequirements(mDevice, buffer, &memReqs);

    vk->vkDestroyBuffer(mDevice, buffer, nullptr);
}

}  // namespace aemu
