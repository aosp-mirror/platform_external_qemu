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

#ifdef _WIN32
typedef void* HANDLE;
#endif

namespace goldfish_vk {

class VulkanDispatch;

// This class provides methods to create and query information about Android
// native buffers in the context of creating Android swapchain images that have
// Android native buffer backing.

// External memory objects are HANDLE on Windows and fd's on POSIX systems.
#ifdef _WIN32
typedef HANDLE VK_ANB_EXT_MEMORY_HANDLE;
// corresponds to INVALID_HANDLE_VALUE
#define VK_ANB_EXT_MEMORY_HANDLE_INVALID (VK_ANB_EXT_MEMORY_HANDLE)(uintptr_t)(-1);
#else
typedef int VK_ANB_EXT_MEMORY_HANDLE;
#define VK_ANB_EXT_MEMORY_HANDLE_INVALID (-1)
#endif

struct AndroidNativeBufferInfo {
    VkDevice device = VK_NULL_HANDLE;
    VkFormat vkFormat;
    VkExtent3D extent;
    VkImageUsageFlags usage;
    VkSharingMode sharingMode;
    std::vector<uint32_t> queueFamilyIndices;

    int format;
    int stride;
    uint32_t colorBufferHandle;

    // To be populatd later as we go.
    VkImage image = VK_NULL_HANDLE;
    VkMemoryRequirements memReqs;

    // We will be using separate allocations for image versus staging memory,
    // because not all host Vulkan drivers will support directly rendering to
    // host visible memory in a layout that glTexSubImage2D can consume.
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;

    uint32_t imageMemoryTypeIndex;
    uint32_t stagingMemoryTypeIndex;

    uint8_t* mappedStagingPtr = nullptr;

    // The queue over which we send the buffer/image copy commands depends on
    // the queue over which vkQueueSignalReleaseImageANDROID happens.
    // It is assumed that the VkImage object has been created by Android swapchain layer
    // with all the relevant queue family indices for sharing set properly.
    struct QueueState {
        VkQueue queue = VK_NULL_HANDLE;
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandBuffer cb = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
    };
    // We keep one QueueState for each queue family index used by the guest
    // in vkQueuePresentKHR.
    std::vector<QueueState> queueStates;
    // Did we ever sync the Vulkan image with a ColorBuffer?
    // If so, set everSynced along with the queue family index
    // used to do that.
    // If the swapchain image was created with exclusive sharing
    // mode (reflected in this struct's |sharingMode| field),
    // this part doesn't really matter.
    bool everSynced = false;
    uint32_t lastUsedQueueFamilyIndex;

    // TODO: Use external memory features when available.
    bool externalMemorySupported = false;
    VK_ANB_EXT_MEMORY_HANDLE externalMemory =
        VK_ANB_EXT_MEMORY_HANDLE_INVALID;
};

bool parseAndroidNativeBufferInfo(
    const VkImageCreateInfo* pCreateInfo,
    AndroidNativeBufferInfo* info_out);

VkResult prepareAndroidNativeBufferImage(
    VulkanDispatch* vk,
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    const VkPhysicalDeviceMemoryProperties* memProps,
    AndroidNativeBufferInfo* out);

void teardownAndroidNativeBufferImage(
    VulkanDispatch* vk,
    AndroidNativeBufferInfo* anbInfo);

void getGralloc0Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      int* usage_out);
void getGralloc1Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
                      uint64_t* consumerUsage_out,
                      uint64_t* producerUsage_out);

VkResult syncImageToColorBuffer(
    VulkanDispatch* vk,
    uint32_t queueFamilyIndex,
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    int* pNativeFenceFd,
    AndroidNativeBufferInfo* anbInfo);

} // namespace goldfish_vk
