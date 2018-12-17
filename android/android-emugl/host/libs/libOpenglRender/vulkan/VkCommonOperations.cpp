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
#include "VkCommonOperations.h"

#include "android/base/containers/StaticMap.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Log.h"

#include "VulkanDispatch.h"

#include <stdio.h>

#include <iomanip>
#include <ostream>
#include <sstream>

using android::base::LazyInstance;
using android::base::StaticMap;

namespace goldfish_vk {

static LazyInstance<StaticMap<VkDevice, uint32_t>>
sKnownStagingTypeIndices = LAZY_INSTANCE_INIT;

bool getStagingMemoryTypeIndex(
    VulkanDispatch* vk,
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties* memProps,
    uint32_t* typeIndex) {

    auto res = sKnownStagingTypeIndices->get(device);

    if (res) {
        *typeIndex = *res;
        return true;
    }

    VkBufferCreateInfo testCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
        4096,
        // To be a staging buffer, it must support being
        // both a transfer src and dst.
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // TODO: See if buffers over shared queues need to be
        // considered separately
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
    };

    VkBuffer testBuffer;
    VkResult testBufferCreateRes =
        vk->vkCreateBuffer(device, &testCreateInfo, nullptr, &testBuffer);

    if (testBufferCreateRes != VK_SUCCESS) {
        LOG(ERROR) <<
            "Could not create test buffer "
            "for staging buffer query. VkResult: " <<
                testBufferCreateRes;
        return false;
    }

    VkMemoryRequirements memReqs;
    vk->vkGetBufferMemoryRequirements(device, testBuffer, &memReqs);

    // To be a staging buffer, we need to allow CPU read/write access.
    // Thus, we need the memory type index both to be host visible
    // and to be supported in the memory requirements of the buffer.
    bool foundSuitableStagingMemoryType = false;
    uint32_t stagingMemoryTypeIndex = 0;

    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        const auto& typeInfo = memProps->memoryTypes[i];
        bool hostVisible =
            typeInfo.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bool allowedInBuffer = (1 << i) & memReqs.memoryTypeBits;
        if (hostVisible && allowedInBuffer) {
            foundSuitableStagingMemoryType = true;
            stagingMemoryTypeIndex = i;
            break;
        }
    }

    vk->vkDestroyBuffer(device, testBuffer, nullptr);

    if (!foundSuitableStagingMemoryType) {
        std::stringstream ss;
        ss <<
            "Could not find suitable memory type index " <<
            "for staging buffer. Memory type bits: " <<
            std::hex << memReqs.memoryTypeBits << "\n" <<
            "Available host visible memory type indices:" << "\n";
        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
            if (memProps->memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                ss << "Host visible memory type index: %u" << i << "\n";
            }
        }

        LOG(ERROR) << ss.str();

        return false;
    }

    sKnownStagingTypeIndices->set(device, stagingMemoryTypeIndex);
    *typeIndex = stagingMemoryTypeIndex;

    return true;
}

} // namespace goldfish_vk