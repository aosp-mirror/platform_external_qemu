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
#include "VkAndroidNativeBuffer.h"

#include "cereal/common/goldfish_vk_private_defs.h"
#include "cereal/common/goldfish_vk_extension_structs.h"

#include "GrallocDefs.h"

namespace goldfish_vk {

bool parseAndroidNativeBufferInfo(
    const VkImageCreateInfo* pCreateInfo,
    AndroidNativeBufferInfo* info_out) {

    // Look through the extension chain.
    const void* curr_pNext = pCreateInfo->pNext;
    if (!curr_pNext) return false;

    uint32_t structType = goldfish_vk_struct_type(curr_pNext);

    if (structType != VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID)
        return false;

    const VkNativeBufferANDROID* nativeBufferANDROID =
        reinterpret_cast<const VkNativeBufferANDROID*>(curr_pNext);

    info_out->vkFormat = pCreateInfo->format;
    info_out->extent = pCreateInfo->extent;
    info_out->usage = pCreateInfo->usage;
    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; ++i) {
        info_out->queueFamilyIndices.push_back(
                pCreateInfo->pQueueFamilyIndices[i]);
    }

    info_out->format = nativeBufferANDROID->format;
    info_out->stride = nativeBufferANDROID->stride;
    info_out->colorBufferHandle = *(nativeBufferANDROID->handle);

    return true;
}

void getGralloc0Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      int* usage_out) {
    // Pick some default flexible values for gralloc usage for now.
    (void)format;
    (void)imageUsage;
    *usage_out =
        GRALLOC_USAGE_SW_READ_OFTEN |
        GRALLOC_USAGE_SW_WRITE_OFTEN |
        GRALLOC_USAGE_HW_RENDER |
        GRALLOC_USAGE_HW_TEXTURE;
}

// Taken from Android GrallocUsageConversion.h
void getGralloc1Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
                      uint64_t* consumerUsage_out,
                      uint64_t* producerUsage_out) {
    // Pick some default flexible values for gralloc usage for now.
    (void)format;
    (void)imageUsage;
    (void)swapchainImageUsage;

    constexpr int usage =
        GRALLOC_USAGE_SW_READ_OFTEN |
        GRALLOC_USAGE_SW_WRITE_OFTEN |
        GRALLOC_USAGE_HW_RENDER |
        GRALLOC_USAGE_HW_TEXTURE;

    constexpr uint64_t PRODUCER_MASK =
            GRALLOC1_PRODUCER_USAGE_CPU_READ |
            /* GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN | */
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE |
            /* GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | */
            GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET |
            GRALLOC1_PRODUCER_USAGE_PROTECTED |
            GRALLOC1_PRODUCER_USAGE_CAMERA |
            GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER |
            GRALLOC1_PRODUCER_USAGE_SENSOR_DIRECT_DATA;
    constexpr uint64_t CONSUMER_MASK =
            GRALLOC1_CONSUMER_USAGE_CPU_READ |
            /* GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN | */
            GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE |
            GRALLOC1_CONSUMER_USAGE_HWCOMPOSER |
            GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET |
            GRALLOC1_CONSUMER_USAGE_CURSOR |
            GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER |
            GRALLOC1_CONSUMER_USAGE_CAMERA |
            GRALLOC1_CONSUMER_USAGE_RENDERSCRIPT |
            GRALLOC1_CONSUMER_USAGE_GPU_DATA_BUFFER;

    *producerUsage_out = static_cast<uint64_t>(usage) & PRODUCER_MASK;
    *consumerUsage_out = static_cast<uint64_t>(usage) & CONSUMER_MASK;

    if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_READ_OFTEN) ==
        GRALLOC_USAGE_SW_READ_OFTEN) {
        *producerUsage_out |= GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN;
        *consumerUsage_out |= GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN;
    }

    if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_WRITE_OFTEN) ==
        GRALLOC_USAGE_SW_WRITE_OFTEN) {
        *producerUsage_out |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN;
    }
}

} // namespace goldfish_vk