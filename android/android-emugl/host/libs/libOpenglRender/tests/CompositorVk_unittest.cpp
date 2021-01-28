// Copyright (C) 2021 The Android Open Source Project
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
#include "CompositorVk.h"

#include <gtest/gtest.h>

#include <optional>

#include "vulkan/VulkanDispatch.h"

class CompositorVkTest : public ::testing::Test {
   protected:
    static void SetUpTestCase() {
        k_vk = emugl::vkDispatch(false);
    }

    void SetUp() override {
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void TearDown() override {
        k_vk->vkDestroyDevice(m_vkDevice, nullptr);
        m_vkDevice = VK_NULL_HANDLE;
        k_vk->vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = VK_NULL_HANDLE;
    }

    static const goldfish_vk::VulkanDispatch *k_vk;
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t m_compositorQueueFamilyIndex = 0;
    VkDevice m_vkDevice = VK_NULL_HANDLE;

   private:
    void createInstance() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "emulator CompositorVk unittest";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo instanceCi = {};
        instanceCi.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCi.ppEnabledExtensionNames = nullptr;
        instanceCi.enabledExtensionCount = 0;
        instanceCi.pApplicationInfo = &appInfo;
        ASSERT_EQ(k_vk->vkCreateInstance(&instanceCi, nullptr, &m_vkInstance),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkInstance != VK_NULL_HANDLE);
    }

    void pickPhysicalDevice() {
        uint32_t physicalDeviceCount = 0;
        ASSERT_EQ(k_vk->vkEnumeratePhysicalDevices(
                      m_vkInstance, &physicalDeviceCount, nullptr),
                  VK_SUCCESS);
        ASSERT_GT(physicalDeviceCount, 0);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        ASSERT_EQ(
            k_vk->vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount,
                                             physicalDevices.data()),
            VK_SUCCESS);
        for (const auto &device : physicalDevices) {
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures =
                {};
            descIndexingFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &descIndexingFeatures;
            k_vk->vkGetPhysicalDeviceFeatures2(device, &features);
            if (!CompositorVk::validatePhysicalDeviceFeatures(features)) {
                continue;
            }
            uint32_t queueFamilyCount = 0;
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, nullptr);
            ASSERT_GT(queueFamilyCount, 0);
            std::vector<VkQueueFamilyProperties> queueFamilyProperties(
                queueFamilyCount);
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, queueFamilyProperties.data());
            uint32_t queueFamilyIndex = 0;
            for (; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
                if (CompositorVk::validateQueueFamilyProperties(
                        queueFamilyProperties[queueFamilyIndex])) {
                    break;
                }
            }
            if (queueFamilyIndex == queueFamilyCount) {
                continue;
            }

            m_compositorQueueFamilyIndex = queueFamilyIndex;
            m_vkPhysicalDevice = device;
            return;
        }
        FAIL() << "Can't find a suitable VkPhysicalDevice.";
    }

    void createLogicalDevice() {
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCi = {};
        queueCi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCi.queueFamilyIndex = m_compositorQueueFamilyIndex;
        queueCi.queueCount = 1;
        queueCi.pQueuePriorities = &queuePriority;
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
        descIndexingFeatures.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 features = {};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &descIndexingFeatures;
        ASSERT_TRUE(CompositorVk::enablePhysicalDeviceFeatures(features));
        const std::vector<const char *> enabledDeviceExtensions =
            CompositorVk::getRequiredDeviceExtensions();
        VkDeviceCreateInfo deviceCi = {};
        deviceCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCi.pQueueCreateInfos = &queueCi;
        deviceCi.queueCreateInfoCount = 1;
        deviceCi.pEnabledFeatures = nullptr;
        deviceCi.enabledLayerCount = 0;
        deviceCi.enabledExtensionCount =
            static_cast<uint32_t>(enabledDeviceExtensions.size());
        deviceCi.ppEnabledExtensionNames = enabledDeviceExtensions.data();
        deviceCi.pNext = &features;
        ASSERT_EQ(k_vk->vkCreateDevice(m_vkPhysicalDevice, &deviceCi, nullptr,
                                       &m_vkDevice),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkDevice != VK_NULL_HANDLE);
    }
};

const goldfish_vk::VulkanDispatch *CompositorVkTest::k_vk = nullptr;

TEST_F(CompositorVkTest, Init) {
    ASSERT_NE(k_vk, nullptr);

    ASSERT_NE(CompositorVk::create(*k_vk, m_vkDevice), nullptr);
}

TEST_F(CompositorVkTest, ValidatePhysicalDeviceFeatures) {
    ASSERT_NE(k_vk, nullptr);

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    ASSERT_FALSE(CompositorVk::validatePhysicalDeviceFeatures(features));
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
    descIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    features.pNext = &descIndexingFeatures;
    ASSERT_FALSE(CompositorVk::validatePhysicalDeviceFeatures(features));
    descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    ASSERT_TRUE(CompositorVk::validatePhysicalDeviceFeatures(features));
}

TEST_F(CompositorVkTest, ValidateQueueFamilyProperties) {
    ASSERT_NE(k_vk, nullptr);

    VkQueueFamilyProperties properties = {};
    properties.queueFlags &= ~VK_QUEUE_GRAPHICS_BIT;
    ASSERT_FALSE(CompositorVk::validateQueueFamilyProperties(properties));
    properties.queueFlags |= VK_QUEUE_GRAPHICS_BIT;
    ASSERT_TRUE(CompositorVk::validateQueueFamilyProperties(properties));
}

TEST_F(CompositorVkTest, EnablePhysicalDeviceFeatures) {
    ASSERT_NE(k_vk, nullptr);

    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    ASSERT_FALSE(CompositorVk::enablePhysicalDeviceFeatures(features));
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
    descIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    features.pNext = &descIndexingFeatures;
    ASSERT_TRUE(CompositorVk::enablePhysicalDeviceFeatures(features));
    ASSERT_EQ(descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind,
              VK_TRUE);
}