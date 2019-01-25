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
#include "VkCommonOperations.h"

#include "android/base/containers/StaticMap.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Log.h"

#include "VulkanDispatch.h"

#include <iomanip>
#include <ostream>
#include <sstream>

#include <stdio.h>
#include <string.h>

using android::base::LazyInstance;
using android::base::StaticMap;

namespace goldfish_vk {

static LazyInstance<StaticMap<VkDevice, uint32_t>>
sKnownStagingTypeIndices = LAZY_INSTANCE_INIT;

bool getStagingMemoryTypeIndex(
    VulkanDispatch* vk,
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties* memProps,
    uint32_t* typeIndex) {

    auto res = sKnownStagingTypeIndices->get(device);

    if (res) {
        *typeIndex = *res;
        return true;
    }

    VkBufferCreateInfo testCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
        4096,
        // To be a staging buffer, it must support being
        // both a transfer src and dst.
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        // TODO: See if buffers over shared queues need to be
        // considered separately
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
    };

    VkBuffer testBuffer;
    VkResult testBufferCreateRes =
        vk->vkCreateBuffer(device, &testCreateInfo, nullptr, &testBuffer);

    if (testBufferCreateRes != VK_SUCCESS) {
        LOG(ERROR) <<
            "Could not create test buffer "
            "for staging buffer query. VkResult: " <<
                testBufferCreateRes;
        return false;
    }

    VkMemoryRequirements memReqs;
    vk->vkGetBufferMemoryRequirements(device, testBuffer, &memReqs);

    // To be a staging buffer, we need to allow CPU read/write access.
    // Thus, we need the memory type index both to be host visible
    // and to be supported in the memory requirements of the buffer.
    bool foundSuitableStagingMemoryType = false;
    uint32_t stagingMemoryTypeIndex = 0;

    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        const auto& typeInfo = memProps->memoryTypes[i];
        bool hostVisible =
            typeInfo.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        bool hostCached =
            typeInfo.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        bool allowedInBuffer = (1 << i) & memReqs.memoryTypeBits;
        if (hostVisible && hostCached && allowedInBuffer) {
            foundSuitableStagingMemoryType = true;
            stagingMemoryTypeIndex = i;
            break;
        }
    }

    vk->vkDestroyBuffer(device, testBuffer, nullptr);

    if (!foundSuitableStagingMemoryType) {
        std::stringstream ss;
        ss <<
            "Could not find suitable memory type index " <<
            "for staging buffer. Memory type bits: " <<
            std::hex << memReqs.memoryTypeBits << "\n" <<
            "Available host visible memory type indices:" << "\n";
        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
            if (memProps->memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                ss << "Host visible memory type index: %u" << i << "\n";
            }
            if (memProps->memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                ss << "Host cached memory type index: %u" << i << "\n";
            }
        }

        LOG(ERROR) << ss.str();

        return false;
    }

    sKnownStagingTypeIndices->set(device, stagingMemoryTypeIndex);
    *typeIndex = stagingMemoryTypeIndex;

    return true;
}

static VkEmulation* sVkEmulation = nullptr;

static bool extensionsSupported(
    const std::vector<VkExtensionProperties>& currentProps,
    const std::vector<const char*>& wantedExtNames) {

    std::vector<bool> foundExts(wantedExtNames.size(), false);

    for (uint32_t i = 0; i < currentProps.size(); ++i) {
        printf("%s: extension: %s\n", __func__, currentProps[i].extensionName);
        for (size_t j = 0; j < wantedExtNames.size(); ++j) {
            if (!strcmp(wantedExtNames[j], currentProps[i].extensionName)) {
                foundExts[j] = true;
            }
        }
    }

    for (size_t i = 0; i < wantedExtNames.size(); ++i) {
        bool found = foundExts[i];
        printf("%s: needed extension: %s: found: %d\n", __func__,
               wantedExtNames[i], found);
        if (!found) {
            printf("%s: %s not found, bailing\n", __func__,
                   wantedExtNames[i]);
            return false;
        }
    }

    return true;
}

VkEmulation* createOrGetGlobalVkEmulation(VulkanDispatch* vk) {
    if (sVkEmulation) return sVkEmulation;

    sVkEmulation = new VkEmulation;

    std::vector<const char*> externalMemoryInstanceExtNames = {
        "VK_KHR_external_memory_capabilities",
    };

    std::vector<const char*> externalMemoryDeviceExtNames = {
        // TODO: VK_KHR_dedicated_allocation
        // TODO: VK_KHR_get_memory_requirements2
        "VK_KHR_external_memory",
#ifdef _WIN32
        "VK_KHR_external_memory_win32",
#else
        "VK_KHR_external_memory_fd",
#endif
    };

    uint32_t extCount = 0;
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> exts(extCount);
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

    sVkEmulation->externalMemoryCapabilitiesSupported =
        extensionsSupported(exts, externalMemoryInstanceExtNames);

    VkInstanceCreateInfo instCi = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        0, 0, nullptr, 0, nullptr,
        0, nullptr,
    };

    if (sVkEmulation->externalMemoryCapabilitiesSupported) {
        instCi.enabledExtensionCount =
            externalMemoryInstanceExtNames.size();
        instCi.ppEnabledExtensionNames =
            externalMemoryInstanceExtNames.data();
    }

    vk->vkCreateInstance(&instCi, nullptr, &sVkEmulation->instance);

    uint32_t physdevCount = 0;
    vk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount, nullptr);
    std::vector<VkPhysicalDevice> physdevs(physdevCount);
    vk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount, physdevs.data());

    if (physdevCount == 0) {
        vk->vkDestroyInstance(sVkEmulation->instance, nullptr);
        delete sVkEmulation;
        printf("Vulkan physical device not found.\n");
        return nullptr;
    }

    bool foundDevice = false;
    uint32_t bestPhysicalDeviceIndex = 0;
    uint32_t bestQueueFamilyIndex = 0;

    for (int i = 0; i < physdevCount; ++i) {
        VkPhysicalDeviceProperties props;
        vk->vkGetPhysicalDeviceProperties(physdevs[i], &props);

        printf("Vulkan physical device %d: %s\n", i, props.deviceName);

        uint32_t deviceExtensionCount = 0;
        vk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> deviceExts(deviceExtensionCount);
        vk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, deviceExts.data());

        bool deviceSupportsExternalMemory =
            extensionsSupported(deviceExts, externalMemoryDeviceExtNames);
        
        if (!deviceSupportsExternalMemory) continue;

        uint32_t queueFamilyCount = 0;
        vk->vkGetPhysicalDeviceQueueFamilyProperties(
                physdevs[i], &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
        vk->vkGetPhysicalDeviceQueueFamilyProperties(
                physdevs[i], &queueFamilyCount, queueFamilyProps.data());
        
        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
            auto count = queueFamilyProps[j].queueCount;
            auto flags = queueFamilyProps[j].queueFlags;
            if (count > 0 && (flags & VK_QUEUE_GRAPHICS_BIT)) {
                bestPhysicalDeviceIndex = i;
                bestQueueFamilyIndex = j;
                foundDevice = true;
                break;
            }
        }

        if (foundDevice) {
            printf("Found Vulkan device and queue family index "
                   "supporting external memory.\n");
            break;
        }
    }

    if (!foundDevice) {
        printf("Could not find a suitable Vulkan device for "
               "AndroidHardwareBuffer support. "
               "The device needs to support external memory "
               "and have at least one graphics queue.\n");
        delete sVkEmulation;
        sVkEmulation = nullptr;
        return nullptr;
    }

    return sVkEmulation;
}

} // namespace goldfish_vk