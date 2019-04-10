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
#include <vector>

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
#define VK_EXT_MEMORY_HANDLE_INVALID (VK_EXT_MEMORY_HANDLE)(uintptr_t)(-1)
#define VK_EXT_MEMORY_HANDLE_TYPE_BIT VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
#else
typedef int VK_EXT_MEMORY_HANDLE;
#define VK_EXT_MEMORY_HANDLE_INVALID (-1)
#define VK_EXT_MEMORY_HANDLE_TYPE_BIT VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
#endif

VK_EXT_MEMORY_HANDLE dupExternalMemory(VK_EXT_MEMORY_HANDLE);

// Global state that holds a global Vulkan instance along with globally
// exported memory allocations + images. This is in order to service things
// like AndroidHardwareBuffer/FuchsiaImagePipeHandle. Each such allocation is
// associated with a ColorBuffer handle, and depending on host-side support for
// GL_EXT_memory_object, also be able to zero-copy render into and readback
// with the traditional GL pipeline.
struct VkEmulation {
    // Whether initialization succeeded.
    bool live = false;

    // Whether to use deferred command submission.
    bool useDeferredCommands = false;

    // Instance and device for creating the system-wide shareable objects.
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physdev = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    // Global, instance and device dispatch tables.
    VulkanDispatch* gvk = nullptr;
    VulkanDispatch* ivk = nullptr;
    VulkanDispatch* dvk = nullptr;

    bool instanceSupportsExternalMemoryCapabilities = false;
    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR
            getImageFormatProperties2Func = nullptr;

    // Queue, command pool, and command buffer
    // for running commands to sync stuff system-wide.
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence commandBufferFence = VK_NULL_HANDLE;

    struct ImageSupportInfo {
        // Input parameters
        VkFormat format;
        VkImageType type;
        VkImageTiling tiling;
        VkImageUsageFlags usageFlags;
        VkImageCreateFlags createFlags;

        // Output parameters
        bool supported = false;
        bool supportsExternalMemory = false;
        bool requiresDedicatedAllocation = false;

        // Keep the raw output around.
        VkFormatProperties2 formatProps2;
        VkImageFormatProperties2 imageFormatProps2;
        VkExternalImageFormatProperties extFormatProps;

        // Populated later when device is available.
        uint32_t memoryTypeBits = 0;
        bool memoryTypeBitsKnown = false;
    };

    std::vector<ImageSupportInfo> imageSupportInfo;

    struct DeviceSupportInfo {
        bool hasGraphicsQueueFamily = false;
        bool hasComputeQueueFamily = false;
        bool supportsExternalMemory = false;
        bool glInteropSupported = false;

        std::vector<uint32_t> graphicsQueueFamilyIndices;
        std::vector<uint32_t> computeQueueFamilyIndices;

        VkPhysicalDeviceProperties physdevProps;
        VkPhysicalDeviceMemoryProperties memProps;

        PFN_vkGetImageMemoryRequirements2KHR getImageMemoryRequirements2Func = nullptr;
        PFN_vkGetBufferMemoryRequirements2KHR getBufferMemoryRequirements2Func = nullptr;

#ifdef _WIN32
        PFN_vkGetMemoryWin32HandleKHR getMemoryHandleFunc = nullptr;
#else
        PFN_vkGetMemoryFdKHR getMemoryHandleFunc = nullptr;
#endif
    };

    struct ExternalMemoryInfo {
        // Input fields
        VkDeviceSize size;
        uint32_t typeIndex;

        // Output fields
        uint32_t id = 0;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mappedPtr = nullptr;
        VK_EXT_MEMORY_HANDLE exportedHandle =
            VK_EXT_MEMORY_HANDLE_INVALID;
        bool actuallyExternal = false;
    };

    // 128 mb staging buffer (really, just a few 4K frames or one 4k HDR frame)
    // ought to be big enough for anybody!
    static constexpr VkDeviceSize kDefaultStagingBufferSize =
        128ULL * 1048576ULL;

    struct StagingBufferInfo {
        // TODO: Don't actually use this as external memory until host visible
        // external is supported on all platforms
        ExternalMemoryInfo memory;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize size = kDefaultStagingBufferSize;
    };

    struct ColorBufferInfo {
        enum class VulkanMode {
            // Default: ColorBuffers can still be used with the existing GL-based
            // API.  Synchronization with (if it exists) Vulkan images happens on
            // every one of the GL-based API calls:
            //
            // rcReadColorBuffer
            // rcUpdateColorBuffer
            // rcBindTexture
            // rcBindRenderbuffer
            // rcFlushWindowColorBuffer
            //
            // either through explicit CPU copies or implicit in the host driver
            // if OpenGL interop is supported.
            //
            // When images are posted (rcFBPost),
            // eglSwapBuffers is used, even if that requires a CPU readback.

            Default = 0,

            // VulkanOnly: It is assumed that the guest interacts entirely with
            // the underlying Vulkan image in the guest and does not use the
            // GL-based API.  This means we can assume those APIs are not called:
            //
            // rcReadColorBuffer
            // rcUpdateColorBuffer
            // rcBindTexture
            // rcBindRenderbuffer
            // rcFlushWindowColorBuffer
            //
            // and thus we skip a lot of GL/Vk synchronization.
            //
            // When images are posted, eglSwapBuffers is only used if OpenGL
            // interop is supported. If OpenGL interop is not supported, then we
            // use a host platform-specific Vulkan swapchain to display the
            // results.

            VulkanOnly = 1,
        };

        ExternalMemoryInfo memory;

        uint32_t handle;

        int frameworkFormat;
        int frameworkStride;

        VkExtent3D extent;

        VkFormat format;
        VkImageType type;
        VkImageTiling tiling;
        VkImageUsageFlags usageFlags;
        VkImageCreateFlags createFlags;

        VkSharingMode sharingMode;

        VkImage image = VK_NULL_HANDLE;
        VkMemoryRequirements memReqs;

        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        bool glExported = false;

        VulkanMode vulkanMode = VulkanMode::Default;
    };

    // Track what is supported on whatever device was selected.
    DeviceSupportInfo deviceInfo;

    // A single staging buffer to perform most transfers to/from OpenGL on the
    // host. It is shareable across instances. The memory is shareable but the
    // buffer is not; other users need to create buffers that
    // bind to imported versions of the memory.
    StagingBufferInfo staging;

    // ColorBuffers are intended to back the guest's shareable images.
    // For example:
    // Android: gralloc
    // Fuchsia: ImagePipeHandle
    // Linux: dmabuf
    std::unordered_map<uint32_t, ColorBufferInfo> colorBuffers;

    // In order to support VK_KHR_external_memory_(fd|win32) we need also to
    // support the concept of plain external memories that are just memory and
    // not necessarily images. These are then intended to pass through to the
    // guest in some way, with 1:1 mapping between guest and host external
    // memory handles.
    std::unordered_map<uint32_t, ExternalMemoryInfo> externalMemories;

    // We can also consider using a single external memory object to back all
    // host visible allocations in the guest. This would save memory, but we
    // would also need to automatically add
    // VkExternalMemory(Image|Buffer)CreateInfo, or if it is already there, OR
    // it with the handle types on the host.
    // A rough sketch: Some memories/images/buffers in the guest
    // are backed by host visible memory:
    // There is already a virtual memory type for those things in the current
    // implementation. The guest doesn't know whether the pointer or the
    // VkDeviceMemory object is backed by host external or non external.
    // TODO: are all possible buffer / image usages compatible with
    // external backing?
    // TODO: try switching to this
    ExternalMemoryInfo virtualHostVisibleHeap;
};

VkEmulation* createOrGetGlobalVkEmulation(VulkanDispatch* vk);
void setUseDeferredCommands(VkEmulation* emu, bool useDeferred);

VkEmulation* getGlobalVkEmulation();
void teardownGlobalVkEmulation();

bool allocExternalMemory(VulkanDispatch* vk, VkEmulation::ExternalMemoryInfo* info, bool actuallyExternal = true);
void freeExternalMemory(VulkanDispatch* vk, VkEmulation::ExternalMemoryInfo* info);

bool importExternalMemory(VulkanDispatch* vk,
                          VkDevice targetDevice,
                          const VkEmulation::ExternalMemoryInfo* info,
                          VkDeviceMemory* out);
bool importExternalMemoryDedicatedImage(
    VulkanDispatch* vk,
    VkDevice targetDevice,
    const VkEmulation::ExternalMemoryInfo* info,
    VkImage image,
    VkDeviceMemory* out);

// ColorBuffer operations

bool isColorBufferVulkanCompatible(uint32_t colorBufferHandle);

bool setupVkColorBuffer(uint32_t colorBufferHandle, bool vulkanOnly = false, bool* exported = nullptr, VkDeviceSize* allocSize = nullptr);
bool teardownVkColorBuffer(uint32_t colorBufferHandle);
VkEmulation::ColorBufferInfo getColorBufferInfo(uint32_t colorBufferHandle);
bool updateColorBufferFromVkImage(uint32_t colorBufferHandle);
bool updateVkImageFromColorBuffer(uint32_t colorBufferHandle);
VK_EXT_MEMORY_HANDLE getColorBufferExtMemoryHandle(uint32_t colorBufferHandle);
bool setColorBufferVulkanMode(uint32_t colorBufferHandle, uint32_t vulkanMode);

VkExternalMemoryHandleTypeFlags
transformExternalMemoryHandleTypeFlags_tohost(
    VkExternalMemoryHandleTypeFlags bits);

VkExternalMemoryHandleTypeFlags
transformExternalMemoryHandleTypeFlags_fromhost(
    VkExternalMemoryHandleTypeFlags hostBits,
    VkExternalMemoryHandleTypeFlags wantedGuestHandleType);

VkExternalMemoryProperties
transformExternalMemoryProperties_tohost(
    VkExternalMemoryProperties props);

VkExternalMemoryProperties
transformExternalMemoryProperties_fromhost(
    VkExternalMemoryProperties props,
    VkExternalMemoryHandleTypeFlags wantedGuestHandleType);

} // namespace goldfish_vk
