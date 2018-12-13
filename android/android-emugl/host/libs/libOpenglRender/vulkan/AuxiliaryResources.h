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

#include <vector>

// In order to implement certain guest-platform-specific features on the host
// side, we need to be able to create resources on the host side for
// accomplishing certain tasks, such as allocating extra memory to back
// VK_ANDROID_native_buffer images.
// This file contains definitions that let us track these kinds of things.
namespace goldfish_vk {

// Often, we can end up hijacking a guest instance or creating a separate instance on
// the host side in order to support features such as guest platform buffers.
// We will need to create our own memory objects etc.
// AuxiliaryMemory/Device contain all the information necessary to make that happen.
struct AuxiliaryMemory {
    uint32_t typeIndex = 0;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* ptr = nullptr;

    // The staging buffer associated with |memory|,
    // if this will be used perform CPU/GPU transfers.
    VkBuffer stagingBuffer = VK_NULL_HANDLE;

    // External memory object?
#ifdef _WIN32
    HANDLE ext_mem_handle = 0;
    bool validExtMemHandle() const { return ext_mem_handle != 0; }
#else
    int ext_mem_handle = -1;
    bool validExtMemHandle() const { return ext_mem_handle >= 0; }
#endif
};

struct AuxiliaryDevice {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    uint32_t queueFamilyIndex = 0;
    uint32_t queueIndex = 0;

    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    // Helper functions for common operations on auxiliary devices.
    // Creates the command pool/command buffer/fence.
    static void setupFromExisting(
        VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue,
        AuxiliaryDevice* aux_out);
    // Undoes the extra setup in setupFromExisting; leaves the existing
    // objects alone.
    static void teardownFromExisting(AuxiliaryDevice* aux_);

    static void allocMappedMemory(
        const AuxiliaryDevice& device, uint32_t typeIndex, VkDeviceSize size,
        AuxiliaryMemory* aux_out);
    static void allocDeviceLocal(
        const AuxiliaryDevice& device, VkDeviceSize size,
        AuxiliaryMemory* aux_out);
    static AuxiliaryMemory teardownMemory(
        const AuxiliaryDevice& device, AuxiliaryMemory* mem);
    // TODO: static AuxiliaryDevice setup/teardown not based on
    // existing Vulkan objects
};

// Following are structs that are specific to particular extensions.

// VK_ANDROID_native_buffer
struct ColorBufferInfo {
    VkImage image;
    VkExtent2D dimensions;
    int format;
    uint32_t colorBuffer;
};

struct AndroidNativeBuffers {
    AuxiliaryDevice* auxDevice = nullptr;
    std::vector<AuxiliaryMemory> auxMemories;
    std::vector<ColorBufferInfo> colorBufferInfos;
};

} // namespace goldfish_vk