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

#include "VkDecoderComponent.h"

#include <vulkan/vulkan.h>

namespace vk_emu {

struct DeviceMemoryInfo {
    // When ptr is null, it means the VkDeviceMemory object
    // was not allocated with the HOST_VISIBLE property.
    void* ptr = nullptr;
    VkDeviceSize size;
};

class DeviceMemoryComponent :
    public VkDecoderComponent<VkDevice, DeviceMemoryInfo> {
public:
    DeviceMemoryComponent() = default;

    VkResult on_vkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory);

    void on_vkFreeMemory(VkDevice device,
                         VkDeviceMemory memory,
                         const VkAllocationCallbacks* pAllocator);
    
    VkResult on_vkMapMemory(VkDevice device,
                            VkDeviceMemory memory,
                            VkDeviceSize offset,
                            VkDeviceSize size,
                            VkMemoryMapFlags flags,
                            void** ppData);

    void on_vkUnmapMemory(VkDevice device, VkDeviceMemory memory);

    uint8_t* getMappedHostPointer(VkDeviceMemory memory);
    VkDeviceSize getDeviceMemorySize(VkDeviceMemory memory);
private:
};

} // namespace vk_emu