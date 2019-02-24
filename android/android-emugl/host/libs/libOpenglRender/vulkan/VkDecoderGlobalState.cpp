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

#include "VkAndroidNativeBuffer.h"
#include "VkCommonOperations.h"
#include "VkFormatUtils.h"
#include "VulkanDispatch.h"
#include "vk_util.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"

#include "common/goldfish_vk_deepcopy.h"
#include "common/goldfish_vk_dispatch.h"

#include "emugl/common/crash_reporter.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/vm_operations.h"

#include "GLcommon/etc.h"

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::Optional;

namespace goldfish_vk {

// A list of extensions that should not be passed to the host driver.
// These will mainly include Vulkan features that we emulate ourselves.
static constexpr const char* const
kEmulatedExtensions[] = {
    "VK_ANDROID_native_buffer",
    "VK_KHR_external_memory_fuchsia",
    "VK_KHR_external_semaphore_fuchsia",
};

static constexpr uint32_t kMaxSafeVersion = VK_MAKE_VERSION(1, 0, 65);

class VkDecoderGlobalState::Impl {
public:
    Impl() :
        m_vk(emugl::vkDispatch()),
        m_emu(createOrGetGlobalVkEmulation(m_vk)) { }
    ~Impl() = default;

    VkResult on_vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {

        std::vector<const char*> finalExts =
            filteredExtensionNames(
                pCreateInfo->enabledExtensionCount,
                pCreateInfo->ppEnabledExtensionNames);

        VkInstanceCreateInfo createInfoFiltered = *pCreateInfo;
        createInfoFiltered.enabledExtensionCount = (uint32_t)finalExts.size();
        createInfoFiltered.ppEnabledExtensionNames = finalExts.data();

        VkResult res = m_vk->vkCreateInstance(&createInfoFiltered, pAllocator, pInstance);

        if (res != VK_SUCCESS) return res;

        AutoLock lock(mLock);

        InstanceInfo info;
        for (uint32_t i = 0; i < createInfoFiltered.enabledExtensionCount;
             ++i) {
            info.enabledExtensionNames.push_back(
                    createInfoFiltered.ppEnabledExtensionNames[i]);
        }

        // Box it up
        auto boxed = new_boxed_VkInstance(*pInstance);
        init_vulkan_dispatch_from_instance(m_vk, *pInstance, boxed->dispatch);
        info.boxed = boxed;

        mInstanceInfo[*pInstance] = info;

        *pInstance = (VkInstance)info.boxed;

        auto fb = FrameBuffer::getFB();
        if (!fb) return res;

        fb->registerProcessCleanupCallback(
            boxed->underlying,
            [this, boxed] {
                vkDestroyInstanceImpl(boxed->underlying, nullptr);
            });

        return res;
    }

    void vkDestroyInstanceImpl(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
        AutoLock lock(mLock);

        teardownInstanceLocked(instance);

        m_vk->vkDestroyInstance(instance, pAllocator);

        auto it = mPhysicalDeviceToInstance.begin();

        while (it != mPhysicalDeviceToInstance.end()) {
            if (it->second == instance) {
                it = mPhysicalDeviceToInstance.erase(it);
            } else {
                ++it;
            }
        }

        auto instInfo = android::base::find(mInstanceInfo, instance);
        delete_boxed_VkInstance(instInfo->boxed);
        mInstanceInfo.erase(instance);
    }

    void on_vkDestroyInstance(VkInstance boxed_instance, const VkAllocationCallbacks* pAllocator) {
        auto instance = unbox_VkInstance(boxed_instance);

        vkDestroyInstanceImpl(instance, pAllocator);

        auto fb = FrameBuffer::getFB();
        if (!fb) return;

        fb->unregisterProcessCleanupCallback(instance);
    }

    VkResult on_vkEnumeratePhysicalDevices(
        VkInstance boxed_instance,
        uint32_t* physicalDeviceCount,
        VkPhysicalDevice* physicalDevices) {

        auto instance = unbox_VkInstance(boxed_instance);
        auto vk = dispatch_VkInstance(boxed_instance);

        auto res = vk->vkEnumeratePhysicalDevices(instance, physicalDeviceCount, physicalDevices);

        if (res != VK_SUCCESS) return res;

        AutoLock lock(mLock);

        if (physicalDeviceCount && physicalDevices) {
            // Box them up
            for (uint32_t i = 0; i < *physicalDeviceCount; ++i) {
                mPhysicalDeviceToInstance[physicalDevices[i]] = instance;

                auto& physdevInfo = mPhysdevInfo[physicalDevices[i]];


                    physdevInfo.boxed =
                            new_boxed_VkPhysicalDevice(physicalDevices[i], vk);

                    vk->vkGetPhysicalDeviceProperties(physicalDevices[i],
                                                      &physdevInfo.props);

                    // if (physdevInfo.props.apiVersion > kMaxSafeVersion) {
                        // physdevInfo.props.apiVersion = kMaxSafeVersion;
                    // }

                    vk->vkGetPhysicalDeviceMemoryProperties(
                            physicalDevices[i], &physdevInfo.memoryProperties);

                    uint32_t queueFamilyPropCount = 0;

                    vk->vkGetPhysicalDeviceQueueFamilyProperties(
                            physicalDevices[i], &queueFamilyPropCount, nullptr);

                    physdevInfo.queueFamilyProperties.resize(
                            (size_t)queueFamilyPropCount);

                    vk->vkGetPhysicalDeviceQueueFamilyProperties(
                            physicalDevices[i], &queueFamilyPropCount,
                            physdevInfo.queueFamilyProperties.data());

                physicalDevices[i] = (VkPhysicalDevice)physdevInfo.boxed;
            }
        }

        return res;
    }

    void on_vkGetPhysicalDeviceFeatures(VkPhysicalDevice boxed_physicalDevice,
                                        VkPhysicalDeviceFeatures* pFeatures) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
        pFeatures->textureCompressionETC2 = true;
    }

    void on_vkGetPhysicalDeviceFeatures2(VkPhysicalDevice boxed_physicalDevice,
                                         VkPhysicalDeviceFeatures2* pFeatures) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo =
                android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];

        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
        } else {
            // No instance extension, fake it!!!!
            if (pFeatures->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceFeatures2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, 0, };
            vk->vkGetPhysicalDeviceFeatures(physicalDevice, &pFeatures->features);
        }

        pFeatures->features.textureCompressionETC2 = true;
    }

    VkResult on_vkGetPhysicalDeviceImageFormatProperties(
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkImageType type,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkImageCreateFlags flags,
            VkImageFormatProperties* pImageFormatProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        if (needEmulatedEtc2(physicalDevice, vk)) {
            CompressedImageInfo cmpInfo = createCompressedImageInfo(format);
            format = cmpInfo.dstFormat;
        }
        return vk->vkGetPhysicalDeviceImageFormatProperties(
                physicalDevice, format, type, tiling, usage, flags,
                pImageFormatProperties);
    }

    VkResult on_vkGetPhysicalDeviceImageFormatProperties2(
            VkPhysicalDevice boxed_physicalDevice,
            const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
            VkImageFormatProperties2* pImageFormatProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        VkPhysicalDeviceImageFormatInfo2 imageFormatInfo;
        if (needEmulatedEtc2(physicalDevice, vk)) {
            CompressedImageInfo cmpInfo =
                    createCompressedImageInfo(pImageFormatInfo->format);
            if (cmpInfo.isCompressed) {
                imageFormatInfo = *pImageFormatInfo;
                pImageFormatInfo = &imageFormatInfo;
                imageFormatInfo.format = cmpInfo.dstFormat;
            }
        }
        AutoLock lock(mLock);

        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            return vk->vkGetPhysicalDeviceImageFormatProperties2(
                    physicalDevice, pImageFormatInfo, pImageFormatProperties);
        } else if (hasInstanceExtension(
                           instance,
                           "VK_KHR_get_physical_device_properties2")) {
            return vk->vkGetPhysicalDeviceImageFormatProperties2KHR(
                    physicalDevice, pImageFormatInfo, pImageFormatProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pImageFormatProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceFeatures2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pImageFormatProperties = {
                    VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
                    0,
            };
            return vk->vkGetPhysicalDeviceImageFormatProperties(
                    physicalDevice, pImageFormatInfo->format,
                    pImageFormatInfo->type, pImageFormatInfo->tiling,
                    pImageFormatInfo->usage, pImageFormatInfo->flags,
                    &pImageFormatProperties->imageFormatProperties);
        }
    }

    void on_vkGetPhysicalDeviceFormatProperties(
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkFormatProperties* pFormatProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        getPhysicalDeviceFormatPropertiesCore<VkFormatProperties>(
                [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                     VkFormatProperties* pFormatProperties) {
                    vk->vkGetPhysicalDeviceFormatProperties(
                            physicalDevice, format, pFormatProperties);
                },
                vk, physicalDevice, format, pFormatProperties);
    }

    void on_vkGetPhysicalDeviceFormatProperties2(
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkFormatProperties2* pFormatProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo)
            return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];

        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties2>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                         VkFormatProperties2* pFormatProperties) {
                        vk->vkGetPhysicalDeviceFormatProperties2(
                                physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format, pFormatProperties);
        } else if (hasInstanceExtension(
                           instance,
                           "VK_KHR_get_physical_device_properties2")) {
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties2>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                         VkFormatProperties2* pFormatProperties) {
                        vk->vkGetPhysicalDeviceFormatProperties2KHR(
                                physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format, pFormatProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pFormatProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "vkGetPhysicalDeviceFormatProperties2 without having "
                        "enabled the extension!!!!11111\n",
                        __func__);
            }
            pFormatProperties->sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                         VkFormatProperties* pFormatProperties) {
                        vk->vkGetPhysicalDeviceFormatProperties(
                                physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format,
                    &pFormatProperties->formatProperties);
        }
    }

    void on_vkGetPhysicalDeviceProperties(
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceProperties* pProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceProperties(
            physicalDevice, pProperties);

        if (pProperties->apiVersion > kMaxSafeVersion) {
            pProperties->apiVersion = kMaxSafeVersion;
        }
    }

    void on_vkGetPhysicalDeviceProperties2(
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceProperties2* pProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo =
                android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];

        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceProperties2KHR(physicalDevice, pProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceProperties2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, 0, };
            vk->vkGetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);
        }

        if (pProperties->properties.apiVersion > kMaxSafeVersion) {
            pProperties->properties.apiVersion = kMaxSafeVersion;
        }
    }

    void on_vkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice boxed_physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceMemoryProperties(
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

            if (!emugl::emugl_feature_is_enabled(
                    android::featurecontrol::GLDirectMem)) {
                pMemoryProperties->memoryTypes[i].propertyFlags =
                    pMemoryProperties->memoryTypes[i].propertyFlags &
                    ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }
    }

    void on_vkGetPhysicalDeviceMemoryProperties2(
        VkPhysicalDevice boxed_physicalDevice,
        VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        auto physdevInfo =
                android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];

        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pMemoryProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceMemoryProperties2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pMemoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, 0, };
            vk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &pMemoryProperties->memoryProperties);
        }

        // Pick a max heap size that will work around
        // drivers that give bad suggestions (such as 0xFFFFFFFFFFFFFFFF for the heap size)
        // plus won't break the bank on 32-bit userspace.
        static constexpr VkDeviceSize kMaxSafeHeapSize =
            2ULL * 1024ULL * 1024ULL * 1024ULL;

        for (uint32_t i = 0; i < pMemoryProperties->memoryProperties.memoryTypeCount; ++i) {
            uint32_t heapIndex = pMemoryProperties->memoryProperties.memoryTypes[i].heapIndex;
            auto& heap = pMemoryProperties->memoryProperties.memoryHeaps[heapIndex];

            if (heap.size > kMaxSafeHeapSize) {
                heap.size = kMaxSafeHeapSize;
            }

            if (!emugl::emugl_feature_is_enabled(
                    android::featurecontrol::GLDirectMem)) {
                pMemoryProperties->memoryProperties.memoryTypes[i].propertyFlags =
                    pMemoryProperties->memoryProperties.memoryTypes[i].propertyFlags &
                    ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }
    }

    VkResult on_vkCreateDevice(VkPhysicalDevice boxed_physicalDevice,
                               const VkDeviceCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkDevice* pDevice) {
        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        std::vector<const char*> finalExts =
            filteredExtensionNames(
                pCreateInfo->enabledExtensionCount,
                pCreateInfo->ppEnabledExtensionNames);


        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
            auto extName =
                pCreateInfo->ppEnabledExtensionNames[i];
            if (!isEmulatedExtension(extName)) {
                finalExts.push_back(extName);
            }
        }

        // Run the underlying API call, filtering extensions.
        VkDeviceCreateInfo createInfoFiltered = *pCreateInfo;
        bool emulateTextureEtc2 = false;
        VkPhysicalDeviceFeatures featuresFiltered;

        if (pCreateInfo->pEnabledFeatures) {
            featuresFiltered = *pCreateInfo->pEnabledFeatures;
            if (featuresFiltered.textureCompressionETC2) {
                if (needEmulatedEtc2(physicalDevice, vk)) {
                    emulateTextureEtc2 = true;
                    featuresFiltered.textureCompressionETC2 = false;
                }
            }
            createInfoFiltered.pEnabledFeatures = &featuresFiltered;
        }

        vk_foreach_struct(ext, pCreateInfo->pNext) {
            switch (ext->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2:
                    if (needEmulatedEtc2(physicalDevice, vk)) {
                        emulateTextureEtc2 = true;
                        VkPhysicalDeviceFeatures2* features2 =
                            (VkPhysicalDeviceFeatures2*)ext;
                        features2->features.textureCompressionETC2 = false;
                    }
                    break;
                default:
                    break;
            }
        }

        createInfoFiltered.enabledExtensionCount = (uint32_t)finalExts.size();
        createInfoFiltered.ppEnabledExtensionNames = finalExts.data();

        VkResult result =
            vk->vkCreateDevice(
                physicalDevice, &createInfoFiltered, pAllocator, pDevice);

        if (result != VK_SUCCESS) return result;

        AutoLock lock(mLock);

        mDeviceToPhysicalDevice[*pDevice] = physicalDevice;

        // Fill out information about the logical device here.
        auto& deviceInfo = mDeviceInfo[*pDevice];
        deviceInfo.physicalDevice = physicalDevice;
        deviceInfo.emulateTextureEtc2 = emulateTextureEtc2;

        // First, get the dispatch table.
        auto boxed = new_boxed_VkDevice(*pDevice);
        init_vulkan_dispatch_from_device(vk, *pDevice, boxed->dispatch);
        deviceInfo.boxed = boxed;

        // Next, get information about the queue families used by this device.
        std::unordered_map<uint32_t, uint32_t> queueFamilyIndexCounts;
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            const auto& queueCreateInfo =
                pCreateInfo->pQueueCreateInfos[i];
            // Check only queues created with flags = 0 in VkDeviceQueueCreateInfo.
            auto flags = queueCreateInfo.flags;
            if (flags) continue;
            uint32_t queueFamilyIndex = queueCreateInfo.queueFamilyIndex;
            uint32_t queueCount = queueCreateInfo.queueCount;
            queueFamilyIndexCounts[queueFamilyIndex] = queueCount;
        }

        auto it = mPhysdevInfo.find(physicalDevice);

        for (auto it : queueFamilyIndexCounts) {
            auto index = it.first;
            auto count = it.second;
            auto& queues = deviceInfo.queues[index];
            for (uint32_t i = 0; i < count; ++i) {
                VkQueue queueOut;
                vk->vkGetDeviceQueue(
                    *pDevice, index, i, &queueOut);
                queues.push_back(queueOut);
                mQueueInfo[queueOut].device = *pDevice;
                mQueueInfo[queueOut].queueFamilyIndex = index;

                auto boxed = new_boxed_VkQueue(queueOut, deviceInfo.boxed->dispatch);
                mQueueInfo[queueOut].boxed = boxed;
            }
        }

        // Box the device.
        *pDevice = (VkDevice)deviceInfo.boxed;
        return VK_SUCCESS;
    }

    void on_vkGetDeviceQueue(
        VkDevice boxed_device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        VkQueue* pQueue) {

        auto device = unbox_VkDevice(boxed_device);

        AutoLock lock(mLock);

        *pQueue = VK_NULL_HANDLE;

        auto deviceInfo = android::base::find(mDeviceInfo, device);
        if (!deviceInfo) return;

        const auto& queues =
            deviceInfo->queues;

        const auto queueList =
            android::base::find(queues, queueFamilyIndex);

        if (!queueList) return;
        if (queueIndex >= queueList->size()) return;

        VkQueue unboxedQueue = (*queueList)[queueIndex];

        auto queueInfo = android::base::find(mQueueInfo, unboxedQueue);

        if (!queueInfo) return;

        *pQueue = (VkQueue)queueInfo->boxed;
    }

    void destroyDeviceLocked(VkDevice device, const VkAllocationCallbacks* pAllocator) {
        auto it = mDeviceInfo.find(device);
        if (it == mDeviceInfo.end()) return;

        auto eraseIt = mQueueInfo.begin();
        for(; eraseIt != mQueueInfo.end();) {
            if (eraseIt->second.device == device) {
                delete_boxed_VkQueue(eraseIt->second.boxed,
                                     false /* dispatch not owned */);
                eraseIt = mQueueInfo.erase(eraseIt);
            } else {
                ++eraseIt;
            }
        }

        // Run the underlying API call.
        m_vk->vkDestroyDevice(device, pAllocator);

        delete_boxed_VkDevice(it->second.boxed);
    }

    void on_vkDestroyDevice(
        VkDevice boxed_device,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);

        AutoLock lock(mLock);

        destroyDeviceLocked(device, pAllocator);

        mDeviceInfo.erase(device);
        mDeviceToPhysicalDevice.erase(device);
    }

    VkResult on_vkCreateBuffer(VkDevice boxed_device,
                               const VkBufferCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkBuffer* pBuffer) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
                vk->vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            auto& bufInfo = mBufferInfo[*pBuffer];
            bufInfo.device = device;
            bufInfo.size = pCreateInfo->size;
            bufInfo.vk = vk;
        }
        return result;
    }

    void on_vkDestroyBuffer(VkDevice boxed_device, VkBuffer buffer,
                            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyBuffer(device, buffer, pAllocator);

        AutoLock lock(mLock);
        mBufferInfo.erase(buffer);
    }

    void setBufferMemoryBindInfoLocked(
        VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        auto it = mBufferInfo.find(buffer);
        if (it == mBufferInfo.end()) {
            return;
        }
        it->second.memory = memory;
        it->second.memoryOffset = memoryOffset;
    }

    VkResult on_vkBindBufferMemory(VkDevice boxed_device,
                                   VkBuffer buffer,
                                   VkDeviceMemory memory,
                                   VkDeviceSize memoryOffset) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
                vk->vkBindBufferMemory(device, buffer, memory, memoryOffset);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            setBufferMemoryBindInfoLocked(buffer, memory, memoryOffset);
        }
        return result;
    }

    VkResult on_vkBindBufferMemory2(VkDevice boxed_device,
                                    uint32_t bindInfoCount,
                                    const VkBindBufferMemoryInfo* pBindInfos) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
                vk->vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
        
        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            for (uint32_t i = 0; i < bindInfoCount; ++i) {
                setBufferMemoryBindInfoLocked(
                    pBindInfos[i].buffer,
                    pBindInfos[i].memory,
                    pBindInfos[i].memoryOffset);
            }
        }

        return result;
    }

    VkResult on_vkBindBufferMemory2KHR(VkDevice boxed_device,
                                       uint32_t bindInfoCount,
                                       const VkBindBufferMemoryInfo* pBindInfos) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
                vk->vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
        
        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            for (uint32_t i = 0; i < bindInfoCount; ++i) {
                setBufferMemoryBindInfoLocked(
                    pBindInfos[i].buffer,
                    pBindInfos[i].memory,
                    pBindInfos[i].memoryOffset);
            }
        }

        return result;

    }

    VkResult on_vkCreateImage(
        VkDevice boxed_device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        CompressedImageInfo cmpInfo = {};
        VkImageCreateInfo localInfo;
        if (deviceInfoIt->second.emulateTextureEtc2) {
            cmpInfo = createCompressedImageInfo(
                pCreateInfo->format
            );
            if (cmpInfo.isCompressed) {
                localInfo = *pCreateInfo;
                localInfo.format = cmpInfo.dstFormat;
                pCreateInfo = &localInfo;

                cmpInfo.extent = pCreateInfo->extent;
                cmpInfo.mipLevels = pCreateInfo->mipLevels;
            }
        }

        AndroidNativeBufferInfo anbInfo;
        bool isAndroidNativeBuffer =
            parseAndroidNativeBufferInfo(pCreateInfo, &anbInfo);

        VkResult createRes = VK_SUCCESS;

        if (isAndroidNativeBuffer) {

            auto memProps = memPropsOfDeviceLocked(device);

            createRes =
                prepareAndroidNativeBufferImage(
                    vk, device, pCreateInfo, pAllocator,
                    memProps, &anbInfo);
            if (createRes == VK_SUCCESS) {
                *pImage = anbInfo.image;
            }
        } else {
            createRes =
                vk->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
        }

        if (createRes != VK_SUCCESS) return createRes;

        auto& imageInfo = mImageInfo[*pImage];
        imageInfo.anbInfo = anbInfo;
        imageInfo.cmpInfo = cmpInfo;

        return createRes;
    }

    void on_vkDestroyImage(
        VkDevice boxed_device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(image);

        if (it == mImageInfo.end()) return;

        auto info = it->second;

        if (info.anbInfo.image) {
            teardownAndroidNativeBufferImage(vk, &info.anbInfo);
        } else {
            if (info.cmpInfo.isCompressed) {
                for (const auto& buffer : info.cmpInfo.tmpBuffer) {
                    vk->vkDestroyBuffer(device, buffer, nullptr);
                }
                for (const auto& memory : info.cmpInfo.tmpMemory) {
                    vk->vkFreeMemory(device, memory, nullptr);
                }
            }
            vk->vkDestroyImage(device, image, pAllocator);
        }

        mImageInfo.erase(image);
    }

    VkResult on_vkCreateImageView(
        VkDevice boxed_device,
        const VkImageViewCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImageView* pView) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!pCreateInfo) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        AutoLock lock(mLock);
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        VkImageViewCreateInfo createInfo;
        bool needEmulatedAlpha = false;
        if (deviceInfoIt->second.emulateTextureEtc2) {
            CompressedImageInfo cmpInfo = createCompressedImageInfo(
                pCreateInfo->format
            );
            if (cmpInfo.isCompressed) {
                createInfo = *pCreateInfo;
                createInfo.format = cmpInfo.dstFormat;
                pCreateInfo = &createInfo;
                needEmulatedAlpha = cmpInfo.needEmulatedAlpha();
            }
        }
        VkResult result =
                vk->vkCreateImageView(device, pCreateInfo, pAllocator, pView);
        if (result != VK_SUCCESS) {
            return result;
        }

        auto& imageViewInfo = mImageViewInfo[*pView];
        imageViewInfo.needEmulatedAlpha = needEmulatedAlpha;
        return result;
    }

    void on_vkDestroyImageView(VkDevice boxed_device,
                               VkImageView imageView,
                               const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyImageView(device, imageView, pAllocator);
        AutoLock lock(mLock);
        mImageViewInfo.erase(imageView);
    }

    VkResult on_vkCreateSampler(VkDevice boxed_device,
                                const VkSamplerCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkSampler* pSampler) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        VkResult result =
                vk->vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        auto& samplerInfo = mSamplerInfo[*pSampler];
        samplerInfo.createInfo = *pCreateInfo;
        // We emulate RGB with RGBA for some compressed textures, which does not
        // handle translarent border correctly.
        samplerInfo.needEmulatedAlpha =
                (pCreateInfo->addressModeU ==
                         VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
                 pCreateInfo->addressModeV ==
                         VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
                 pCreateInfo->addressModeW ==
                         VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) &&
                (pCreateInfo->borderColor ==
                         VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK ||
                 pCreateInfo->borderColor ==
                         VK_BORDER_COLOR_INT_TRANSPARENT_BLACK);
        return result;
    }

    void on_vkDestroySampler(VkDevice boxed_device,
                             VkSampler sampler,
                             const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        vk->vkDestroySampler(device, sampler, pAllocator);
        AutoLock lock(mLock);
        const auto& samplerInfoIt = mSamplerInfo.find(sampler);
        if (samplerInfoIt != mSamplerInfo.end()) {
            if (samplerInfoIt->second.emulatedborderSampler != VK_NULL_HANDLE) {
                vk->vkDestroySampler(
                        device, samplerInfoIt->second.emulatedborderSampler,
                        nullptr);
            }
            mSamplerInfo.erase(samplerInfoIt);
        }
    }

    void on_vkUpdateDescriptorSets(
            VkDevice boxed_device,
            uint32_t descriptorWriteCount,
            const VkWriteDescriptorSet* pDescriptorWrites,
            uint32_t descriptorCopyCount,
            const VkCopyDescriptorSet* pDescriptorCopies) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        bool needEmulateWriteDescriptor = false;
        // c++ seems to allow for 0-size array allocation
        std::unique_ptr<bool[]> descriptorWritesNeedDeepCopy(
                new bool[descriptorWriteCount]);
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& descriptorWrite = pDescriptorWrites[i];
            descriptorWritesNeedDeepCopy[i] = false;
            if (descriptorWrite.descriptorType !=
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                continue;
            }
            for (uint32_t j = 0; j < descriptorWrite.descriptorCount; j++) {
                const VkDescriptorImageInfo& imageInfo =
                        descriptorWrite.pImageInfo[j];
                const auto& viewIt = mImageViewInfo.find(imageInfo.imageView);
                if (viewIt == mImageViewInfo.end()) {
                    continue;
                }
                const auto& samplerIt = mSamplerInfo.find(imageInfo.sampler);
                if (samplerIt == mSamplerInfo.end()) {
                    continue;
                }
                if (viewIt->second.needEmulatedAlpha &&
                    samplerIt->second.needEmulatedAlpha) {
                    needEmulateWriteDescriptor = true;
                    descriptorWritesNeedDeepCopy[i] = true;
                    break;
                }
            }
        }
        if (!needEmulateWriteDescriptor) {
            vk->vkUpdateDescriptorSets(device, descriptorWriteCount,
                                       pDescriptorWrites, descriptorCopyCount,
                                       pDescriptorCopies);
            return;
        }
        std::list<std::unique_ptr<VkDescriptorImageInfo[]>> imageInfoPool;
        std::unique_ptr<VkWriteDescriptorSet[]> descriptorWrites(
                new VkWriteDescriptorSet[descriptorWriteCount]);
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& srcDescriptorWrite =
                    pDescriptorWrites[i];
            VkWriteDescriptorSet& dstDescriptorWrite = descriptorWrites[i];
            // Shallow copy first
            dstDescriptorWrite = srcDescriptorWrite;
            if (!descriptorWritesNeedDeepCopy[i]) {
                continue;
            }
            // Deep copy
            assert(dstDescriptorWrite.descriptorType ==
                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            imageInfoPool.emplace_back(
                    new VkDescriptorImageInfo[dstDescriptorWrite
                                                      .descriptorCount]);
            VkDescriptorImageInfo* imageInfos = imageInfoPool.back().get();
            memcpy(imageInfos, srcDescriptorWrite.pImageInfo,
                   dstDescriptorWrite.descriptorCount *
                           sizeof(VkDescriptorImageInfo));
            dstDescriptorWrite.pImageInfo = imageInfos;
            for (uint32_t j = 0; j < dstDescriptorWrite.descriptorCount; j++) {
                VkDescriptorImageInfo& imageInfo = imageInfos[j];
                const auto& viewIt = mImageViewInfo.find(imageInfo.imageView);
                if (viewIt == mImageViewInfo.end()) {
                    continue;
                }
                const auto& samplerIt = mSamplerInfo.find(imageInfo.sampler);
                if (samplerIt == mSamplerInfo.end()) {
                    continue;
                }
                if (viewIt->second.needEmulatedAlpha &&
                    samplerIt->second.needEmulatedAlpha) {
                    SamplerInfo& samplerInfo = samplerIt->second;
                    if (samplerInfo.emulatedborderSampler == VK_NULL_HANDLE) {
                        // create the emulated sampler
                        VkSamplerCreateInfo createInfo = samplerInfo.createInfo;
                        switch (createInfo.borderColor) {
                            case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
                                createInfo.borderColor =
                                        VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                                break;
                            case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
                                createInfo.borderColor =
                                        VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                                break;
                            default:
                                break;
                        }
                        vk->vkCreateSampler(device, &createInfo, nullptr,
                                            &samplerInfo.emulatedborderSampler);
                    }
                    imageInfo.sampler = samplerInfo.emulatedborderSampler;
                }
            }
        }
        vk->vkUpdateDescriptorSets(device, descriptorWriteCount,
                                   descriptorWrites.get(), descriptorCopyCount,
                                   pDescriptorCopies);
    }

    void on_vkCmdCopyBufferToImage(
        VkCommandBuffer boxed_commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(dstImage);
        if (it == mImageInfo.end()) return;
        auto bufferInfoIt = mBufferInfo.find(srcBuffer);
        if (bufferInfoIt == mBufferInfo.end()) {
            return;
        }
        VkDevice device = bufferInfoIt->second.device;
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        if (!it->second.cmpInfo.isCompressed ||
            !deviceInfoIt->second.emulateTextureEtc2) {
            vk->vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage,
                    dstImageLayout, regionCount, pRegions);
            return;
        }
        auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
        if (cmdBufferInfoIt == mCmdBufferInfo.end()) {
            return;
        }
        CompressedImageInfo& cmp = it->second.cmpInfo;
        // regions and buffers for the decompressed data
        std::vector<VkBufferImageCopy> regions(regionCount);
        VkDeviceSize offset = 0;
        VkDeviceSize maxOffset = 0;
        const VkDeviceSize dstPixelSize = cmp.dstPixelSize();
        for (uint32_t r = 0; r < regionCount; r++) {
            VkBufferImageCopy& dstRegion = regions[r];
            dstRegion.bufferOffset = offset;
            dstRegion.bufferRowLength = 0;
            dstRegion.bufferImageHeight = 0;
            dstRegion.imageSubresource = pRegions[r].imageSubresource;
            dstRegion.imageOffset = pRegions[r].imageOffset;
            dstRegion.imageExtent = pRegions[r].imageExtent;
            uint32_t width =
                    cmp.mipmapWidth(dstRegion.imageSubresource.mipLevel);
            uint32_t height =
                    cmp.mipmapHeight(dstRegion.imageSubresource.mipLevel);
            uint32_t depth =
                    cmp.mipmapDepth(dstRegion.imageSubresource.mipLevel);
            dstRegion.imageExtent.width =
                    std::min(dstRegion.imageExtent.width, width);
            dstRegion.imageExtent.height =
                    std::min(dstRegion.imageExtent.height, height);
            dstRegion.imageExtent.depth =
                    std::min(dstRegion.imageExtent.depth, depth);
            offset += dstRegion.imageExtent.width *
                      dstRegion.imageExtent.height *
                      dstRegion.imageExtent.depth * dstPixelSize;
        }

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = offset;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        if (vk->vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            fprintf(stderr, "create buffer failed!\n");
            return;
        }
        cmp.tmpBuffer.push_back(buffer);

        VkMemoryRequirements memRequirements;
        vk->vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkPhysicalDevice physicalDevice = mDeviceInfo[device].physicalDevice;
        VkPhysicalDeviceMemoryProperties memProperties;
        auto ivk = mInstanceInfo[mPhysicalDeviceToInstance[physicalDevice]]
                           .boxed->dispatch;

        ivk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        uint32_t memIdx = 0;
        bool foundMemIdx = false;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memRequirements.memoryTypeBits & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & properties)
                    == properties) {
                memIdx = i;
                foundMemIdx = true;
            }
        }
        if (!foundMemIdx) {
            fprintf(stderr, "Error: cannot find memory property!\n");
            return;
        }

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memIdx;

        VkDeviceMemory memory;

        if (vk->vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            fprintf(stderr, "failed to allocate vertex buffer memory!");
            return;
        }
        cmp.tmpMemory.push_back(memory);

        vk->vkBindBufferMemory(device, buffer, memory, 0);
        cmdBufferInfoIt->second.preprocessFuncs.push_back(
                [this, cmp, srcBuffer, memory, regions,
                 srcRegions = std::vector<VkBufferImageCopy>(
                         pRegions, pRegions + regionCount)]() {
                    decompressBufferToImage(cmp, srcBuffer, memory,
                                            srcRegions.size(),
                                            srcRegions.data(), regions.data());
                });

        vk->vkCmdCopyBufferToImage(commandBuffer, buffer, dstImage, dstImageLayout, regionCount,
                regions.data());
    }

    bool mapHostVisibleMemoryToGuestPhysicalAddressLocked(
        VulkanDispatch* vk,
        VkDevice device,
        VkDeviceMemory memory,
        uint64_t physAddr) {

        if (!emugl::emugl_feature_is_enabled(
                android::featurecontrol::GLDirectMem)) {
            emugl::emugl_crash_reporter(
                "FATAL: Tried to use direct mapping "
                "while GLDirectMem is not enabled!");
        }

        auto info = android::base::find(mMapInfo, memory);

        if (!info) return false;

        info->guestPhysAddr = physAddr;

        constexpr size_t PAGE_BITS = 12;
        constexpr size_t PAGE_SIZE = 1u << PAGE_BITS;
        constexpr size_t PAGE_OFFSET_MASK = PAGE_SIZE - 1;

        uintptr_t addr = reinterpret_cast<uintptr_t>(info->ptr);
        uintptr_t pageOffset = addr & PAGE_OFFSET_MASK;

        info->pageAlignedHva =
            reinterpret_cast<void*>(addr - pageOffset);
        info->sizeToPage =
            ((info->size + pageOffset + PAGE_SIZE - 1) >>
                 PAGE_BITS) << PAGE_BITS;

        printf("%s: map: %p -> [0x%llx 0x%llx]\n", __func__,
               info->pageAlignedHva,
               (unsigned long long)info->guestPhysAddr,
               (unsigned long long)info->guestPhysAddr + info->sizeToPage);
        get_emugl_vm_operations().mapUserBackedRam(
            info->guestPhysAddr,
            info->pageAlignedHva,
            info->sizeToPage);

        info->directMapped = true;

        return true;
    }

    VkResult on_vkAllocateMemory(
        VkDevice boxed_device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!pAllocateInfo) return VK_ERROR_INITIALIZATION_FAILED;

        VkMemoryAllocateInfo allocInfo = *pAllocateInfo;

        vk_struct_common* structChain =
            vk_init_struct_chain((vk_struct_common*)(&allocInfo));

        VkExportMemoryAllocateInfo exportAllocInfo;
        VkImportColorBufferGOOGLE importCbInfo;
        VkImportPhysicalAddressGOOGLE importPhysAddrInfo;
        VkMemoryDedicatedAllocateInfo dedicatedAllocInfo;

        // handle type should already be converted in unmarshaling
        VkExportMemoryAllocateInfo* exportAllocInfoPtr =
            (VkExportMemoryAllocateInfo*)
                vk_find_struct((vk_struct_common*)pAllocateInfo,
                    VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO);

        if (exportAllocInfoPtr) {
            fprintf(stderr,
                    "%s: Fatal: Export allocs are to be handled "
                    "on the guest side / VkCommonOperations.\n",
                    __func__);
            abort();
        }

        VkImportColorBufferGOOGLE* importCbInfoPtr =
            (VkImportColorBufferGOOGLE*)
                vk_find_struct((vk_struct_common*)pAllocateInfo,
                    VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE);

        if (importCbInfoPtr) {
            // TODO: Implement what happens on importing a color bufer:
            // - setup Vk for the color buffer
            // - associate device memory with the color buffer
        }

        VkImportPhysicalAddressGOOGLE* importPhysAddrInfoPtr =
            (VkImportPhysicalAddressGOOGLE*)
                vk_find_struct((vk_struct_common*)pAllocateInfo,
                    VK_STRUCTURE_TYPE_IMPORT_PHYSICAL_ADDRESS_GOOGLE);

        if (importPhysAddrInfoPtr) {
            // TODO: Implement what happens on importing a physical address:
            // 1 - perform action of vkMapMemoryIntoAddressSpaceGOOGLE if
            //     host visible
            // 2 - create color buffer, setup Vk for it,
            //     and associate it with the physical address
        }

        VkMemoryDedicatedAllocateInfo* dedicatedAllocInfoPtr =
            (VkMemoryDedicatedAllocateInfo*)
                vk_find_struct((vk_struct_common*)pAllocateInfo,
                VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);

        if (dedicatedAllocInfoPtr) {
            dedicatedAllocInfo = *dedicatedAllocInfoPtr;
            structChain =
                vk_append_struct(
                    (vk_struct_common*)structChain,
                    (vk_struct_common*)&dedicatedAllocInfo);
        }

        // TODO: Import the corresponding host-side external memory handle.
#ifdef _WIN32
        VkImportMemoryWin32HandleInfoKHR importWin32HandleInfo;
        (void)importWin32HandleInfo;
#else
        VkImportMemoryFdInfoKHR importFdInfo;
        (void)importFdInfo;
#endif

        VkResult result =
            vk->vkAllocateMemory(device, &allocInfo, pAllocator, pMemory);

        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);

        auto physdev = android::base::find(mDeviceToPhysicalDevice, device);

        if (!physdev) {
            // User app gave an invalid VkDevice,
            // but we don't really want to crash here.
            // We should allow invalid apps.
            return VK_ERROR_DEVICE_LOST;
        }

        auto physdevInfo =
                android::base::find(mPhysdevInfo, *physdev);

        if (!physdevInfo) {
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
        if (allocInfo.memoryTypeIndex >=
            physdevInfo->memoryProperties.memoryTypeCount) {
            // Continue allowing invalid behavior.
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }

        mMapInfo[*pMemory] = MappedMemoryInfo();
        auto& mapInfo = mMapInfo[*pMemory];
        mapInfo.size = allocInfo.allocationSize;
        mapInfo.device = device;

        VkMemoryPropertyFlags flags =
                physdevInfo->
                    memoryProperties
                        .memoryTypes[allocInfo.memoryTypeIndex]
                        .propertyFlags;

        bool hostVisible =
            flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        if (!hostVisible) return result;

        VkResult mapResult =
            vk->vkMapMemory(device, *pMemory, 0,
                            mapInfo.size, 0, &mapInfo.ptr);

        if (mapResult != VK_SUCCESS) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        return result;
    }

    void freeMemoryLocked(
        VulkanDispatch* vk,
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        auto info = android::base::find(mMapInfo, memory);

        if (!info) {
            // Invalid usage.
            return;
        }

        if (info->directMapped) {
            printf("%s: unmap: [0x%llx 0x%llx]\n", __func__,
                   (unsigned long long)info->guestPhysAddr,
                   (unsigned long long)info->guestPhysAddr + info->sizeToPage);
            get_emugl_vm_operations().unmapUserBackedRam(
                info->guestPhysAddr,
                info->sizeToPage);
        }

        if (info->ptr) {
            vk->vkUnmapMemory(device, memory);
        }

        vk->vkFreeMemory(device, memory, pAllocator);
    }

    void on_vkFreeMemory(
        VkDevice boxed_device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        freeMemoryLocked(vk, device, memory, pAllocator);

        mMapInfo.erase(memory);
    }

    VkResult on_vkMapMemory(VkDevice,
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

    void on_vkUnmapMemory(VkDevice, VkDeviceMemory) {
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

    bool usingDirectMapping() const {
        return emugl::emugl_feature_is_enabled(
                android::featurecontrol::GLDirectMem);
    }

    HostFeatureSupport getHostFeatureSupport() const {
        HostFeatureSupport res;

        if (!m_vk) return res;

        auto emu = createOrGetGlobalVkEmulation(m_vk);

        res.supportsVulkan = emu && emu->live;

        if (!res.supportsVulkan) return res;

        const auto& props = emu->deviceInfo.physdevProps;

        res.supportsVulkan1_1 =
            props.apiVersion >= VK_API_VERSION_1_1;
        res.supportsExternalMemory =
            emu->deviceInfo.supportsExternalMemory;

        res.apiVersion = props.apiVersion;
        res.driverVersion = props.driverVersion;
        res.deviceID = props.deviceID;
        res.vendorID = props.vendorID;
        return res;
    }

    bool hasInstanceExtension(VkInstance instance, const std::string& name) {
        auto info = android::base::find(mInstanceInfo, instance);
        if (!info) return false;

        for (const auto& enabledName : info->enabledExtensionNames) {
            if (name == enabledName) return true;
        }

        return false;
    }

    // VK_ANDROID_native_buffer
    VkResult on_vkGetSwapchainGrallocUsageANDROID(
        VkDevice,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        int* grallocUsage) {
        getGralloc0Usage(format, imageUsage, grallocUsage);
        return VK_SUCCESS;
    }

    VkResult on_vkGetSwapchainGrallocUsage2ANDROID(
        VkDevice,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
        uint64_t* grallocConsumerUsage,
        uint64_t* grallocProducerUsage) {
        getGralloc1Usage(format, imageUsage, swapchainImageUsage,
                         grallocConsumerUsage,
                         grallocProducerUsage);
        return VK_SUCCESS;
    }

    VkResult on_vkAcquireImageANDROID(
        VkDevice boxed_device,
        VkImage image,
        int nativeFenceFd,
        VkSemaphore semaphore,
        VkFence fence) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto imageInfo = android::base::find(mImageInfo, image);
        if (!imageInfo) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        VkQueue defaultQueue;
        uint32_t defaultQueueFamilyIndex;
        if (!getDefaultQueueForDeviceLocked(
                device, &defaultQueue, &defaultQueueFamilyIndex)) {
                    fprintf(stderr, "%s: cant get the default q\n", __func__);
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        AndroidNativeBufferInfo* anbInfo = &imageInfo->anbInfo;

        return
            setAndroidNativeImageSemaphoreSignaled(
                vk, device,
                defaultQueue, defaultQueueFamilyIndex,
                semaphore, fence, anbInfo);
    }

    VkResult on_vkQueueSignalReleaseImageANDROID(
        VkQueue boxed_queue,
        uint32_t waitSemaphoreCount,
        const VkSemaphore* pWaitSemaphores,
        VkImage image,
        int* pNativeFenceFd) {

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        AutoLock lock(mLock);

        auto queueFamilyIndex = queueFamilyIndexOfQueueLocked(queue);

        if (!queueFamilyIndex) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto imageInfo = android::base::find(mImageInfo, image);
        AndroidNativeBufferInfo* anbInfo = &imageInfo->anbInfo;

        return
            syncImageToColorBuffer(
                vk,
                *queueFamilyIndex,
                queue,
                waitSemaphoreCount, pWaitSemaphores,
                pNativeFenceFd, anbInfo);
    }

    VkResult on_vkMapMemoryIntoAddressSpaceGOOGLE(
        VkDevice boxed_device, VkDeviceMemory memory, uint64_t* pAddress) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!emugl::emugl_feature_is_enabled(
                android::featurecontrol::GLDirectMem)) {
            emugl::emugl_crash_reporter(
                "FATAL: Tried to use direct mapping "
                "while GLDirectMem is not enabled!");
        }

        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

        if (!mapHostVisibleMemoryToGuestPhysicalAddressLocked(
            vk, device, memory, *pAddress)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if (!info) return VK_ERROR_INITIALIZATION_FAILED;

        *pAddress = (uint64_t)(uintptr_t)info->ptr;

        return VK_SUCCESS;
    }

    VkResult on_vkRegisterImageColorBufferGOOGLE(
       VkDevice device, VkImage image, uint32_t colorBuffer) {

        (void)image;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkRegisterBufferColorBufferGOOGLE(
       VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {

        (void)buffer;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkAllocateCommandBuffers(
            VkDevice boxed_device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result = vk->vkAllocateCommandBuffers(
            device, pAllocateInfo, pCommandBuffers);

        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);
        for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
            mCmdBufferInfo[pCommandBuffers[i]] = CommandBufferInfo();
            mCmdBufferInfo[pCommandBuffers[i]].cmdPool =
                    pAllocateInfo->commandPool;
            auto boxed = new_boxed_VkCommandBuffer(pCommandBuffers[i], vk);
            mCmdBufferInfo[pCommandBuffers[i]].boxed = boxed;
            pCommandBuffers[i] = (VkCommandBuffer)boxed;
        }
        return result;
    }

    VkResult on_vkCreateCommandPool(VkDevice boxed_device,
                                    const VkCommandPoolCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkCommandPool* pCommandPool) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result = vk->vkCreateCommandPool(device, pCreateInfo,
                                                    pAllocator, pCommandPool);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        mCmdPoolInfo[*pCommandPool] = CommandPoolInfo();

        return result;
    }

    void on_vkDestroyCommandPool(VkDevice boxed_device,
                                 VkCommandPool commandPool,
                                 const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyCommandPool(device, commandPool, pAllocator);
        AutoLock lock(mLock);
        const auto ite = mCmdPoolInfo.find(commandPool);
        if (ite != mCmdPoolInfo.end()) {
            removeCommandBufferInfo(ite->second.cmdBuffers);
            mCmdPoolInfo.erase(ite);
        }
    }

    VkResult on_vkResetCommandPool(VkDevice boxed_device,
                                   VkCommandPool commandPool,
                                   VkCommandPoolResetFlags flags) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
                vk->vkResetCommandPool(device, commandPool, flags);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        const auto ite = mCmdPoolInfo.find(commandPool);
        if (ite != mCmdPoolInfo.end()) {
            removeCommandBufferInfo(ite->second.cmdBuffers);
        }
        return result;
    }

    void on_vkCmdExecuteCommands(VkCommandBuffer boxed_commandBuffer,
                                 uint32_t commandBufferCount,
                                 const VkCommandBuffer* pCommandBuffers) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        vk->vkCmdExecuteCommands(commandBuffer, commandBufferCount,
                                      pCommandBuffers);
        AutoLock lock(mLock);
        CommandBufferInfo& cmdBuffer = mCmdBufferInfo[commandBuffer];
        cmdBuffer.subCmds.insert(cmdBuffer.subCmds.end(),
                pCommandBuffers, pCommandBuffers + commandBufferCount);
    }

    VkResult on_vkQueueSubmit(VkQueue boxed_queue,
                          uint32_t submitCount,
                          const VkSubmitInfo* pSubmits,
                          VkFence fence) {

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        AutoLock lock(mLock);

        for (uint32_t i = 0; i < submitCount; i++) {
            const VkSubmitInfo& submit = pSubmits[i];
            for (uint32_t c = 0; c < submit.commandBufferCount; c++) {
                executePreprocessRecursive(0, submit.pCommandBuffers[c]);
            }
        }
        return vk->vkQueueSubmit(queue, submitCount, pSubmits, fence);
    }

    VkResult on_vkResetCommandBuffer(VkCommandBuffer boxed_commandBuffer,
                                     VkCommandBufferResetFlags flags) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        VkResult result = vk->vkResetCommandBuffer(commandBuffer, flags);
        if (VK_SUCCESS == result) {
            AutoLock lock(mLock);
            auto boxed = mCmdBufferInfo[commandBuffer].boxed;
            mCmdBufferInfo[commandBuffer] = {};
            mCmdBufferInfo[commandBuffer].boxed = boxed;
        }
        return result;
    }

    void on_vkFreeCommandBuffers(VkDevice boxed_device,
                                 VkCommandPool commandPool,
                                 uint32_t commandBufferCount,
                                 const VkCommandBuffer* pCommandBuffers) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!device) return;
        vk->vkFreeCommandBuffers(device, commandPool, commandBufferCount,
                                   pCommandBuffers);
        AutoLock lock(mLock);
        for (uint32_t i = 0; i < commandBufferCount; i++) {
            const auto& cmdBufferInfoIt =
                    mCmdBufferInfo.find(pCommandBuffers[i]);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                const auto& cmdPoolInfoIt =
                        mCmdPoolInfo.find(cmdBufferInfoIt->second.cmdPool);
                if (cmdPoolInfoIt != mCmdPoolInfo.end()) {
                    cmdPoolInfoIt->second.cmdBuffers.erase(pCommandBuffers[i]);
                }
                delete_boxed_VkCommandBuffer(cmdBufferInfoIt->second.boxed,
                                             false /* dispatch not owned*/);
                mCmdBufferInfo.erase(cmdBufferInfoIt);
            }
        }
    }

    // TODO: Support more than one kind of guest external memory handle type
#define GUEST_EXTERNAL_MEMORY_HANDLE_TYPE VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID

    // Transforms

    void transformImpl_VkExternalMemoryProperties_tohost(
        const VkExternalMemoryProperties* props, uint32_t count) {
        VkExternalMemoryProperties* mut =
            (VkExternalMemoryProperties*)props;
        for (uint32_t i = 0; i < count; ++i) {
            mut[i] = transformExternalMemoryProperties_tohost(mut[i]);
        }
    }
    void transformImpl_VkExternalMemoryProperties_fromhost(
        const VkExternalMemoryProperties* props, uint32_t count) {
        VkExternalMemoryProperties* mut =
            (VkExternalMemoryProperties*)props;
        for (uint32_t i = 0; i < count; ++i) {
            mut[i] = transformExternalMemoryProperties_fromhost(mut[i], GUEST_EXTERNAL_MEMORY_HANDLE_TYPE);
        }
    }

#define DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(type, field) \
    void transformImpl_##type##_tohost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].field = (VkExternalMemoryHandleTypeFlagBits) \
                transformExternalMemoryHandleTypeFlags_tohost( \
                    mut[i].field); \
        } \
    } \
    void transformImpl_##type##_fromhost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].field = (VkExternalMemoryHandleTypeFlagBits) \
                transformExternalMemoryHandleTypeFlags_fromhost( \
                    mut[i].field, GUEST_EXTERNAL_MEMORY_HANDLE_TYPE); \
        } \
    } \

#define DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(type) \
    void transformImpl_##type##_tohost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].externalMemoryProperties = transformExternalMemoryProperties_tohost( \
                    mut[i].externalMemoryProperties); \
        } \
    } \
    void transformImpl_##type##_fromhost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].externalMemoryProperties = transformExternalMemoryProperties_fromhost( \
                    mut[i].externalMemoryProperties, GUEST_EXTERNAL_MEMORY_HANDLE_TYPE); \
        } \
    } \

    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkPhysicalDeviceExternalImageFormatInfo, handleType)
    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkPhysicalDeviceExternalBufferInfo, handleType)
    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExternalMemoryImageCreateInfo, handleTypes)
    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExternalMemoryBufferCreateInfo, handleTypes)
    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExportMemoryAllocateInfo, handleTypes)
    DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(VkExternalImageFormatProperties)
    DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(VkExternalBufferProperties)

private:
    bool isEmulatedExtension(const char* name) const {
        for (auto emulatedExt : kEmulatedExtensions) {
            if (!strcmp(emulatedExt, name)) return true;
        }
        return false;
    }

    std::vector<const char*>
    filteredExtensionNames(
            uint32_t count, const char* const* extNames) {
        std::vector<const char*> res;
        for (uint32_t i = 0; i < count; ++i) {
            auto extName = extNames[i];
            if (!isEmulatedExtension(extName)) {
                res.push_back(extName);
            }
        }
        return res;
    }

    VkPhysicalDeviceMemoryProperties* memPropsOfDeviceLocked(VkDevice device) {
        auto physdev = android::base::find(mDeviceToPhysicalDevice, device);
        if (!physdev) return nullptr;

        auto physdevInfo = android::base::find(mPhysdevInfo, *physdev);
        if (!physdevInfo) return nullptr;

        return &physdevInfo->memoryProperties;
    }

    Optional<uint32_t> queueFamilyIndexOfQueueLocked(VkQueue queue) {
        auto info = android::base::find(mQueueInfo, queue);
        if (!info) return {};

        return info->queueFamilyIndex;
    }

    bool getDefaultQueueForDeviceLocked(
        VkDevice device, VkQueue* queue, uint32_t* queueFamilyIndex) {

        auto deviceInfo = android::base::find(mDeviceInfo, device);
        if (!deviceInfo) return false;

        auto zeroIt = deviceInfo->queues.find(0);
        if (zeroIt == deviceInfo->queues.end() ||
            zeroIt->second.size() == 0) {
            // Get the first queue / queueFamilyIndex
            // that does show up.
            for (auto it : deviceInfo->queues) {
                auto index = it.first;
                for (auto deviceQueue : it.second) {
                    *queue = deviceQueue;
                    *queueFamilyIndex = index;
                    return true;
                }
            }
            // Didn't find anything, fail.
            return false;
        } else {
            // Use queue family index 0.
            *queue = zeroIt->second[0];
            *queueFamilyIndex = 0;
            return true;
        }

        return false;
    }

    struct CompressedImageInfo {
        bool isCompressed = false;
        VkFormat srcFormat;
        VkFormat dstFormat = VK_FORMAT_R8G8B8A8_UNORM;
        std::vector<VkBuffer> tmpBuffer = {};
        std::vector<VkDeviceMemory> tmpMemory = {};
        VkExtent3D extent;
        uint32_t mipLevels = 1;
        uint32_t mipmapWidth(uint32_t level) {
            return std::max<uint32_t>(extent.width >> level, 1);
        }
        uint32_t mipmapHeight(uint32_t level) {
            return std::max<uint32_t>(extent.height >> level, 1);
        }
        uint32_t mipmapDepth(uint32_t level) {
            return std::max<uint32_t>(extent.depth >> level, 1);
        }
        uint32_t alignSize(uint32_t inputSize) {
            if (isCompressed) {
                return (inputSize + 3) & (~0x3);
            } else {
                return inputSize;
            }
        }
        VkDeviceSize dstPixelSize() {
            return getLinearFormatPixelSize(dstFormat);
        }
        bool needEmulatedAlpha() {
            if (!isCompressed) {
                return false;
            }
            switch (srcFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                    return true;
                default:
                    return false;
            }
        }
    };

    static bool needEmulatedEtc2(VkPhysicalDevice physicalDevice,
                                 goldfish_vk::VulkanDispatch* vk) {
        VkPhysicalDeviceFeatures feature;
        vk->vkGetPhysicalDeviceFeatures(physicalDevice, &feature);
        return !feature.textureCompressionETC2;
    }

    static ETC2ImageFormat getEtc2Format(VkFormat fmt) {
        switch (fmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                return EtcRGB8;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return EtcRGBA8;
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                return EtcRGB8A1;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                return EtcR11;
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return EtcSignedR11;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                return EtcRG11;
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return EtcSignedRG11;
            default:
                fprintf(stderr,
                        "TODO: unsupported compressed texture format %d\n",
                        fmt);
                return EtcRGB8;
        }
    }

    CompressedImageInfo createCompressedImageInfo(VkFormat srcFmt) {
        CompressedImageInfo cmpInfo;
        cmpInfo.srcFormat = srcFmt;
        cmpInfo.isCompressed = true;
        switch (srcFmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R32_SFLOAT;
                break;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                cmpInfo.dstFormat = VK_FORMAT_R32G32_SFLOAT;
                break;
            default:
                cmpInfo.isCompressed = false;
                cmpInfo.dstFormat = srcFmt;
                break;
        }
        return cmpInfo;
    }

    static const VkFormatFeatureFlags kEmulatedEtc2BufferFeatureMask =
            VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
            VK_FORMAT_FEATURE_BLIT_SRC_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

    void maskFormatPropertiesForEmulatedEtc2(
            VkFormatProperties* pFormatProperties) {
        pFormatProperties->bufferFeatures &= kEmulatedEtc2BufferFeatureMask;
    }

    void maskFormatPropertiesForEmulatedEtc2(
            VkFormatProperties2* pFormatProperties) {
        pFormatProperties->formatProperties.bufferFeatures &=
                kEmulatedEtc2BufferFeatureMask;
    }

    template <class VkFormatProperties1or2>
    void getPhysicalDeviceFormatPropertiesCore(
            std::function<
                    void(VkPhysicalDevice, VkFormat, VkFormatProperties1or2*)>
                    getPhysicalDeviceFormatPropertiesFunc,
            goldfish_vk::VulkanDispatch* vk,
            VkPhysicalDevice physicalDevice,
            VkFormat format,
            VkFormatProperties1or2* pFormatProperties) {
        switch (format) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: {
                if (!needEmulatedEtc2(physicalDevice, vk)) {
                    // Hardware supported ETC2
                    getPhysicalDeviceFormatPropertiesFunc(
                            physicalDevice, format, pFormatProperties);
                    return;
                }
                // Emulate ETC formats
                CompressedImageInfo cmpInfo = createCompressedImageInfo(format);
                getPhysicalDeviceFormatPropertiesFunc(
                        physicalDevice, cmpInfo.dstFormat, pFormatProperties);
                maskFormatPropertiesForEmulatedEtc2(pFormatProperties);
                break;
            }
            default:
                getPhysicalDeviceFormatPropertiesFunc(physicalDevice, format,
                                                      pFormatProperties);
                break;
        }
    }

    void decompressBufferToImage(CompressedImageInfo cmp,
                                 VkBuffer srcBuffer,
                                 VkDeviceMemory dstMemory,
                                 uint32_t regionCount,
                                 const VkBufferImageCopy* srcRegions,
                                 const VkBufferImageCopy* dstRegions) {
        ETC2ImageFormat imgFmt = getEtc2Format(cmp.srcFormat);
        // regions and buffers for the decompressed data
        int decodedPixelSize = etc_get_decoded_pixel_size(imgFmt);

        auto srcBufferInfoIt = mBufferInfo.find(srcBuffer);
        if (srcBufferInfoIt == mBufferInfo.end()) {
            return;
        }
        const auto& srcBufferInfo = srcBufferInfoIt->second;
        VkDevice device = srcBufferInfo.device;

        uint8_t* srcRawData;
        uint8_t* dstRawData;
        if (VK_SUCCESS !=
            m_vk->vkMapMemory(device, srcBufferInfo.memory,
                              srcBufferInfo.memoryOffset,
                              srcBufferInfo.size, 0,
                              (void**)&srcRawData)) {
            fprintf(stderr,
                    "WARNING: map memory failure. Textures might be broken\n");
        }
        m_vk->vkMapMemory(device, dstMemory, 0, VK_WHOLE_SIZE, 0,
                          (void**)&dstRawData);

        // The software decompression work
        std::vector<uint8_t> decompBuffer;
        for (uint32_t r = 0; r < regionCount; r++) {
            const VkBufferImageCopy& dstRegion = dstRegions[r];
            const VkBufferImageCopy& srcRegion = srcRegions[r];

            const uint8_t* srcPtr = srcRawData + srcRegion.bufferOffset;
            uint8_t* dstPtr = dstRawData + dstRegion.bufferOffset;
            assert(srcRegion.bufferOffset + etc_get_encoded_data_size(imgFmt,
                srcRegion.imageExtent.width, srcRegion.imageExtent.height)
                <= srcBufferInfo.size);
            VkExtent3D alignedSrcImgExtent;
            alignedSrcImgExtent.width =
                    cmp.alignSize(srcRegion.imageExtent.width);
            alignedSrcImgExtent.height =
                    cmp.alignSize(srcRegion.imageExtent.height);
            // not used
            alignedSrcImgExtent.depth = srcRegion.imageExtent.depth;

            decompBuffer.resize(alignedSrcImgExtent.width *
                                alignedSrcImgExtent.height * decodedPixelSize);
            for (uint32_t d = 0; d < dstRegion.imageExtent.depth; d++) {
                int err = etc2_decode_image(
                        srcPtr + d * etc_get_encoded_data_size(
                                             imgFmt, alignedSrcImgExtent.width,
                                             alignedSrcImgExtent.height),
                        imgFmt, decompBuffer.data(), alignedSrcImgExtent.width,
                        alignedSrcImgExtent.height,
                        decodedPixelSize * alignedSrcImgExtent.width);
                int dstPixelSize = getLinearFormatPixelSize(cmp.dstFormat);
                for (int h = 0; h < dstRegion.imageExtent.height; h++) {
                    for (int w = 0; w < dstRegion.imageExtent.width; w++) {
                        // RGB to RGBA
                        const uint8_t* srcPixel =
                                decompBuffer.data() +
                                decodedPixelSize *
                                        (w + h * alignedSrcImgExtent.width);
                        uint8_t* dstPixel =
                                dstPtr +
                                dstPixelSize *
                                        (w +
                                         (h +
                                          d * dstRegion.imageExtent.height) *
                                                 dstRegion.imageExtent.width);
                        // In case the source is not an RGBA format, we set all
                        // channels to default values (except for R channel)
                        switch (cmp.srcFormat) {
                            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                                dstPixel[3] = 255;
                                memcpy(dstPixel, srcPixel, decodedPixelSize);
                                break;
                            default:
                                assert(dstPixelSize == decodedPixelSize);
                                memcpy(dstPixel, srcPixel, decodedPixelSize);
                        }
                    }
                }
            }
        }
        m_vk->vkUnmapMemory(device, srcBufferInfo.memory);
        m_vk->vkUnmapMemory(device, dstMemory);
    }

    void executePreprocessRecursive(int level, VkCommandBuffer cmdBuffer) {
        auto cmdBufferIt = mCmdBufferInfo.find(cmdBuffer);
        if (cmdBufferIt == mCmdBufferInfo.end()) {
            return;
        }
        for (const auto& func : cmdBufferIt->second.preprocessFuncs) {
            func();
        }
        // TODO: fix
        // for (const auto& subCmd : cmdBufferIt->second.subCmds) {
            // executePreprocessRecursive(level + 1, subCmd);
        // }
    }

    void teardownInstanceLocked(VkInstance instance) {

        std::vector<VkDevice> devicesToDestroy;
        std::vector<VulkanDispatch*> devicesToDestroyDispatches;

        for (auto it : mDeviceToPhysicalDevice) {
            auto otherInstance = android::base::find(mPhysicalDeviceToInstance, it.second);
            if (!otherInstance) continue;

            if (instance == *otherInstance) {
                devicesToDestroy.push_back(it.first);
                devicesToDestroyDispatches.push_back(mDeviceInfo[it.first].boxed->dispatch);
            }
        }

        for (uint32_t i = 0; i < devicesToDestroy.size(); ++i) {
            auto it = mMapInfo.begin();
            while (it != mMapInfo.end()) {
                if (it->second.device == devicesToDestroy[i]) {
                    auto mem = it->first;
                    freeMemoryLocked(devicesToDestroyDispatches[i],
                        devicesToDestroy[i],
                        mem, nullptr);
                    it = mMapInfo.erase(it);
                } else {
                    ++it;
                }
            }
        }

        for (uint32_t i = 0; i < devicesToDestroy.size(); ++i) {
            destroyDeviceLocked(devicesToDestroy[i], nullptr);
            mDeviceInfo.erase(devicesToDestroy[i]);
            mDeviceToPhysicalDevice.erase(devicesToDestroy[i]);
        }
    }

    typedef std::function<void()> PreprocessFunc;
    struct CommandBufferInfo {
        std::vector<PreprocessFunc> preprocessFuncs = {};
        std::vector<VkCommandBuffer> subCmds = {};
        VkCommandPool cmdPool = nullptr;
        BoxedDispatchable_VkCommandBuffer* boxed = nullptr;
    };

    struct CommandPoolInfo {
        std::unordered_set<VkCommandBuffer> cmdBuffers = {};
    };

    void removeCommandBufferInfo(
            const std::unordered_set<VkCommandBuffer>& cmdBuffers) {
        for (const auto& cmdBuffer : cmdBuffers) {
            mCmdBufferInfo.erase(cmdBuffer);
        }
    }

    VulkanDispatch* m_vk;
    VkEmulation* m_emu;

    Lock mLock;

    // We always map the whole size on host.
    // This makes it much easier to implement
    // the memory map API.
    struct MappedMemoryInfo {
        // When ptr is null, it means the VkDeviceMemory object
        // was not allocated with the HOST_VISIBLE property.
        void* ptr = nullptr;
        VkDeviceSize size;
        // GLDirectMem info
        bool directMapped = false;
        uint64_t guestPhysAddr = 0;
        void* pageAlignedHva = nullptr;
        uint64_t sizeToPage = 0;
        VkDevice device = VK_NULL_HANDLE;
    };

    struct InstanceInfo {
        std::vector<std::string> enabledExtensionNames;
        BoxedDispatchable_VkInstance* boxed = nullptr;
    };

    struct PhysicalDeviceInfo {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        BoxedDispatchable_VkPhysicalDevice* boxed = nullptr;
    };

    struct DeviceInfo {
        std::unordered_map<uint32_t, std::vector<VkQueue>> queues;
        bool emulateTextureEtc2 = false;
        VkPhysicalDevice physicalDevice;
        BoxedDispatchable_VkDevice* boxed = nullptr;
    };

    struct QueueInfo {
        VkDevice device;
        uint32_t queueFamilyIndex;
        BoxedDispatchable_VkQueue* boxed = nullptr;
    };

    struct BufferInfo {
        VkDevice device;
        VkDeviceMemory memory = 0;
        VkDeviceSize memoryOffset = 0;
        VkDeviceSize size;
        // For compressed texture emulation
        VulkanDispatch* vk = nullptr;
    };

    struct ImageInfo {
        AndroidNativeBufferInfo anbInfo;
        CompressedImageInfo cmpInfo;
    };

    struct ImageViewInfo {
        bool needEmulatedAlpha = false;
    };

    struct SamplerInfo {
        bool needEmulatedAlpha = false;
        VkSamplerCreateInfo createInfo = {};
        VkSampler emulatedborderSampler = VK_NULL_HANDLE;
    };

    std::unordered_map<VkInstance, InstanceInfo>
        mInstanceInfo;
    std::unordered_map<VkPhysicalDevice, PhysicalDeviceInfo>
        mPhysdevInfo;
    std::unordered_map<VkDevice, DeviceInfo>
        mDeviceInfo;
    std::unordered_map<VkImage, ImageInfo>
        mImageInfo;
    std::unordered_map<VkImageView, ImageViewInfo> mImageViewInfo;
    std::unordered_map<VkSampler, SamplerInfo> mSamplerInfo;
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> mCmdBufferInfo;
    std::unordered_map<VkCommandPool, CommandPoolInfo> mCmdPoolInfo;
    // TODO: release CommandBufferInfo when a command pool is reset/released

    // Back-reference to the physical device associated with a particular
    // VkDevice, and the VkDevice corresponding to a VkQueue.
    std::unordered_map<VkDevice, VkPhysicalDevice> mDeviceToPhysicalDevice;
    std::unordered_map<VkPhysicalDevice, VkInstance> mPhysicalDeviceToInstance;

    std::unordered_map<VkQueue, QueueInfo> mQueueInfo;
    std::unordered_map<VkBuffer, BufferInfo> mBufferInfo;

    std::unordered_map<VkDeviceMemory, MappedMemoryInfo> mMapInfo;
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

VkResult VkDecoderGlobalState::on_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    return mImpl->on_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void VkDecoderGlobalState::on_vkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyInstance(instance, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* physicalDeviceCount,
    VkPhysicalDevice* physicalDevices) {
    return mImpl->on_vkEnumeratePhysicalDevices(instance, physicalDeviceCount, physicalDevices);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceFeatures* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceFeatures2* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2KHR(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceFeatures2KHR* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties(
        VkPhysicalDevice physicalDevice,
        VkFormat format,
        VkImageType type,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkImageCreateFlags flags,
        VkImageFormatProperties* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties(
            physicalDevice, format, type, tiling, usage, flags,
            pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2(
        VkPhysicalDevice physicalDevice,
        const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
        VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2KHR(
        VkPhysicalDevice physicalDevice,
        const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
        VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties(
        VkPhysicalDevice physicalDevice,
        VkFormat format,
        VkFormatProperties* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties(physicalDevice, format,
                                                  pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2(
        VkPhysicalDevice physicalDevice,
        VkFormat format,
        VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2KHR(
        VkPhysicalDevice physicalDevice,
        VkFormat format,
        VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2KHR(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties(
        physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2KHR(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        physicalDevice, pMemoryProperties);
}

VkResult VkDecoderGlobalState::on_vkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {
    return mImpl->on_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void VkDecoderGlobalState::on_vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    mImpl->on_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

void VkDecoderGlobalState::on_vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDevice(device, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateBuffer(
        VkDevice device,
        const VkBufferCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkBuffer* pBuffer) {
    return mImpl->on_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

void VkDecoderGlobalState::on_vkDestroyBuffer(
    VkDevice device,
    VkBuffer buffer,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyBuffer(device, buffer, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory(
        VkDevice device,
        VkBuffer buffer,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset) {
    return mImpl->on_vkBindBufferMemory(device, buffer, memory, memoryOffset);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2(VkDevice device,
                                uint32_t bindInfoCount,
                                const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2(device, bindInfoCount, pBindInfos);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2KHR(VkDevice device,
                                   uint32_t bindInfoCount,
                                   const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
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

VkResult VkDecoderGlobalState::on_vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView) {
    return mImpl->on_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
}

void VkDecoderGlobalState::on_vkDestroyImageView(
        VkDevice device,
        VkImageView imageView,
        const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImageView(device, imageView, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateSampler(
        VkDevice device,
        const VkSamplerCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSampler* pSampler) {
    return mImpl->on_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

void VkDecoderGlobalState::on_vkDestroySampler(
        VkDevice device,
        VkSampler sampler,
        const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroySampler(device, sampler, pAllocator);
}

void VkDecoderGlobalState::on_vkUpdateDescriptorSets(
        VkDevice device,
        uint32_t descriptorWriteCount,
        const VkWriteDescriptorSet* pDescriptorWrites,
        uint32_t descriptorCopyCount,
        const VkCopyDescriptorSet* pDescriptorCopies) {
    mImpl->on_vkUpdateDescriptorSets(device, descriptorWriteCount,
                                     pDescriptorWrites, descriptorCopyCount,
                                     pDescriptorCopies);
}

void VkDecoderGlobalState::on_vkCmdCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions) {
    mImpl->on_vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage,
            dstImageLayout, regionCount, pRegions);
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

bool VkDecoderGlobalState::usingDirectMapping() const {
    return mImpl->usingDirectMapping();
}

VkDecoderGlobalState::HostFeatureSupport
VkDecoderGlobalState::getHostFeatureSupport() const {
    return mImpl->getHostFeatureSupport();
}

// VK_ANDROID_native_buffer
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
        device, format, imageUsage,
        swapchainImageUsage,
        grallocConsumerUsage,
        grallocProducerUsage);
}

VkResult VkDecoderGlobalState::on_vkAcquireImageANDROID(
    VkDevice device,
    VkImage image,
    int nativeFenceFd,
    VkSemaphore semaphore,
    VkFence fence) {
    return mImpl->on_vkAcquireImageANDROID(
        device, image, nativeFenceFd, semaphore, fence);
}

VkResult VkDecoderGlobalState::on_vkQueueSignalReleaseImageANDROID(
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    VkImage image,
    int* pNativeFenceFd) {
    return mImpl->on_vkQueueSignalReleaseImageANDROID(
        queue, waitSemaphoreCount, pWaitSemaphores,
        image, pNativeFenceFd);
}

// VK_GOOGLE_address_space
VkResult VkDecoderGlobalState::on_vkMapMemoryIntoAddressSpaceGOOGLE(
    VkDevice device, VkDeviceMemory memory, uint64_t* pAddress) {
    return mImpl->on_vkMapMemoryIntoAddressSpaceGOOGLE(
        device, memory, pAddress);
}

// VK_GOOGLE_color_buffer
VkResult VkDecoderGlobalState::on_vkRegisterImageColorBufferGOOGLE(
   VkDevice device, VkImage image, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterImageColorBufferGOOGLE(
        device, image, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkRegisterBufferColorBufferGOOGLE(
   VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterBufferColorBufferGOOGLE(
        device, buffer, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkAllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkAllocateCommandBuffers(device, pAllocateInfo,
                                              pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkCreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool) {
    return mImpl->on_vkCreateCommandPool(device, pCreateInfo, pAllocator,
                                         pCommandPool);
}

void VkDecoderGlobalState::on_vkDestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkResetCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        VkCommandPoolResetFlags flags) {
    return mImpl->on_vkResetCommandPool(device, commandPool, flags);
}

void VkDecoderGlobalState::on_vkCmdExecuteCommands(
        VkCommandBuffer commandBuffer,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkCmdExecuteCommands(commandBuffer, commandBufferCount,
                                          pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkQueueSubmit(VkQueue queue,
                                            uint32_t submitCount,
                                            const VkSubmitInfo* pSubmits,
                                            VkFence fence) {
    return mImpl->on_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult VkDecoderGlobalState::on_vkResetCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags) {
    return mImpl->on_vkResetCommandBuffer(commandBuffer, flags);
}

void VkDecoderGlobalState::on_vkFreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkFreeCommandBuffers(device, commandPool,
                                          commandBufferCount, pCommandBuffers);
}

void VkDecoderGlobalState::deviceMemoryTransform_tohost(
    VkDeviceMemory* memory, uint32_t memoryCount,
    VkDeviceSize* offset, uint32_t offsetCount,
    VkDeviceSize* size, uint32_t sizeCount,
    uint32_t* typeIndex, uint32_t typeIndexCount,
    uint32_t* typeBits, uint32_t typeBitsCount) {
    // Not used currently
    (void)memory; (void)memoryCount;
    (void)offset; (void)offsetCount;
    (void)size; (void)sizeCount;
    (void)typeIndex; (void)typeIndexCount;
    (void)typeBits; (void)typeBitsCount;
}

void VkDecoderGlobalState::deviceMemoryTransform_fromhost(
    VkDeviceMemory* memory, uint32_t memoryCount,
    VkDeviceSize* offset, uint32_t offsetCount,
    VkDeviceSize* size, uint32_t sizeCount,
    uint32_t* typeIndex, uint32_t typeIndexCount,
    uint32_t* typeBits, uint32_t typeBitsCount) {
    // Not used currently
    (void)memory; (void)memoryCount;
    (void)offset; (void)offsetCount;
    (void)size; (void)sizeCount;
    (void)typeIndex; (void)typeIndexCount;
    (void)typeBits; (void)typeBitsCount;
}

#define DEFINE_TRANSFORMED_TYPE_IMPL(type) \
    void VkDecoderGlobalState::transformImpl_##type##_tohost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_tohost(val, count); \
    } \
    void VkDecoderGlobalState::transformImpl_##type##_fromhost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_tohost(val, count); \
    } \

LIST_TRANSFORMED_TYPES(DEFINE_TRANSFORMED_TYPE_IMPL)

}  // namespace goldfish_vk
