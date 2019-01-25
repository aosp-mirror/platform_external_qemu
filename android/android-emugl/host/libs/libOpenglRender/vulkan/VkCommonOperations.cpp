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

    bool externalMemoryCapabilitiesSupported =
        extensionsSupported(exts, externalMemoryInstanceExtNames);

    VkInstanceCreateInfo instCi = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        0, 0, nullptr, 0, nullptr,
        0, nullptr,
    };

    if (externalMemoryCapabilitiesSupported) {
        instCi.enabledExtensionCount =
            externalMemoryInstanceExtNames.size();
        instCi.ppEnabledExtensionNames =
            externalMemoryInstanceExtNames.data();
    }

    VkResult res = vk->vkCreateInstance(&instCi, nullptr, &sVkEmulation->instance);

    if (res != VK_SUCCESS) {
        printf("Failed to create Vulkan instance.\n");
        return sVkEmulation;
    }

    uint32_t physdevCount = 0;
    vk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount,
                                   nullptr);
    std::vector<VkPhysicalDevice> physdevs(physdevCount);
    vk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount,
                                   physdevs.data());

    printf("Found %d Vulkan physical devices.\n", physdevCount);

    if (physdevCount == 0) {
        printf("No physical devices available\n");
        return sVkEmulation;
    }

    std::vector<VkEmulation::DeviceSupportInfo> deviceInfos(physdevCount);

    for (int i = 0; i < physdevCount; ++i) {
        vk->vkGetPhysicalDeviceProperties(physdevs[i],
                                          &deviceInfos[i].physdevProps);

        printf("Considering Vulkan physical device %d: %s\n", i,
               deviceInfos[i].physdevProps.deviceName);

        uint32_t deviceExtensionCount = 0;
        vk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> deviceExts(deviceExtensionCount);
        vk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, deviceExts.data());

        deviceInfos[i].supportsExternalMemory =
            extensionsSupported(deviceExts, externalMemoryDeviceExtNames);
        
        uint32_t queueFamilyCount = 0;
        vk->vkGetPhysicalDeviceQueueFamilyProperties(
                physdevs[i], &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
        vk->vkGetPhysicalDeviceQueueFamilyProperties(
                physdevs[i], &queueFamilyCount, queueFamilyProps.data());
        
        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
            auto count = queueFamilyProps[j].queueCount;
            auto flags = queueFamilyProps[j].queueFlags;

            bool hasGraphicsQueueFamily =
                (count > 0 && (flags & VK_QUEUE_GRAPHICS_BIT));
            bool hasComputeQueueFamily =
                (count > 0 && (flags & VK_QUEUE_COMPUTE_BIT));

            deviceInfos[i].hasGraphicsQueueFamily =
                deviceInfos[i].hasGraphicsQueueFamily ||
                hasGraphicsQueueFamily;

            deviceInfos[i].hasComputeQueueFamily =
                deviceInfos[i].hasComputeQueueFamily ||
                hasComputeQueueFamily;

            if (hasGraphicsQueueFamily) {
                deviceInfos[i].graphicsQueueFamilyIndices.push_back(j);
                printf("Graphics queue family index: %u\n", j);
            }

            if (hasComputeQueueFamily) {
                deviceInfos[i].computeQueueFamilyIndices.push_back(j);
                printf("Compute queue family index: %u\n", j);
            }
        }
    }

    // Of all the devices enumerated, find the best one.
    std::vector<uint32_t> deviceScores(physdevCount, 0);

    // Try to find a device with graphics queue as the highest priority,
    // then ext memory, then compute.
    for (uint32_t i = 0; i < physdevCount; ++i) {
        uint32_t deviceScore = 0;
        if (deviceInfos[i].hasGraphicsQueueFamily) deviceScore += 100;
        if (deviceInfos[i].supportsExternalMemory) deviceScore += 10;
        if (deviceInfos[i].hasComputeQueueFamily) deviceScore += 1;
        deviceScores[i] = deviceScore;
    }

    uint32_t maxScoringIndex = 0;
    uint32_t maxScore = 0;

    for (uint32_t i = 0; i < physdevCount; ++i) {
        if (deviceScores[i] > maxScore) {
            maxScoringIndex = i;
            maxScore = deviceScores[i];
        }
    }

    sVkEmulation->physdev = physdevs[maxScoringIndex];
    sVkEmulation->deviceSupportInfo = deviceInfos[maxScoringIndex];

    if (!sVkEmulation->deviceSupportInfo.hasGraphicsQueueFamily) {
        printf("No Vulkan devices with graphics queues found.\n");
        return sVkEmulation;
    }

    printf("Vulkan device found: %s\n",
           sVkEmulation->deviceSupportInfo.physdevProps.deviceName);
    printf("Has graphics queue: %d\n",
           sVkEmulation->deviceSupportInfo.hasGraphicsQueueFamily);
    printf("Has external memory support: %d\n",
           sVkEmulation->deviceSupportInfo.supportsExternalMemory);
    printf("Has compute queue: %d\n",
           sVkEmulation->deviceSupportInfo.hasComputeQueueFamily);

    float priority = 1.0f;
    VkDeviceQueueCreateInfo dqCi = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
        sVkEmulation->deviceSupportInfo.graphicsQueueFamilyIndices[0],
        1, &priority,
    };

    uint32_t selectedDeviceExtensionCount = 0;
    const char* const* selectedDeviceExtensionNames = nullptr;

    if (sVkEmulation->deviceSupportInfo.supportsExternalMemory) {
        selectedDeviceExtensionCount = externalMemoryDeviceExtNames.size();
        selectedDeviceExtensionNames = externalMemoryDeviceExtNames.data();
    }

    VkDeviceCreateInfo dCi = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
        // TODO: add compute queue as well if appropriate
        1, &dqCi,
        0, nullptr, // no layers
        selectedDeviceExtensionCount,
        selectedDeviceExtensionNames, // no layers
        nullptr, // no features
    };

    vk->vkCreateDevice(sVkEmulation->physdev, &dCi, nullptr,
                       &sVkEmulation->device);

    if (res != VK_SUCCESS) {
        printf("Failed to create Vulkan device.\n");
        return sVkEmulation;
    }

    printf("Vulkan logical device created.\n");

    vk->vkGetDeviceQueue(
            sVkEmulation->device,
            sVkEmulation->deviceSupportInfo.graphicsQueueFamilyIndices[0], 0,
            &sVkEmulation->queue);
    
    sVkEmulation->queueFamilyIndex =
            sVkEmulation->deviceSupportInfo.graphicsQueueFamilyIndices[0];

    printf("Vulkan device queue obtained.\n");

    VkCommandPoolCreateInfo poolCi = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        sVkEmulation->queueFamilyIndex,
    };

    VkResult poolCreateRes = vk->vkCreateCommandPool(
            sVkEmulation->device, &poolCi, nullptr, &sVkEmulation->commandPool);

    if (poolCreateRes != VK_SUCCESS) {
        printf("Failed to create command pool. Error %d\n", poolCreateRes);
        return sVkEmulation;
    }

    VkCommandBufferAllocateInfo cbAi = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
        sVkEmulation->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
    };

    VkResult cbAllocRes = vk->vkAllocateCommandBuffers(
            sVkEmulation->device, &cbAi, &sVkEmulation->commandBuffer);

    if (cbAllocRes != VK_SUCCESS) {
        printf("Failed to allocate command buffer. Error %d\n", cbAllocRes);
        return sVkEmulation;
    }

    printf("Vulkan global emulation state successfully initialized.\n");
    sVkEmulation->live = true;

    return sVkEmulation;
}

} // namespace goldfish_vk