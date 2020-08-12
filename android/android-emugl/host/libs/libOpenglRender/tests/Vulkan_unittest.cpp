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

#include "FrameBuffer.h"
#include "VkCommonOperations.h"
#include "VulkanDispatch.h"
#include "emugl/common/feature_control.h"

#include "android/base/ArraySize.h"
#include "android/base/GLObjectCounter.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/emulation/control/AndroidAgentFactory.h"

#include "Standalone.h"

#include <sstream>
#include <string>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#include <windows.h>
#include "android/base/system/Win32UnicodeString.h"
using android::base::Win32UnicodeString;
#else
#include <dlfcn.h>
#endif

using android::base::arraySize;
using android::base::pj;
using android::base::TestSystem;

namespace emugl {

static std::string libDir() {
    return
        pj(TestSystem::getProgramDirectoryFromPlatform(),
#ifdef _WIN32
           // Windows uses mock Vulkan ICD.
           "testlib64"
#else
           "lib64", "vulkan"
#endif
        );
}

static std::string testIcdFilename() {
    return pj(libDir(),
#ifdef _WIN32
        // Windows uses mock Vulkan ICD.
        "VkICD_mock_icd.json"
#else
        "vk_swiftshader_icd.json"
#endif
    );
}

#ifdef _WIN32
#define SKIP_TEST_IF_WIN32() GTEST_SKIP()
#else
#define SKIP_TEST_IF_WIN32()
#endif

static void* dlOpenFuncForTesting() {
#ifdef _WIN32
    const Win32UnicodeString name(
        pj(libDir(), "vulkan-1.dll"));
    return LoadLibraryW(name.c_str());
#else

#ifdef __APPLE__
    constexpr char suffix[] = ".dylib";
#else
    constexpr char suffix[] = ".so";
#endif

    std::string libName =
        std::string("libvulkan") + suffix;

    auto name = pj(libDir(), libName);
    return dlopen(name.c_str(), RTLD_NOW);
#endif
}

static void* dlSymFuncForTesting(void* lib, const char* sym) {
#ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)lib, sym);
#else
    return dlsym(lib, sym);
#endif
}

static std::string deviceTypeToString(VkPhysicalDeviceType type) {
#define DO_ENUM_RETURN_STRING(e) \
    case e: \
        return #e; \

    switch (type) {
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_OTHER)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_CPU)
        default:
            return "Unknown";
    }
}

static std::string queueFlagsToString(VkQueueFlags queueFlags) {
    std::stringstream ss;

    if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        ss << "VK_QUEUE_GRAPHICS_BIT | ";
    }
    if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
        ss << "VK_QUEUE_COMPUTE_BIT | ";
    }
    if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
        ss << "VK_QUEUE_TRANSFER_BIT | ";
    }
    if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        ss << "VK_QUEUE_SPARSE_BINDING_BIT | ";
    }
    if (queueFlags & VK_QUEUE_PROTECTED_BIT) {
        ss << "VK_QUEUE_PROTECTED_BIT";
    }

    return ss.str();
}

static std::string getPhysicalDevicePropertiesString(
    const goldfish_vk::VulkanDispatch* vk,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceProperties& props) {

    std::stringstream ss;

    uint16_t apiMaj = (uint16_t)(props.apiVersion >> 22);
    uint16_t apiMin = (uint16_t)(0x000003ff & (props.apiVersion >> 12));
    uint16_t apiPatch = (uint16_t)(0x000007ff & (props.apiVersion));

    ss << "API version: " << apiMaj << "." << apiMin << "." << apiPatch << "\n";
    ss << "Driver version: " << std::hex << props.driverVersion << "\n";
    ss << "Vendor ID: " << std::hex << props.vendorID << "\n";
    ss << "Device ID: " << std::hex << props.deviceID << "\n";
    ss << "Device type: " << deviceTypeToString(props.deviceType) << "\n";
    ss << "Device name: " << props.deviceName << "\n";

    uint32_t deviceExtensionCount;
    std::vector<VkExtensionProperties> deviceExtensionProperties;
    vk->vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &deviceExtensionCount,
        nullptr);

    deviceExtensionProperties.resize(deviceExtensionCount);
    vk->vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &deviceExtensionCount,
        deviceExtensionProperties.data());

    for (uint32_t i = 0; i < deviceExtensionCount; ++i) {
        ss << "Device extension: " <<
            deviceExtensionProperties[i].extensionName << "\n";
    }

    return ss.str();
}

static void testInstanceCreation(const VulkanDispatch* vk,
                                 VkInstance* instance_out) {

    EXPECT_TRUE(vk->vkEnumerateInstanceExtensionProperties);
    EXPECT_TRUE(vk->vkCreateInstance);

    uint32_t count;
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    fprintf(stderr, "%s: exts: %u\n", __func__, count);

    std::vector<VkExtensionProperties> props(count);
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

    for (uint32_t i = 0; i < count; i++) {
        fprintf(stderr, "%s: ext: %s\n", __func__, props[i].extensionName);
    }

    VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        0, nullptr,
    };

    VkInstance instance;

    EXPECT_EQ(VK_SUCCESS,
        vk->vkCreateInstance(
            &instanceCreateInfo, nullptr, &instance));

    *instance_out = instance;
}

static void testDeviceCreation(const VulkanDispatch* vk,
                               VkInstance instance,
                               VkPhysicalDevice* physDevice_out,
                               VkDevice* device_out) {

    fprintf(stderr, "%s: call\n", __func__);

    EXPECT_TRUE(vk->vkEnumeratePhysicalDevices);
    EXPECT_TRUE(vk->vkGetPhysicalDeviceProperties);
    EXPECT_TRUE(vk->vkGetPhysicalDeviceQueueFamilyProperties);

    uint32_t physicalDeviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;

    EXPECT_EQ(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, nullptr));

    physicalDevices.resize(physicalDeviceCount);

    EXPECT_EQ(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, physicalDevices.data()));

    std::vector<VkPhysicalDeviceProperties> physicalDeviceProps(physicalDeviceCount);

    // at the end of the day, we need to pick a physical device.
    // Pick one that has graphics + compute if possible, otherwise settle for a device
    // that has at least one queue family capable of graphics.
    // TODO: Pick the device that has present capability for that queue if
    // we are not running in no-window mode.

    bool bestPhysicalDeviceFound = false;
    uint32_t bestPhysicalDeviceIndex = 0;

    std::vector<uint32_t> physDevsWithBothGraphicsAndCompute;
    std::vector<uint32_t> physDevsWithGraphicsOnly;

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        uint32_t deviceExtensionCount;
        std::vector<VkExtensionProperties> deviceExtensionProperties;
        vk->vkEnumerateDeviceExtensionProperties(
            physicalDevices[i],
            nullptr,
            &deviceExtensionCount,
            nullptr);

        deviceExtensionProperties.resize(deviceExtensionCount);
        vk->vkEnumerateDeviceExtensionProperties(
            physicalDevices[i],
            nullptr,
            &deviceExtensionCount,
            deviceExtensionProperties.data());

        bool hasSwapchainExtension = false;

        fprintf(stderr, "%s: check swapchain ext\n", __func__);
        for (uint32_t j = 0; j < deviceExtensionCount; j++) {
            std::string ext = deviceExtensionProperties[j].extensionName;
            if (ext == "VK_KHR_swapchain") {
                hasSwapchainExtension = true;
            }
        }

        if (!hasSwapchainExtension) continue;

        vk->vkGetPhysicalDeviceProperties(
            physicalDevices[i],
            physicalDeviceProps.data() + i);

        auto str = getPhysicalDevicePropertiesString(vk, physicalDevices[i], physicalDeviceProps[i]);
        fprintf(stderr, "device %u: %s\n", i, str.c_str());

        uint32_t queueFamilyCount;
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilies.data());

        bool hasGraphicsQueue = false;
        bool hasComputeQueue = false;

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueCount > 0) {

                auto flags = queueFamilies[j].queueFlags;
                auto flagsAsString =
                    queueFlagsToString(flags);

                fprintf(stderr, "%s: found %u @ family %u with caps: %s\n",
                        __func__,
                        queueFamilies[j].queueCount, j,
                        flagsAsString.c_str());

                if ((flags & VK_QUEUE_GRAPHICS_BIT) &&
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                    hasGraphicsQueue = true;
                    hasComputeQueue = true;
                    bestPhysicalDeviceFound = true;
                    break;
                }

                if (flags & VK_QUEUE_GRAPHICS_BIT) {
                    hasGraphicsQueue = true;
                    bestPhysicalDeviceFound = true;
                }

                if (flags & VK_QUEUE_COMPUTE_BIT) {
                    hasComputeQueue = true;
                    bestPhysicalDeviceFound = true;
                }
            }
        }

        if (hasGraphicsQueue && hasComputeQueue) {
            physDevsWithBothGraphicsAndCompute.push_back(i);
            break;
        }

        if (hasGraphicsQueue) {
            physDevsWithGraphicsOnly.push_back(i);
        }
    }

    EXPECT_TRUE(bestPhysicalDeviceFound);

    if (physDevsWithBothGraphicsAndCompute.size() > 0) {
        bestPhysicalDeviceIndex = physDevsWithBothGraphicsAndCompute[0];
    } else if (physDevsWithGraphicsOnly.size() > 0) {
        bestPhysicalDeviceIndex = physDevsWithGraphicsOnly[0];
    } else {
        EXPECT_TRUE(false);
        return;
    }

    // Now we got our device; select it
    VkPhysicalDevice bestPhysicalDevice = physicalDevices[bestPhysicalDeviceIndex];

    uint32_t queueFamilyCount;
    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        bestPhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        bestPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t> wantedQueueFamilies;
    std::vector<uint32_t> wantedQueueFamilyCounts;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueCount > 0) {
            auto flags = queueFamilies[i].queueFlags;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) &&
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                wantedQueueFamilies.push_back(i);
                wantedQueueFamilyCounts.push_back(queueFamilies[i].queueCount);
                break;
            }

            if ((flags & VK_QUEUE_GRAPHICS_BIT) ||
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                wantedQueueFamilies.push_back(i);
                wantedQueueFamilyCounts.push_back(queueFamilies[i].queueCount);
            }
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCis;

    for (uint32_t i = 0; i < wantedQueueFamilies.size(); ++i) {
        auto familyIndex = wantedQueueFamilies[i];
        auto queueCount = wantedQueueFamilyCounts[i];

        std::vector<float> priorities;

        for (uint32_t j = 0; j < queueCount; ++j) {
            priorities.push_back(1.0f);
        }

        VkDeviceQueueCreateInfo dqci = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
            familyIndex,
            queueCount,
            priorities.data(),
        };

        queueCis.push_back(dqci);
    }

    const char* exts[] = {
        "VK_KHR_swapchain",
    };

    VkDeviceCreateInfo ci = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
        (uint32_t)queueCis.size(),
        queueCis.data(),
        0, nullptr,
        arraySize(exts), exts,
        nullptr,
    };

    VkDevice device;
    EXPECT_EQ(VK_SUCCESS,
        vk->vkCreateDevice(bestPhysicalDevice, &ci, nullptr, &device));

    *physDevice_out = bestPhysicalDevice;
    *device_out = device;
}

static void teardownVulkanTest(const VulkanDispatch* vk,
                               VkDevice dev,
                               VkInstance instance) {
    vk->vkDestroyDevice(dev, nullptr);
    vk->vkDestroyInstance(instance, nullptr);
}

class VulkanTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestSystem::setEnvironmentVariable(
            "VK_ICD_FILENAMES",
            testIcdFilename());

        goldfish_vk::init_vulkan_dispatch_from_system_loader(
                dlOpenFuncForTesting,
                dlSymFuncForTesting,
                &mVk);

        testInstanceCreation(&mVk, &mInstance);
        testDeviceCreation(
            &mVk, mInstance, &mPhysicalDevice, &mDevice);
    }

    void TearDown() override {
        teardownVulkanTest(&mVk, mDevice, mInstance);
        TestSystem::setEnvironmentVariable(
            "VK_ICD_FILENAMES", "");
    }

    VulkanDispatch mVk;
    VkInstance mInstance;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
};

// Basic Vulkan instance/device setup.
TEST_F(VulkanTest, Basic) { }

// Checks that staging memory query is successful.
TEST_F(VulkanTest, StagingMemoryQuery) {
    VkPhysicalDeviceMemoryProperties memProps;
    uint32_t typeIndex;

    mVk.vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProps);

    EXPECT_TRUE(goldfish_vk::getStagingMemoryTypeIndex(
        &mVk, mDevice, &memProps, &typeIndex));
}

#ifndef _WIN32 // TODO: Get this working w/ Swiftshader vk on Windows
class VulkanFrameBufferTest : public VulkanTest {
protected:
    void SetUp() override {
        // SwiftShader Vulkan doesn't work on Windows, so we skip all
        // the rendering tests on Windows for now.
        SKIP_TEST_IF_WIN32();

        VulkanTest::SetUp();

        emugl::set_emugl_feature_is_enabled(
                [](android::featurecontrol::Feature feature) {
                    return feature == android::featurecontrol::Vulkan;
                });

        setupStandaloneLibrarySearchPaths();
        emugl::setGLObjectCounter(android::base::GLObjectCounter::get());
        emugl::set_emugl_window_operations(*getConsoleAgents()->emu);
        emugl::set_emugl_multi_display_operations(*getConsoleAgents()->multi_display);
        const EGLDispatch* egl = LazyLoadedEGLDispatch::get();
        ASSERT_NE(nullptr, egl);
        ASSERT_NE(nullptr, LazyLoadedGLESv2Dispatch::get());

        mRenderThreadInfo = std::make_unique<RenderThreadInfo>();

        bool useHostGpu = shouldUseHostGpu();
        EXPECT_TRUE(FrameBuffer::initialize(mWidth, mHeight, false,
                                            !useHostGpu /* egl2egl */));
        mFb = FrameBuffer::getFB();
        ASSERT_NE(nullptr, mFb);
    }

    void TearDown() override { VulkanTest::TearDown(); }

    FrameBuffer* mFb = nullptr;
    std::unique_ptr<RenderThreadInfo> mRenderThreadInfo;

    constexpr static uint32_t mWidth = 640u;
    constexpr static uint32_t mHeight = 480u;
};

TEST_F(VulkanFrameBufferTest, VkColorBufferWithoutMemoryProperties) {
    // Create a color buffer without any memory properties restriction.
    HandleType colorBuffer = mFb->createColorBuffer(
            mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    ASSERT_NE(colorBuffer, 0u);
    EXPECT_TRUE(goldfish_vk::setupVkColorBuffer(colorBuffer,
                                                true, /* vulkanOnly */
                                                0     /* memoryProperty */
                                                ));
    EXPECT_TRUE(goldfish_vk::teardownVkColorBuffer(colorBuffer));
    mFb->closeColorBuffer(colorBuffer);
}

TEST_F(VulkanFrameBufferTest, VkColorBufferWithMemoryPropertyFlags) {
    auto* vkEmulation = goldfish_vk::getGlobalVkEmulation();
    VkMemoryPropertyFlags kTargetMemoryPropertyFlags =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    // Create a Vulkan image with the same dimension and usage as
    // the color buffer, to get a possible memory type index.
    VkImageCreateInfo testImageCi = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            nullptr,
            0,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            {
                    mWidth,
                    mHeight,
                    1,
            },
            1,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr /* shared queue families */,
            VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage image;
    mVk.vkCreateImage(mDevice, &testImageCi, nullptr, &image);

    VkMemoryRequirements memReq;
    mVk.vkGetImageMemoryRequirements(mDevice, image, &memReq);

    mVk.vkDestroyImage(mDevice, image, nullptr);

    if (!memReq.memoryTypeBits) {
        GTEST_SKIP();
    }

    int32_t memoryTypeIndex = 31;
    do {
        if (((1 << memoryTypeIndex) & memReq.memoryTypeBits) &&
            (vkEmulation->deviceInfo.memProps.memoryTypes[memoryTypeIndex]
                     .propertyFlags &
             kTargetMemoryPropertyFlags)) {
            break;
        }
    } while (--memoryTypeIndex >= 0);

    if (memoryTypeIndex < 0) {
        fprintf(stderr,
                "No memory type supported for HOST_VISBILE memory "
                "properties. Test skipped.\n");
        GTEST_SKIP();
    }

    // Create a color buffer with the target memory property flags.
    uint32_t allocatedTypeIndex = 0u;
    HandleType colorBuffer = mFb->createColorBuffer(
            mWidth, mHeight, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
    ASSERT_NE(colorBuffer, 0u);
    EXPECT_TRUE(goldfish_vk::setupVkColorBuffer(
            colorBuffer, true, /* vulkanOnly */
            static_cast<uint32_t>(kTargetMemoryPropertyFlags), nullptr, nullptr,
            &allocatedTypeIndex));
    EXPECT_TRUE(vkEmulation->deviceInfo.memProps.memoryTypes[allocatedTypeIndex]
                        .propertyFlags &
                kTargetMemoryPropertyFlags);
    EXPECT_TRUE(goldfish_vk::teardownVkColorBuffer(colorBuffer));
    mFb->closeColorBuffer(colorBuffer);
}
#endif // !_WIN32
} // namespace emugl
