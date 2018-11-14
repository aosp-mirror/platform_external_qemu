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

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "emugl/common/crash_reporter.h"

#include "vulkan/cereal/common/goldfish_vk_private_defs.h"

#include <algorithm>
#include <unordered_map>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

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

        for (uint32_t i = 0; i < pMemoryProperties->memoryTypeCount; ++i) {
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
            mDevices[*pDevice] = physicalDevice;
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
            mPhysicalDeviceMemoryProperties[physicalDevice] = props;
            mPhysicalDeviceQueueFamilyProperties[physicalDevice] = queueFamilyProps;
        }

        return result;
    }

    void on_vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {

        AutoLock lock(mLock);
        mDevices.erase(device);
        
        auto queueInfos = android::base::find(mDeviceQueues, device);

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

        auto staging = android::base::find(mStaging, device);

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

        auto physdev = android::base::find(mDevices, device);

        if (!physdev) {
            // User app gave an invalid VkDevice,
            // but we don't really want to crash here.
            // We should allow invalid apps.
            return VK_ERROR_DEVICE_LOST;
        }

        auto memProps =
                android::base::find(mPhysicalDeviceMemoryProperties, *physdev);

        if (!memProps) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl_crash_reporter(
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

        mMapInfo[*pMemory] = MappedMemoryInfo();
        auto& mapInfo = mMapInfo[*pMemory];
        mapInfo.size = pAllocateInfo->allocationSize;

        VkMemoryPropertyFlags flags =
                memProps->memoryTypes[pAllocateInfo->memoryTypeIndex]
                        .propertyFlags;

        bool shouldMap = flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        if (!shouldMap) return result;

        VkResult mapResult = m_vk->vkMapMemory(device, *pMemory, 0,
                                               mapInfo.size, 0, &mapInfo.ptr);


        if (mapResult != VK_SUCCESS) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        return result;
    }

    void on_vkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

        if (!info) {
            // Invalid usage.
            return;
        }

        if (info->ptr) {
            m_vk->vkUnmapMemory(device, memory);
        }

        mMapInfo.erase(memory);

        m_vk->vkFreeMemory(device, memory, pAllocator);
    }

    VkResult on_vkMapMemory(VkDevice device,
                            VkDeviceMemory memory,
                            VkDeviceSize offset,
                            VkDeviceSize size,
                            VkMemoryMapFlags flags,
                            void** ppData) {
        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

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

        auto info = android::base::find(mMapInfo, memory);

        if (!info) {
            // Invalid usage.
            return nullptr;
        }

        return (uint8_t*)(info->ptr);
    }

    VkDeviceSize getDeviceMemorySize(VkDeviceMemory memory) {
        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

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
        auto& queueInfos = mDeviceQueues[device];
        queueInfos.push_back(info);
        mQueueToDevice[*pQueue] = device;
    }

    VkResult on_vkCreateImage(VkDevice device,
                              const VkImageCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkImage* pImage) {

        auto native = getNativeBufferInfo(pCreateInfo);

        if (!native) {
            return m_vk->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
        }

        uint32_t colorBuffer = *(native->handle);

        // delete the info struct and pass to vkCreateImage.
        VkImageCreateInfo infoNoNative = *pCreateInfo;
        infoNoNative.pNext = nullptr;

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
            mem, fence,
        };

        mNativeBufferImages[*pImage] = nativeInfo;

        return createResult;
    }

    void on_vkDestroyImage(VkDevice device,
                           VkImage image,
                           const VkAllocationCallbacks* pAllocator) {
        AutoLock lock(mLock);
        auto nativeInfo = android::base::find(mNativeBufferImages, image);
        if (!nativeInfo) {
            lock.unlock();
            m_vk->vkDestroyImage(device, image, pAllocator);
            return;
        }
        m_vk->vkDestroyImage(device, image, pAllocator);
        m_vk->vkFreeMemory(device, nativeInfo->backing, nullptr);
    }

    VkResult on_vkGetSwapchainGrallocUsageANDROID(VkDevice device,
                                                  VkFormat format,
                                                  VkImageUsageFlags imageUsage,
                                                  int* grallocUsage) {
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

        // Dont give a fence
        *pNativeFenceFd = -1;

        // These command buffers are always recorded under a lock
        AutoLock lock(mLock);

        auto nativeInfo = android::base::find(mNativeBufferImages, image);

        // No-op if native buffer info is missing.
        if (!nativeInfo) {
            return VK_SUCCESS;
        }

        auto cmdBuf = initOrGetPrivateCommandBuffer(queue);

        VkCommandBufferBeginInfo beginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFER_BEGIN_INFO, 0,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0 };

        // Assume this happens after semaphore is signaled,
        // so we can copy directly to staging buffer.
        m_vk->vkBeginCommandBuffer(cmdBuf, &beginInfo);
        m_vk->vkEndCommandBuffer(cmdBuf);

        lock.unlock();

        // Implicitly synchronize using vkQueueSubmit
        // on that queue
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
            waitSemaphoreCount, pWaitSemaphores,
            nullptr /* no dst stage mask */,
            1, &cmdBuf,
            0, nullptr, /* no signal semaphores */
        };

        VkResult submitResult =
            m_vk->vkQueueSubmit(
                queue, 1, &submitInfo,
                nativeInfo->queueSignaledFence);

        return submitResult;
    }

private:

    VkPhysicalDeviceMemoryProperties
    getMemoryPropertiesLocked(VkDevice device) {
        auto physdev = android::base::find(mDevices, device);

        if (!physdev) { return {}; }

        auto memProps =
                android::base::find(mPhysicalDeviceMemoryProperties, *physdev);

        if (!memProps) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl_crash_reporter(
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
        auto currentStaging = android::base::find(mStaging, device);

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
                    emugl_crash_reporter("FATAL: Failed to realloc staging");
                }

                VkResult mapRes = m_vk->vkMapMemory(
                        device, currentStaging->memory, 0, VK_WHOLE_SIZE, 0,
                        &currentStaging->ptr);
                if (mapRes != VK_SUCCESS) {
                    emugl_crash_reporter("FATAL: Failed to remap staging");
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

        } else {
            auto hostVisibles = getHostVisibleMemoryTypeIndicesLocked(device);

            if (hostVisibles.empty()) {
                emugl_crash_reporter("FATAL: device has no host-visible memory");
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
                emugl_crash_reporter("FATAL: Failed to allocate staging buffer memory");
            }

            VkResult mapRes =
                m_vk->vkMapMemory(device, mem, 0, VK_WHOLE_SIZE, 0, &mappedPtr);

            if (mapRes != VK_SUCCESS) {
                emugl_crash_reporter("FATAL: Failed to map staging buffer memory");
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
                emugl_crash_reporter("FATAL: Failed to create staging buffer");
            }

            VkResult bindRes =
                m_vk->vkBindBufferMemory(device, buffer, mem, 0);

            if (bindRes != VK_SUCCESS) {
                emugl_crash_reporter("FATAL: Failed to bind staging buffer memory");
            }

            DeviceStaging stageInfo = {
                stagingTypeIndex, mem, stagingSize, mappedPtr, buffer,
            };

            mStaging[device] = stageInfo;

            if (hasCommonTypeIndex) {
                return commonTypeIndex;
            } else {
                return fallbackTypeIndex;
            }
        }
    }

    DeviceQueueInfo& getQueueInfoForQueueLocked(VkQueue queue) {
        auto device = android::base::find(mQueueToDevice, queue);

        if (!device) {
            emugl_crash_reporter("FATAL: queue did not correspond to a device");
        }

        auto deviceQueueInfo = android::base::find(mDeviceQueues, *device);

        if (!deviceQueueInfo) {
            emugl_crash_reporter("FATAL: device has no queue info but queue was created");
        }

        for (auto& info : *deviceQueueInfo) {
            if (queue == info.queue) {
                return info;
            }
        }

        emugl_crash_reporter("FATAL: device queue info has no record of queue");
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
            m_vk->vkCreateCommandPool(*device, &createInfo, nullptr,
                    &info.privateCommandPool);

        if (poolCreateResult != VK_SUCCESS) {
            emugl_crash_reporter("FATAL: Could not create private command pool");
        }

        VkCommandBufferAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
            info.privateCommandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
        };

        VkResult allocateResult =
            m_vk->vkAllocateCommandBuffers(
                *device, &allocInfo, &info.privateCommandBuffer);

        if (allocateResult != VK_SUCCESS) {
            emugl_crash_reporter("FATAL: Could not allocate private command buffer");
        }

        return info.privateCommandBuffer;
    }

    // We always map the whole size on host.
    // This makes it much easier to implement
    // the memory map API.
    struct MappedMemoryInfo {
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

    VulkanDispatch* m_vk;

    Lock mLock;

    // Back-reference to the physical device associated
    // with a particular VkDevice.
    std::unordered_map<VkDevice, VkPhysicalDevice> mDevices;
    // Keep the physical device memory properties around.
    std::unordered_map<VkPhysicalDevice, VkPhysicalDeviceMemoryProperties>
        mPhysicalDeviceMemoryProperties;
    // Queue family properties
    std::unordered_map<VkPhysicalDevice, std::vector<VkQueueFamilyProperties>>
        mPhysicalDeviceQueueFamilyProperties;

    std::unordered_map<VkDeviceMemory, MappedMemoryInfo> mMapInfo;

    // Images that correspond to gralloc buffers and thus
    // needing some kind of custom backing of some sort.
    std::unordered_map<VkDevice, DeviceStaging> mStaging;
    std::unordered_map<VkImage, NativeBufferInfo> mNativeBufferImages;
    // Correlate devices with queues
    std::unordered_map<VkDevice, std::vector<DeviceQueueInfo>> mDeviceQueues;
    std::unordered_map<VkQueue, VkDevice> mQueueToDevice;
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
