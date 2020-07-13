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
#include "android/base/files/MemStream.h"
#include "android/base/files/Stream.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/opengles.h"
#include "android/snapshot/interface.h"

#include <android/hardware_buffer.h>
#include <cutils/properties.h>

#include <atomic>
#include <random>
#include <memory>
#include <vector>

using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::FunctorThread;
using android::base::Lock;
using android::base::pj;
using android::base::System;

namespace aemu {

static constexpr int kWindowSize = 256;

static android_vulkan_dispatch* vk = nullptr;

class VulkanHalTest :
    public ::testing::Test,
    public ::testing::WithParamInterface<const char*> {
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

    bool usingAddressSpaceGraphics() {
        char value[PROPERTY_VALUE_MAX];
        if (property_get(
            "ro.kernel.qemu.gltransport", value, "pipe") > 0) {
            return !strcmp("asg", value);
        }
        return false;
    }

    void SetUp() override {
        property_set("ro.kernel.qemu.gltransport", GetParam());
        printf("%s: using transport: %s\n", __func__, GetParam());

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

        EXPECT_NE(nullptr, mGralloc.alloc_dev);
        EXPECT_NE(nullptr, mGralloc.alloc_module);
    }

    void teardownGralloc() { unload_gralloc_module(&mGralloc); }

    buffer_handle_t createTestGrallocBuffer(
            int usage, int format,
            int width, int height, int* stride_out) {
        buffer_handle_t buffer;
        int res;

        res = mGralloc.alloc(width, height, format, usage, &buffer, stride_out);
        if (res) {
            fprintf(stderr, "%s:%d res=%d buffer=%p\n", __func__, __LINE__, res, buffer);
            ::abort();
        }

        res = mGralloc.registerBuffer(buffer);
        if (res) {
            fprintf(stderr, "%s:%d res=%d buffer=%p\n", __func__, __LINE__, res, buffer);
            ::abort();
        }

        return buffer;
    }

    void destroyTestGrallocBuffer(buffer_handle_t buffer) {
        int res;

        res = mGralloc.unregisterBuffer(buffer);
        if (res) {
            fprintf(stderr, "%s:%d res=%d buffer=%p\n", __func__, __LINE__, res, buffer);
            ::abort();
        }

        res = mGralloc.free(buffer);
        if (res) {
            fprintf(stderr, "%s:%d res=%d buffer=%p\n", __func__, __LINE__, res, buffer);
            ::abort();
        }
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

        VkApplicationInfo appInfo = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
            "someAppName", 1,
            "someEngineName", 1,
            VK_VERSION_1_0,
        };

        VkInstanceCreateInfo instCi = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            0, 0, &appInfo,
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

        // Mostly for MoltenVK or any other driver that doesn't support
        // external memory.
        std::vector<const char*> usefulExtensions = {
            "VK_KHR_get_memory_requirements2",
            "VK_KHR_dedicated_allocation",
        };

        if (mDeviceHasExternalMemorySupport) {
            dCi.enabledExtensionCount =
                (uint32_t)externalMemoryExtensions.size();
            dCi.ppEnabledExtensionNames =
            externalMemoryExtensions.data();
        } else {
            dCi.enabledExtensionCount =
                (uint32_t)usefulExtensions.size();
            dCi.ppEnabledExtensionNames =
            usefulExtensions.data();
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

    VkResult allocateTestDescriptorSetsFromExistingPool(
        uint32_t setsToAllocate,
        VkDescriptorPool pool,
        VkDescriptorSetLayout setLayout,
        VkDescriptorSet* sets_out) {

        std::vector<VkDescriptorSetLayout> setLayouts;
        for (uint32_t i = 0; i < setsToAllocate; ++i) {
            setLayouts.push_back(setLayout);
        }

        VkDescriptorSetAllocateInfo setAi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 0,
            pool, setsToAllocate, setLayouts.data(),
        };

        return vk->vkAllocateDescriptorSets(
                    mDevice, &setAi, sets_out);
    }

    VkResult allocateTestDescriptorSets(
        uint32_t maxSetCount,
        uint32_t setsToAllocate,
        VkDescriptorType descriptorType,
        VkDescriptorPoolCreateFlags poolCreateFlags,
        VkDescriptorPool* pool_out,
        VkDescriptorSetLayout* setLayout_out,
        VkDescriptorSet* sets_out) {

        VkDescriptorPoolSize poolSize = {
            descriptorType,
            maxSetCount,
        };

        VkDescriptorPoolCreateInfo poolCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            maxSetCount /* maxSets */,
            1 /* poolSizeCount */,
            &poolSize,
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateDescriptorPool(mDevice, &poolCi, nullptr, pool_out));

        VkDescriptorSetLayoutBinding binding = {
            0,
            descriptorType,
            1,
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr,
        };

        VkDescriptorSetLayoutCreateInfo setLayoutCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, 0, 0,
            1,
            &binding,
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateDescriptorSetLayout(
            mDevice, &setLayoutCi, nullptr, setLayout_out));

        return allocateTestDescriptorSetsFromExistingPool(
            setsToAllocate, *pool_out, *setLayout_out, sets_out);
    }

    VkResult allocateImmutableSamplerDescriptorSets(
        uint32_t maxSetCount,
        uint32_t setsToAllocate,
        std::vector<bool> bindingImmutabilities,
        VkSampler* sampler_out,
        VkDescriptorPool* pool_out,
        VkDescriptorSetLayout* setLayout_out,
        VkDescriptorSet* sets_out) {

        VkSamplerCreateInfo samplerCi = {
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, 0, 0,
            VK_FILTER_NEAREST,
            VK_FILTER_NEAREST,
            VK_SAMPLER_MIPMAP_MODE_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            0.0f,
            VK_FALSE,
            1.0f,
            VK_FALSE,
            VK_COMPARE_OP_NEVER,
            0.0f,
            1.0f,
            VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            VK_FALSE,
        };

        EXPECT_EQ(VK_SUCCESS,
            vk->vkCreateSampler(
                mDevice, &samplerCi, nullptr, sampler_out));

        VkDescriptorPoolSize poolSize = {
            VK_DESCRIPTOR_TYPE_SAMPLER,
            maxSetCount,
        };

        VkDescriptorPoolCreateInfo poolCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            maxSetCount /* maxSets */,
            1 /* poolSizeCount */,
            &poolSize,
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateDescriptorPool(mDevice, &poolCi, nullptr, pool_out));

        std::vector<VkDescriptorSetLayoutBinding> samplerBindings;

        for (size_t i = 0; i < bindingImmutabilities.size(); ++i) {
            VkDescriptorSetLayoutBinding samplerBinding = {
                (uint32_t)i, VK_DESCRIPTOR_TYPE_SAMPLER,
                1, VK_SHADER_STAGE_FRAGMENT_BIT,
                bindingImmutabilities[i] ? sampler_out : nullptr,
            };
            samplerBindings.push_back(samplerBinding);
        }

        VkDescriptorSetLayoutCreateInfo setLayoutCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, 0, 0,
            (uint32_t)samplerBindings.size(),
            samplerBindings.data(),
        };

        EXPECT_EQ(VK_SUCCESS, vk->vkCreateDescriptorSetLayout(
            mDevice, &setLayoutCi, nullptr, setLayout_out));

        return allocateTestDescriptorSetsFromExistingPool(
            setsToAllocate, *pool_out, *setLayout_out, sets_out);
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
TEST_P(VulkanHalTest, Basic) { }

// Test: Allocate, map, flush, invalidate some host visible memory.
TEST_P(VulkanHalTest, MemoryMapping) {
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
TEST_P(VulkanHalTest, AndroidNativeImageCreation) {
    VkImage image;
    buffer_handle_t buffer;
    createAndroidNativeImage(&buffer, &image);
    destroyAndroidNativeImage(buffer, image);
}

// Tests the path to sync Android native buffers with Gralloc buffers.
TEST_P(VulkanHalTest, AndroidNativeImageQueueSignal) {
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
TEST_P(VulkanHalTest, GetPhysicalDeviceProperties2) {
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
TEST_P(VulkanHalTest, GetPhysicalDeviceFeatures2KHR) {
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
TEST_P(VulkanHalTest, GetPhysicalDeviceImageFormatProperties2KHR) {
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
TEST_P(VulkanHalTest, Hide1_1FunctionPointers) {
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
// Disabled for now: currently goes down invalid paths in the GL side
TEST_P(VulkanHalTest, DISABLED_AndroidHardwareBufferAllocate_ExportDeviceLocal) {
    if (!mDeviceHasExternalMemorySupport) return;

    VkDeviceMemory memory;
    AHardwareBuffer* ahw;
    exportAllocateAndroidHardwareBuffer(
        nullptr, 4096, mDeviceLocalMemoryTypeIndex,
        &memory, &ahw);

    vk->vkFreeMemory(mDevice, memory, nullptr);
}

// Test AHB allocation via import.
// Disabled for now: currently goes down invalid paths in the GL side
TEST_P(VulkanHalTest, DISABLED_AndroidHardwareBufferAllocate_ImportDeviceLocal) {
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
// Disabled for now: currently goes down invalid paths in the GL side
TEST_P(VulkanHalTest, DISABLED_AndroidHardwareBufferAllocate_Dedicated_Export) {
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
// Disabled for now: currently goes down invalid paths in the GL side
TEST_P(VulkanHalTest, DISABLED_AndroidHardwareBufferAllocate_Dedicated_Import) {
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
TEST_P(VulkanHalTest, HostVisibleAllocations) {
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

TEST_P(VulkanHalTest, BufferCreate) {
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

TEST_P(VulkanHalTest, SnapshotSaveLoad) {
    // TODO: Skip if using address space graphics
    if (usingAddressSpaceGraphics()) {
        printf("%s: skipping, ASG does not yet support snapshots\n", __func__);
        return;
    }
    androidSnapshot_save("test_snapshot");
    androidSnapshot_load("test_snapshot");
}

TEST_P(VulkanHalTest, SnapshotSaveLoadSimpleNonDispatchable) {
    // TODO: Skip if using address space graphics
    if (usingAddressSpaceGraphics()) {
        printf("%s: skipping, ASG does not yet support snapshots\n", __func__);
        return;
    }
    VkBufferCreateInfo bufCi = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
        4096,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
    };

    VkFence fence;
    VkFenceCreateInfo fenceCi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0, };
    vk->vkCreateFence(mDevice, &fenceCi, nullptr, &fence);

    fprintf(stderr, "%s: guest fence: %p\n", __func__, fence);

    VkBuffer buffer;
    vk->vkCreateBuffer(mDevice, &bufCi, nullptr, &buffer);

    fprintf(stderr, "%s: guest buffer: %p\n", __func__, buffer);

    androidSnapshot_save("test_snapshot");
    androidSnapshot_load("test_snapshot");

    VkMemoryRequirements memReqs;
    vk->vkGetBufferMemoryRequirements(mDevice, buffer, &memReqs);
    vk->vkDestroyBuffer(mDevice, buffer, nullptr);

    vk->vkDestroyFence(mDevice, fence, nullptr);
}

// Tests save/load of host visible memory.  This is not yet a viable host-only
// test because, the only way to really test it is to be able to preserve a
// host visible address for the simulated guest while the backing memory under
// it changes due to the new snapshot.  In other words, this is arbitrary
// remapping of virtual addrs and is functionality that does not exist on
// Linux/macOS. It would ironically require a hypervisor (or an OS that
// supports freer ways of mapping memory) in order to test properly.
// Disabled for now: currently goes down invalid paths in the GL side
TEST_P(VulkanHalTest, DISABLED_SnapshotSaveLoadHostVisibleMemory) {
    // TODO: Skip if using address space graphics
    if (usingAddressSpaceGraphics()) {
        printf("%s: skipping, ASG does not yet support snapshots\n", __func__);
        return;
    }

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
    androidSnapshot_save("test_snapshot");
    androidSnapshot_load("test_snapshot");


    memset(hostPtr, 0xff, kTestAlloc);

    VkMappedMemoryRange toFlush = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
        mem, 0, kTestAlloc,
    };

    EXPECT_EQ(VK_SUCCESS, vk->vkFlushMappedMemoryRanges(mDevice, 1, &toFlush));
    EXPECT_EQ(VK_SUCCESS, vk->vkInvalidateMappedMemoryRanges(mDevice, 1, &toFlush));

    vk->vkUnmapMemory(mDevice, mem);
    vk->vkFreeMemory(mDevice, mem, nullptr);
}

// Tests save/load of a dispatchable handle, such as VkCommandBuffer.
// Note that the internal state of the command buffer is not snapshotted yet.
TEST_P(VulkanHalTest, SnapshotSaveLoadSimpleDispatchable) {
    // TODO: Skip if using address space graphics
    if (usingAddressSpaceGraphics()) {
        printf("%s: skipping, ASG does not yet support snapshots\n", __func__);
        return;
    }
    VkCommandPoolCreateInfo poolCi = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0, 0, mGraphicsQueueFamily,
    };

    VkCommandPool pool;
    vk->vkCreateCommandPool(mDevice, &poolCi, nullptr, &pool);

    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo cbAi = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
        pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
    };

    vk->vkAllocateCommandBuffers(mDevice, &cbAi, &cb);

    androidSnapshot_save("test_snapshot");
    androidSnapshot_load("test_snapshot");

    vk->vkFreeCommandBuffers(mDevice, pool, 1, &cb);
    vk->vkDestroyCommandPool(mDevice, pool, nullptr);
}

// Tests that dependencies are respected between different handle types,
// such as VkImage and VkImageView.
TEST_P(VulkanHalTest, SnapshotSaveLoadDependentHandlesImageView) {
    // TODO: Skip if using address space graphics
    if (usingAddressSpaceGraphics()) {
        printf("%s: skipping, ASG does not yet support snapshots\n", __func__);
        return;
    }
    VkImage image;
    VkImageCreateInfo imageCi = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 0, 0,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        { 1, 1, 1, },
        1, 1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED,
    };

    vk->vkCreateImage(mDevice, &imageCi, nullptr, &image);

    VkImageView imageView;
    VkImageViewCreateInfo imageViewCi = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 0, 0,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, },
    };

    vk->vkCreateImageView(mDevice, &imageViewCi, nullptr, &imageView);

    androidSnapshot_save("test_snapshot");
    androidSnapshot_load("test_snapshot");

    vk->vkDestroyImageView(mDevice, imageView, nullptr);
    vk->vkCreateImageView(mDevice, &imageViewCi, nullptr, &imageView);
    vk->vkDestroyImageView(mDevice, imageView, nullptr);

    vk->vkDestroyImage(mDevice, image, nullptr);
}

// Tests beginning and ending command buffers from separate threads.
TEST_P(VulkanHalTest, SeparateThreadCommandBufferBeginEnd) {
    Lock lock;
    ConditionVariable cvSequence;
    uint32_t begins = 0;
    uint32_t ends = 0;
    constexpr uint32_t kTrials = 1000;

    VkCommandPoolCreateInfo poolCi = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0, 0, mGraphicsQueueFamily,
    };

    VkCommandPool pool;
    vk->vkCreateCommandPool(mDevice, &poolCi, nullptr, &pool);

    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo cbAi = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
        pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
    };

    vk->vkAllocateCommandBuffers(mDevice, &cbAi, &cb);

    auto timeoutDeadline = []() {
        return System::get()->getUnixTimeUs() + 5000000; // 5 s
    };

    FunctorThread beginThread([this, cb, &lock, &cvSequence, &begins, &ends, timeoutDeadline]() {

        while (begins < kTrials) {
            AutoLock a(lock);

            while (ends != begins) {
                if (!cvSequence.timedWait(&lock, timeoutDeadline())) {
                    EXPECT_TRUE(false) << "Error: begin thread timed out!";
                    return 0;
                }
            }

            VkCommandBufferBeginInfo beginInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0,
            };

            vk->vkBeginCommandBuffer(cb, &beginInfo);

            ++begins;
            cvSequence.signal();
        }

        vk->vkDeviceWaitIdle(mDevice);
        return 0;
    });

    FunctorThread endThread([this, cb, &lock, &cvSequence, &begins, &ends, timeoutDeadline]() {

        while (ends < kTrials) {
            AutoLock a(lock);

            while (begins - ends != 1) {
                if (!cvSequence.timedWait(&lock, timeoutDeadline())) {
                    EXPECT_TRUE(false) << "Error: end thread timed out!";
                    return 0;
                }
            }

            vk->vkEndCommandBuffer(cb);

            ++ends;
            cvSequence.signal();
        }

        vk->vkDeviceWaitIdle(mDevice);
        return 0;
    });

    beginThread.start();
    endThread.start();
    beginThread.wait();
    endThread.wait();

    vk->vkFreeCommandBuffers(mDevice, pool, 1, &cb);
    vk->vkDestroyCommandPool(mDevice, pool, nullptr);
}

// Tests creating a bunch of descriptor sets and freeing them via vkFreeDescriptorSet.
// 1. Via vkFreeDescriptorSet directly
// 2. Via vkResetDescriptorPool
// 3. Via vkDestroyDescriptorPool
// 4. Via vkResetDescriptorPool and double frees in vkFreeDescriptorSet
// 5. Via vkResetDescriptorPool and double frees in vkFreeDescriptorSet
// 4. Via vkResetDescriptorPool, creating more, and freeing vai vkFreeDescriptorSet
// (because vkFree* APIs are expected to never fail)
// https://github.com/KhronosGroup/Vulkan-Docs/issues/1070
TEST_P(VulkanHalTest, DescriptorSetAllocFreeBasic) {
    const uint32_t kSetCount = 4;
    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets(kSetCount);

    EXPECT_EQ(VK_SUCCESS, allocateTestDescriptorSets(
        kSetCount, kSetCount,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        &pool, &setLayout, sets.data()));

    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));

    // The double free should also work
    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));

    // Alloc/free again should also work
    EXPECT_EQ(VK_SUCCESS,
        allocateTestDescriptorSetsFromExistingPool(
        kSetCount, pool, setLayout, sets.data()));

    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));

    vk->vkDestroyDescriptorPool(mDevice, pool, nullptr);
}

// Tests creating a bunch of descriptor sets and freeing them via
// vkResetDescriptorPool, and that vkFreeDescriptorSets still succeeds.
TEST_P(VulkanHalTest, DescriptorSetAllocFreeReset) {
    const uint32_t kSetCount = 4;
    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets(kSetCount);

    EXPECT_EQ(VK_SUCCESS, allocateTestDescriptorSets(
        kSetCount, kSetCount,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        &pool, &setLayout, sets.data()));

    EXPECT_EQ(VK_SUCCESS, vk->vkResetDescriptorPool(
        mDevice, pool, 0));

    // The double free should also work
    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));

    // Alloc/reset/free again should also work
    EXPECT_EQ(VK_SUCCESS,
        allocateTestDescriptorSetsFromExistingPool(
        kSetCount, pool, setLayout, sets.data()));

    EXPECT_EQ(VK_SUCCESS, vk->vkResetDescriptorPool(
        mDevice, pool, 0));

    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));

    vk->vkDestroyDescriptorPool(mDevice, pool, nullptr);
}

// Tests creating a bunch of descriptor sets and freeing them via vkDestroyDescriptorPool, and that vkFreeDescriptorSets still succeeds.
TEST_P(VulkanHalTest, DescriptorSetAllocFreeDestroy) {
    const uint32_t kSetCount = 4;
    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets(kSetCount);

    EXPECT_EQ(VK_SUCCESS, allocateTestDescriptorSets(
        kSetCount, kSetCount,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        &pool, &setLayout, sets.data()));

    vk->vkDestroyDescriptorPool(mDevice, pool, nullptr);

    EXPECT_EQ(VK_SUCCESS, vk->vkFreeDescriptorSets(
        mDevice, pool, kSetCount, sets.data()));
}

// Tests that immutable sampler descriptors properly cause
// the |sampler| field of VkWriteDescriptorSet's descriptor image info
// to be ignored.
TEST_P(VulkanHalTest, ImmutableSamplersSuppressVkWriteDescriptorSetSampler) {
    const uint32_t kSetCount = 4;
    std::vector<bool> bindingImmutabilities = {
        false,
        true,
        false,
        false,
    };

    VkSampler sampler;
    VkDescriptorPool pool;
    VkDescriptorSetLayout setLayout;
    std::vector<VkDescriptorSet> sets(kSetCount);

    EXPECT_EQ(VK_SUCCESS,
        allocateImmutableSamplerDescriptorSets(
            kSetCount * bindingImmutabilities.size(),
            kSetCount,
            bindingImmutabilities,
            &sampler,
            &pool,
            &setLayout,
            sets.data()));

    for (uint32_t i = 0; i < bindingImmutabilities.size(); ++i) {
        VkDescriptorImageInfo imageInfo = {
            bindingImmutabilities[i] ? (VkSampler)0xdeadbeef : sampler,
            0,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet write = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 0,
            sets[0],
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_SAMPLER,
            &imageInfo,
            nullptr,
            nullptr,
        };

        vk->vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);
    }

    vk->vkDestroyDescriptorPool(mDevice, pool, nullptr);
    vk->vkDestroyDescriptorSetLayout(mDevice, setLayout, nullptr);
    vk->vkDestroySampler(mDevice, sampler, nullptr);
}


// Tests vkGetImageMemoryRequirements2
TEST_P(VulkanHalTest, GetImageMemoryRequirements2) {
    VkImageCreateInfo testImageCi = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr,
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

    VkImage testImage;
    EXPECT_EQ(VK_SUCCESS, vk->vkCreateImage(mDevice, &testImageCi, nullptr,
                                        &testImage));

    VkImageMemoryRequirementsInfo2 info2 = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR, 0,
        testImage,
    };

    VkMemoryDedicatedRequirements dedicatedReqs {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR, 0,
    };

    VkMemoryRequirements2 reqs2 = {
        VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR, 0,
    };

    reqs2.pNext = &dedicatedReqs;

    PFN_vkGetImageMemoryRequirements2KHR func =
        (PFN_vkGetImageMemoryRequirements2KHR)
        vk->vkGetDeviceProcAddr(mDevice, "vkGetImageMemoryRequirements2KHR");

    EXPECT_NE(nullptr, func);

    func(mDevice, &info2, &reqs2);

    vk->vkDestroyImage(mDevice, testImage, nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    MultipleTransports,
    VulkanHalTest,
    testing::ValuesIn(GoldfishOpenglTestEnv::getTransportsToTest()));

}  // namespace aemu
