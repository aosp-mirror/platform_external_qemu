// Copyright (C) 2018 The Android Open Source Project
// Copyright (C) 2018 Google Inc.
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

#include <vulkan/vulkan.h>

#ifdef __cplusplus
#include <algorithm>
extern "C" {
#endif

#define VK_ANDROID_native_buffer 1
#define VK_ANDROID_NATIVE_BUFFER_EXTENSION_NUMBER 11

/* NOTE ON VK_ANDROID_NATIVE_BUFFER_SPEC_VERSION 6
 *
 * This version of the extension transitions from gralloc0 to gralloc1 usage
 * flags (int -> 2x uint64_t). The WSI implementation will temporarily continue
 * to fill out deprecated fields in VkNativeBufferANDROID, and will call the
 * deprecated vkGetSwapchainGrallocUsageANDROID if the new
 * vkGetSwapchainGrallocUsage2ANDROID is not supported. This transitionary
 * backwards-compatibility support is temporary, and will likely be removed in
 * (along with all gralloc0 support) in a future release.
 */
#define VK_ANDROID_NATIVE_BUFFER_SPEC_VERSION     7
#define VK_ANDROID_NATIVE_BUFFER_EXTENSION_NAME   "VK_ANDROID_native_buffer"

#define VK_ANDROID_NATIVE_BUFFER_ENUM(type,id)    ((type)(1000000000 + (1000 * (VK_ANDROID_NATIVE_BUFFER_EXTENSION_NUMBER - 1)) + (id)))
#define VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID   VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_SWAPCHAIN_IMAGE_CREATE_INFO_ANDROID VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 1)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID VK_ANDROID_NATIVE_BUFFER_ENUM(VkStructureType, 2)

typedef enum VkSwapchainImageUsageFlagBitsANDROID {
    VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID = 0x00000001,
    VK_SWAPCHAIN_IMAGE_USAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VkSwapchainImageUsageFlagBitsANDROID;
typedef VkFlags VkSwapchainImageUsageFlagsANDROID;

typedef struct {
    VkStructureType             sType; // must be VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID
    const void*                 pNext;

    // Buffer handle and stride returned from gralloc alloc()
    const uint32_t*             handle;
    int                         stride;

    // Gralloc format and usage requested when the buffer was allocated.
    int                         format;
    int                         usage; // DEPRECATED in SPEC_VERSION 6
    // -- Added in SPEC_VERSION 6 --
    uint64_t                consumer;
    uint64_t                producer;
} VkNativeBufferANDROID;

typedef struct {
    VkStructureType                        sType; // must be VK_STRUCTURE_TYPE_SWAPCHAIN_IMAGE_CREATE_INFO_ANDROID
    const void*                            pNext;

    VkSwapchainImageUsageFlagsANDROID      usage;
} VkSwapchainImageCreateInfoANDROID;

typedef struct {
    VkStructureType                        sType; // must be VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID
    const void*                            pNext;

    VkBool32                               sharedImage;
} VkPhysicalDevicePresentationPropertiesANDROID;

// -- DEPRECATED in SPEC_VERSION 6 --
typedef VkResult (VKAPI_PTR *PFN_vkGetSwapchainGrallocUsageANDROID)(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, int* grallocUsage);
// -- ADDED in SPEC_VERSION 6 --
typedef VkResult (VKAPI_PTR *PFN_vkGetSwapchainGrallocUsage2ANDROID)(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, VkSwapchainImageUsageFlagsANDROID swapchainImageUsage, uint64_t* grallocConsumerUsage, uint64_t* grallocProducerUsage);
typedef VkResult (VKAPI_PTR *PFN_vkAcquireImageANDROID)(VkDevice device, VkImage image, int nativeFenceFd, VkSemaphore semaphore, VkFence fence);
typedef VkResult (VKAPI_PTR *PFN_vkQueueSignalReleaseImageANDROID)(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore* pWaitSemaphores, VkImage image, int* pNativeFenceFd);

#define VK_GOOGLE_address_space 1

typedef VkResult (VKAPI_PTR *PFN_vkMapMemoryIntoAddressSpaceGOOGLE)(VkDevice device, VkDeviceMemory memory, uint64_t* pAddress);

#define VK_GOOGLE_color_buffer 1
#define VK_GOOGLE_COLOR_BUFFER_EXTENSION_NUMBER 219

#define VK_GOOGLE_COLOR_BUFFER_ENUM(type,id)    ((type)(1000000000 + (1000 * (VK_GOOGLE_COLOR_BUFFER_EXTENSION_NUMBER - 1)) + (id)))
#define VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE   VK_GOOGLE_COLOR_BUFFER_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_IMPORT_PHYSICAL_ADDRESS_GOOGLE   VK_GOOGLE_COLOR_BUFFER_ENUM(VkStructureType, 1)
#define VK_STRUCTURE_TYPE_IMPORT_BUFFER_GOOGLE   VK_GOOGLE_COLOR_BUFFER_ENUM(VkStructureType, 2)

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE
    const void* pNext;
    uint32_t colorBuffer;
} VkImportColorBufferGOOGLE;

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_PHYSICAL_ADDRESS_GOOGLE
    const void* pNext;
    uint64_t physicalAddress;
    VkDeviceSize size;
    VkFormat format;
    VkImageTiling tiling;
    uint32_t tilingParameter;
} VkImportPhysicalAddressGOOGLE;

typedef struct {
    VkStructureType sType; // must be VK_STRUCTURE_TYPE_IMPORT_BUFFER_GOOGLE
    const void* pNext;
    uint32_t buffer;
} VkImportBufferGOOGLE;

typedef VkResult (VKAPI_PTR *PFN_vkRegisterImageColorBufferGOOGLE)(VkDevice device, VkImage image, uint32_t colorBuffer);
typedef VkResult (VKAPI_PTR *PFN_vkRegisterBufferColorBufferGOOGLE)(VkDevice device, VkBuffer image, uint32_t colorBuffer);

#define VK_GOOGLE_sized_descriptor_update_template 1

typedef void (VKAPI_PTR *PFN_vkUpdateDescriptorSetWithTemplateSizedGOOGLE)(
    VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    uint32_t imageInfoCount,
    uint32_t bufferInfoCount,
    uint32_t bufferViewCount,
    const uint32_t* pImageInfoEntryIndices,
    const uint32_t* pBufferInfoEntryIndices,
    const uint32_t* pBufferViewEntryIndices,
    const VkDescriptorImageInfo* pImageInfos,
    const VkDescriptorBufferInfo* pBufferInfos,
    const VkBufferView* pBufferViews);

#define VK_GOOGLE_async_command_buffers 1

typedef void (VKAPI_PTR *PFN_vkBeginCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo);
typedef void (VKAPI_PTR *PFN_vkEndCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer);
typedef void (VKAPI_PTR *PFN_vkResetCommandBufferAsyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags);
typedef void (VKAPI_PTR *PFN_vkCommandBufferHostSyncGOOGLE)(
    VkCommandBuffer commandBuffer,
    uint32_t needHostSync,
    uint32_t sequenceNumber);

#define VK_GOOGLE_create_resources_with_requirements 1

typedef void (VKAPI_PTR *PFN_vkCreateImageWithRequirementsGOOGLE)(
    VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage, VkMemoryRequirements* pMemoryRequirements);
typedef void (VKAPI_PTR *PFN_vkCreateBufferWithRequirementsGOOGLE)(
    VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer, VkMemoryRequirements* pMemoryRequirements);

#define VK_GOOGLE_address_space_info 1

typedef VkResult (VKAPI_PTR *PFN_vkGetMemoryHostAddressInfoGOOGLE)(VkDevice device, VkDeviceMemory memory, uint64_t* pAddress, uint64_t* pSize);

#define VK_GOOGLE_free_memory_sync 1

typedef VkResult (VKAPI_PTR *PFN_vkFreeMemorySyncGOOGLE)(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator);

#ifndef VK_FUCHSIA_external_memory
#define VK_FUCHSIA_external_memory 1
#define VK_FUCHSIA_EXTERNAL_MEMORY_SPEC_VERSION 1
#define VK_FUCHSIA_EXTERNAL_MEMORY_EXTENSION_NAME "VK_FUCHSIA_external_memory"

typedef struct VkImportMemoryZirconHandleInfoFUCHSIA {
    VkStructureType                       sType;
    const void*                           pNext;
    VkExternalMemoryHandleTypeFlagBits    handleType;
    uint32_t                              handle;
} VkImportMemoryZirconHandleInfoFUCHSIA;

typedef struct VkMemoryZirconHandlePropertiesFUCHSIA {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           memoryTypeBits;
} VkMemoryZirconHandlePropertiesFUCHSIA;

typedef struct VkMemoryGetZirconHandleInfoFUCHSIA {
    VkStructureType                       sType;
    const void*                           pNext;
    VkDeviceMemory                        memory;
    VkExternalMemoryHandleTypeFlagBits    handleType;
} VkMemoryGetZirconHandleInfoFUCHSIA;

#define VK_STRUCTURE_TYPE_TEMP_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA \
    ((VkStructureType)1001005000)
#define VK_STRUCTURE_TYPE_TEMP_MEMORY_ZIRCON_HANDLE_PROPERTIES_FUCHSIA \
    ((VkStructureType)1001005001)
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_TEMP_ZIRCON_VMO_BIT_FUCHSIA \
    ((VkExternalMemoryHandleTypeFlagBits)0x00100000)
#endif  // VK_FUCHSIA_external_memory

#ifndef VK_MVK_moltenvk
#define VK_MVK_moltenvk 1
#define VK_MVK_MOLTENVK_SPEC_VERSION            3
#define VK_MVK_MOLTENVK_EXTENSION_NAME          "VK_MVK_moltenvk"

typedef struct __IOSurface *IOSurfaceRef;

typedef VkResult (VKAPI_PTR *PFN_vkUseIOSurfaceMVK)(VkImage image, IOSurfaceRef ioSurface);
typedef void (VKAPI_PTR *PFN_vkGetIOSurfaceMVK)(VkImage image, IOSurfaceRef* pIOSurface);

#endif  // VK_MVK_moltenvk

// VulkanStream features
#define VULKAN_STREAM_FEATURE_NULL_OPTIONAL_STRINGS_BIT (1 << 0)
#define VULKAN_STREAM_FEATURE_IGNORED_HANDLES_BIT (1 << 1)
#define VULKAN_STREAM_FEATURE_SHADER_FLOAT16_INT8_BIT (1 << 2)

// Stuff we advertised but didn't define the structs for it yet because
// we also needed to update our vulkan headers and xml
typedef struct VkPhysicalDeviceShaderFloat16Int8Features {
    VkStructureType    sType;
    void*              pNext;
    VkBool32           shaderFloat16;
    VkBool32           shaderInt8;
} VkPhysicalDeviceShaderFloat16Int8Features;

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES \
    ((VkStructureType)1000082000)

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR \
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES

#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR \
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES

#define VK_KHR_shader_float16_int8 1
#define VK_KHR_SHADER_FLOAT16_INT8_SPEC_VERSION 1
#define VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME "VK_KHR_shader_float16_int8"
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceShaderFloat16Int8FeaturesKHR;
typedef VkPhysicalDeviceShaderFloat16Int8Features VkPhysicalDeviceFloat16Int8FeaturesKHR;

#define VK_GOOGLE_async_queue_submit 1

typedef void (VKAPI_PTR *PFN_vkQueueHostSyncGOOGLE)(
    VkQueue queue, uint32_t needHostSync, uint32_t sequenceNumber);
typedef void (VKAPI_PTR *PFN_vkQueueSubmitAsyncGOOGLE)(
    VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);
typedef void (VKAPI_PTR *PFN_vkQueueWaitIdleAsyncGOOGLE)(VkQueue queue);
typedef void (VKAPI_PTR *PFN_vkQueueBindSparseAsyncGOOGLE)(
    VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

template<class T, typename F>
bool arrayany(const T* arr, uint32_t begin, uint32_t end, const F& func) {
    const T* e = arr + end;
    return std::find_if(arr + begin, e, func) != e;
}

#endif
