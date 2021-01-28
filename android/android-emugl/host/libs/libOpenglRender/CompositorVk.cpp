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

#include "vulkan/vk_util.h"

std::unique_ptr<CompositorVk> CompositorVk::create(
    const goldfish_vk::VulkanDispatch &vk, VkDevice vkDevice) {
    return std::unique_ptr<CompositorVk>(new CompositorVk(vk, vkDevice));
}

CompositorVk::CompositorVk(const goldfish_vk::VulkanDispatch &vk,
                           VkDevice vkDevice)
    : m_vk(vk), m_vkDevice(vkDevice) {}

bool CompositorVk::validatePhysicalDeviceFeatures(
    const VkPhysicalDeviceFeatures2 &features) {
    auto descIndexingFeatures =
        vk_find_struct<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            &features);
    if (descIndexingFeatures == nullptr) {
        return false;
    }
    return descIndexingFeatures->descriptorBindingSampledImageUpdateAfterBind ==
           VK_TRUE;
}

bool CompositorVk::validateQueueFamilyProperties(
    const VkQueueFamilyProperties &properties) {
    return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool CompositorVk::enablePhysicalDeviceFeatures(
    VkPhysicalDeviceFeatures2 &features) {
    auto descIndexingFeatures =
        vk_find_struct<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            &features);
    if (descIndexingFeatures == nullptr) {
        return false;
    }
    descIndexingFeatures->descriptorBindingSampledImageUpdateAfterBind =
        VK_TRUE;
    return true;
}
std::vector<const char *> CompositorVk::getRequiredDeviceExtensions() {
    return {VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
}