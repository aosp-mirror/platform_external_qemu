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
#pragma once

#include <memory>
#include <vector>

#include "vulkan/cereal/common/goldfish_vk_dispatch.h"

class CompositorVk {
   public:
    static std::unique_ptr<CompositorVk> create(
        const goldfish_vk::VulkanDispatch &vk, VkDevice);
    static bool validatePhysicalDeviceFeatures(
        const VkPhysicalDeviceFeatures2 &features);
    static bool validateQueueFamilyProperties(
        const VkQueueFamilyProperties &properties);
    static bool enablePhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures2 &features);
    static std::vector<const char *> getRequiredDeviceExtensions();

   private:
    explicit CompositorVk(const goldfish_vk::VulkanDispatch &, VkDevice);

    const goldfish_vk::VulkanDispatch &m_vk;
    const VkDevice m_vkDevice;
};
