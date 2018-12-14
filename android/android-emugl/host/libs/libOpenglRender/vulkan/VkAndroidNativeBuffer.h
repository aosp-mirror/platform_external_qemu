// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <vulkan/vulkan.h>

#include "cereal/common/goldfish_vk_private_defs.h"

#include <vector>

namespace goldfish_vk {

// This class provides methods to create and query information about Android
// native buffers in the context of creating Android swapchain images that have
// Android native buffer backing.

struct AndroidNativeBufferInfo {
    VkFormat vkFormat;
    VkExtent3D extent;
    VkImageUsageFlags usage;
    std::vector<uint32_t> queueFamilyIndices;

    int format;
    int stride;
    uint32_t colorBufferHandle;
};

bool parseAndroidNativeBufferInfo(
    const VkImageCreateInfo* pCreateInfo,
    AndroidNativeBufferInfo* info_out);

void getGralloc0Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      int* usage_out);
void getGralloc1Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
                      uint64_t* consumerUsage_out,
                      uint64_t* producerUsage_out);

} // namespace goldfish_vk