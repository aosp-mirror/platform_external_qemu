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
#include "VkDecoderGlobalState.h"

#include "FrameBuffer.h"
#include "VulkanDispatch.h"
#include "VkDecoderComponent.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "emugl/common/crash_reporter.h"

#include <algorithm>
#include <unordered_map>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

typedef enum {
    HAL_PIXEL_FORMAT_RGBA_8888 = 1,
    HAL_PIXEL_FORMAT_RGBX_8888 = 2,
    HAL_PIXEL_FORMAT_RGB_888 = 3,
    HAL_PIXEL_FORMAT_RGB_565 = 4,
    HAL_PIXEL_FORMAT_BGRA_8888 = 5,
    HAL_PIXEL_FORMAT_RGBA_1010102 = 43, // 0x2B
    HAL_PIXEL_FORMAT_RGBA_FP16 = 22, // 0x16
    HAL_PIXEL_FORMAT_YV12 = 842094169, // 0x32315659
    HAL_PIXEL_FORMAT_Y8 = 538982489, // 0x20203859
    HAL_PIXEL_FORMAT_Y16 = 540422489, // 0x20363159
    HAL_PIXEL_FORMAT_RAW16 = 32, // 0x20
    HAL_PIXEL_FORMAT_RAW10 = 37, // 0x25
    HAL_PIXEL_FORMAT_RAW12 = 38, // 0x26
    HAL_PIXEL_FORMAT_RAW_OPAQUE = 36, // 0x24
    HAL_PIXEL_FORMAT_BLOB = 33, // 0x21
    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 34, // 0x22
    HAL_PIXEL_FORMAT_YCBCR_420_888 = 35, // 0x23
    HAL_PIXEL_FORMAT_YCBCR_422_888 = 39, // 0x27
    HAL_PIXEL_FORMAT_YCBCR_444_888 = 40, // 0x28
    HAL_PIXEL_FORMAT_FLEX_RGB_888 = 41, // 0x29
    HAL_PIXEL_FORMAT_FLEX_RGBA_8888 = 42, // 0x2A
    HAL_PIXEL_FORMAT_YCBCR_422_SP = 16, // 0x10
    HAL_PIXEL_FORMAT_YCRCB_420_SP = 17, // 0x11
    HAL_PIXEL_FORMAT_YCBCR_422_I = 20, // 0x14
    HAL_PIXEL_FORMAT_JPEG = 256, // 0x100
} android_pixel_format_t;

enum {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000U,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002U,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003U,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK          = 0x0000000FU,

    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000U,
    /* buffer is rarely written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020U,
    /* buffer is often written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030U,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0U,

    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE            = 0x00000100U,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER             = 0x00000200U,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D                 = 0x00000400U,
    /* buffer will be used by the HWComposer HAL module */
    GRALLOC_USAGE_HW_COMPOSER           = 0x00000800U,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB                 = 0x00001000U,

    /* buffer should be displayed full-screen on an external display when
     * possible */
    GRALLOC_USAGE_EXTERNAL_DISP         = 0x00002000U,

    /* Must have a hardware-protected path to external display sink for
     * this buffer.  If a hardware-protected path is not available, then
     * either don't composite only this buffer (preferred) to the
     * external sink, or (less desirable) do not route the entire
     * composition to the external sink.  */
    GRALLOC_USAGE_PROTECTED             = 0x00004000U,

    /* buffer may be used as a cursor */
    GRALLOC_USAGE_CURSOR                = 0x00008000U,

    /* buffer will be used with the HW video encoder */
    GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000U,
    /* buffer will be written by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000U,
    /* buffer will be read by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000U,
    /* buffer will be used as part of zero-shutter-lag queue */
    GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000U,
    /* mask for the camera access values */
    GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000U,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK               = 0x00071F00U,

    /* buffer will be used as a RenderScript Allocation */
    GRALLOC_USAGE_RENDERSCRIPT          = 0x00100000U,

    /* Set by the consumer to indicate to the producer that they may attach a
     * buffer that they did not detach from the BufferQueue. Will be filtered
     * out by GRALLOC_USAGE_ALLOC_MASK, so gralloc modules will not need to
     * handle this flag. */
    GRALLOC_USAGE_FOREIGN_BUFFERS       = 0x00200000U,

    /* Mask of all flags which could be passed to a gralloc module for buffer
     * allocation. Any flags not in this mask do not need to be handled by
     * gralloc modules. */
    GRALLOC_USAGE_ALLOC_MASK            = ~(GRALLOC_USAGE_FOREIGN_BUFFERS),

    /* implementation-specific private usage flags */
    GRALLOC_USAGE_PRIVATE_0             = 0x10000000U,
    GRALLOC_USAGE_PRIVATE_1             = 0x20000000U,
    GRALLOC_USAGE_PRIVATE_2             = 0x40000000U,
    GRALLOC_USAGE_PRIVATE_3             = 0x80000000U,
    GRALLOC_USAGE_PRIVATE_MASK          = 0xF0000000U,
};

typedef enum {
    GRALLOC1_CONSUMER_USAGE_NONE = 0,
    GRALLOC1_CONSUMER_USAGE_CPU_READ_NEVER = 0,
    /* 1ULL << 0 */
    GRALLOC1_CONSUMER_USAGE_CPU_READ = 1ULL << 1,
    GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN = 1ULL << 2 |
            GRALLOC1_CONSUMER_USAGE_CPU_READ,
    /* 1ULL << 3 */
    /* 1ULL << 4 */
    /* 1ULL << 5 */
    /* 1ULL << 6 */
    /* 1ULL << 7 */
    GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE = 1ULL << 8,
    /* 1ULL << 9 */
    /* 1ULL << 10 */
    GRALLOC1_CONSUMER_USAGE_HWCOMPOSER = 1ULL << 11,
    GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET = 1ULL << 12,
    /* 1ULL << 13 */
    /* 1ULL << 14 */
    GRALLOC1_CONSUMER_USAGE_CURSOR = 1ULL << 15,
    GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER = 1ULL << 16,
    /* 1ULL << 17 */
    GRALLOC1_CONSUMER_USAGE_CAMERA = 1ULL << 18,
    /* 1ULL << 19 */
    GRALLOC1_CONSUMER_USAGE_RENDERSCRIPT = 1ULL << 20,

    /* Indicates that the consumer may attach buffers to their end of the
     * BufferQueue, which means that the producer may never have seen a given
     * dequeued buffer before. May be ignored by the gralloc device. */
    GRALLOC1_CONSUMER_USAGE_FOREIGN_BUFFERS = 1ULL << 21,

    /* 1ULL << 22 */
    GRALLOC1_CONSUMER_USAGE_GPU_DATA_BUFFER = 1ULL << 23,
    /* 1ULL << 24 */
    /* 1ULL << 25 */
    /* 1ULL << 26 */
    /* 1ULL << 27 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_CONSUMER_USAGE_PRIVATE_0 = 1ULL << 28,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_1 = 1ULL << 29,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_2 = 1ULL << 30,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_3 = 1ULL << 31,

    /* 1ULL << 32 */
    /* 1ULL << 33 */
    /* 1ULL << 34 */
    /* 1ULL << 35 */
    /* 1ULL << 36 */
    /* 1ULL << 37 */
    /* 1ULL << 38 */
    /* 1ULL << 39 */
    /* 1ULL << 40 */
    /* 1ULL << 41 */
    /* 1ULL << 42 */
    /* 1ULL << 43 */
    /* 1ULL << 44 */
    /* 1ULL << 45 */
    /* 1ULL << 46 */
    /* 1ULL << 47 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_CONSUMER_USAGE_PRIVATE_19 = 1ULL << 48,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_18 = 1ULL << 49,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_17 = 1ULL << 50,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_16 = 1ULL << 51,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_15 = 1ULL << 52,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_14 = 1ULL << 53,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_13 = 1ULL << 54,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_12 = 1ULL << 55,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_11 = 1ULL << 56,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_10 = 1ULL << 57,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_9 = 1ULL << 58,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_8 = 1ULL << 59,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_7 = 1ULL << 60,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_6 = 1ULL << 61,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_5 = 1ULL << 62,
    GRALLOC1_CONSUMER_USAGE_PRIVATE_4 = 1ULL << 63,
} gralloc1_consumer_usage_t;

typedef enum {
    GRALLOC1_PRODUCER_USAGE_NONE = 0,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE_NEVER = 0,
    /* 1ULL << 0 */
    GRALLOC1_PRODUCER_USAGE_CPU_READ = 1ULL << 1,
    GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN = 1ULL << 2 |
            GRALLOC1_PRODUCER_USAGE_CPU_READ,
    /* 1ULL << 3 */
    /* 1ULL << 4 */
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE = 1ULL << 5,
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN = 1ULL << 6 |
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    /* 1ULL << 7 */
    /* 1ULL << 8 */
    GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET = 1ULL << 9,
    /* 1ULL << 10 */
    /* 1ULL << 11 */
    /* 1ULL << 12 */
    /* 1ULL << 13 */

    /* The consumer must have a hardware-protected path to an external display
     * sink for this buffer. If a hardware-protected path is not available, then
     * do not attempt to display this buffer. */
    GRALLOC1_PRODUCER_USAGE_PROTECTED = 1ULL << 14,

    /* 1ULL << 15 */
    /* 1ULL << 16 */
    GRALLOC1_PRODUCER_USAGE_CAMERA = 1ULL << 17,
    /* 1ULL << 18 */
    /* 1ULL << 19 */
    /* 1ULL << 20 */
    /* 1ULL << 21 */
    GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER = 1ULL << 22,
    GRALLOC1_PRODUCER_USAGE_SENSOR_DIRECT_DATA = 1ULL << 23,
    /* 1ULL << 24 */
    /* 1ULL << 25 */
    /* 1ULL << 26 */
    /* 1ULL << 27 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_PRODUCER_USAGE_PRIVATE_0 = 1ULL << 28,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_1 = 1ULL << 29,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_2 = 1ULL << 30,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_3 = 1ULL << 31,

    /* 1ULL << 32 */
    /* 1ULL << 33 */
    /* 1ULL << 34 */
    /* 1ULL << 35 */
    /* 1ULL << 36 */
    /* 1ULL << 37 */
    /* 1ULL << 38 */
    /* 1ULL << 39 */
    /* 1ULL << 40 */
    /* 1ULL << 41 */
    /* 1ULL << 42 */
    /* 1ULL << 43 */
    /* 1ULL << 44 */
    /* 1ULL << 45 */
    /* 1ULL << 46 */
    /* 1ULL << 47 */

    /* Bits reserved for implementation-specific usage flags */
    GRALLOC1_PRODUCER_USAGE_PRIVATE_19 = 1ULL << 48,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_18 = 1ULL << 49,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_17 = 1ULL << 50,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_16 = 1ULL << 51,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_15 = 1ULL << 52,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_14 = 1ULL << 53,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_13 = 1ULL << 54,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_12 = 1ULL << 55,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_11 = 1ULL << 56,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_10 = 1ULL << 57,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_9 = 1ULL << 58,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_8 = 1ULL << 59,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_7 = 1ULL << 60,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_6 = 1ULL << 61,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_5 = 1ULL << 62,
    GRALLOC1_PRODUCER_USAGE_PRIVATE_4 = 1ULL << 63,
} gralloc1_producer_usage_t;

class VkDecoderGlobalState::Impl {
public:
    Impl() : m_vk(emugl::vkDispatch()) { }
    ~Impl() = default;

    void on_vkGetPhysicalDeviceProperties(
            VkPhysicalDevice physicalDevice,
            VkPhysicalDeviceProperties* pProperties) {
        m_vk->vkGetPhysicalDeviceProperties(
            physicalDevice, pProperties);

        static constexpr uint32_t kMaxSafeVersion = VK_MAKE_VERSION(1, 0, 65);

        if (pProperties->apiVersion > kMaxSafeVersion) {
            pProperties->apiVersion = kMaxSafeVersion;
        }
    }

    void on_vkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties) {

        m_vk->vkGetPhysicalDeviceMemoryProperties(
            physicalDevice, pMemoryProperties);

        // Pick a max heap size that will work around
        // drivers that give bad suggestions (such as 0xFFFFFFFFFFFFFFFF for the heap size)
        // plus won't break the bank on 32-bit userspace.
        static constexpr VkDeviceSize kMaxSafeHeapSize =
            2ULL * 1024ULL * 1024ULL * 1024ULL;

        for (uint32_t i = 0; i < pMemoryProperties->memoryTypeCount; ++i) {
            uint32_t heapIndex = pMemoryProperties->memoryTypes[i].heapIndex;
            auto& heap = pMemoryProperties->memoryHeaps[heapIndex];

            if (heap.size > kMaxSafeHeapSize) {
                heap.size = kMaxSafeHeapSize;
            }

            // Un-advertise coherent mappings until goldfish_address_space
            // is landed.
            pMemoryProperties->memoryTypes[i].propertyFlags =
                pMemoryProperties->memoryTypes[i].propertyFlags &
                ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    VkResult on_vkCreateDevice(VkPhysicalDevice physicalDevice,
                           const VkDeviceCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkDevice* pDevice) {
        // Run the underlying API call.
        VkResult result =
            m_vk->vkCreateDevice(
                physicalDevice, pCreateInfo, pAllocator, pDevice);

        if (result != VK_SUCCESS) {
            // Allow invalid usage.
            return result;
        }

        {
            AutoLock lock(mLock);
            mPhysicalDevices.set(*pDevice, physicalDevice);
        }

        VkPhysicalDeviceMemoryProperties props;
        m_vk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &props);

        uint32_t queueFamilyPropCount = 0;

        std::vector<VkQueueFamilyProperties> queueFamilyProps;

        m_vk->vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropCount, nullptr);

        queueFamilyProps.resize(queueFamilyPropCount);

        m_vk->vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropCount, queueFamilyProps.data());

        {
            AutoLock lock(mLock);
            mPhysicalDeviceMemoryProperties.set(physicalDevice, props);
            mPhysicalDeviceQueueFamilyProperties.set(physicalDevice, queueFamilyProps);
        }

        return result;
    }

    void on_vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {

        AutoLock lock(mLock);
        mPhysicalDevices.erase(device);
        
        auto queueInfos = mDeviceQueues.get(device);

        // Destroy our private command pool.
        if (queueInfos) {
            for (const auto& info : *queueInfos) {
                if (info.privateCommandPool != VK_NULL_HANDLE) {
                    m_vk->vkFreeCommandBuffers(
                        device, info.privateCommandPool, 1, &info.privateCommandBuffer);
                    m_vk->vkDestroyCommandPool(
                        device, info.privateCommandPool, nullptr);
                }
            }
        }

        auto staging = mStaging.get(device);

        if (staging) {
            if (staging->stagingBuffer != VK_NULL_HANDLE) {
                m_vk->vkDestroyBuffer(device, staging->stagingBuffer, nullptr);
            }
            if (staging->memory != VK_NULL_HANDLE) {
                m_vk->vkUnmapMemory(device, staging->memory);
                m_vk->vkFreeMemory(device, staging->memory, nullptr);
            }
        }

        // Run the underlying API call.
        m_vk->vkDestroyDevice(device, pAllocator);
    }

    // Memory mapping API
    VkResult on_vkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory) {

        VkResult result =
            m_vk->vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);

        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);

        auto physdev = mPhysicalDevices.get(device);

        if (!physdev) {
            // User app gave an invalid VkDevice,
            // but we don't really want to crash here.
            // We should allow invalid apps.
            return VK_ERROR_DEVICE_LOST;
        }

        auto memProps = mPhysicalDeviceMemoryProperties.get(*physdev);

        if (!memProps) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl::emugl_crash_reporter(
                    "FATAL: Could not get memory properties for "
                    "VkPhysicalDevice");
        }

        // If the memory was allocated with a type index that corresponds
        // to a memory type that is host visible, let's also map the entire
        // thing.

        // First, check validity of the user's type index.
        if (pAllocateInfo->memoryTypeIndex >= memProps->memoryTypeCount) {
            // Continue allowing invalid behavior.
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }

        DeviceMemoryInfo memInfo;
        memInfo.size = pAllocateInfo->allocationSize;

        VkMemoryPropertyFlags flags =
                memProps->memoryTypes[pAllocateInfo->memoryTypeIndex]
                        .propertyFlags;

        bool shouldMap = flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        if (!shouldMap) return result;

        VkResult mapResult = m_vk->vkMapMemory(device, *pMemory, 0,
                                               memInfo.size, 0, &memInfo.ptr);


        if (mapResult != VK_SUCCESS) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        mMemInfo.set(*pMemory, memInfo);

        return result;
    }

    void on_vkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        AutoLock lock(mLock);

        auto info = mMemInfo.get(memory);

        if (!info) {
            // Invalid usage.
            return;
        }

        if (info->ptr) {
            m_vk->vkUnmapMemory(device, memory);
        }

        mMemInfo.erase(memory);

        m_vk->vkFreeMemory(device, memory, pAllocator);
    }

    VkResult on_vkMapMemory(VkDevice device,
                            VkDeviceMemory memory,
                            VkDeviceSize offset,
                            VkDeviceSize size,
                            VkMemoryMapFlags flags,
                            void** ppData) {
        AutoLock lock(mLock);

        auto info = mMemInfo.get(memory);

        if (!info) {
            // Invalid usage.
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        if (!info->ptr) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        *ppData = (void*)((uint8_t*)info->ptr + offset);

        return VK_SUCCESS;
    }

    void on_vkUnmapMemory(VkDevice device, VkDeviceMemory memory) {
        // no-op; user-level mapping does not correspond
        // to any operation here.
    }

    uint8_t* getMappedHostPointer(VkDeviceMemory memory) {
        AutoLock lock(mLock);

        auto info = mMemInfo.get(memory);

        if (!info) {
            // Invalid usage.
            return nullptr;
        }

        return (uint8_t*)(info->ptr);
    }

    VkDeviceSize getDeviceMemorySize(VkDeviceMemory memory) {
        AutoLock lock(mLock);

        auto info = mMemInfo.get(memory);

        if (!info) {
            // Invalid usage.
            return 0;
        }

        return info->size;
    }

    // VK_ANDROID_native_buffer
    const VkNativeBufferANDROID* getNativeBufferInfo(
            const VkImageCreateInfo* pCreateInfo) {
        if (!pCreateInfo->pNext) return nullptr;

        const VkNativeBufferANDROID* info =
            reinterpret_cast<const VkNativeBufferANDROID*>(pCreateInfo->pNext);

        if (VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID != info->sType) return nullptr;

        // If no color buffer, no need to do anything. Maybe?
        const uint32_t* colorBufferPtr = info->handle;

        if (!colorBufferPtr) return nullptr;

        return info;
    }

    void on_vkGetDeviceQueue(VkDevice device,
                             uint32_t queueFamilyIndex,
                             uint32_t queueIndex,
                            VkQueue* pQueue) {

        m_vk->vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);

        if (!pQueue) return;

        AutoLock lock(mLock);

        DeviceQueueInfo info = {
            device,
            queueFamilyIndex,
            queueIndex,
            *pQueue,
        };
        mDeviceQueues.set(device, {});
        auto queueInfos = mDeviceQueues.get(device);
        queueInfos->push_back(info);
        mQueueToDevice.set(*pQueue, device);
    }

    VkResult on_vkCreateImage(VkDevice device,
                              const VkImageCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkImage* pImage) {

        auto native = getNativeBufferInfo(pCreateInfo);

        if (!native) {
            return m_vk->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
        }

                fprintf(stderr, "%s: call\n", __func__);
        uint32_t colorBuffer = *(native->handle);

        // delete the info struct and pass to vkCreateImage, and also add
        // transfer src capability.
        VkImageCreateInfo infoNoNative = *pCreateInfo;
        infoNoNative.pNext = nullptr;
        infoNoNative.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        VkResult createResult =
                m_vk->vkCreateImage(device, pCreateInfo, pAllocator, pImage);

        if (createResult != VK_SUCCESS) return createResult;

        VkMemoryRequirements memReqs;
        m_vk->vkGetImageMemoryRequirements(device, *pImage, &memReqs);

        // Prefer allocating in host-visible memory.
        uint32_t selectedMemoryTypeIndex =
            initStagingSelectTypeIndex(device, memReqs);


        VkDeviceMemory mem; 
        VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
            memReqs.size, selectedMemoryTypeIndex,
        };

        if (m_vk->vkAllocateMemory(device, &allocInfo, nullptr, &mem) !=
            VK_SUCCESS) {
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;
        }

        VkResult bindResult =
            m_vk->vkBindImageMemory(device, *pImage, mem, 0);
        
        if (bindResult != VK_SUCCESS) {
            return bindResult;
        }

        VkFence fence;
        VkFenceCreateInfo fenceCreateInfo = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0, 0,
        };
        VkResult fenceCreateResult =
            m_vk->vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

        if (fenceCreateResult != VK_SUCCESS) {
            return fenceCreateResult;
        }

        AutoLock lock(mLock);

        NativeBufferInfo nativeInfo = {
            colorBuffer,
            native->format,
            { pCreateInfo->extent.width, pCreateInfo->extent.height },
            mem, fence, memReqs.size,
        };

        mNativeBufferImages.set(*pImage, nativeInfo);

        return createResult;
    }

    void on_vkDestroyImage(VkDevice device,
                           VkImage image,
                           const VkAllocationCallbacks* pAllocator) {
        AutoLock lock(mLock);
        auto nativeInfo = mNativeBufferImages.get(image);
        if (!nativeInfo) {
            lock.unlock();
            m_vk->vkDestroyImage(device, image, pAllocator);
            return;
        }
                fprintf(stderr, "%s: call\n", __func__);
        m_vk->vkDestroyImage(device, image, pAllocator);
        m_vk->vkFreeMemory(device, nativeInfo->backing, nullptr);
    }

    VkResult on_vkGetSwapchainGrallocUsageANDROID(VkDevice device,
                                                  VkFormat format,
                                                  VkImageUsageFlags imageUsage,
                                                  int* grallocUsage) {
                fprintf(stderr, "%s: call\n", __func__);
        *grallocUsage |=
            GRALLOC_USAGE_SW_READ_OFTEN |
            GRALLOC_USAGE_SW_WRITE_OFTEN |
            GRALLOC_USAGE_HW_RENDER |
            GRALLOC_USAGE_HW_TEXTURE;

        return VK_SUCCESS;
    }

    VkResult on_vkGetSwapchainGrallocUsage2ANDROID(
            VkDevice device,
            VkFormat format,
            VkImageUsageFlags imageUsage,
            VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
            uint64_t* grallocConsumerUsage,
            uint64_t* grallocProducerUsage) {
                fprintf(stderr, "%s: call\n", __func__);

        int usage =
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

        *grallocProducerUsage = static_cast<uint64_t>(usage) & PRODUCER_MASK;
        *grallocConsumerUsage = static_cast<uint64_t>(usage) & CONSUMER_MASK;

        if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_READ_OFTEN) ==
            GRALLOC_USAGE_SW_READ_OFTEN) {
            *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN;
            *grallocConsumerUsage |= GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN;
        }

        if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_WRITE_OFTEN) ==
            GRALLOC_USAGE_SW_WRITE_OFTEN) {
            *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN;
        }

        return VK_SUCCESS;
    }

    VkResult on_vkAcquireImageANDROID(VkDevice device,
                                      VkImage image,
                                      int nativeFenceFd,
                                      VkSemaphore semaphore,
                                      VkFence fence) {
                fprintf(stderr, "%s: call\n", __func__);
        (void)device;
        (void)image;
        // Assume we waited on the fd in the guest already.
        (void)nativeFenceFd;
        (void)semaphore;
        (void)fence;
        return VK_SUCCESS;
    }


    VkResult on_vkQueueSignalReleaseImageANDROID(
            VkQueue queue,
            uint32_t waitSemaphoreCount,
            const VkSemaphore* pWaitSemaphores,
            VkImage image,
            int* pNativeFenceFd) {
                fprintf(stderr, "%s: call\n", __func__);

        // Dont give a fence
        *pNativeFenceFd = -1;

        // These command buffers are always recorded under a lock
        AutoLock lock(mLock);

        auto nativeInfo = mNativeBufferImages.get(image);

        // No-op if native buffer info is missing.
        if (!nativeInfo) {
            return VK_SUCCESS;
        }

        const auto& queueInfo = getQueueInfoForQueueLocked(queue);
        VkDevice device = queueInfo.device;

        auto staging = mStaging.get(device);

        if (!staging) {
            emugl::emugl_crash_reporter(
                    "FATAL: Could not get staging buffer in "
                    "vkQueueSignalReleaseImageANDROID!");
        }

        auto cmdBuf = initOrGetPrivateCommandBuffer(queue);

        VkCommandBufferBeginInfo beginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0 };

        // Assume this happens after semaphore is signaled,
        // so we can copy directly to staging buffer.
        m_vk->vkBeginCommandBuffer(cmdBuf, &beginInfo);
        m_vk->vkCmdCopyImageToBuffer(cmdBuf, image,
                                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                     staging->stagingBuffer, 1,
            (VkBufferImageCopy[]){
                0, 0, 0,
                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, },
                { (int32_t)nativeInfo->dimensions.width,
                  (int32_t)nativeInfo->dimensions.height, 1},
            });
        m_vk->vkEndCommandBuffer(cmdBuf);


        // Implicitly synchronize using vkQueueSubmit
        // on that queue
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
            waitSemaphoreCount, pWaitSemaphores,
            nullptr /* no dst stage mask */,
            1, &cmdBuf,
            0, nullptr, /* no signal semaphores */
        };

        m_vk->vkResetFences(device, 1, &nativeInfo->queueSignaledFence);;
        VkResult submitResult =
            m_vk->vkQueueSubmit(
                queue, 1, &submitInfo,
                nativeInfo->queueSignaledFence);
        m_vk->vkWaitForFences(device, 1, &nativeInfo->queueSignaledFence, VK_TRUE, (uint64_t)-1);
        m_vk->vkInvalidateMappedMemoryRanges(
                device, 1,
                (VkMappedMemoryRange[]){
                        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                        0,
                        staging->memory,
                        0,
                        nativeInfo->memorySize,
                });
        
        auto fb = FrameBuffer::getFB();
        fb->replaceColorBufferContents(nativeInfo->colorBuffer, staging->ptr);
        lock.unlock();

        return submitResult;
    }

private:

    // We always map the whole size on host.
    // This makes it much easier to implement
    // the memory map API.
    struct DeviceMemoryInfo {
        // When ptr is null, it means the VkDeviceMemory object
        // was not allocated with the HOST_VISIBLE property.
        void* ptr = nullptr;
        VkDeviceSize size;
    };

    // DeviceStaging: per-device memories for reading back
    // VkImages to paint back into ColorBuffers.
    struct DeviceStaging {
        uint32_t typeIndex;
        VkDeviceMemory memory;
        VkDeviceSize size;
        void* ptr;
        VkBuffer stagingBuffer;
    };

    struct NativeBufferInfo {
        uint32_t colorBuffer;
        int format;
        VkExtent2D dimensions;
        VkDeviceMemory backing;
        VkFence queueSignaledFence;
        VkDeviceSize memorySize;
    };

    struct DeviceQueueInfo {
        VkDevice device;
        uint32_t queueFamilyIndex;
        uint32_t queueIndex;
        VkQueue queue;
        // Private optional command pool
        // to send commands on that queue
        VkCommandPool privateCommandPool = VK_NULL_HANDLE;
        // Private command buffer
        VkCommandBuffer privateCommandBuffer = VK_NULL_HANDLE;
    };

    VkPhysicalDeviceMemoryProperties
    getMemoryPropertiesLocked(VkDevice device) {
        auto physdev = mPhysicalDevices.get(device);

        if (!physdev) { return {}; }

        auto memProps = mPhysicalDeviceMemoryProperties.get(*physdev);

        if (!memProps) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl::emugl_crash_reporter(
                    "FATAL: Could not get memory properties for "
                    "VkPhysicalDevice");
        }

        return *memProps;
    }

    std::vector<uint32_t>
    getHostVisibleMemoryTypeIndicesLocked(VkDevice device) {

        auto memProps = getMemoryPropertiesLocked(device);

        std::vector<uint32_t> res;

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if (memProps.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                res.push_back(i);
            }
        }

        return res;
    }

    uint32_t initStagingSelectTypeIndex(VkDevice device,
                                        const VkMemoryRequirements& memReqs) {
        AutoLock lock(mLock);
        auto currentStaging = mStaging.get(device);

        if (currentStaging) {

            if (memReqs.size > currentStaging->size) {
                m_vk->vkUnmapMemory(device, currentStaging->memory);
                m_vk->vkFreeMemory(device, currentStaging->memory, nullptr);
                currentStaging->size = 2 * memReqs.size;
                VkMemoryAllocateInfo allocInfo = {
                    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
                    currentStaging->size,
                    currentStaging->typeIndex,
                };
                VkResult allocRes = m_vk->vkAllocateMemory(
                        device, &allocInfo, nullptr, &currentStaging->memory);
                if (allocRes != VK_SUCCESS) {
                    emugl::emugl_crash_reporter("FATAL: Failed to realloc staging");
                }

                VkResult mapRes = m_vk->vkMapMemory(
                        device, currentStaging->memory, 0, VK_WHOLE_SIZE, 0,
                        &currentStaging->ptr);
                if (mapRes != VK_SUCCESS) {
                    emugl::emugl_crash_reporter("FATAL: Failed to remap staging");
                }
            }

            uint32_t currentStagingTypeIndex =
                currentStaging->typeIndex;

            // Try to select the same type index
            // as our staging buffer.
            for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
                if (memReqs.memoryTypeBits & (1 << i)) {
                    if (i == currentStagingTypeIndex) {
                        return i;
                    }
                }
            }

            // Otherwise just go with the first type index
            // that shows up.
            for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
                if (memReqs.memoryTypeBits & (1 << i)) {
                    return i;
                }
            }

            return 0;
        } else {
            auto hostVisibles = getHostVisibleMemoryTypeIndicesLocked(device);

            if (hostVisibles.empty()) {
                emugl::emugl_crash_reporter("FATAL: device has no host-visible memory");
            }

            uint32_t fallbackTypeIndex = 0;
            uint32_t commonTypeIndex = 0;
            bool hasCommonTypeIndex = false;
            for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
                if (memReqs.memoryTypeBits & (1 << i)) {
                    fallbackTypeIndex = i;
                    for (uint32_t j = 0; j < hostVisibles.size(); ++j) {
                        if (hostVisibles[j] == i &&
                            !hasCommonTypeIndex) {
                            commonTypeIndex = i;
                            hasCommonTypeIndex = true;
                            break;
                        }
                    }
                }
                if (hasCommonTypeIndex) break;
            }

            VkDeviceMemory mem;
            void* mappedPtr;
            uint32_t stagingTypeIndex =
                hasCommonTypeIndex ? commonTypeIndex : hostVisibles[0];
            VkDeviceSize stagingSize = memReqs.size * 2;

            VkMemoryAllocateInfo allocInfo = {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
                stagingSize, stagingTypeIndex,
            };

            VkResult allocRes =
                m_vk->vkAllocateMemory(device, &allocInfo, nullptr, &mem);

            if (allocRes != VK_SUCCESS) {
                emugl::emugl_crash_reporter("FATAL: Failed to allocate staging buffer memory");
            }

            VkResult mapRes =
                m_vk->vkMapMemory(device, mem, 0, VK_WHOLE_SIZE, 0, &mappedPtr);

            if (mapRes != VK_SUCCESS) {
                emugl::emugl_crash_reporter("FATAL: Failed to map staging buffer memory");
            }

            VkBuffer buffer;
            VkBufferCreateInfo bufInfo = {
                VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
                stagingSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_SHARING_MODE_EXCLUSIVE, 0, nullptr,
            };

            VkResult bufferRes =
                m_vk->vkCreateBuffer(device, &bufInfo, nullptr, &buffer);

            if (bufferRes != VK_SUCCESS) {
                emugl::emugl_crash_reporter("FATAL: Failed to create staging buffer");
            }

            VkResult bindRes =
                m_vk->vkBindBufferMemory(device, buffer, mem, 0);

            if (bindRes != VK_SUCCESS) {
                emugl::emugl_crash_reporter("FATAL: Failed to bind staging buffer memory");
            }

            DeviceStaging stageInfo = {
                stagingTypeIndex, mem, stagingSize, mappedPtr, buffer,
            };

            mStaging.set(device, stageInfo);

            if (hasCommonTypeIndex) {
                return commonTypeIndex;
            } else {
                return fallbackTypeIndex;
            }
        }
    }

    DeviceQueueInfo& getQueueInfoForQueueLocked(VkQueue queue) {
        auto device = mQueueToDevice.get(queue);

        if (!device) {
            emugl::emugl_crash_reporter("FATAL: queue did not correspond to a device");
        }

        auto deviceQueueInfo = mDeviceQueues.get(*device);

        if (!deviceQueueInfo) {
            emugl::emugl_crash_reporter("FATAL: device has no queue info but queue was created");
        }

        for (auto& info : *deviceQueueInfo) {
            if (queue == info.queue) {
                return info;
            }
        }

        emugl::emugl_crash_reporter("FATAL: device queue info has no record of queue");
        return (*deviceQueueInfo)[0];
    }

    VkCommandBuffer initOrGetPrivateCommandBuffer(VkQueue queue) {
        auto& info = getQueueInfoForQueueLocked(queue);

        if (info.privateCommandPool != VK_NULL_HANDLE) {
            return info.privateCommandBuffer;
        }

        // Allocate the command pool if it does not exist already.
        VkCommandPoolCreateInfo createInfo = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0, 0,
            info.queueFamilyIndex,
        };

        VkResult poolCreateResult =
            m_vk->vkCreateCommandPool(info.device, &createInfo, nullptr,
                    &info.privateCommandPool);

        if (poolCreateResult != VK_SUCCESS) {
            emugl::emugl_crash_reporter("FATAL: Could not create private command pool");
        }

        VkCommandBufferAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
            info.privateCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
        };

        VkResult allocateResult =
            m_vk->vkAllocateCommandBuffers(
                info.device, &allocInfo, &info.privateCommandBuffer);

        if (allocateResult != VK_SUCCESS) {
            emugl::emugl_crash_reporter("FATAL: Could not allocate private command buffer");
        }

        return info.privateCommandBuffer;
    }

    VulkanDispatch* m_vk;

    Lock mLock;

    // Back-reference to the physical device associated
    // with a particular VkDevice.
    vk_emu::VkDecoderComponent<VkDevice, VkPhysicalDevice> mPhysicalDevices;
    // Keep the physical device memory properties around.
    vk_emu::VkDecoderComponent<VkPhysicalDevice, VkPhysicalDeviceMemoryProperties>
        mPhysicalDeviceMemoryProperties;
    // Queue family properties
    vk_emu::VkDecoderComponent<VkPhysicalDevice, std::vector<VkQueueFamilyProperties>>
        mPhysicalDeviceQueueFamilyProperties;

    vk_emu::VkDecoderComponent<VkDeviceMemory, DeviceMemoryInfo> mMemInfo;

    // Images that correspond to gralloc buffers and thus
    // needing some kind of custom backing of some sort.
    vk_emu::VkDecoderComponent<VkDevice, DeviceStaging> mStaging;
    vk_emu::VkDecoderComponent<VkImage, NativeBufferInfo> mNativeBufferImages;
    // Correlate devices with queues
    vk_emu::VkDecoderComponent<VkDevice, std::vector<DeviceQueueInfo>> mDeviceQueues;
    vk_emu::VkDecoderComponent<VkQueue, VkDevice> mQueueToDevice;
};

VkDecoderGlobalState::VkDecoderGlobalState()
    : mImpl(new VkDecoderGlobalState::Impl()) {}

VkDecoderGlobalState::~VkDecoderGlobalState() = default;

static LazyInstance<VkDecoderGlobalState> sGlobalDecoderState =
        LAZY_INSTANCE_INIT;

// static
VkDecoderGlobalState* VkDecoderGlobalState::get() {
    return sGlobalDecoderState.ptr();
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties(
        physicalDevice, pMemoryProperties);
}

VkResult VkDecoderGlobalState::on_vkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {
    return mImpl->on_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void VkDecoderGlobalState::on_vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDevice(device, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory) {
    return mImpl->on_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

void VkDecoderGlobalState::on_vkFreeMemory(
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkFreeMemory(device, memory, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkMapMemory(VkDevice device,
                                              VkDeviceMemory memory,
                                              VkDeviceSize offset,
                                              VkDeviceSize size,
                                              VkMemoryMapFlags flags,
                                              void** ppData) {
    return mImpl->on_vkMapMemory(device, memory, offset, size, flags, ppData);
}

void VkDecoderGlobalState::on_vkUnmapMemory(VkDevice device,
                                            VkDeviceMemory memory) {
    mImpl->on_vkUnmapMemory(device, memory);
}

uint8_t* VkDecoderGlobalState::getMappedHostPointer(VkDeviceMemory memory) {
    return mImpl->getMappedHostPointer(memory);
}

VkDeviceSize VkDecoderGlobalState::getDeviceMemorySize(VkDeviceMemory memory) {
    return mImpl->getDeviceMemorySize(memory);
}

// VK_ANDROID_native_buffer
void VkDecoderGlobalState::on_vkGetDeviceQueue(VkDevice device,
                                               uint32_t queueFamilyIndex,
                                               uint32_t queueIndex,
                                               VkQueue* pQueue) {
    mImpl->on_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
VkResult VkDecoderGlobalState::on_vkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage) {
    return mImpl->on_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}
void VkDecoderGlobalState::on_vkDestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImage(device, image, pAllocator);
}
VkResult VkDecoderGlobalState::on_vkGetSwapchainGrallocUsageANDROID(
        VkDevice device,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        int* grallocUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsageANDROID(
            device, format, imageUsage, grallocUsage);
}
VkResult VkDecoderGlobalState::on_vkGetSwapchainGrallocUsage2ANDROID(
        VkDevice device,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
        uint64_t* grallocConsumerUsage,
        uint64_t* grallocProducerUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsage2ANDROID(
            device, format, imageUsage, swapchainImageUsage,
            grallocConsumerUsage, grallocProducerUsage);
}
VkResult VkDecoderGlobalState::on_vkAcquireImageANDROID(VkDevice device,
                                                        VkImage image,
                                                        int nativeFenceFd,
                                                        VkSemaphore semaphore,
                                                        VkFence fence) {
    return mImpl->on_vkAcquireImageANDROID(device, image, nativeFenceFd,
                                           semaphore, fence);
}
VkResult VkDecoderGlobalState::on_vkQueueSignalReleaseImageANDROID(
        VkQueue queue,
        uint32_t waitSemaphoreCount,
        const VkSemaphore* pWaitSemaphores,
        VkImage image,
        int* pNativeFenceFd) {
    return mImpl->on_vkQueueSignalReleaseImageANDROID(
            queue, waitSemaphoreCount, pWaitSemaphores, image, pNativeFenceFd);
}
