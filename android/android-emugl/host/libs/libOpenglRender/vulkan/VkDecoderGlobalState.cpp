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

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "FrameBuffer.h"
#include "GLcommon/etc.h"
#include "VkAndroidNativeBuffer.h"
#include "VkCommonOperations.h"
#include "VkDecoderSnapshot.h"
#include "VkFormatUtils.h"
#include "VulkanDispatch.h"
#include "android/base/Optional.h"
#include "android/base/containers/EntityManager.h"
#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "common/goldfish_vk_deepcopy.h"
#include "common/goldfish_vk_dispatch.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/vm_operations.h"
#include "vk_util.h"

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
    "VK_ANDROID_external_memory_android_hardware_buffer",
    "VK_FUCHSIA_external_memory",
    "VK_FUCHSIA_external_semaphore",
    "VK_KHR_external_semaphore_fd",
    "VK_FUCHSIA_buffer_collection",
};

static constexpr uint32_t kMaxSafeVersion = VK_MAKE_VERSION(1, 1, 0);
static constexpr uint32_t kMinVersion = VK_MAKE_VERSION(1, 0, 0);
static const uint32_t kCompressedTexBlockSize = 4;

class VkDecoderGlobalState::Impl {
public:
    Impl() :
        m_vk(emugl::vkDispatch()),
        m_emu(getGlobalVkEmulation()) { }
    ~Impl() = default;

    VkResult on_vkEnumerateInstanceVersion(
        android::base::Pool* pool,
        uint32_t* pApiVersion) {
        if (m_vk->vkEnumerateInstanceVersion) {
            return m_vk->vkEnumerateInstanceVersion(pApiVersion);
        }
        *pApiVersion = kMinVersion;
        return VK_SUCCESS;
    }

    VkResult on_vkCreateInstance(
        android::base::Pool* pool,
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

        // TODO: bug 129484301
        get_emugl_vm_operations().setSkipSnapshotSave(true /* skip saving */);

        InstanceInfo info;
        for (uint32_t i = 0; i < createInfoFiltered.enabledExtensionCount;
             ++i) {
            info.enabledExtensionNames.push_back(
                    createInfoFiltered.ppEnabledExtensionNames[i]);
        }

        // Box it up
        VkInstance boxed = new_boxed_VkInstance(*pInstance, nullptr, true /* own dispatch */);
        init_vulkan_dispatch_from_instance(
            m_vk, *pInstance,
            dispatch_VkInstance(boxed));
        info.boxed = boxed;

        mInstanceInfo[*pInstance] = info;

        *pInstance = (VkInstance)info.boxed;

        auto fb = FrameBuffer::getFB();
        if (!fb) return res;

        fb->registerProcessCleanupCallback(
            unbox_VkInstance(boxed),
            [this, boxed] {
                vkDestroyInstanceImpl(
                    unbox_VkInstance(boxed),
                    nullptr);
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

    void on_vkDestroyInstance(
        android::base::Pool* pool,
        VkInstance boxed_instance,
        const VkAllocationCallbacks* pAllocator) {

        auto instance = unbox_VkInstance(boxed_instance);

        vkDestroyInstanceImpl(instance, pAllocator);

        auto fb = FrameBuffer::getFB();
        if (!fb) return;

        fb->unregisterProcessCleanupCallback(instance);
    }

    VkResult on_vkEnumeratePhysicalDevices(
        android::base::Pool* pool,
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
                            new_boxed_VkPhysicalDevice(physicalDevices[i], vk, false /* does not own dispatch */);

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

    void on_vkGetPhysicalDeviceFeatures(
        android::base::Pool* pool,
        VkPhysicalDevice boxed_physicalDevice,
        VkPhysicalDeviceFeatures* pFeatures) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
        pFeatures->textureCompressionETC2 = true;
    }

    void on_vkGetPhysicalDeviceFeatures2(
        android::base::Pool* pool,
        VkPhysicalDevice boxed_physicalDevice,
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
        android::base::Pool* pool,
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
            if (cmpInfo.isCompressed) {
                flags &= ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                format = cmpInfo.sizeCompFormat;
            }
        }
        return vk->vkGetPhysicalDeviceImageFormatProperties(
                physicalDevice, format, type, tiling, usage, flags,
                pImageFormatProperties);
    }

    VkResult on_vkGetPhysicalDeviceImageFormatProperties2(
        android::base::Pool* pool,
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
                imageFormatInfo.flags &= ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                imageFormatInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                imageFormatInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                imageFormatInfo.format = cmpInfo.sizeCompFormat;
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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

    VkResult on_vkCreateDevice(
        android::base::Pool* pool,
        VkPhysicalDevice boxed_physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        std::vector<const char*> finalExts =
            filteredExtensionNames(
                pCreateInfo->enabledExtensionCount,
                pCreateInfo->ppEnabledExtensionNames);

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
        VkDevice boxed = new_boxed_VkDevice(*pDevice, nullptr, true /* own dispatch */);
        init_vulkan_dispatch_from_device(
            vk, *pDevice,
            dispatch_VkDevice(boxed));
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

                auto boxed = new_boxed_VkQueue(queueOut, dispatch_VkDevice(deviceInfo.boxed), false /* does not own dispatch */);
                mQueueInfo[queueOut].boxed = boxed;
            }
        }

        // Box the device.
        *pDevice = (VkDevice)deviceInfo.boxed;
        return VK_SUCCESS;
    }

    void on_vkGetDeviceQueue(
        android::base::Pool* pool,
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
                delete_boxed_VkQueue(eraseIt->second.boxed);
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
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);

        AutoLock lock(mLock);

        destroyDeviceLocked(device, pAllocator);

        mDeviceInfo.erase(device);
        mDeviceToPhysicalDevice.erase(device);
    }

    VkResult on_vkCreateBuffer(
        android::base::Pool* pool,
        VkDevice boxed_device,
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
            *pBuffer = new_boxed_non_dispatchable_VkBuffer(*pBuffer);
        }

        return result;
    }

    void on_vkDestroyBuffer(
        android::base::Pool* pool,
        VkDevice boxed_device, VkBuffer buffer,
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

    VkResult on_vkBindBufferMemory(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

    VkResult on_vkBindBufferMemory2(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

    VkResult on_vkBindBufferMemory2KHR(
        android::base::Pool* pool,
        VkDevice boxed_device,
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
        android::base::Pool* pool,
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
        VkImageCreateInfo& sizeCompInfo = cmpInfo.sizeCompImgCreateInfo;
        VkImageCreateInfo decompInfo;
        if (deviceInfoIt->second.emulateTextureEtc2) {
            cmpInfo = createCompressedImageInfo(
                pCreateInfo->format
            );
            cmpInfo.device = device;
            if (cmpInfo.isCompressed) {
                cmpInfo.imageType = pCreateInfo->imageType;
                cmpInfo.extent = pCreateInfo->extent;
                cmpInfo.mipLevels = pCreateInfo->mipLevels;
                cmpInfo.layerCount = pCreateInfo->arrayLayers;
                sizeCompInfo = *pCreateInfo;
                sizeCompInfo.format = cmpInfo.sizeCompFormat;
                sizeCompInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                sizeCompInfo.flags &=
                        ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                sizeCompInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                // Each block is 4x4 in ETC2 compressed texture
                sizeCompInfo.extent.width = (sizeCompInfo.extent.width +
                                             kCompressedTexBlockSize - 1) /
                                            kCompressedTexBlockSize;
                sizeCompInfo.extent.height = (sizeCompInfo.extent.height +
                                              kCompressedTexBlockSize - 1) /
                                             kCompressedTexBlockSize;
                sizeCompInfo.mipLevels = 1;
                if (pCreateInfo->queueFamilyIndexCount) {
                    cmpInfo.sizeCompImgQueueFamilyIndices.assign(
                            pCreateInfo->pQueueFamilyIndices,
                            pCreateInfo->pQueueFamilyIndices +
                                    pCreateInfo->queueFamilyIndexCount);
                    sizeCompInfo.pQueueFamilyIndices =
                            cmpInfo.sizeCompImgQueueFamilyIndices.data();
                }
                decompInfo = *pCreateInfo;
                decompInfo.format = cmpInfo.decompFormat;
                decompInfo.flags &=
                        ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                decompInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                decompInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                pCreateInfo = &decompInfo;
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

        if (cmpInfo.isCompressed && deviceInfoIt->second.emulateTextureEtc2) {
            cmpInfo.decompImg = *pImage;
            createSizeCompImages(vk, &cmpInfo);
        }

        auto& imageInfo = mImageInfo[*pImage];
        imageInfo.anbInfo = anbInfo;
        imageInfo.cmpInfo = cmpInfo;

        *pImage = new_boxed_non_dispatchable_VkImage(*pImage);

        return createRes;
    }

    void on_vkDestroyImage(
        android::base::Pool* pool,
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
                CompressedImageInfo& cmpInfo = info.cmpInfo;
                if (image != cmpInfo.decompImg) {
                    vk->vkDestroyImage(device, info.cmpInfo.decompImg, nullptr);
                }
                for (const auto& image : cmpInfo.sizeCompImgs) {
                    vk->vkDestroyImage(device, image, nullptr);
                }
                vk->vkDestroyDescriptorSetLayout(
                        device, cmpInfo.decompDescriptorSetLayout, nullptr);
                vk->vkDestroyDescriptorPool(
                        device, cmpInfo.decompDescriptorPool, nullptr);
                vk->vkDestroyShaderModule(device, cmpInfo.decompShader,
                                          nullptr);
                vk->vkDestroyPipelineLayout(
                        device, cmpInfo.decompPipelineLayout, nullptr);
                vk->vkDestroyPipeline(device, cmpInfo.decompPipeline, nullptr);
                for (const auto& imageView : cmpInfo.sizeCompImageViews) {
                    vk->vkDestroyImageView(device, imageView, nullptr);
                }
                for (const auto& imageView : cmpInfo.decompImageViews) {
                    vk->vkDestroyImageView(device, imageView, nullptr);
                }
            }
            vk->vkDestroyImage(device, image, pAllocator);
        }
        mImageInfo.erase(image);
    }

    VkResult on_vkBindImageMemory(android::base::Pool* pool,
                                  VkDevice boxed_device,
                                  VkImage image,
                                  VkDeviceMemory memory,
                                  VkDeviceSize memoryOffset) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        VkResult result =
                vk->vkBindImageMemory(device, image, memory, memoryOffset);
        if (VK_SUCCESS != result) {
            return result;
        }
        AutoLock lock(mLock);
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        if (!deviceInfoIt->second.emulateTextureEtc2) {
            return VK_SUCCESS;
        }
        auto imageInfoIt = mImageInfo.find(image);
        if (imageInfoIt == mImageInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        CompressedImageInfo& cmp = imageInfoIt->second.cmpInfo;
        if (!cmp.isCompressed) {
            return VK_SUCCESS;
        }
        for (size_t i = 0; i < cmp.sizeCompImgs.size(); i++) {
            result = vk->vkBindImageMemory(device, cmp.sizeCompImgs[i], memory,
                                           memoryOffset + cmp.memoryOffsets[i]);
        }

        return VK_SUCCESS;
    }

    VkResult on_vkCreateImageView(
        android::base::Pool* pool,
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
        auto imageInfoIt = mImageInfo.find(pCreateInfo->image);
        if (imageInfoIt == mImageInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        VkImageViewCreateInfo createInfo;
        bool needEmulatedAlpha = false;
        if (deviceInfoIt->second.emulateTextureEtc2) {
            CompressedImageInfo cmpInfo = createCompressedImageInfo(
                pCreateInfo->format
            );
            if (cmpInfo.isCompressed) {
                if (imageInfoIt->second.cmpInfo.decompImg) {
                    createInfo = *pCreateInfo;
                    createInfo.format = cmpInfo.decompFormat;
                    needEmulatedAlpha = cmpInfo.needEmulatedAlpha();
                    createInfo.image = imageInfoIt->second.cmpInfo.decompImg;
                    pCreateInfo = &createInfo;
                }
            } else if (imageInfoIt->second.cmpInfo.isCompressed) {
                // Size compatible image view
                createInfo = *pCreateInfo;
                createInfo.format = cmpInfo.sizeCompFormat;
                needEmulatedAlpha = false;
                createInfo.image =
                        imageInfoIt->second.cmpInfo.sizeCompImgs
                                [pCreateInfo->subresourceRange.baseMipLevel];
                createInfo.subresourceRange.baseMipLevel = 0;
                pCreateInfo = &createInfo;
            }
        }
        if (imageInfoIt->second.anbInfo.externallyBacked) {
            createInfo = *pCreateInfo;
            createInfo.format = imageInfoIt->second.anbInfo.vkFormat;
            pCreateInfo = &createInfo;
        }

        VkResult result =
                vk->vkCreateImageView(device, pCreateInfo, pAllocator, pView);
        if (result != VK_SUCCESS) {
            return result;
        }

        auto& imageViewInfo = mImageViewInfo[*pView];
        imageViewInfo.needEmulatedAlpha = needEmulatedAlpha;

        *pView = new_boxed_non_dispatchable_VkImageView(*pView);

        return result;
    }

    void on_vkDestroyImageView(
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkImageView imageView,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyImageView(device, imageView, pAllocator);
        AutoLock lock(mLock);
        mImageViewInfo.erase(imageView);
    }

    VkResult on_vkCreateSampler(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

        *pSampler = new_boxed_non_dispatchable_VkSampler(*pSampler);

        return result;
    }

    void on_vkDestroySampler(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

    VkResult on_vkCreateSemaphore(
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkSemaphoreCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSemaphore* pSemaphore) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkSemaphoreCreateInfo finalCreateInfo = *pCreateInfo;

        vk_struct_common* structChain =
            vk_init_struct_chain(
                (vk_struct_common*)&finalCreateInfo);


        VkExportSemaphoreCreateInfo* exportCiPtr =
            (VkExportSemaphoreCreateInfo*)
            vk_find_struct(
                (vk_struct_common*)pCreateInfo,
                VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO);

        if (exportCiPtr) {

#ifdef _WIN32
            if (exportCiPtr->handleTypes) {
                exportCiPtr->handleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
            }
#endif

            structChain =
                vk_append_struct(
                    (vk_struct_common*)structChain,
                    (vk_struct_common*)exportCiPtr);
        }


        VkResult res = vk->vkCreateSemaphore(device, &finalCreateInfo, pAllocator, pSemaphore);

        if (res != VK_SUCCESS) return res;

        *pSemaphore = new_boxed_non_dispatchable_VkSemaphore(*pSemaphore);

        return res;
    }

    VkResult on_vkImportSemaphoreFdKHR(
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

#ifdef _WIN32
        AutoLock lock(mLock);

        auto infoPtr = android::base::find(
            mSemaphoreInfo, pImportSemaphoreFdInfo->semaphore);

        if (!infoPtr) {
            return VK_ERROR_INVALID_EXTERNAL_HANDLE;
        }

        VK_EXT_MEMORY_HANDLE handle =
            dupExternalMemory(infoPtr->externalHandle);

        VkImportSemaphoreWin32HandleInfoKHR win32ImportInfo = {
            VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR, 0,
            pImportSemaphoreFdInfo->semaphore,
            pImportSemaphoreFdInfo->flags,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            handle, L"",
        };

        return vk->vkImportSemaphoreWin32HandleKHR(
            device, &win32ImportInfo);
#else
        VkImportSemaphoreFdInfoKHR importInfo = *pImportSemaphoreFdInfo;
        importInfo.fd = dup(pImportSemaphoreFdInfo->fd);
        return vk->vkImportSemaphoreFdKHR(device, &importInfo);
#endif
    }

    VkResult on_vkGetSemaphoreFdKHR(
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
        int* pFd) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
#ifdef _WIN32
        VkSemaphoreGetWin32HandleInfoKHR getWin32 = {
            VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR, 0,
            pGetFdInfo->semaphore,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT,
        };
        VK_EXT_MEMORY_HANDLE handle;
        VkResult result = vk->vkGetSemaphoreWin32HandleKHR(device, &getWin32, &handle);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        mSemaphoreInfo[pGetFdInfo->semaphore].externalHandle = handle;
        int nextId = genSemaphoreId();
        mExternalSemaphoresById[nextId] = pGetFdInfo->semaphore;
        *pFd = nextId;
#else
        VkResult result = vk->vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);

        mSemaphoreInfo[pGetFdInfo->semaphore].externalHandle = *pFd;
        // No next id; its already an fd
#endif
        return result;
    }

    void on_vkDestroySemaphore(
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkSemaphore semaphore,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

#ifndef _WIN32
        AutoLock lock(mLock);
        const auto& ite = mSemaphoreInfo.find(semaphore);
        if (ite != mSemaphoreInfo.end() &&
            (ite->second.externalHandle != VK_EXT_MEMORY_HANDLE_INVALID)) {
            close(ite->second.externalHandle);
        }
#endif
        vk->vkDestroySemaphore(device, semaphore, pAllocator);
    }

    void on_vkUpdateDescriptorSets(
        android::base::Pool* pool,
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

    void on_vkCmdCopyImageToBuffer(android::base::Pool* pool,
                                   VkCommandBuffer boxed_commandBuffer,
                                   VkImage srcImage,
                                   VkImageLayout srcImageLayout,
                                   VkBuffer dstBuffer,
                                   uint32_t regionCount,
                                   const VkBufferImageCopy* pRegions) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(srcImage);
        if (it == mImageInfo.end())
            return;
        auto bufferInfoIt = mBufferInfo.find(dstBuffer);
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
            vk->vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout,
                                       dstBuffer, regionCount, pRegions);
            return;
        }
        CompressedImageInfo& cmp = it->second.cmpInfo;
        for (uint32_t r = 0; r < regionCount; r++) {
            VkBufferImageCopy region;
            region = pRegions[r];
            uint32_t mipLevel = region.imageSubresource.mipLevel;
            region.imageSubresource.mipLevel = 0;
            region.bufferRowLength /= kCompressedTexBlockSize;
            region.bufferImageHeight /= kCompressedTexBlockSize;
            region.imageOffset.x /= kCompressedTexBlockSize;
            region.imageOffset.y /= kCompressedTexBlockSize;
            uint32_t width = cmp.sizeCompMipmapWidth(mipLevel);
            uint32_t height = cmp.sizeCompMipmapHeight(mipLevel);
            region.imageExtent.width =
                    (region.imageExtent.width + kCompressedTexBlockSize - 1) /
                    kCompressedTexBlockSize;
            region.imageExtent.height =
                    (region.imageExtent.height + kCompressedTexBlockSize - 1) /
                    kCompressedTexBlockSize;
            region.imageExtent.width =
                    std::min(region.imageExtent.width, width);
            region.imageExtent.height =
                    std::min(region.imageExtent.height, height);
            vk->vkCmdCopyImageToBuffer(commandBuffer,
                                       cmp.sizeCompImgs[mipLevel],
                                       srcImageLayout, dstBuffer, 1, &region);
        }
    }

    void on_vkGetImageMemoryRequirements(
            android::base::Pool* pool,
            VkDevice boxed_device,
            VkImage image,
            VkMemoryRequirements* pMemoryRequirements) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        vk->vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
        AutoLock lock(mLock);
        updateImageMemorySizeLocked(device, image, pMemoryRequirements);
    }

    void on_vkGetImageMemoryRequirements2(
            android::base::Pool* pool,
            VkDevice boxed_device,
            const VkImageMemoryRequirementsInfo2* pInfo,
            VkMemoryRequirements2* pMemoryRequirements) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        AutoLock lock(mLock);
        auto physicalDevice = mDeviceToPhysicalDevice[device];
        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);

        if (!physdevInfo) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl::emugl_crash_reporter(
                    "FATAL: Could not get image memory requirement for "
                    "VkPhysicalDevice");
        }
        auto instance = mPhysicalDeviceToInstance[physicalDevice];

        if (physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetImageMemoryRequirements2(device, pInfo,
                                              pMemoryRequirements);
        } else if (hasInstanceExtension(instance,
                                        "VK_KHR_get_memory_requirements2")) {
            vk->vkGetImageMemoryRequirements2KHR(device, pInfo,
                                                 pMemoryRequirements);
        } else {
            if (pInfo->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkMemoryRequirements2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pMemoryRequirements = {
                    VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
                    nullptr,
            };
            vk->vkGetImageMemoryRequirements(
                    device, pInfo->image,
                    &pMemoryRequirements->memoryRequirements);
        }
        updateImageMemorySizeLocked(device, pInfo->image,
                                    &pMemoryRequirements->memoryRequirements);
    }

    void on_vkCmdCopyBufferToImage(
        android::base::Pool* pool,
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
        for (uint32_t r = 0; r < regionCount; r++) {
            VkBufferImageCopy dstRegion;
            dstRegion = pRegions[r];
            uint32_t mipLevel = dstRegion.imageSubresource.mipLevel;
            dstRegion.imageSubresource.mipLevel = 0;
            dstRegion.bufferRowLength /= kCompressedTexBlockSize;
            dstRegion.bufferImageHeight /= kCompressedTexBlockSize;
            dstRegion.imageOffset.x /= kCompressedTexBlockSize;
            dstRegion.imageOffset.y /= kCompressedTexBlockSize;
            uint32_t width = cmp.sizeCompMipmapWidth(mipLevel);
            uint32_t height = cmp.sizeCompMipmapHeight(mipLevel);
            dstRegion.imageExtent.width = (dstRegion.imageExtent.width +
                                           kCompressedTexBlockSize - 1) /
                                          kCompressedTexBlockSize;
            dstRegion.imageExtent.height = (dstRegion.imageExtent.height +
                                            kCompressedTexBlockSize - 1) /
                                           kCompressedTexBlockSize;
            dstRegion.imageExtent.width =
                    std::min(dstRegion.imageExtent.width, width);
            dstRegion.imageExtent.height =
                    std::min(dstRegion.imageExtent.height, height);
            vk->vkCmdCopyBufferToImage(commandBuffer, srcBuffer,
                                       cmp.sizeCompImgs[mipLevel],
                                       dstImageLayout, 1, &dstRegion);
        }
    }

    void on_vkCmdPipelineBarrier(
            android::base::Pool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        if (imageMemoryBarrierCount == 0) {
            vk->vkCmdPipelineBarrier(
                    commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                    memoryBarrierCount, pMemoryBarriers,
                    bufferMemoryBarrierCount, pBufferMemoryBarriers,
                    imageMemoryBarrierCount, pImageMemoryBarriers);
            return;
        }
        AutoLock lock(mLock);
        auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
        if (cmdBufferInfoIt == mCmdBufferInfo.end()) {
            return;
        }
        auto deviceInfoIt = mDeviceInfo.find(cmdBufferInfoIt->second.device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        if (!deviceInfoIt->second.emulateTextureEtc2) {
            vk->vkCmdPipelineBarrier(
                    commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                    memoryBarrierCount, pMemoryBarriers,
                    bufferMemoryBarrierCount, pBufferMemoryBarriers,
                    imageMemoryBarrierCount, pImageMemoryBarriers);
            return;
        }
        // Add barrier for decompressed image
        std::vector<VkImageMemoryBarrier> persistentImageBarriers;
        bool needRebind = false;
        for (uint32_t i = 0; i < imageMemoryBarrierCount; i++) {
            const VkImageMemoryBarrier& srcBarrier = pImageMemoryBarriers[i];
            auto image = srcBarrier.image;
            auto it = mImageInfo.find(image);
            if (it == mImageInfo.end() || !it->second.cmpInfo.isCompressed) {
                persistentImageBarriers.push_back(srcBarrier);
                continue;
            }
            uint32_t baseMipLevel = srcBarrier.subresourceRange.baseMipLevel;
            uint32_t levelCount = srcBarrier.subresourceRange.levelCount;
            VkImageMemoryBarrier decompBarrier = srcBarrier;
            decompBarrier.image = it->second.cmpInfo.decompImg;
            VkImageMemoryBarrier sizeCompBarrierTemplate = srcBarrier;
            sizeCompBarrierTemplate.subresourceRange.baseMipLevel = 0;
            sizeCompBarrierTemplate.subresourceRange.levelCount = 1;
            std::vector<VkImageMemoryBarrier> sizeCompBarriers(
                    srcBarrier.subresourceRange.levelCount,
                    sizeCompBarrierTemplate);
            for (uint32_t j = 0; j < levelCount; j++) {
                sizeCompBarriers[j].image =
                        it->second.cmpInfo.sizeCompImgs[baseMipLevel + j];
            }

            // TODO: should we use image layout or access bit?
            if (srcBarrier.oldLayout == 0 ||
                (srcBarrier.newLayout != VK_IMAGE_LAYOUT_GENERAL &&
                 srcBarrier.newLayout !=
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
                // TODO: might only need to push one of them?
                persistentImageBarriers.push_back(decompBarrier);
                persistentImageBarriers.insert(persistentImageBarriers.end(),
                                               sizeCompBarriers.begin(),
                                               sizeCompBarriers.end());
                continue;
            }
            if (srcBarrier.newLayout !=
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                srcBarrier.newLayout != VK_IMAGE_LAYOUT_GENERAL) {
                fprintf(stderr,
                        "WARNING: unexpected usage to transfer "
                        "compressed image layout from %d to %d\n",
                        srcBarrier.oldLayout, srcBarrier.newLayout);
            }

            VkResult result = it->second.cmpInfo.initDecomp(
                    vk, cmdBufferInfoIt->second.device, image);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "WARNING: texture decompression failed\n");
                continue;
            }

            std::vector<VkImageMemoryBarrier> currImageBarriers;
            currImageBarriers.reserve(sizeCompBarriers.size() + 1);
            currImageBarriers.insert(currImageBarriers.end(),
                                     sizeCompBarriers.begin(),
                                     sizeCompBarriers.end());
            for (auto& barrier : currImageBarriers) {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            currImageBarriers.push_back(decompBarrier);
            {
                VkImageMemoryBarrier& barrier = currImageBarriers[levelCount];
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            vk->vkCmdPipelineBarrier(commandBuffer, srcStageMask,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                     nullptr, 0, nullptr,
                                     currImageBarriers.size(),
                                     currImageBarriers.data());
            it->second.cmpInfo.cmdDecompress(
                    vk, commandBuffer, dstStageMask, decompBarrier.newLayout,
                    decompBarrier.dstAccessMask, baseMipLevel, levelCount,
                    srcBarrier.subresourceRange.baseArrayLayer,
                    srcBarrier.subresourceRange.layerCount);
            needRebind = true;

            for (uint32_t j = 0; j < currImageBarriers.size(); j++) {
                VkImageMemoryBarrier& barrier = currImageBarriers[j];
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = srcBarrier.dstAccessMask;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = srcBarrier.newLayout;
            }
            {
                VkImageMemoryBarrier& barrier = currImageBarriers[levelCount];
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = srcBarrier.dstAccessMask;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = srcBarrier.newLayout;
            }

            vk->vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                    dstStageMask,                          // dstStageMask
                    0,                                     // dependencyFlags
                    0,                                     // memoryBarrierCount
                    nullptr,                               // pMemoryBarriers
                    0,                         // bufferMemoryBarrierCount
                    nullptr,                   // pBufferMemoryBarriers
                    currImageBarriers.size(),  // imageMemoryBarrierCount
                    currImageBarriers.data()   // pImageMemoryBarriers
            );
        }
        if (needRebind && cmdBufferInfoIt->second.computePipeline) {
            // Recover pipeline bindings
            vk->vkCmdBindPipeline(commandBuffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  cmdBufferInfoIt->second.computePipeline);
            if (cmdBufferInfoIt->second.descriptorSets.size() > 0) {
                vk->vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        cmdBufferInfoIt->second.descriptorLayout,
                        cmdBufferInfoIt->second.firstSet,
                        cmdBufferInfoIt->second.descriptorSets.size(),
                        cmdBufferInfoIt->second.descriptorSets.data(), 0,
                        nullptr);
            }
        }
        if (memoryBarrierCount || bufferMemoryBarrierCount ||
            !persistentImageBarriers.empty()) {
            vk->vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask,
                                     dependencyFlags, memoryBarrierCount,
                                     pMemoryBarriers, bufferMemoryBarrierCount,
                                     pBufferMemoryBarriers,
                                     persistentImageBarriers.size(),
                                     persistentImageBarriers.data());
        }
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
        android::base::Pool* pool,
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

        VkImportColorBufferGOOGLE* importCbInfoPtr =
            (VkImportColorBufferGOOGLE*)
                vk_find_struct((vk_struct_common*)pAllocateInfo,
                    VK_STRUCTURE_TYPE_IMPORT_COLOR_BUFFER_GOOGLE);

#ifdef _WIN32
        VkImportMemoryWin32HandleInfoKHR importInfo {
            VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, 0,
            VK_EXT_MEMORY_HANDLE_TYPE_BIT,
            VK_EXT_MEMORY_HANDLE_INVALID, L"",
        };
#else
        VkImportMemoryFdInfoKHR importInfo {
            VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, 0,
            VK_EXT_MEMORY_HANDLE_TYPE_BIT,
            VK_EXT_MEMORY_HANDLE_INVALID,
        };
#endif
        if (importCbInfoPtr) {
            // Ensure color buffer has Vulkan backing.
            setupVkColorBuffer(
                importCbInfoPtr->colorBuffer,
                false /* not vulkan only */,
                nullptr,
                // Modify the allocation size to suit the resulting image memory size.
                &allocInfo.allocationSize);

            VK_EXT_MEMORY_HANDLE cbExtMemoryHandle =
                getColorBufferExtMemoryHandle(importCbInfoPtr->colorBuffer);

            if (cbExtMemoryHandle == VK_EXT_MEMORY_HANDLE_INVALID) {
                fprintf(stderr,
                    "%s: VK_ERROR_OUT_OF_DEVICE_MEMORY: "
                    "colorBuffer 0x%x does not have Vulkan external memory backing\n", __func__,
                    importCbInfoPtr->colorBuffer);
                return VK_ERROR_OUT_OF_DEVICE_MEMORY;
            }

            cbExtMemoryHandle = dupExternalMemory(cbExtMemoryHandle);

#ifdef _WIN32
            importInfo.handle = cbExtMemoryHandle;
#else
            importInfo.fd = cbExtMemoryHandle;
#endif
            structChain =
                vk_append_struct(
                    (vk_struct_common*)structChain,
                    (vk_struct_common*)&importInfo);
        }

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

        if (!hostVisible) {
            *pMemory = new_boxed_non_dispatchable_VkDeviceMemory(*pMemory);
            return result;
        }

        VkResult mapResult =
            vk->vkMapMemory(device, *pMemory, 0,
                            mapInfo.size, 0, &mapInfo.ptr);

        if (mapResult != VK_SUCCESS) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        *pMemory = new_boxed_non_dispatchable_VkDeviceMemory(*pMemory);

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
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        freeMemoryLocked(vk, device, memory, pAllocator);

        mMapInfo.erase(memory);
    }

    VkResult on_vkMapMemory(
        android::base::Pool* pool,
        VkDevice,
        VkDeviceMemory memory,
        VkDeviceSize offset,
        VkDeviceSize size,
        VkMemoryMapFlags flags,
        void** ppData) {

        AutoLock lock(mLock);
        return on_vkMapMemoryLocked(0, memory, offset, size, flags, ppData);
    }
    VkResult on_vkMapMemoryLocked(VkDevice,
                                  VkDeviceMemory memory,
                                  VkDeviceSize offset,
                                  VkDeviceSize size,
                                  VkMemoryMapFlags flags,
                                  void** ppData) {
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

    void on_vkUnmapMemory(
        android::base::Pool* pool,
        VkDevice, VkDeviceMemory) {
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

        auto emu = getGlobalVkEmulation();

        res.supportsVulkan = emu && emu->live;

        if (!res.supportsVulkan) return res;

        const auto& props = emu->deviceInfo.physdevProps;

        res.supportsVulkan1_1 =
            props.apiVersion >= VK_API_VERSION_1_1;
        res.supportsExternalMemory =
            emu->deviceInfo.supportsExternalMemory;
        res.useDeferredCommands =
            emu->useDeferredCommands;

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
        android::base::Pool* pool,
        VkDevice,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        int* grallocUsage) {
        getGralloc0Usage(format, imageUsage, grallocUsage);
        return VK_SUCCESS;
    }

    VkResult on_vkGetSwapchainGrallocUsage2ANDROID(
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
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
        android::base::Pool* pool,
        VkDevice device, VkImage image, uint32_t colorBuffer) {

        (void)image;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkRegisterBufferColorBufferGOOGLE(
        android::base::Pool* pool,
        VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {

        (void)buffer;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkAllocateCommandBuffers(
        android::base::Pool* pool,
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
            mCmdBufferInfo[pCommandBuffers[i]].device = device;
            mCmdBufferInfo[pCommandBuffers[i]].cmdPool =
                    pAllocateInfo->commandPool;
            auto boxed = new_boxed_VkCommandBuffer(pCommandBuffers[i], vk, false /* does not own dispatch */);
            mCmdBufferInfo[pCommandBuffers[i]].boxed = boxed;
            pCommandBuffers[i] = (VkCommandBuffer)boxed;
        }
        return result;
    }

    VkResult on_vkCreateCommandPool(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

        *pCommandPool = new_boxed_non_dispatchable_VkCommandPool(*pCommandPool);

        return result;
    }

    void on_vkDestroyCommandPool(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

    VkResult on_vkResetCommandPool(
        android::base::Pool* pool,
        VkDevice boxed_device,
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

    void on_vkCmdExecuteCommands(
        android::base::Pool* pool,
        VkCommandBuffer boxed_commandBuffer,
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

    VkResult on_vkQueueSubmit(
        android::base::Pool* pool,
        VkQueue boxed_queue,
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

    VkResult on_vkResetCommandBuffer(
        android::base::Pool* pool,
        VkCommandBuffer boxed_commandBuffer,
        VkCommandBufferResetFlags flags) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        VkResult result = vk->vkResetCommandBuffer(commandBuffer, flags);
        if (VK_SUCCESS == result) {
            AutoLock lock(mLock);
            mCmdBufferInfo[commandBuffer].preprocessFuncs.clear();
            mCmdBufferInfo[commandBuffer].subCmds.clear();
            mCmdBufferInfo[commandBuffer].computePipeline = 0;
            mCmdBufferInfo[commandBuffer].firstSet = 0;
            mCmdBufferInfo[commandBuffer].descriptorLayout = 0;
            mCmdBufferInfo[commandBuffer].descriptorSets.clear();
        }
        return result;
    }

    void on_vkFreeCommandBuffers(
        android::base::Pool* pool,
        VkDevice boxed_device,
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
                delete_boxed_VkCommandBuffer(cmdBufferInfoIt->second.boxed);
                mCmdBufferInfo.erase(cmdBufferInfoIt);
            }
        }
    }

    void on_vkGetPhysicalDeviceExternalSemaphoreProperties(
        android::base::Pool* pool,
        VkPhysicalDevice boxed_physicalDevice,
        const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
        VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        if (!physicalDevice) {
            return;
        }
        // Cannot forward this call to driver because nVidia linux driver crahses on it.
        switch (pExternalSemaphoreInfo->handleType) {
            case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT:
                pExternalSemaphoreProperties->exportFromImportedHandleTypes =
                        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
                pExternalSemaphoreProperties->compatibleHandleTypes =
                        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
                pExternalSemaphoreProperties->externalSemaphoreFeatures =
                        VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                        VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT;
                return;
            case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT:
                pExternalSemaphoreProperties->exportFromImportedHandleTypes =
                        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
                pExternalSemaphoreProperties->compatibleHandleTypes =
                        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
                pExternalSemaphoreProperties->externalSemaphoreFeatures =
                        VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                        VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT;
                return;
            default:
                break;
        }

        pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
        pExternalSemaphoreProperties->compatibleHandleTypes = 0;
        pExternalSemaphoreProperties->externalSemaphoreFeatures = 0;
    }

    VkResult on_vkCreateDescriptorUpdateTemplate(
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto descriptorUpdateTemplateInfo =
           calcLinearizedDescriptorUpdateTemplateInfo(pCreateInfo);

        VkResult res = vk->vkCreateDescriptorUpdateTemplate(
            device, &descriptorUpdateTemplateInfo.createInfo,
            pAllocator, pDescriptorUpdateTemplate);

        if (res == VK_SUCCESS) {
            registerDescriptorUpdateTemplate(
                *pDescriptorUpdateTemplate,
                descriptorUpdateTemplateInfo);
            *pDescriptorUpdateTemplate = new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(*pDescriptorUpdateTemplate);
        }

        return res;
    }

    VkResult on_vkCreateDescriptorUpdateTemplateKHR(
        android::base::Pool* pool,
        VkDevice boxed_device,
        const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto descriptorUpdateTemplateInfo =
           calcLinearizedDescriptorUpdateTemplateInfo(pCreateInfo);

        VkResult res = vk->vkCreateDescriptorUpdateTemplateKHR(
            device, &descriptorUpdateTemplateInfo.createInfo,
            pAllocator, pDescriptorUpdateTemplate);

        if (res == VK_SUCCESS) {
            registerDescriptorUpdateTemplate(
                *pDescriptorUpdateTemplate,
                descriptorUpdateTemplateInfo);
            *pDescriptorUpdateTemplate = new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(*pDescriptorUpdateTemplate);
        }

        return res;
    }

    void on_vkDestroyDescriptorUpdateTemplate(
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
        const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorUpdateTemplate(
            device, descriptorUpdateTemplate, pAllocator);

        unregisterDescriptorUpdateTemplate(descriptorUpdateTemplate);
    }

    void on_vkDestroyDescriptorUpdateTemplateKHR(
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
        const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorUpdateTemplateKHR(
            device, descriptorUpdateTemplate, pAllocator);

        unregisterDescriptorUpdateTemplate(descriptorUpdateTemplate);
    }

    void on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
        android::base::Pool* pool,
        VkDevice boxed_device,
        VkDescriptorSet descriptorSet,
        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
        uint32_t imageInfoCount,
        uint32_t bufferInfoCount,
        uint32_t bufferViewCount,
        const uint32_t* pImageInfoEntryIndices,
        const uint32_t* pBufferInfoEntryIndices,
        const uint32_t* pBufferViewEntryIndices,
        const VkDescriptorImageInfo* pImageInfos,
        const VkDescriptorBufferInfo* pBufferInfos,
        const VkBufferView* pBufferViews) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        auto info = android::base::find(
            mDescriptorUpdateTemplateInfo,
            descriptorUpdateTemplate);

        if (!info) return;

        memcpy(info->data.data() + info->imageInfoStart,
               pImageInfos,
               imageInfoCount * sizeof(VkDescriptorImageInfo));
        memcpy(info->data.data() + info->bufferInfoStart,
               pBufferInfos,
               bufferInfoCount * sizeof(VkDescriptorBufferInfo));
        memcpy(info->data.data() + info->bufferViewStart,
               pBufferViews,
               bufferViewCount * sizeof(VkBufferView));

        vk->vkUpdateDescriptorSetWithTemplate(
            device, descriptorSet, descriptorUpdateTemplate,
            info->data.data());
    }

    VkResult on_vkBeginCommandBuffer(
            android::base::Pool* pool,
            VkCommandBuffer boxed_commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        VkResult result = vk->vkBeginCommandBuffer(commandBuffer, pBeginInfo);
        if (result != VK_SUCCESS) {
            return result;
        }
        // TODO: Check VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT?
        AutoLock lock(mLock);
        mCmdBufferInfo[commandBuffer].preprocessFuncs.clear();
        mCmdBufferInfo[commandBuffer].subCmds.clear();
        return VK_SUCCESS;
    }

    void on_vkEndCommandBufferAsyncGOOGLE(
        android::base::Pool* pool,
        VkCommandBuffer boxed_commandBuffer) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        vk->vkEndCommandBuffer(commandBuffer);
    }

    void on_vkResetCommandBufferAsyncGOOGLE(
        android::base::Pool* pool,
        VkCommandBuffer boxed_commandBuffer,
        VkCommandBufferResetFlags flags) {
        on_vkResetCommandBuffer(pool, boxed_commandBuffer, flags);
    }

    void on_vkCmdBindPipeline(android::base::Pool* pool,
                              VkCommandBuffer boxed_commandBuffer,
                              VkPipelineBindPoint pipelineBindPoint,
                              VkPipeline pipeline) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        vk->vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
        if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
            AutoLock lock(mLock);
            auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
                    cmdBufferInfoIt->second.computePipeline = pipeline;
                }
            }
        }
    }

    void on_vkCmdBindDescriptorSets(android::base::Pool* pool,
                                    VkCommandBuffer boxed_commandBuffer,
                                    VkPipelineBindPoint pipelineBindPoint,
                                    VkPipelineLayout layout,
                                    uint32_t firstSet,
                                    uint32_t descriptorSetCount,
                                    const VkDescriptorSet* pDescriptorSets,
                                    uint32_t dynamicOffsetCount,
                                    const uint32_t* pDynamicOffsets) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        vk->vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout,
                                    firstSet, descriptorSetCount,
                                    pDescriptorSets, dynamicOffsetCount,
                                    pDynamicOffsets);
        if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
            AutoLock lock(mLock);
            auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                cmdBufferInfoIt->second.descriptorLayout = layout;
                if (descriptorSetCount) {
                    cmdBufferInfoIt->second.firstSet = firstSet;
                    cmdBufferInfoIt->second.descriptorSets.assign(
                            pDescriptorSets,
                            pDescriptorSets + descriptorSetCount);
                }
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

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_IMPL(type) \
    type new_boxed_##type(type underlying, VulkanDispatch* dispatch, bool ownDispatch) { \
        DispatchableHandleInfo<type> item; \
        item.underlying = underlying; \
        item.dispatch = dispatch ? dispatch : new VulkanDispatch; \
        item.ownDispatch = ownDispatch; \
        auto res = (type)mBoxedDispatchableHandles_##type.add(item, Tag_##type); \
        return res; \
    } \
    void delete_boxed_##type(type boxed) { \
        mBoxedDispatchableHandles_##type.remove((uint64_t)boxed); \
    } \
    type unbox_##type(type boxed) { \
        AutoLock lock(mBoxedDispatchableHandles_##type.lock); \
        auto elt = mBoxedDispatchableHandles_##type.getLocked( \
            (uint64_t)(uintptr_t)boxed); \
        if (!elt) return VK_NULL_HANDLE; \
        return elt->underlying; \
    } \
    type unboxed_to_boxed_##type(type unboxed) { \
        AutoLock lock(mBoxedDispatchableHandles_##type.lock); \
        return (type)mBoxedDispatchableHandles_##type.getBoxedFromUnboxedLocked( \
            (uint64_t)(uintptr_t)unboxed); \
    } \
    VulkanDispatch* dispatch_##type(type boxed) { \
        AutoLock lock(mBoxedDispatchableHandles_##type.lock); \
        auto elt = mBoxedDispatchableHandles_##type.getLocked( \
            (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: err not found boxed %p\n", __func__, boxed); return nullptr; } \
        return elt->dispatch; \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_IMPL(type) \
    type new_boxed_non_dispatchable_##type(type underlying) { \
        NonDispatchableHandleInfo<type> item; \
        item.underlying = underlying; \
        auto res = (type)mBoxedNonDispatchableHandles_##type.add(item, Tag_##type); \
        return res; \
    } \
    void delete_boxed_non_dispatchable_##type(type boxed) { \
        mBoxedNonDispatchableHandles_##type.remove((uint64_t)boxed); \
    } \
    type unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        AutoLock lock(mBoxedNonDispatchableHandles_##type.lock); \
        return (type)mBoxedNonDispatchableHandles_##type.getBoxedFromUnboxedLocked( \
            (uint64_t)(uintptr_t)unboxed); \
    } \
    type unbox_non_dispatchable_##type(type boxed) { \
        AutoLock lock(mBoxedNonDispatchableHandles_##type.lock); \
        auto elt = mBoxedNonDispatchableHandles_##type.getLocked( \
            (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: unbox %p failed, not found\n", __func__, boxed); return VK_NULL_HANDLE; } \
        return elt->underlying; \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_IMPL)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_IMPL)

    VkDecoderSnapshot* snapshot() { return &mSnapshot; }

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
            if (!strcmp("VK_ANDROID_external_memory_android_hardware_buffer", extName)) {
#ifdef _WIN32
                res.push_back("VK_KHR_external_memory_win32");
#else
                res.push_back("VK_KHR_external_memory_fd");
#endif
            }
            // External semaphore maps to the win32 version on windows,
            // continues with external semaphore fd on non-windows
            if (!strcmp("VK_KHR_external_semaphore_fd", extName)) {
#ifdef _WIN32
                res.push_back("VK_KHR_external_semaphore_win32");
#else
                res.push_back("VK_KHR_external_semaphore_fd");
#endif
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

    static std::vector<char> loadShaderSource(const char* filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            fprintf(stderr, "WARNING: shader source open failed! %s\n",
                    filename);
            return {};
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
    struct CompressedImageInfo {
        bool isCompressed = false;
        VkDevice device = 0;
        VkFormat compFormat;  // The compressed format
        VkImageType imageType;
        VkImageCreateInfo sizeCompImgCreateInfo;
        std::vector<uint32_t> sizeCompImgQueueFamilyIndices;
        VkFormat sizeCompFormat;  // Size compatible format
        VkDeviceSize alignment = 0;
        std::vector<VkDeviceSize> memoryOffsets = {};
        std::vector<VkImage> sizeCompImgs;  // Size compatible images
        VkFormat decompFormat =
                VK_FORMAT_R8G8B8A8_UNORM;  // Decompressed format
        VkImage decompImg = 0;  // Decompressed image
        VkExtent3D extent;
        uint32_t layerCount;
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
        uint32_t sizeCompMipmapWidth(uint32_t level) {
            return (mipmapWidth(level) + kCompressedTexBlockSize - 1) /
                   kCompressedTexBlockSize;
        }
        uint32_t sizeCompMipmapHeight(uint32_t level) {
            return (mipmapHeight(level) + kCompressedTexBlockSize - 1) /
                   kCompressedTexBlockSize;
        }
        uint32_t sizeCompMipmapDepth(uint32_t level) {
            return mipmapDepth(level);
        }
        VkDeviceSize decompPixelSize() {
            return getLinearFormatPixelSize(decompFormat);
        }
        bool needEmulatedAlpha() {
            if (!isCompressed) {
                return false;
            }
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                    return true;
                default:
                    return false;
            }
        }

        struct PushConstant {
            uint32_t compFormat;
            uint32_t baseLayer;
        };

        VkDescriptorSetLayout decompDescriptorSetLayout = 0;
        VkDescriptorPool decompDescriptorPool = 0;
        std::vector<VkDescriptorSet> decompDescriptorSets = {};
        VkShaderModule decompShader = 0;
        VkPipelineLayout decompPipelineLayout = 0;
        VkPipeline decompPipeline = 0;
        std::vector<VkImageView> sizeCompImageViews = {};
        std::vector<VkImageView> decompImageViews = {};

        static VkImageView createDefaultImageView(
                goldfish_vk::VulkanDispatch* vk,
                VkDevice device,
                VkImage image,
                VkFormat format,
                VkImageType imageType,
                uint32_t mipLevel,
                uint32_t layerCount) {
            VkImageViewCreateInfo imageViewInfo = {};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = image;
            imageViewInfo.viewType = imageType == VK_IMAGE_TYPE_3D
                                             ? VK_IMAGE_VIEW_TYPE_3D
                                             : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            imageViewInfo.format = format;
            imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewInfo.subresourceRange.aspectMask =
                    VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = mipLevel;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = layerCount;
            VkImageView imageView;
            if (VK_SUCCESS != vk->vkCreateImageView(device, &imageViewInfo,
                                                    nullptr, &imageView)) {
                fprintf(stderr, "Warning: %s %s:%d failure\n", __func__,
                        __FILE__, __LINE__);
                return 0;
            }
            return imageView;
        }

        VkResult initDecomp(goldfish_vk::VulkanDispatch* vk,
                            VkDevice device,
                            VkImage image) {
            if (decompPipeline != 0) {
                return VK_SUCCESS;
            }
            // TODO: release resources on failure

#define _RETURN_ON_FAILURE(cmd)                                                \
    {                                                                          \
        VkResult result = cmd;                                                 \
        if (VK_SUCCESS != result) {                                            \
            fprintf(stderr, "Warning: %s %s:%d vulkan failure %d\n", __func__, \
                    __FILE__, __LINE__, result);                               \
            return (result);                                                   \
        }                                                                      \
    }

            std::string shaderSrcFileName;
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                    shaderSrcFileName = "Etc2RGB8_";
                    break;
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                    shaderSrcFileName = "Etc2RGBA8_";
                    break;
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                    shaderSrcFileName = "EacR11Unorm_";
                    break;
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                    shaderSrcFileName = "EacR11Snorm_";
                    break;
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                    shaderSrcFileName = "EacRG11Unorm_";
                    break;
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    shaderSrcFileName = "EacRG11Snorm_";
                    break;
                default:
                    shaderSrcFileName = "Etc2RGB8_";
                    break;
            }
            if (imageType == VK_IMAGE_TYPE_3D) {
                shaderSrcFileName += "3D.spv";
            } else {
                shaderSrcFileName += "2DArray.spv";
            }
            using android::base::pj;
            using android::base::System;
            const std::string fullPath =
                    pj(System::get()->getProgramDirectory(), "lib64", "vulkan",
                       "shaders", shaderSrcFileName);
            std::vector<char> shaderSource = loadShaderSource(fullPath.c_str());
            VkShaderModuleCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.codeSize = shaderSource.size();
            // std::vector aligns the pointer for us, so it is safe to cast
            shaderInfo.pCode =
                    reinterpret_cast<const uint32_t*>(shaderSource.data());
            _RETURN_ON_FAILURE(vk->vkCreateShaderModule(
                    device, &shaderInfo, nullptr, &decompShader));

            VkDescriptorSetLayoutBinding dsLayoutBindings[] = {
                    {
                            0,                                 // bindings
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
                            1,                            // descriptorCount
                            VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
                            0,                            // pImmutableSamplers
                    },
                    {
                            1,                                 // bindings
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
                            1,                            // descriptorCount
                            VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
                            0,                            // pImmutableSamplers
                    },
            };
            VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
            dsLayoutInfo.sType =
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dsLayoutInfo.bindingCount = sizeof(dsLayoutBindings) /
                                        sizeof(VkDescriptorSetLayoutBinding);
            dsLayoutInfo.pBindings = dsLayoutBindings;
            _RETURN_ON_FAILURE(vk->vkCreateDescriptorSetLayout(
                    device, &dsLayoutInfo, nullptr,
                    &decompDescriptorSetLayout));

            VkDescriptorPoolSize poolSize[1] = {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 * mipLevels},
            };
            VkDescriptorPoolCreateInfo dsPoolInfo = {};
            dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            dsPoolInfo.flags =
                    VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            dsPoolInfo.maxSets = mipLevels;
            dsPoolInfo.poolSizeCount = 1;
            dsPoolInfo.pPoolSizes = poolSize;
            _RETURN_ON_FAILURE(vk->vkCreateDescriptorPool(
                    device, &dsPoolInfo, nullptr, &decompDescriptorPool));
            std::vector<VkDescriptorSetLayout> layouts(
                    mipLevels, decompDescriptorSetLayout);

            VkDescriptorSetAllocateInfo dsInfo = {};
            dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsInfo.descriptorPool = decompDescriptorPool;
            dsInfo.descriptorSetCount = mipLevels;
            dsInfo.pSetLayouts = layouts.data();
            decompDescriptorSets.resize(mipLevels);
            _RETURN_ON_FAILURE(vk->vkAllocateDescriptorSets(
                    device, &dsInfo, decompDescriptorSets.data()));

            VkPushConstantRange pushConstant = {};
            pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            pushConstant.offset = 0;
            pushConstant.size = sizeof(PushConstant);

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &decompDescriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
            _RETURN_ON_FAILURE(
                    vk->vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                                               nullptr, &decompPipelineLayout));

            VkComputePipelineCreateInfo computePipelineInfo = {};
            computePipelineInfo.sType =
                    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineInfo.stage.sType =
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            computePipelineInfo.stage.module = decompShader;
            computePipelineInfo.stage.pName = "main";
            computePipelineInfo.layout = decompPipelineLayout;
            _RETURN_ON_FAILURE(vk->vkCreateComputePipelines(
                    device, 0, 1, &computePipelineInfo, nullptr,
                    &decompPipeline));

            VkFormat intermediateFormat = decompFormat;
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                    intermediateFormat = VK_FORMAT_R8G8B8A8_UINT;
                    break;
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    intermediateFormat = decompFormat;
                    break;
                default:
                    intermediateFormat = decompFormat;
                    break;  // should not happen
            }

            sizeCompImageViews.resize(mipLevels);
            decompImageViews.resize(mipLevels);
            VkDescriptorImageInfo sizeCompDescriptorImageInfo[1] = {{}};
            sizeCompDescriptorImageInfo[0].sampler = 0;
            sizeCompDescriptorImageInfo[0].imageLayout =
                    VK_IMAGE_LAYOUT_GENERAL;

            VkDescriptorImageInfo decompDescriptorImageInfo[1] = {{}};
            decompDescriptorImageInfo[0].sampler = 0;
            decompDescriptorImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet writeDescriptorSets[2] = {{}, {}};
            writeDescriptorSets[0].sType =
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[0].dstBinding = 0;
            writeDescriptorSets[0].descriptorCount = 1;
            writeDescriptorSets[0].descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[0].pImageInfo = sizeCompDescriptorImageInfo;

            writeDescriptorSets[1].sType =
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[1].dstBinding = 1;
            writeDescriptorSets[1].descriptorCount = 1;
            writeDescriptorSets[1].descriptorType =
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[1].pImageInfo = decompDescriptorImageInfo;

            for (uint32_t i = 0; i < mipLevels; i++) {
                sizeCompImageViews[i] = createDefaultImageView(
                        vk, device, sizeCompImgs[i], sizeCompFormat, imageType,
                        0, layerCount);
                decompImageViews[i] = createDefaultImageView(
                        vk, device, decompImg, intermediateFormat, imageType, i,
                        layerCount);
                sizeCompDescriptorImageInfo[0].imageView =
                        sizeCompImageViews[i];
                decompDescriptorImageInfo[0].imageView = decompImageViews[i];
                writeDescriptorSets[0].dstSet = decompDescriptorSets[i];
                writeDescriptorSets[1].dstSet = decompDescriptorSets[i];
                vk->vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0,
                                           nullptr);
            }
            return VK_SUCCESS;
        }

        void cmdDecompress(goldfish_vk::VulkanDispatch* vk,
                           VkCommandBuffer commandBuffer,
                           VkPipelineStageFlags dstStageMask,
                           VkImageLayout newLayout,
                           VkAccessFlags dstAccessMask,
                           uint32_t baseMipLevel,
                           uint32_t levelCount,
                           uint32_t baseLayer,
                           uint32_t _layerCount) {
            vk->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                  decompPipeline);

            PushConstant pushConstant = {.compFormat = compFormat,
                                         .baseLayer = baseLayer};
            int dispatchZ = _layerCount;
            if (extent.depth > 1) {
                // 3D texture
                pushConstant.baseLayer = 0;
                dispatchZ = extent.depth;
            }
            vk->vkCmdPushConstants(commandBuffer, decompPipelineLayout,
                                   VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                   sizeof(pushConstant), &pushConstant);
            for (uint32_t i = baseMipLevel; i < baseMipLevel + levelCount;
                 i++) {
                vk->vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        decompPipelineLayout, 0, 1,
                        decompDescriptorSets.data() + i, 0, nullptr);

                vk->vkCmdDispatch(commandBuffer, sizeCompMipmapWidth(i),
                                  sizeCompMipmapHeight(i), dispatchZ);
            }
        }
    };

    void createSizeCompImages(goldfish_vk::VulkanDispatch* vk,
                              CompressedImageInfo* cmpInfo) {
        if (cmpInfo->sizeCompImgs.size() > 0) {
            return;
        }
        VkDevice device = cmpInfo->device;
        uint32_t mipLevels = cmpInfo->mipLevels;
        cmpInfo->sizeCompImgs.resize(mipLevels);
        VkImageCreateInfo imageInfo = cmpInfo->sizeCompImgCreateInfo;
        imageInfo.mipLevels = 1;
        for (uint32_t i = 0; i < mipLevels; i++) {
            imageInfo.extent.width = cmpInfo->sizeCompMipmapWidth(i);
            imageInfo.extent.height = cmpInfo->sizeCompMipmapHeight(i);
            imageInfo.extent.depth = cmpInfo->sizeCompMipmapDepth(i);
            VkDevice device = cmpInfo->device;
            vk->vkCreateImage(device, &imageInfo, nullptr,
                              cmpInfo->sizeCompImgs.data() + i);
        }

        VkPhysicalDevice physicalDevice = mDeviceInfo[device].physicalDevice;
        int32_t memIdx = -1;
        VkDeviceSize& alignment = cmpInfo->alignment;
        std::vector<VkDeviceSize> memSizes(mipLevels);
        VkDeviceSize decompImageSize = 0;
        {
            VkMemoryRequirements memRequirements;
            vk->vkGetImageMemoryRequirements(device, cmpInfo->decompImg,
                                             &memRequirements);
            memIdx = findProperties(physicalDevice,
                                    memRequirements.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (memIdx < 0) {
                fprintf(stderr, "Error: cannot find memory property!\n");
                return;
            }
            alignment = std::max(alignment, memRequirements.alignment);
            decompImageSize = memRequirements.size;
        }
        for (size_t i = 0; i < mipLevels; i++) {
            VkMemoryRequirements memRequirements;
            vk->vkGetImageMemoryRequirements(device, cmpInfo->sizeCompImgs[i],
                                             &memRequirements);
            alignment = std::max(alignment, memRequirements.alignment);
            memSizes[i] = memRequirements.size;
        }
        auto& memoryOffsets = cmpInfo->memoryOffsets;
        memoryOffsets.resize(mipLevels + 1);
        {
            VkDeviceSize alignedSize = decompImageSize;
            if (alignment != 0) {
                alignedSize =
                        (alignedSize + alignment - 1) / alignment * alignment;
            }
            memoryOffsets[0] = alignedSize;
        }
        for (size_t i = 0; i < cmpInfo->sizeCompImgs.size(); i++) {
            VkDeviceSize alignedSize = memSizes[i];
            if (alignment != 0) {
                alignedSize =
                        (alignedSize + alignment - 1) / alignment * alignment;
            }
            memoryOffsets[i + 1] = memoryOffsets[i] + alignedSize;
        }
    }

    void updateImageMemorySizeLocked(
            VkDevice device,
            VkImage image,
            VkMemoryRequirements* pMemoryRequirements) {
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (!deviceInfoIt->second.emulateTextureEtc2) {
            return;
        }
        auto it = mImageInfo.find(image);
        if (it == mImageInfo.end()) {
            return;
        }
        CompressedImageInfo& cmpInfo = it->second.cmpInfo;
        if (!cmpInfo.isCompressed) {
            return;
        }
        pMemoryRequirements->alignment =
                std::max(pMemoryRequirements->alignment, cmpInfo.alignment);
        pMemoryRequirements->size += cmpInfo.memoryOffsets[cmpInfo.mipLevels];
    }

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

    VkFormat getDecompFormat(VkFormat compFmt) {
        switch (compFmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                return VK_FORMAT_R16_UNORM;
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return VK_FORMAT_R16_SNORM;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                return VK_FORMAT_R16G16_UNORM;
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return VK_FORMAT_R16G16_SNORM;
            default:
                return compFmt;
        }
    }

    VkFormat getSizeCompFormat(VkFormat compFmt) {
        switch (compFmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                return VK_FORMAT_R16G16B16A16_UINT;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return VK_FORMAT_R32G32B32A32_UINT;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return VK_FORMAT_R32G32_UINT;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return VK_FORMAT_R32G32B32A32_UINT;
            default:
                return compFmt;
        }
    }

    bool isEtc2Compatible(VkFormat compFmt1, VkFormat compFmt2) {
        const VkFormat kCmpSets[][2] = {
                {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
                 VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
                {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
                 VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
                {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
                 VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
                {VK_FORMAT_EAC_R11_UNORM_BLOCK, VK_FORMAT_EAC_R11_SNORM_BLOCK},
                {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
                 VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
        };
        if (compFmt1 == compFmt2) {
            return true;
        }
        for (auto& cmpSet : kCmpSets) {
            if (compFmt1 == cmpSet[0] || compFmt1 == cmpSet[1]) {
                return compFmt2 == cmpSet[0] || compFmt2 == cmpSet[1];
            }
        }
        return false;
    }
    CompressedImageInfo createCompressedImageInfo(VkFormat compFmt) {
        CompressedImageInfo cmpInfo;
        cmpInfo.compFormat = compFmt;
        cmpInfo.decompFormat = getDecompFormat(compFmt);
        cmpInfo.sizeCompFormat = getSizeCompFormat(compFmt);
        cmpInfo.isCompressed = (cmpInfo.decompFormat != compFmt);

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
                getPhysicalDeviceFormatPropertiesFunc(physicalDevice,
                                                      cmpInfo.decompFormat,
                                                      pFormatProperties);
                maskFormatPropertiesForEmulatedEtc2(pFormatProperties);
                break;
            }
            default:
                getPhysicalDeviceFormatPropertiesFunc(physicalDevice, format,
                                                      pFormatProperties);
                break;
        }
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
                devicesToDestroyDispatches.push_back(
                    dispatch_VkDevice(
                        mDeviceInfo[it.first].boxed));
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
        VkDevice device = 0;
        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer boxed = nullptr;
        VkPipeline computePipeline = 0;
        uint32_t firstSet = 0;
        VkPipelineLayout descriptorLayout = 0;
        std::vector<VkDescriptorSet> descriptorSets;
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

    bool isDescriptorTypeImageInfo(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
               (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
               (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
               (descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
               (descType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    }

    bool isDescriptorTypeBufferInfo(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
               (descType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
               (descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
               (descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    }

    bool isDescriptorTypeBufferView(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
               (descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
    }

    struct DescriptorUpdateTemplateInfo {
        VkDescriptorUpdateTemplateCreateInfo createInfo;
        std::vector<VkDescriptorUpdateTemplateEntry>
            linearizedTemplateEntries;
        // Preallocated pData
        std::vector<uint8_t> data;
        size_t imageInfoStart;
        size_t bufferInfoStart;
        size_t bufferViewStart;
    };

    DescriptorUpdateTemplateInfo calcLinearizedDescriptorUpdateTemplateInfo(
        const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo) {

        DescriptorUpdateTemplateInfo res;
        res.createInfo = *pCreateInfo;

        size_t numImageInfos = 0;
        size_t numBufferInfos = 0;
        size_t numBufferViews = 0;

        for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; ++i) {
            const auto& entry = pCreateInfo->pDescriptorUpdateEntries[i];
            auto type = entry.descriptorType;
            auto count = entry.descriptorCount;
            if (isDescriptorTypeImageInfo(type)) {
                numImageInfos += count;
            } else if (isDescriptorTypeBufferInfo(type)) {
                numBufferInfos += count;
            } else if (isDescriptorTypeBufferView(type)) {
                numBufferViews += count;
            } else {
                fprintf(stderr, "%s: fatal: unknown descriptor type 0x%x\n", __func__, type);
                abort();
            }
        }

        size_t imageInfoBytes = numImageInfos * sizeof(VkDescriptorImageInfo);
        size_t bufferInfoBytes = numBufferInfos * sizeof(VkDescriptorBufferInfo);
        size_t bufferViewBytes = numBufferViews * sizeof(VkBufferView);

        res.data.resize(imageInfoBytes + bufferInfoBytes + bufferViewBytes);
        res.imageInfoStart = 0;
        res.bufferInfoStart = imageInfoBytes;
        res.bufferViewStart = imageInfoBytes + bufferInfoBytes;

        size_t imageInfoCount = 0;
        size_t bufferInfoCount = 0;
        size_t bufferViewCount = 0;

        for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; ++i) {

            const auto& entry = pCreateInfo->pDescriptorUpdateEntries[i];
            VkDescriptorUpdateTemplateEntry entryForHost = entry;

            auto type = entry.descriptorType;
            auto count = entry.descriptorCount;

            if (isDescriptorTypeImageInfo(type)) {
                entryForHost.offset =
                    res.imageInfoStart +
                    imageInfoCount * sizeof(VkDescriptorImageInfo);
                entryForHost.stride = sizeof(VkDescriptorImageInfo);
                ++imageInfoCount;
            } else if (isDescriptorTypeBufferInfo(type)) {
                entryForHost.offset =
                    res.bufferInfoStart +
                    bufferInfoCount * sizeof(VkDescriptorBufferInfo);
                entryForHost.stride = sizeof(VkDescriptorBufferInfo);
                ++bufferInfoCount;
            } else if (isDescriptorTypeBufferView(type)) {
                entryForHost.offset =
                    res.bufferViewStart +
                    bufferViewCount * sizeof(VkBufferView);
                entryForHost.stride = sizeof(VkBufferView);
                ++bufferViewCount;
            } else {
                fprintf(stderr, "%s: fatal: unknown descriptor type 0x%x\n", __func__, type);
                abort();
            }

            res.linearizedTemplateEntries.push_back(entryForHost);
        }

        res.createInfo.pDescriptorUpdateEntries =
            res.linearizedTemplateEntries.data();

        return res;
    }

    void registerDescriptorUpdateTemplate(
        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
        const DescriptorUpdateTemplateInfo& info) {
        AutoLock lock(mLock);
        mDescriptorUpdateTemplateInfo[descriptorUpdateTemplate] = info;
    }

    void unregisterDescriptorUpdateTemplate(
        VkDescriptorUpdateTemplate descriptorUpdateTemplate) {
        AutoLock lock(mLock);
        mDescriptorUpdateTemplateInfo.erase(descriptorUpdateTemplate);
    }

    // Returns the momory property index when succeeds; returns -1 when fails.
    int32_t findProperties(VkPhysicalDevice physicalDevice,
                           uint32_t memoryTypeBitsRequirement,
                           VkMemoryPropertyFlags requiredProperties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        auto ivk = dispatch_VkInstance(
                mInstanceInfo[mPhysicalDeviceToInstance[physicalDevice]].boxed);

        ivk->vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                                 &memProperties);

        const uint32_t memoryCount = memProperties.memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryCount;
             ++memoryIndex) {
            const uint32_t memoryTypeBits = (1 << memoryIndex);
            const bool isRequiredMemoryType =
                    memoryTypeBitsRequirement & memoryTypeBits;

            const VkMemoryPropertyFlags properties =
                    memProperties.memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties =
                    (properties & requiredProperties) == requiredProperties;

            if (isRequiredMemoryType && hasRequiredProperties)
                return static_cast<int32_t>(memoryIndex);
        }

        // failed to find memory type
        return -1;
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
        VkInstance boxed = nullptr;
    };

    struct PhysicalDeviceInfo {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkPhysicalDevice boxed = nullptr;
    };

    struct DeviceInfo {
        std::unordered_map<uint32_t, std::vector<VkQueue>> queues;
        bool emulateTextureEtc2 = false;
        VkPhysicalDevice physicalDevice;
        VkDevice boxed = nullptr;
    };

    struct QueueInfo {
        VkDevice device;
        uint32_t queueFamilyIndex;
        VkQueue boxed = nullptr;
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

    struct SemaphoreInfo {
        int externalHandleId = 0;
        VK_EXT_MEMORY_HANDLE externalHandle =
            VK_EXT_MEMORY_HANDLE_INVALID;
    };

#define DEFINE_BOXED_HANDLE_TYPE_TAG(type) \
        Tag_##type, \

    enum BoxedHandleTypeTag {
        Tag_Invalid = 0,
        GOLDFISH_VK_LIST_HANDLE_TYPES(DEFINE_BOXED_HANDLE_TYPE_TAG)
    };

    template <class T>
    class BoxedHandleManager {
    public:
        using Store = android::base::EntityManager<32, 16, 16, T>;

        Lock lock;
        Store store;
        std::unordered_map<uint64_t, uint64_t> reverseMap;

        uint64_t add(const T& item, BoxedHandleTypeTag tag) {
            AutoLock l(lock);
            auto res = (uint64_t)store.add(item, (size_t)tag);
            reverseMap[(uint64_t)(item.underlying)] = res;
            return res;
        }

        void remove(uint64_t h) {
            AutoLock l(lock);
            auto item = getLocked(h);
            if (item) {
                reverseMap.erase((uint64_t)(item->underlying));
            }
            store.remove(h);
        }

        T* getLocked(uint64_t h) {
            return store.get(h);
        }

        uint64_t getBoxedFromUnboxedLocked(uint64_t unboxed) {
            auto res = android::base::find(reverseMap, unboxed);
            if (!res) return 0;
            return *res;
        }
    };

    template <class T>
    class DispatchableHandleInfo {
    public:
        T underlying;
        VulkanDispatch* dispatch = nullptr;
        bool ownDispatch = false;
    };

    template <class T>
    class NonDispatchableHandleInfo {
    public:
        T underlying;
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

    std::unordered_map<VkSemaphore, SemaphoreInfo> mSemaphoreInfo;
#ifdef _WIN32
    int mSemaphoreId = 1;
    int genSemaphoreId() {
        if (mSemaphoreId == -1) {
            mSemaphoreId = 1;
        }
        int res = mSemaphoreId;
        ++mSemaphoreId;
        return res;
    }
    std::unordered_map<int, VkSemaphore> mExternalSemaphoresById;
#endif
    std::unordered_map<VkDescriptorUpdateTemplate, DescriptorUpdateTemplateInfo> mDescriptorUpdateTemplateInfo;

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_STORE(type) \
    BoxedHandleManager<DispatchableHandleInfo<type>> mBoxedDispatchableHandles_##type;

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_STORE(type) \
    BoxedHandleManager<NonDispatchableHandleInfo<type>> mBoxedNonDispatchableHandles_##type;

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_STORE)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_STORE)

    VkDecoderSnapshot mSnapshot;
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

VkResult VkDecoderGlobalState::on_vkEnumerateInstanceVersion(
    android::base::Pool* pool,
    uint32_t* pApiVersion) {
    return mImpl->on_vkEnumerateInstanceVersion(pool, pApiVersion);
}

VkResult VkDecoderGlobalState::on_vkCreateInstance(
    android::base::Pool* pool,
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    return mImpl->on_vkCreateInstance(pool, pCreateInfo, pAllocator, pInstance);
}

void VkDecoderGlobalState::on_vkDestroyInstance(
    android::base::Pool* pool,
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyInstance(pool, instance, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkEnumeratePhysicalDevices(
    android::base::Pool* pool,
    VkInstance instance,
    uint32_t* physicalDeviceCount,
    VkPhysicalDevice* physicalDevices) {
    return mImpl->on_vkEnumeratePhysicalDevices(pool, instance, physicalDeviceCount, physicalDevices);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures(pool, physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(pool, physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2KHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2KHR* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(pool, physicalDevice, pFeatures);
}

VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties(
            pool, physicalDevice, format, type, tiling, usage, flags,
            pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            pool, physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2KHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            pool, physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties(pool, physicalDevice, format,
                                                  pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(pool, physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2KHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(pool, physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2KHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties(
        pool, physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        pool, physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2KHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        pool, physicalDevice, pMemoryProperties);
}

VkResult VkDecoderGlobalState::on_vkCreateDevice(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    return mImpl->on_vkCreateDevice(pool, physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void VkDecoderGlobalState::on_vkGetDeviceQueue(
    android::base::Pool* pool,
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    mImpl->on_vkGetDeviceQueue(pool, device, queueFamilyIndex, queueIndex, pQueue);
}

void VkDecoderGlobalState::on_vkDestroyDevice(
    android::base::Pool* pool,
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDevice(pool, device, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateBuffer(
    android::base::Pool* pool,
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer) {
    return mImpl->on_vkCreateBuffer(pool, device, pCreateInfo, pAllocator, pBuffer);
}

void VkDecoderGlobalState::on_vkDestroyBuffer(
    android::base::Pool* pool,
    VkDevice device,
    VkBuffer buffer,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyBuffer(pool, device, buffer, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory(
    android::base::Pool* pool,
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset) {
    return mImpl->on_vkBindBufferMemory(pool, device, buffer, memory, memoryOffset);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2(
    android::base::Pool* pool,
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2(pool, device, bindInfoCount, pBindInfos);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2KHR(
    android::base::Pool* pool,
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2KHR(pool, device, bindInfoCount, pBindInfos);
}

VkResult VkDecoderGlobalState::on_vkCreateImage(
    android::base::Pool* pool,
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage* pImage) {
    return mImpl->on_vkCreateImage(pool, device, pCreateInfo, pAllocator, pImage);
}

void VkDecoderGlobalState::on_vkDestroyImage(
    android::base::Pool* pool,
    VkDevice device,
    VkImage image,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImage(pool, device, image, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkBindImageMemory(android::base::Pool* pool,
                                                    VkDevice device,
                                                    VkImage image,
                                                    VkDeviceMemory memory,
                                                    VkDeviceSize memoryOffset) {
    return mImpl->on_vkBindImageMemory(pool, device, image, memory,
                                       memoryOffset);
}

VkResult VkDecoderGlobalState::on_vkCreateImageView(
    android::base::Pool* pool,
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView) {
    return mImpl->on_vkCreateImageView(pool, device, pCreateInfo, pAllocator, pView);
}

void VkDecoderGlobalState::on_vkDestroyImageView(
    android::base::Pool* pool,
    VkDevice device,
    VkImageView imageView,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImageView(pool, device, imageView, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateSampler(
    android::base::Pool* pool,
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSampler* pSampler) {
    return mImpl->on_vkCreateSampler(pool, device, pCreateInfo, pAllocator, pSampler);
}

void VkDecoderGlobalState::on_vkDestroySampler(
    android::base::Pool* pool,
    VkDevice device,
    VkSampler sampler,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroySampler(pool, device, sampler, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateSemaphore(
    android::base::Pool* pool,
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore) {
    return mImpl->on_vkCreateSemaphore(pool, device, pCreateInfo, pAllocator, pSemaphore);
}

VkResult VkDecoderGlobalState::on_vkImportSemaphoreFdKHR(
    android::base::Pool* pool,
    VkDevice device,
    const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) {
    return mImpl->on_vkImportSemaphoreFdKHR(pool, device, pImportSemaphoreFdInfo);
}

VkResult VkDecoderGlobalState::on_vkGetSemaphoreFdKHR(
    android::base::Pool* pool,
    VkDevice device,
    const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
    int* pFd) {
    return mImpl->on_vkGetSemaphoreFdKHR(pool, device, pGetFdInfo, pFd);
}

void VkDecoderGlobalState::on_vkDestroySemaphore(
    android::base::Pool* pool,
    VkDevice device,
    VkSemaphore semaphore,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroySemaphore(pool, device, semaphore, pAllocator);
}

void VkDecoderGlobalState::on_vkUpdateDescriptorSets(
    android::base::Pool* pool,
    VkDevice device,
    uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies) {
    mImpl->on_vkUpdateDescriptorSets(pool, device, descriptorWriteCount,
                                     pDescriptorWrites, descriptorCopyCount,
                                     pDescriptorCopies);
}

void VkDecoderGlobalState::on_vkCmdCopyBufferToImage(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions) {
    mImpl->on_vkCmdCopyBufferToImage(pool, commandBuffer, srcBuffer, dstImage,
            dstImageLayout, regionCount, pRegions);
}

void VkDecoderGlobalState::on_vkCmdCopyImageToBuffer(
        android::base::Pool* pool,
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkBuffer dstBuffer,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions) {
    mImpl->on_vkCmdCopyImageToBuffer(pool, commandBuffer, srcImage,
                                     srcImageLayout, dstBuffer, regionCount,
                                     pRegions);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements(
        android::base::Pool* pool,
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements(pool, device, image,
                                           pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements2(
        android::base::Pool* pool,
        VkDevice device,
        const VkImageMemoryRequirementsInfo2* pInfo,
        VkMemoryRequirements2* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements2(pool, device, pInfo,
                                            pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements2KHR(
        android::base::Pool* pool,
        VkDevice device,
        const VkImageMemoryRequirementsInfo2* pInfo,
        VkMemoryRequirements2* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements2(pool, device, pInfo,
                                            pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkCmdPipelineBarrier(
        android::base::Pool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const VkImageMemoryBarrier* pImageMemoryBarriers) {
    mImpl->on_vkCmdPipelineBarrier(
            pool, commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
            memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
            pBufferMemoryBarriers, imageMemoryBarrierCount,
            pImageMemoryBarriers);
}

VkResult VkDecoderGlobalState::on_vkAllocateMemory(
    android::base::Pool* pool,
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory) {
    return mImpl->on_vkAllocateMemory(pool, device, pAllocateInfo, pAllocator, pMemory);
}

void VkDecoderGlobalState::on_vkFreeMemory(
    android::base::Pool* pool,
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkFreeMemory(pool, device, memory, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkMapMemory(
    android::base::Pool* pool,
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData) {
    return mImpl->on_vkMapMemory(pool, device, memory, offset, size, flags, ppData);
}

void VkDecoderGlobalState::on_vkUnmapMemory(
    android::base::Pool* pool,
    VkDevice device,
    VkDeviceMemory memory) {
    mImpl->on_vkUnmapMemory(pool, device, memory);
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
    android::base::Pool* pool,
    VkDevice device,
    VkFormat format,
    VkImageUsageFlags imageUsage,
    int* grallocUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsageANDROID(
        pool, device, format, imageUsage, grallocUsage);
}

VkResult VkDecoderGlobalState::on_vkGetSwapchainGrallocUsage2ANDROID(
    android::base::Pool* pool,
    VkDevice device,
    VkFormat format,
    VkImageUsageFlags imageUsage,
    VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
    uint64_t* grallocConsumerUsage,
    uint64_t* grallocProducerUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsage2ANDROID(
        pool, device, format, imageUsage,
        swapchainImageUsage,
        grallocConsumerUsage,
        grallocProducerUsage);
}

VkResult VkDecoderGlobalState::on_vkAcquireImageANDROID(
    android::base::Pool* pool,
    VkDevice device,
    VkImage image,
    int nativeFenceFd,
    VkSemaphore semaphore,
    VkFence fence) {
    return mImpl->on_vkAcquireImageANDROID(
        pool, device, image, nativeFenceFd, semaphore, fence);
}

VkResult VkDecoderGlobalState::on_vkQueueSignalReleaseImageANDROID(
    android::base::Pool* pool,
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    VkImage image,
    int* pNativeFenceFd) {
    return mImpl->on_vkQueueSignalReleaseImageANDROID(
        pool, queue, waitSemaphoreCount, pWaitSemaphores,
        image, pNativeFenceFd);
}

// VK_GOOGLE_address_space
VkResult VkDecoderGlobalState::on_vkMapMemoryIntoAddressSpaceGOOGLE(
    android::base::Pool* pool,
    VkDevice device, VkDeviceMemory memory, uint64_t* pAddress) {
    return mImpl->on_vkMapMemoryIntoAddressSpaceGOOGLE(
        pool, device, memory, pAddress);
}

// VK_GOOGLE_color_buffer
VkResult VkDecoderGlobalState::on_vkRegisterImageColorBufferGOOGLE(
    android::base::Pool* pool,
    VkDevice device, VkImage image, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterImageColorBufferGOOGLE(
        pool, device, image, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkRegisterBufferColorBufferGOOGLE(
    android::base::Pool* pool,
    VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterBufferColorBufferGOOGLE(
        pool, device, buffer, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkAllocateCommandBuffers(
    android::base::Pool* pool,
    VkDevice device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkAllocateCommandBuffers(pool, device, pAllocateInfo,
                                              pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkCreateCommandPool(
    android::base::Pool* pool,
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool) {
    return mImpl->on_vkCreateCommandPool(pool, device, pCreateInfo, pAllocator,
                                         pCommandPool);
}

void VkDecoderGlobalState::on_vkDestroyCommandPool(
    android::base::Pool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyCommandPool(pool, device, commandPool, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkResetCommandPool(
    android::base::Pool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags) {
    return mImpl->on_vkResetCommandPool(pool, device, commandPool, flags);
}

void VkDecoderGlobalState::on_vkCmdExecuteCommands(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkCmdExecuteCommands(pool, commandBuffer, commandBufferCount,
                                          pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkQueueSubmit(
    android::base::Pool* pool,
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence) {
    return mImpl->on_vkQueueSubmit(pool, queue, submitCount, pSubmits, fence);
}

VkResult VkDecoderGlobalState::on_vkResetCommandBuffer(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags) {
    return mImpl->on_vkResetCommandBuffer(pool, commandBuffer, flags);
}

void VkDecoderGlobalState::on_vkFreeCommandBuffers(
    android::base::Pool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkFreeCommandBuffers(pool, device, commandPool,
                                          commandBufferCount, pCommandBuffers);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceExternalSemaphoreProperties(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {
    return mImpl->on_vkGetPhysicalDeviceExternalSemaphoreProperties(
            pool, physicalDevice, pExternalSemaphoreInfo,
            pExternalSemaphoreProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(
    android::base::Pool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {
    return mImpl->on_vkGetPhysicalDeviceExternalSemaphoreProperties(
            pool, physicalDevice, pExternalSemaphoreInfo,
            pExternalSemaphoreProperties);
}

// Descriptor update templates
VkResult VkDecoderGlobalState::on_vkCreateDescriptorUpdateTemplate(
        android::base::Pool* pool,
    VkDevice boxed_device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
    return mImpl->on_vkCreateDescriptorUpdateTemplate(
        pool, boxed_device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult VkDecoderGlobalState::on_vkCreateDescriptorUpdateTemplateKHR(
        android::base::Pool* pool,
    VkDevice boxed_device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
    return mImpl->on_vkCreateDescriptorUpdateTemplateKHR(
        pool, boxed_device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorUpdateTemplate(
        android::base::Pool* pool,
    VkDevice boxed_device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorUpdateTemplate(
        pool, boxed_device, descriptorUpdateTemplate, pAllocator);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorUpdateTemplateKHR(
        android::base::Pool* pool,
    VkDevice boxed_device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorUpdateTemplateKHR(
        pool, boxed_device, descriptorUpdateTemplate, pAllocator);
}

void VkDecoderGlobalState::on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
        android::base::Pool* pool,
    VkDevice boxed_device,
    VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    uint32_t imageInfoCount,
    uint32_t bufferInfoCount,
    uint32_t bufferViewCount,
    const uint32_t* pImageInfoEntryIndices,
    const uint32_t* pBufferInfoEntryIndices,
    const uint32_t* pBufferViewEntryIndices,
    const VkDescriptorImageInfo* pImageInfos,
    const VkDescriptorBufferInfo* pBufferInfos,
    const VkBufferView* pBufferViews) {
    mImpl->on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
        pool, boxed_device,
        descriptorSet,
        descriptorUpdateTemplate,
        imageInfoCount,
        bufferInfoCount,
        bufferViewCount,
        pImageInfoEntryIndices,
        pBufferInfoEntryIndices,
        pBufferViewEntryIndices,
        pImageInfos,
        pBufferInfos,
        pBufferViews);
}

VkResult VkDecoderGlobalState::on_vkBeginCommandBuffer(
        android::base::Pool* pool,
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo) {
    return mImpl->on_vkBeginCommandBuffer(pool, commandBuffer, pBeginInfo);
}

void VkDecoderGlobalState::on_vkBeginCommandBufferAsyncGOOGLE(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo) {
    mImpl->on_vkBeginCommandBuffer(pool, commandBuffer, pBeginInfo);
}

void VkDecoderGlobalState::on_vkEndCommandBufferAsyncGOOGLE(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer) {
    mImpl->on_vkEndCommandBufferAsyncGOOGLE(
        pool, commandBuffer);
}

void VkDecoderGlobalState::on_vkResetCommandBufferAsyncGOOGLE(
    android::base::Pool* pool,
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags) {
    mImpl->on_vkResetCommandBufferAsyncGOOGLE(
        pool, commandBuffer, flags);
}

void VkDecoderGlobalState::on_vkCmdBindPipeline(
        android::base::Pool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint pipelineBindPoint,
        VkPipeline pipeline) {
    mImpl->on_vkCmdBindPipeline(pool, commandBuffer, pipelineBindPoint,
                                pipeline);
}

void VkDecoderGlobalState::on_vkCmdBindDescriptorSets(
        android::base::Pool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint pipelineBindPoint,
        VkPipelineLayout layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets) {
    mImpl->on_vkCmdBindDescriptorSets(pool, commandBuffer, pipelineBindPoint,
                                      layout, firstSet, descriptorSetCount,
                                      pDescriptorSets, dynamicOffsetCount,
                                      pDynamicOffsets);
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

VkDecoderSnapshot* VkDecoderGlobalState::snapshot() {
    return mImpl->snapshot();
}

#define DEFINE_TRANSFORMED_TYPE_IMPL(type) \
    void VkDecoderGlobalState::transformImpl_##type##_tohost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_tohost(val, count); \
    } \
    void VkDecoderGlobalState::transformImpl_##type##_fromhost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_tohost(val, count); \
    } \

LIST_TRANSFORMED_TYPES(DEFINE_TRANSFORMED_TYPE_IMPL)

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEF(type) \
    type VkDecoderGlobalState::new_boxed_##type(type underlying, VulkanDispatch* dispatch, bool ownDispatch) { \
        return mImpl->new_boxed_##type(underlying, dispatch, ownDispatch); \
    } \
    void VkDecoderGlobalState::delete_boxed_##type(type boxed) { \
        mImpl->delete_boxed_##type(boxed); \
    } \
    type VkDecoderGlobalState::unbox_##type(type boxed) { \
        return mImpl->unbox_##type(boxed); \
    } \
    type VkDecoderGlobalState::unboxed_to_boxed_##type(type unboxed) { \
        return mImpl->unboxed_to_boxed_##type(unboxed); \
    } \
    VulkanDispatch* VkDecoderGlobalState::dispatch_##type(type boxed) { \
        return mImpl->dispatch_##type(boxed); \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEF(type) \
    type VkDecoderGlobalState::new_boxed_non_dispatchable_##type(type underlying) { \
        return mImpl->new_boxed_non_dispatchable_##type(underlying); \
    } \
    void VkDecoderGlobalState::delete_boxed_non_dispatchable_##type(type boxed) { \
        mImpl->delete_boxed_non_dispatchable_##type(boxed); \
    } \
    type VkDecoderGlobalState::unbox_non_dispatchable_##type(type boxed) { \
        return mImpl->unbox_non_dispatchable_##type(boxed); \
    } \
    type VkDecoderGlobalState::unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        return mImpl->unboxed_to_boxed_non_dispatchable_##type(unboxed); \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEF)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEF)

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DEF(type) \
    type unbox_##type(type boxed) { \
        return VkDecoderGlobalState::get()->unbox_##type(boxed); \
    } \
    VulkanDispatch* dispatch_##type(type boxed) { \
        return VkDecoderGlobalState::get()->dispatch_##type(boxed); \
    } \
    type unboxed_to_boxed_##type(type unboxed) { \
        return VkDecoderGlobalState::get()->unboxed_to_boxed_##type(unboxed); \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DEF(type) \
    type new_boxed_non_dispatchable_##type(type underlying) { \
        return VkDecoderGlobalState::get()->new_boxed_non_dispatchable_##type(underlying); \
    } \
    void delete_boxed_non_dispatchable_##type(type boxed) { \
        VkDecoderGlobalState::get()->delete_boxed_non_dispatchable_##type(boxed); \
    } \
    type unbox_non_dispatchable_##type(type boxed) { \
        return VkDecoderGlobalState::get()->unbox_non_dispatchable_##type(boxed); \
    } \
    type unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        return VkDecoderGlobalState::get()->unboxed_to_boxed_non_dispatchable_##type(unboxed); \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DEF)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DEF)

}  // namespace goldfish_vk
