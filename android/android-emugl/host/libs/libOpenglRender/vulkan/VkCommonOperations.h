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

#include <unordered_map>

namespace goldfish_vk {

class VulkanDispatch;

// Returns a consistent answer for which memory type index is best for staging
// memory. This is not the simplest thing in the world because even if a memory
// type index is host visible, that doesn't mean a VkBuffer is allowed to be
// associated with it.
bool getStagingMemoryTypeIndex(
    VulkanDispatch* vk,
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties* memProps,
    uint32_t* typeIndex);

#ifdef _WIN32
typedef void* HANDLE;
#endif

// External memory objects are HANDLE on Windows and fd's on POSIX systems.
#ifdef _WIN32
typedef HANDLE VK_EXT_MEMORY_HANDLE;
// corresponds to INVALID_HANDLE_VALUE
#define VK_EXT_MEMORY_HANDLE_INVALID (VK_EXT_MEMORY_HANDLE)(uintptr_t)(-1);
#else
typedef int VK_EXT_MEMORY_HANDLE;
#define VK_EXT_MEMORY_HANDLE_INVALID (-1)
#endif

// Global state that holds a global Vulkan instance along with globally
// exported memory allocations + images. This is in order to service things
// like AndroidHardwareBuffer/FuchsiaImagePipeHandle. Each such allocation is
// associated with a ColorBuffer handle, and depending on host-side support for
// GL_EXT_memory_object, also be able to zero-copy render into and readback
// with the traditional GL pipeline.
struct VkEmulation {
    // Instance and device for creating the system-wide shareable objects.
    VkInstance instance;
    VkPhysicalDevice physdev;
    VkDevice device;

    // TODO: Use OpenGL interop features where it makes sense
    bool glInteropSupported = false;

    struct ExternalMemoryInfo {
        uint32_t id;
        uint32_t typeIndex;
        VkDeviceSize size;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mappedPtr = nullptr;
        VK_EXT_MEMORY_HANDLE exportedHandle =
            VK_EXT_MEMORY_HANDLE_INVALID;
    };

    struct StagingBufferInfo {
        ExternalMemoryInfo memory;
        VkBuffer buffer = VK_NULL_HANDLE;
    };

    struct ColorBufferInfo {
        ExternalMemoryInfo memory;

        uint32_t handle;

        int frameworkFormat;
        int frameworkStride;

        VkExtent3D extent;

        VkFormat format;
        VkImageUsageFlags usage;
        VkSharingMode sharingMode;
        VkMemoryRequirements memReqs;
    };

    // A single staging buffer to perform most transfers
    // to/from OpenGL on the host.
    StagingBufferInfo staging;

    // ColorBuffers are intended to back the guest's shareable images.
    // For example:
    // Android: gralloc
    // Fuchsia: ImagePipeHandle
    // Linux: dmabuf
    std::unordered_map<uint32_t, ColorBufferInfo> guestColorBuffers;

    // In order to support VK_KHR_external_memory_(fd|win32) we need also to
    // support the concept of plain external memories that are just memory and
    // not necessarily images. These are then intended to pass through to the
    // guest in some way, with 1:1 mapping between guest and host external
    // memory handles.
    std::unordered_map<uint32_t, ExternalMemoryInfo> guestExternalMemories;

    // We can also consider using a single external memory object to back all
    // host visible allocations in the guest. This would save memory, but we
    // would also need to automatically add
    // VkExternalMemory(Image|Buffer)CreateInfo, or if it is already there, OR
    // it with the handle types on the host. TODO: try switching to this
    ExternalMemoryInfo virtualHostVisibleHeap;
};

VkEmulation* createOrGetGlobalVkEmulation(VulkanDispatch* vk);

} // namespace goldfish_vk
