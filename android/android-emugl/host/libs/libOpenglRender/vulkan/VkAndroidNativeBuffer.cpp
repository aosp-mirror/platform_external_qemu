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

} // namespace goldfish_vk