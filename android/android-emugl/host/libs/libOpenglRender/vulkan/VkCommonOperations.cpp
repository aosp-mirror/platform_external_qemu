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

#include "android/base/Log.h"
#include "android/base/Optional.h"
#include "android/base/containers/Lookup.h"
#include "android/base/containers/StaticMap.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "FrameBuffer.h"
#include "VulkanDispatch.h"

#include "common/goldfish_vk_dispatch.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/logging.h"
#include "emugl/common/vm_operations.h"
#include "vk_util.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <vulkan/vk_enum_string_helper.h>

#include <iomanip>
#include <ostream>
#include <sstream>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Optional;
using android::base::StaticLock;
using android::base::StaticMap;

using android::base::kNullopt;

namespace goldfish_vk {

namespace {

constexpr size_t kPageBits = 12;
constexpr size_t kPageSize = 1u << kPageBits;
constexpr size_t kPageOffsetMask = kPageSize - 1;

}  // namespace

static LazyInstance<StaticMap<VkDevice, uint32_t>>
sKnownStagingTypeIndices = LAZY_INSTANCE_INIT;

static android::base::StaticLock sVkEmulationLock;

VK_EXT_MEMORY_HANDLE dupExternalMemory(VK_EXT_MEMORY_HANDLE h) {
#ifdef _WIN32
    auto myProcessHandle = GetCurrentProcess();
    VK_EXT_MEMORY_HANDLE res;
    DuplicateHandle(
        myProcessHandle, h, // source process and handle
        myProcessHandle, &res, // target process and pointer to handle
        0 /* desired access (ignored) */,
        true /* inherit */,
        DUPLICATE_SAME_ACCESS /* same access option */);
    return res;
#else
    return dup(h);
#endif
}

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
        LOG(VERBOSE) << "has extension: " << currentProps[i].extensionName;
        for (size_t j = 0; j < wantedExtNames.size(); ++j) {
            if (!strcmp(wantedExtNames[j], currentProps[i].extensionName)) {
                foundExts[j] = true;
            }
        }
    }

    for (size_t i = 0; i < wantedExtNames.size(); ++i) {
        bool found = foundExts[i];
        LOG(VERBOSE) << "needed extension: " << wantedExtNames[i]
                     << " found: " << found;
        if (!found) {
            LOG(VERBOSE) << wantedExtNames[i] << " not found, bailing.";
            return false;
        }
    }

    return true;
}

// For a given ImageSupportInfo, populates usageWithExternalHandles and
// requiresDedicatedAllocation. memoryTypeBits are populated later once the
// device is created, beacuse that needs a test image to be created.
// If we don't support external memory, it's assumed dedicated allocations are
// not needed.
// Precondition: sVkEmulation instance has been created and ext memory caps known.
// Returns false if the query failed.
static bool getImageFormatExternalMemorySupportInfo(
    VulkanDispatch* vk,
    VkPhysicalDevice physdev,
    VkEmulation::ImageSupportInfo* info) {

    // Currently there is nothing special we need to do about
    // VkFormatProperties2, so just use the normal version
    // and put it in the format2 struct.
    VkFormatProperties outFormatProps;
    vk->vkGetPhysicalDeviceFormatProperties(
            physdev, info->format, &outFormatProps);

    info->formatProps2 = {
        VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, 0,
        outFormatProps,
    };

    if (!sVkEmulation->instanceSupportsExternalMemoryCapabilities) {
        info->supportsExternalMemory = false;
        info->requiresDedicatedAllocation = false;

        VkImageFormatProperties outImageFormatProps;
        VkResult res = vk->vkGetPhysicalDeviceImageFormatProperties(
                physdev, info->format, info->type, info->tiling,
                info->usageFlags, info->createFlags, &outImageFormatProps);

        if (res != VK_SUCCESS) {
            if (res == VK_ERROR_FORMAT_NOT_SUPPORTED) {
                info->supported = false;
                return true;
            } else {
                fprintf(stderr,
                        "%s: vkGetPhysicalDeviceImageFormatProperties query "
                        "failed with %d "
                        "for format 0x%x type 0x%x usage 0x%x flags 0x%x\n",
                        __func__, res, info->format, info->type,
                        info->usageFlags, info->createFlags);
                return false;
            }
        }

        info->supported = true;

        info->imageFormatProps2 = {
            VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, 0,
            outImageFormatProps,
        };

        LOG(VERBOSE) << "Supported (not externally): "
            << string_VkFormat(info->format) << " "
            << string_VkImageType(info->type) << " "
            << string_VkImageTiling(info->tiling) << " "
            << string_VkImageUsageFlagBits(
                   (VkImageUsageFlagBits)info->usageFlags);

        return true;
    }

    VkPhysicalDeviceExternalImageFormatInfo extInfo = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO, 0,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };

    VkPhysicalDeviceImageFormatInfo2 formatInfo2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &extInfo,
        info->format, info->type, info->tiling,
        info->usageFlags, info->createFlags,
    };

    VkExternalImageFormatProperties outExternalProps = {
        VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES,
        0,
        {
            (VkExternalMemoryFeatureFlags)0,
            (VkExternalMemoryHandleTypeFlags)0,
            (VkExternalMemoryHandleTypeFlags)0,
        },
    };

    VkImageFormatProperties2 outProps2 = {
        VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, &outExternalProps,
        {
            { 0, 0, 0},
            0, 0,
            1, 0,
        }
    };

    VkResult res = sVkEmulation->getImageFormatProperties2Func(
        physdev,
        &formatInfo2,
        &outProps2);

    if (res != VK_SUCCESS) {
        if (res == VK_ERROR_FORMAT_NOT_SUPPORTED) {
            info->supported = false;
            return true;
        } else {
            fprintf(stderr,
                    "%s: vkGetPhysicalDeviceImageFormatProperties2KHR query "
                    "failed "
                    "for format 0x%x type 0x%x usage 0x%x flags 0x%x\n",
                    __func__, info->format, info->type, info->usageFlags,
                    info->createFlags);
            return false;
        }
    }

    info->supported = true;

    VkExternalMemoryFeatureFlags featureFlags =
        outExternalProps.externalMemoryProperties.externalMemoryFeatures;

    VkExternalMemoryHandleTypeFlags exportImportedFlags =
        outExternalProps.externalMemoryProperties.exportFromImportedHandleTypes;

    // Don't really care about export form imported handle types yet
    (void)exportImportedFlags;

    VkExternalMemoryHandleTypeFlags compatibleHandleTypes =
        outExternalProps.externalMemoryProperties.compatibleHandleTypes;

    info->supportsExternalMemory =
        (VK_EXT_MEMORY_HANDLE_TYPE_BIT & compatibleHandleTypes) &&
        (VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT & featureFlags) &&
        (VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT & featureFlags);

    info->requiresDedicatedAllocation =
        (VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT & featureFlags);

    info->imageFormatProps2 = outProps2;
    info->extFormatProps = outExternalProps;
    info->imageFormatProps2.pNext = &info->extFormatProps;

    LOG(VERBOSE) << "Supported: "
                 << string_VkFormat(info->format) << " "
                 << string_VkImageType(info->type) << " "
                 << string_VkImageTiling(info->tiling) << " "
                 << string_VkImageUsageFlagBits(
                            (VkImageUsageFlagBits)info->usageFlags)
                 << " "
                 << "supportsExternalMemory? " << info->supportsExternalMemory
                 << " "
                 << "requiresDedicated? " << info->requiresDedicatedAllocation;

    return true;
}

static std::vector<VkEmulation::ImageSupportInfo> getBasicImageSupportList() {
    std::vector<VkFormat> formats = {
        // Cover all the gralloc formats
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,

        VK_FORMAT_R5G6B5_UNORM_PACK16,

        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16_SFLOAT,

        VK_FORMAT_B8G8R8A8_UNORM,

        VK_FORMAT_R8_UNORM,

        VK_FORMAT_A2R10G10B10_UINT_PACK32,
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,

        // Compressed texture formats
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK,

        // TODO: YUV formats used in Android
        // Fails on Mac
        VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,

        // R8G8 formats
        VK_FORMAT_R8G8_UNORM,
    };

    std::vector<VkImageType> types = {
        VK_IMAGE_TYPE_2D,
    };

    std::vector<VkImageTiling> tilings = {
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_TILING_OPTIMAL,
    };

    std::vector<VkImageUsageFlags> usageFlags = {
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };

    std::vector<VkImageCreateFlags> createFlags = {
        0,
    };

    std::vector<VkEmulation::ImageSupportInfo> res;

    // Currently: 12 formats, 2 tilings, 5 usage flags -> 120 cases
    // to check
    for (auto f : formats) {
        for (auto t : types) {
            for (auto ti : tilings) {
                for (auto u : usageFlags) {
                    for (auto c : createFlags) {
                        VkEmulation::ImageSupportInfo info;
                        info.format = f;
                        info.type = t;
                        info.tiling = ti;
                        info.usageFlags = u;
                        info.createFlags = c;
                        res.push_back(info);
                    }
                }
            }
        }
    }

    return res;
}

VkEmulation* createOrGetGlobalVkEmulation(VulkanDispatch* vk) {
    AutoLock lock(sVkEmulationLock);

    if (sVkEmulation) return sVkEmulation;

    if (!emugl::vkDispatchValid(vk)) {
        return nullptr;
    }

    sVkEmulation = new VkEmulation;

    sVkEmulation->gvk = vk;
    auto gvk = vk;

    std::vector<const char*> externalMemoryInstanceExtNames = {
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_get_physical_device_properties2",
    };

    std::vector<const char*> externalMemoryDeviceExtNames = {
        "VK_KHR_dedicated_allocation",
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_external_memory",
#ifdef _WIN32
        "VK_KHR_external_memory_win32",
#else
        "VK_KHR_external_memory_fd",
#endif
    };

    uint32_t extCount = 0;
    gvk->vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> exts(extCount);
    gvk->vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());

    bool externalMemoryCapabilitiesSupported =
        extensionsSupported(exts, externalMemoryInstanceExtNames);
    bool moltenVKSupported = (vk->vkGetMTLTextureMVK != nullptr) &&
        (vk->vkSetMTLTextureMVK != nullptr);

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

    if (moltenVKSupported) {
        // We don't need both moltenVK and external memory. Disable
        // external memory if moltenVK is supported.
        externalMemoryCapabilitiesSupported = false;
        instCi.enabledExtensionCount = 0u;
        instCi.ppEnabledExtensionNames = nullptr;
    }

    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
        "AEMU", 1,
        "AEMU", 1,
        VK_MAKE_VERSION(1, 0, 0),
    };

    instCi.pApplicationInfo = &appInfo;

    // Can we know instance version early?
    if (gvk->vkEnumerateInstanceVersion) {
        LOG(VERBOSE) << "global loader has vkEnumerateInstanceVersion.";
        uint32_t instanceVersion;
        VkResult res = gvk->vkEnumerateInstanceVersion(&instanceVersion);
        if (VK_SUCCESS == res) {
            if (instanceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
                LOG(VERBOSE) << "global loader has vkEnumerateInstanceVersion returning >= 1.1.";
                appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);
            }
        }
    }

    LOG(VERBOSE) << "Creating instance, asking for version "
                 << VK_VERSION_MAJOR(appInfo.apiVersion) << "."
                 << VK_VERSION_MINOR(appInfo.apiVersion) << "."
                 << VK_VERSION_PATCH(appInfo.apiVersion) << " ...";

    VkResult res = gvk->vkCreateInstance(&instCi, nullptr, &sVkEmulation->instance);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create Vulkan instance.";
        return sVkEmulation;
    }

    // Create instance level dispatch.
    sVkEmulation->ivk = new VulkanDispatch;
    init_vulkan_dispatch_from_instance(
        vk, sVkEmulation->instance, sVkEmulation->ivk);

    auto ivk = sVkEmulation->ivk;

    if (!vulkan_dispatch_check_instance_VK_VERSION_1_0(ivk)) {
        fprintf(stderr, "%s: Warning: Vulkan 1.0 APIs missing from instance\n", __func__);
    }

    if (ivk->vkEnumerateInstanceVersion) {
        uint32_t instanceVersion;
        VkResult enumInstanceRes = ivk->vkEnumerateInstanceVersion(&instanceVersion);
        if ((VK_SUCCESS == enumInstanceRes) &&
            instanceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            if (!vulkan_dispatch_check_instance_VK_VERSION_1_1(ivk)) {
                fprintf(stderr, "%s: Warning: Vulkan 1.1 APIs missing from instance (1st try)\n", __func__);
            }
        }

        if (appInfo.apiVersion < VK_MAKE_VERSION(1, 1, 0) &&
            instanceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            LOG(VERBOSE) << "Found out that we can create a higher version instance.";
            appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

            gvk->vkDestroyInstance(sVkEmulation->instance, nullptr);

            VkResult res = gvk->vkCreateInstance(&instCi, nullptr, &sVkEmulation->instance);

            if (res != VK_SUCCESS) {
                LOG(ERROR) << "Failed to create Vulkan 1.1 instance.";
                return sVkEmulation;
            }

            init_vulkan_dispatch_from_instance(
                vk, sVkEmulation->instance, sVkEmulation->ivk);

            LOG(VERBOSE) << "Created Vulkan 1.1 instance on second try.";

            if (!vulkan_dispatch_check_instance_VK_VERSION_1_1(ivk)) {
                fprintf(stderr, "%s: Warning: Vulkan 1.1 APIs missing from instance (2nd try)\n", __func__);
            }
        }
    }

    sVkEmulation->instanceSupportsExternalMemoryCapabilities =
        externalMemoryCapabilitiesSupported;
    sVkEmulation->instanceSupportsMoltenVK = moltenVKSupported;

    if (sVkEmulation->instanceSupportsExternalMemoryCapabilities) {
        sVkEmulation->getImageFormatProperties2Func = vk_util::getVkInstanceProcAddrWithFallback<
            vk_util::vk_fn_info::GetPhysicalDeviceImageFormatProperties2>(
            {ivk->vkGetInstanceProcAddr, vk->vkGetInstanceProcAddr}, sVkEmulation->instance);
        sVkEmulation->getPhysicalDeviceProperties2Func = vk_util::getVkInstanceProcAddrWithFallback<
            vk_util::vk_fn_info::GetPhysicalDeviceProperties2>(
            {ivk->vkGetInstanceProcAddr, vk->vkGetInstanceProcAddr}, sVkEmulation->instance);
    }

    if (sVkEmulation->instanceSupportsMoltenVK) {
        sVkEmulation->setMTLTextureFunc = vk->vkSetMTLTextureMVK;
        if (!sVkEmulation->setMTLTextureFunc) {
            LOG(ERROR) << "Cannot find vkSetMTLTextureMVK";
            return sVkEmulation;
        }
        sVkEmulation->getMTLTextureFunc = vk->vkGetMTLTextureMVK;
        if (!sVkEmulation->getMTLTextureFunc) {
            LOG(ERROR) << "Cannot find vkGetMTLTextureMVK";
            return sVkEmulation;
        }
        LOG(VERBOSE) << "Instance supports VK_MVK_moltenvk.";
    }

    uint32_t physdevCount = 0;
    ivk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount,
                                   nullptr);
    std::vector<VkPhysicalDevice> physdevs(physdevCount);
    ivk->vkEnumeratePhysicalDevices(sVkEmulation->instance, &physdevCount,
                                   physdevs.data());

    LOG(VERBOSE) << "Found " << physdevCount << " Vulkan physical devices.";

    if (physdevCount == 0) {
        LOG(VERBOSE) << "No physical devices available.";
        return sVkEmulation;
    }

    std::vector<VkEmulation::DeviceSupportInfo> deviceInfos(physdevCount);

    for (int i = 0; i < physdevCount; ++i) {
        ivk->vkGetPhysicalDeviceProperties(physdevs[i],
                                           &deviceInfos[i].physdevProps);

        LOG(VERBOSE) << "Considering Vulkan physical device " << i << ": "
                     << deviceInfos[i].physdevProps.deviceName;

        // It's easier to figure out the staging buffer along with
        // external memories if we have the memory properties on hand.
        ivk->vkGetPhysicalDeviceMemoryProperties(physdevs[i],
                                                &deviceInfos[i].memProps);

        uint32_t deviceExtensionCount = 0;
        ivk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> deviceExts(deviceExtensionCount);
        ivk->vkEnumerateDeviceExtensionProperties(
            physdevs[i], nullptr, &deviceExtensionCount, deviceExts.data());

        deviceInfos[i].supportsExternalMemory = false;
        deviceInfos[i].glInteropSupported = 0; // set later

        if (sVkEmulation->instanceSupportsExternalMemoryCapabilities) {
            deviceInfos[i].supportsExternalMemory = extensionsSupported(
                    deviceExts, externalMemoryDeviceExtNames);
            deviceInfos[i].supportsIdProperties =
                sVkEmulation->getPhysicalDeviceProperties2Func != nullptr;
            if (!sVkEmulation->getPhysicalDeviceProperties2Func) {
                fprintf(stderr, "%s: warning: device claims to support ID properties "
                        "but vkGetPhysicalDeviceProperties2 could not be found\n", __func__);
            }
        }

        if (deviceInfos[i].supportsIdProperties) {
            VkPhysicalDeviceIDPropertiesKHR idProps = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR, nullptr,
            };
            VkPhysicalDeviceProperties2KHR propsWithId = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, &idProps,
            };
            sVkEmulation->getPhysicalDeviceProperties2Func(
                physdevs[i],
                &propsWithId);
            deviceInfos[i].idProps = idProps;
        }

        uint32_t queueFamilyCount = 0;
        ivk->vkGetPhysicalDeviceQueueFamilyProperties(
                physdevs[i], &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
        ivk->vkGetPhysicalDeviceQueueFamilyProperties(
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
                LOG(VERBOSE) << "Graphics queue family index: " << j;
            }

            if (hasComputeQueueFamily) {
                deviceInfos[i].computeQueueFamilyIndices.push_back(j);
                LOG(VERBOSE) << "Compute queue family index: " << j;
            }
        }
    }

    // Of all the devices enumerated, find the best one. Try to find a device
    // with graphics queue as the highest priority, then ext memory, then
    // compute.

    // Graphics queue is highest priority since without that, we really
    // shouldn't be using the driver. Although, one could make a case for doing
    // some sorts of things if only a compute queue is available (such as for
    // AI), that's not really the priority yet.

    // As for external memory, we really should not be running on any driver
    // without external memory support, but we might be able to pull it off, and
    // single Vulkan apps might work via CPU transfer of the rendered frames.

    // Compute support is treated as icing on the cake and not relied upon yet
    // for anything critical to emulation. However, we might potentially use it
    // to perform image format conversion on GPUs where that's not natively
    // supported.

    // Another implicit choice is to select only one Vulkan device. This makes
    // things simple for now, but we could consider utilizing multiple devices
    // in use cases that make sense, if/when they come up.

    std::vector<uint32_t> deviceScores(physdevCount, 0);

    for (uint32_t i = 0; i < physdevCount; ++i) {
        uint32_t deviceScore = 0;
        if (deviceInfos[i].hasGraphicsQueueFamily) deviceScore += 10000;
        if (deviceInfos[i].supportsExternalMemory) deviceScore += 1000;
        if (deviceInfos[i].physdevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            deviceInfos[i].physdevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
            deviceScore += 100;
        }
        if (deviceInfos[i].physdevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            deviceScore += 50;
        }
        deviceScores[i] = deviceScore;
    }

    uint32_t maxScoringIndex = 0;
    uint32_t maxScore = 0;

    // If we don't support physical devce ID properties,
    // just pick the first physical device.
    if (!sVkEmulation->instanceSupportsExternalMemoryCapabilities) {
        fprintf(stderr, "%s: warning: instance doesn't support "
            "external memory capabilities, picking first physical device\n", __func__);
        maxScoringIndex = 0;
    } else {
        for (uint32_t i = 0; i < physdevCount; ++i) {
            if (deviceScores[i] > maxScore) {
                maxScoringIndex = i;
                maxScore = deviceScores[i];
            }
        }
    }

    sVkEmulation->physdev = physdevs[maxScoringIndex];
    sVkEmulation->deviceInfo = deviceInfos[maxScoringIndex];
    // Postcondition: sVkEmulation has valid device support info

    // Ask about image format support here.
    // TODO: May have to first ask when selecting physical devices
    // (e.g., choose between Intel or NVIDIA GPU for certain image format
    // support)
    sVkEmulation->imageSupportInfo = getBasicImageSupportList();
    for (size_t i = 0; i < sVkEmulation->imageSupportInfo.size(); ++i) {
        getImageFormatExternalMemorySupportInfo(
                ivk, sVkEmulation->physdev, &sVkEmulation->imageSupportInfo[i]);
    }

    if (!sVkEmulation->deviceInfo.hasGraphicsQueueFamily) {
        LOG(VERBOSE) << "No Vulkan devices with graphics queues found.";
        return sVkEmulation;
    }

    auto deviceVersion = sVkEmulation->deviceInfo.physdevProps.apiVersion;

    LOG(VERBOSE) << "Vulkan device found: "
                 << sVkEmulation->deviceInfo.physdevProps.deviceName;
    LOG(VERBOSE) << "Version: "
                 << VK_VERSION_MAJOR(deviceVersion) << "." << VK_VERSION_MINOR(deviceVersion) << "." << VK_VERSION_PATCH(deviceVersion);
    LOG(VERBOSE) << "Has graphics queue? "
                 << sVkEmulation->deviceInfo.hasGraphicsQueueFamily;
    LOG(VERBOSE) << "Has external memory support? "
                 << sVkEmulation->deviceInfo.supportsExternalMemory;
    LOG(VERBOSE) << "Has compute queue? "
                 << sVkEmulation->deviceInfo.hasComputeQueueFamily;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo dqCi = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
        sVkEmulation->deviceInfo.graphicsQueueFamilyIndices[0],
        1, &priority,
    };

    uint32_t selectedDeviceExtensionCount = 0;
    const char* const* selectedDeviceExtensionNames = nullptr;

    if (sVkEmulation->deviceInfo.supportsExternalMemory) {
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

    ivk->vkCreateDevice(sVkEmulation->physdev, &dCi, nullptr,
                        &sVkEmulation->device);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create Vulkan device.";
        return sVkEmulation;
    }

    // device created; populate dispatch table
    sVkEmulation->dvk = new VulkanDispatch;
    init_vulkan_dispatch_from_device(
        ivk, sVkEmulation->device, sVkEmulation->dvk);

    auto dvk = sVkEmulation->dvk;

    // Check if the dispatch table has everything 1.1 related
    if (!vulkan_dispatch_check_device_VK_VERSION_1_0(dvk)) {
        fprintf(stderr, "%s: Warning: Vulkan 1.0 APIs missing from device.\n", __func__);
    }
    if (deviceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
        if (!vulkan_dispatch_check_device_VK_VERSION_1_1(dvk)) {
            fprintf(stderr, "%s: Warning: Vulkan 1.1 APIs missing from device\n", __func__);
        }
    }

    if (sVkEmulation->deviceInfo.supportsExternalMemory) {
        sVkEmulation->deviceInfo.getImageMemoryRequirements2Func =
            reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(
                dvk->vkGetDeviceProcAddr(
                    sVkEmulation->device, "vkGetImageMemoryRequirements2KHR"));
        if (!sVkEmulation->deviceInfo.getImageMemoryRequirements2Func) {
            LOG(ERROR) << "Cannot find vkGetImageMemoryRequirements2KHR";
            return sVkEmulation;
        }
        sVkEmulation->deviceInfo.getBufferMemoryRequirements2Func =
            reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(
                dvk->vkGetDeviceProcAddr(
                    sVkEmulation->device, "vkGetBufferMemoryRequirements2KHR"));
        if (!sVkEmulation->deviceInfo.getBufferMemoryRequirements2Func) {
            LOG(ERROR) << "Cannot find vkGetBufferMemoryRequirements2KHR";
            return sVkEmulation;
        }
#ifdef _WIN32
        sVkEmulation->deviceInfo.getMemoryHandleFunc =
                reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(
                        dvk->vkGetDeviceProcAddr(sVkEmulation->device,
                                                "vkGetMemoryWin32HandleKHR"));
#else
        sVkEmulation->deviceInfo.getMemoryHandleFunc =
                reinterpret_cast<PFN_vkGetMemoryFdKHR>(
                        dvk->vkGetDeviceProcAddr(sVkEmulation->device,
                                                "vkGetMemoryFdKHR"));
#endif
        if (!sVkEmulation->deviceInfo.getMemoryHandleFunc) {
            LOG(ERROR) << "Cannot find vkGetMemory(Fd|Win32Handle)KHR";
            return sVkEmulation;
        }
    }

    LOG(VERBOSE) << "Vulkan logical device created and extension functions obtained.\n";

    dvk->vkGetDeviceQueue(
            sVkEmulation->device,
            sVkEmulation->deviceInfo.graphicsQueueFamilyIndices[0], 0,
            &sVkEmulation->queue);

    sVkEmulation->queueFamilyIndex =
            sVkEmulation->deviceInfo.graphicsQueueFamilyIndices[0];

    LOG(VERBOSE) << "Vulkan device queue obtained.";

    VkCommandPoolCreateInfo poolCi = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        sVkEmulation->queueFamilyIndex,
    };

    VkResult poolCreateRes = dvk->vkCreateCommandPool(
            sVkEmulation->device, &poolCi, nullptr, &sVkEmulation->commandPool);

    if (poolCreateRes != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create command pool. Error: " << poolCreateRes;
        return sVkEmulation;
    }

    VkCommandBufferAllocateInfo cbAi = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
        sVkEmulation->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
    };

    VkResult cbAllocRes = dvk->vkAllocateCommandBuffers(
            sVkEmulation->device, &cbAi, &sVkEmulation->commandBuffer);

    if (cbAllocRes != VK_SUCCESS) {
        LOG(ERROR) << "Failed to allocate command buffer. Error: " << cbAllocRes;
        return sVkEmulation;
    }

    VkFenceCreateInfo fenceCi = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0, 0,
    };

    VkResult fenceCreateRes = dvk->vkCreateFence(
        sVkEmulation->device, &fenceCi, nullptr,
        &sVkEmulation->commandBufferFence);

    if (fenceCreateRes != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create fence for command buffer. Error: " << fenceCreateRes;
        return sVkEmulation;
    }

    // At this point, the global emulation state's logical device can alloc
    // memory and send commands. However, it can't really do much yet to
    // communicate the results without the staging buffer. Set that up here.
    // Note that the staging buffer is meant to use external memory, with a
    // non-external-memory fallback.

    VkBufferCreateInfo bufCi = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
        sVkEmulation->staging.size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0, nullptr,
    };

    VkResult bufCreateRes =
            dvk->vkCreateBuffer(sVkEmulation->device, &bufCi, nullptr,
                               &sVkEmulation->staging.buffer);

    if (bufCreateRes != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create staging buffer index";
        return sVkEmulation;
    }

    VkMemoryRequirements memReqs;
    dvk->vkGetBufferMemoryRequirements(sVkEmulation->device,
                                      sVkEmulation->staging.buffer, &memReqs);

    sVkEmulation->staging.memory.size = memReqs.size;

    bool gotStagingTypeIndex = getStagingMemoryTypeIndex(
            dvk, sVkEmulation->device, &sVkEmulation->deviceInfo.memProps,
            &sVkEmulation->staging.memory.typeIndex);

    if (!gotStagingTypeIndex) {
        LOG(ERROR) << "Failed to determine staging memory type index";
        return sVkEmulation;
    }

    if (!((1 << sVkEmulation->staging.memory.typeIndex) &
          memReqs.memoryTypeBits)) {
        LOG(ERROR) << "Failed: Inconsistent determination of memory type "
                        "index for staging buffer";
        return sVkEmulation;
    }

    if (!allocExternalMemory(dvk, &sVkEmulation->staging.memory,
                             false /* not external */,
                             kNullopt /* deviceAlignment */)) {
        LOG(ERROR) << "Failed to allocate memory for staging buffer";
        return sVkEmulation;
    }

    VkResult stagingBufferBindRes = dvk->vkBindBufferMemory(
        sVkEmulation->device,
        sVkEmulation->staging.buffer,
        sVkEmulation->staging.memory.memory, 0);

    if (stagingBufferBindRes != VK_SUCCESS) {
        LOG(ERROR) << "Failed to bind memory for staging buffer";
        return sVkEmulation;
    }

    LOG(VERBOSE) << "Vulkan global emulation state successfully initialized.";
    sVkEmulation->live = true;

    return sVkEmulation;
}

void setGlInteropSupported(bool supported) {
    if (!sVkEmulation) {
        LOG(VERBOSE) << "Not setting vk/gl interop support, Vulkan not enabled";
        return;
    }

    LOG(VERBOSE) << "Setting gl interop support for Vk to: " << supported;
    sVkEmulation->deviceInfo.glInteropSupported = supported;
}

void setUseDeferredCommands(VkEmulation* emu, bool useDeferredCommands) {
    if (!emu) return;
    if (!emu->live) return;

    LOG(VERBOSE) << "Using deferred Vulkan commands: " << useDeferredCommands;
    emu->useDeferredCommands = useDeferredCommands;
}

void setUseCreateResourcesWithRequirements(VkEmulation* emu, bool useCreateResourcesWithRequirements) {
    if (!emu) return;
    if (!emu->live) return;

    LOG(VERBOSE) << "Using deferred Vulkan commands: " << useCreateResourcesWithRequirements;
    emu->useCreateResourcesWithRequirements = useCreateResourcesWithRequirements;
}


void setEnableAstcLdrEmulation(VkEmulation* emu, bool enableAstcLdrEmulation) {
    if (!emu) return;
    if (!emu->live) return;

    LOG(VERBOSE) << "Enable ASTC LDR emulation: " << enableAstcLdrEmulation;
    emu->enableAstcLdrEmulation = enableAstcLdrEmulation;
}

VkEmulation* getGlobalVkEmulation() {
    if (sVkEmulation && !sVkEmulation->live) return nullptr;
    return sVkEmulation;
}

void teardownGlobalVkEmulation() {
    if (!sVkEmulation) return;

    // Don't try to tear down something that did not set up completely; too risky
    if (!sVkEmulation->live) return;

    freeExternalMemoryLocked(sVkEmulation->dvk, &sVkEmulation->staging.memory);

    sVkEmulation->ivk->vkDestroyDevice(sVkEmulation->device, nullptr);
    sVkEmulation->gvk->vkDestroyInstance(sVkEmulation->instance, nullptr);
}

// Precondition: sVkEmulation has valid device support info
bool allocExternalMemory(VulkanDispatch* vk,
                         VkEmulation::ExternalMemoryInfo* info,
                         bool actuallyExternal,
                         Optional<uint64_t> deviceAlignment,
                         Optional<VkImage> dedicatedImage,
                         Optional<VkBuffer> dedicatedBuffer) {
    VkExportMemoryAllocateInfo exportAi = {
            VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
            0,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };
    bool dedicated = dedicatedImage || dedicatedBuffer;

    VkMemoryDedicatedAllocateInfoKHR dedicated_memory_info = {
            VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
            NULL,
            dedicatedImage ? dedicatedImage.value() : VK_NULL_HANDLE,
            dedicatedBuffer? dedicatedBuffer.value(): VK_NULL_HANDLE,
    };

    void* pNext = nullptr;

    if (sVkEmulation->deviceInfo.supportsExternalMemory && actuallyExternal) {
        pNext = &exportAi;
        if (dedicated) {
            exportAi.pNext = &dedicated_memory_info;
        }
    } else if (dedicated) {
        pNext = &dedicated_memory_info;
    }

    info->actualSize = dedicated ? info->size
                                      : (info->size + 2 * kPageSize - 1) /
                                                kPageSize * kPageSize;
    VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            pNext,
            info->actualSize,
            info->typeIndex,
    };

    bool memoryAllocated = false;
    std::vector<VkDeviceMemory> allocationAttempts;
    constexpr size_t kMaxAllocationAttempts = 20u;

    while (!memoryAllocated) {
        VkResult allocRes = vk->vkAllocateMemory(
                sVkEmulation->device, &allocInfo, nullptr, &info->memory);

        if (allocRes != VK_SUCCESS) {
            LOG(VERBOSE) << "allocExternalMemory: failed in vkAllocateMemory: "
                         << allocRes;
            break;
        }

        if (sVkEmulation->deviceInfo.memProps.memoryTypes[info->typeIndex]
                    .propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            VkResult mapRes =
                    vk->vkMapMemory(sVkEmulation->device, info->memory, 0,
                                    info->actualSize, 0, &info->mappedPtr);
            if (mapRes != VK_SUCCESS) {
                LOG(VERBOSE) << "allocExternalMemory: failed in vkMapMemory: "
                             << mapRes;
                break;
            }
        }

        uint64_t mappedPtrPageOffset =
                reinterpret_cast<uint64_t>(info->mappedPtr) % kPageSize;

        if (  // don't care about alignment (e.g. device-local memory)
                !deviceAlignment.hasValue() ||
                // If device has an alignment requirement larger than current
                // host pointer alignment (i.e. the lowest 1 bit of mappedPtr),
                // the only possible way to make mappedPtr valid is to ensure
                // that it is already aligned to page.
                mappedPtrPageOffset == 0u ||
                // If device has an alignment requirement smaller or equals to
                // current host pointer alignment, clients can set a offset
                // |kPageSize - mappedPtrPageOffset| in vkBindImageMemory to
                // make it aligned to page and compatible with device
                // requirements.
                (kPageSize - mappedPtrPageOffset) % deviceAlignment.value() == 0) {
            // allocation success.
            memoryAllocated = true;
        } else {
            allocationAttempts.push_back(info->memory);

            LOG(VERBOSE) << "allocExternalMemory: attempt #"
                         << allocationAttempts.size()
                         << " failed; deviceAlignment: "
                         << deviceAlignment.valueOr(0)
                         << " mappedPtrPageOffset: " << mappedPtrPageOffset;

            if (allocationAttempts.size() >= kMaxAllocationAttempts) {
                LOG(VERBOSE) << "allocExternalMemory: unable to allocate"
                             << " memory with CPU mapped ptr aligned to page";
                break;
            }
        }
    }

    // clean up previous failed attempts
    for (const auto& mem : allocationAttempts) {
        vk->vkFreeMemory(sVkEmulation->device, mem, nullptr /* allocator */);
    }
    if (!memoryAllocated) {
        return false;
    }

    if (!sVkEmulation->deviceInfo.supportsExternalMemory ||
        !actuallyExternal) {
        return true;
    }

#ifdef _WIN32
    VkMemoryGetWin32HandleInfoKHR getWin32HandleInfo = {
        VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR, 0,
        info->memory, VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };
    VkResult exportRes =
        sVkEmulation->deviceInfo.getMemoryHandleFunc(
            sVkEmulation->device, &getWin32HandleInfo,
            &info->exportedHandle);
#else
    VkMemoryGetFdInfoKHR getFdInfo = {
        VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, 0,
        info->memory, VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };
    VkResult exportRes =
        sVkEmulation->deviceInfo.getMemoryHandleFunc(
            sVkEmulation->device, &getFdInfo,
            &info->exportedHandle);
#endif

    if (exportRes != VK_SUCCESS) {
        LOG(VERBOSE) << "allocExternalMemory: Failed to get external memory "
                        "native handle: "
                     << exportRes;
        return false;
    }

    info->actuallyExternal = true;

    return true;
}

void freeExternalMemoryLocked(VulkanDispatch* vk,
                              VkEmulation::ExternalMemoryInfo* info) {
    if (!info->memory)
        return;

    if (sVkEmulation->deviceInfo.memProps.memoryTypes[info->typeIndex]
                .propertyFlags &
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (sVkEmulation->occupiedGpas.find(info->gpa) !=
            sVkEmulation->occupiedGpas.end()) {
            sVkEmulation->occupiedGpas.erase(info->gpa);
            get_emugl_vm_operations().unmapUserBackedRam(info->gpa,
                                                         info->sizeToPage);
            info->gpa = 0u;
        }

        vk->vkUnmapMemory(sVkEmulation->device, info->memory);
        info->mappedPtr = nullptr;
        info->pageAlignedHva = nullptr;
    }

    vk->vkFreeMemory(sVkEmulation->device, info->memory, nullptr);

    info->memory = VK_NULL_HANDLE;

    if (info->exportedHandle != VK_EXT_MEMORY_HANDLE_INVALID) {
#ifdef _WIN32
        CloseHandle(info->exportedHandle);
#else
        close(info->exportedHandle);
#endif
        info->exportedHandle = VK_EXT_MEMORY_HANDLE_INVALID;
    }
}

bool importExternalMemory(VulkanDispatch* vk,
                          VkDevice targetDevice,
                          const VkEmulation::ExternalMemoryInfo* info,
                          VkDeviceMemory* out) {
#ifdef _WIN32
    VkImportMemoryWin32HandleInfoKHR importInfo = {
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, 0,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
        info->exportedHandle,
        0,
    };
#else
    VkImportMemoryFdInfoKHR importInfo = {
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, 0,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
        dupExternalMemory(info->exportedHandle),
    };
#endif
    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        &importInfo,
        info->size,
        info->typeIndex,
    };

    VkResult res = vk->vkAllocateMemory(targetDevice, &allocInfo, nullptr, out);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "importExternalMemory: Failed with " << res;
        return false;
    }

    return true;
}

bool importExternalMemoryDedicatedImage(
    VulkanDispatch* vk,
    VkDevice targetDevice,
    const VkEmulation::ExternalMemoryInfo* info,
    VkImage image,
    VkDeviceMemory* out) {

    VkMemoryDedicatedAllocateInfo dedicatedInfo = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, 0,
        image,
        VK_NULL_HANDLE,
    };

#ifdef _WIN32
    VkImportMemoryWin32HandleInfoKHR importInfo = {
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
        &dedicatedInfo,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
        info->exportedHandle,
        0,
    };
#else
    VkImportMemoryFdInfoKHR importInfo = {
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
        &dedicatedInfo,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
        info->exportedHandle,
    };
#endif
    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        &importInfo,
        info->size,
        info->typeIndex,
    };

    VkResult res = vk->vkAllocateMemory(targetDevice, &allocInfo, nullptr, out);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "importExternalMemoryDedicatedImage: Failed with " << res;
        return false;
    }

    return true;
}

static VkFormat glFormat2VkFormat(GLint internalformat) {
    switch (internalformat) {
        case GL_LUMINANCE:
        case GL_R8:
            return VK_FORMAT_R8_UNORM;
        case GL_RG:
        case GL_RG8:
            return VK_FORMAT_R8G8_UNORM;
        case GL_RGB:
        case GL_RGB8:
            return VK_FORMAT_R8G8B8_UNORM;
        case GL_RGB565:
            return VK_FORMAT_R5G6B5_UNORM_PACK16;
        case GL_RGB16F:
            return VK_FORMAT_R16G16B16_SFLOAT;
        case GL_RGBA:
        case GL_RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case GL_RGB5_A1_OES:
            return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        case GL_RGBA4_OES:
            return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        case GL_RGB10_A2:
        case GL_UNSIGNED_INT_10_10_10_2_OES:
            return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case GL_BGR10_A2_ANGLEX:
            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case GL_RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case GL_BGRA_EXT:
        case GL_BGRA8_EXT:
            return VK_FORMAT_B8G8R8A8_UNORM;;
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
};

bool isColorBufferVulkanCompatible(uint32_t colorBufferHandle) {
    auto fb = FrameBuffer::getFB();

    int width;
    int height;
    GLint internalformat;

    if (!fb->getColorBufferInfo(colorBufferHandle, &width, &height,
                                &internalformat)) {
        return false;
    }

    VkFormat vkFormat = glFormat2VkFormat(internalformat);

    for (const auto& supportInfo : sVkEmulation->imageSupportInfo) {
        if (supportInfo.format == vkFormat && supportInfo.supported) {
            return true;
        }
    }

    return false;
}

static uint32_t lastGoodTypeIndex(uint32_t indices) {
    for (int32_t i = 31; i >= 0; --i) {
        if (indices & (1 << i)) {
            return i;
        }
    }
    return 0;
}

static uint32_t lastGoodTypeIndexWithMemoryProperties(
        uint32_t indices,
        VkMemoryPropertyFlags memoryProperty) {
    for (int32_t i = 31; i >= 0; --i) {
        if ((indices & (1u << i)) &&
            (!memoryProperty ||
             (sVkEmulation->deviceInfo.memProps.memoryTypes[i].propertyFlags &
              memoryProperty))) {
            return i;
        }
    }
    return 0;
}

// TODO(liyl): Currently we can only specify required memoryProperty
// for a color buffer.
//
// Ideally we would like to specify a memory type index directly from
// localAllocInfo.memoryTypeIndex when allocating color buffers in
// vkAllocateMemory(). But this type index mechanism breaks "Modify the
// allocation size and type index to suit the resulting image memory
// size." which seems to be needed to keep the Android/Fuchsia guest
// memory type index consistent across guest allocations, and without
// which those guests might end up import allocating from a color buffer
// with mismatched type indices.
//
// We should make it so the guest can only allocate external images/
// buffers of one type index for image and one type index for buffer
// to begin with, via filtering from the host.
bool setupVkColorBuffer(uint32_t colorBufferHandle,
                        bool vulkanOnly,
                        uint32_t memoryProperty,
                        bool* exported,
                        VkDeviceSize* allocSize,
                        uint32_t* typeIndex,
                        void** mappedPtr) {
    if (!isColorBufferVulkanCompatible(colorBufferHandle)) return false;

    auto vk = sVkEmulation->dvk;

    auto fb = FrameBuffer::getFB();

    int width;
    int height;
    GLint internalformat;
    FrameworkFormat frameworkFormat;

    if (!fb->getColorBufferInfo(colorBufferHandle, &width, &height,
                                &internalformat, &frameworkFormat)) {
        return false;
    }

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBufferHandle);

    // Already setup
    if (infoPtr) {
        // Setting exported is required for on_vkCreateImage backed by
        // an AHardwareBuffer.
        if (exported) *exported = infoPtr->glExported;
        // Update the allocation size to what the host driver wanted, or we
        // might get VK_ERROR_OUT_OF_DEVICE_MEMORY and a host crash
        if (allocSize) *allocSize = infoPtr->memory.size;
        // Update the type index to what the host driver wanted, or we might
        // get VK_ERROR_DEVICE_LOST
        if (typeIndex) *typeIndex = infoPtr->memory.typeIndex;
        // Update the mappedPtr to what the host driver wanted, otherwise we
        // may map the same memory twice.
        if (mappedPtr)
            *mappedPtr = infoPtr->memory.mappedPtr;
        return true;
    }

    VkFormat vkFormat;
    bool glCompatible = (frameworkFormat == FRAMEWORK_FORMAT_GL_COMPATIBLE);
    switch (frameworkFormat) {
        case FrameworkFormat::FRAMEWORK_FORMAT_GL_COMPATIBLE:
            vkFormat = glFormat2VkFormat(internalformat);
            break;
        case FrameworkFormat::FRAMEWORK_FORMAT_NV12:
            vkFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
            break;
        case FrameworkFormat::FRAMEWORK_FORMAT_YV12:
        case FrameworkFormat::FRAMEWORK_FORMAT_YUV_420_888:
            vkFormat = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
            break;
        default:
            vkFormat = glFormat2VkFormat(internalformat);
            fprintf(stderr, "WARNING: unsupported framework format %d\n", frameworkFormat);
            break;
    }

    VkEmulation::ColorBufferInfo res;

    res.handle = colorBufferHandle;

    // TODO
    res.frameworkFormat = frameworkFormat;
    res.frameworkStride = 0;

    res.extent = { (uint32_t)width, (uint32_t)height, 1 };
    res.format = vkFormat;
    res.type = VK_IMAGE_TYPE_2D;

    res.tiling = (memoryProperty & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                         ? VK_IMAGE_TILING_LINEAR
                         : VK_IMAGE_TILING_OPTIMAL;

    VkFormatProperties formatProperties = {};
    for (const auto& supportInfo : sVkEmulation->imageSupportInfo) {
        if (supportInfo.format == vkFormat && supportInfo.supported) {
            formatProperties = supportInfo.formatProps2.formatProperties;
            break;
        }
    }

    constexpr std::pair<VkFormatFeatureFlags, VkImageUsageFlags>
            formatUsagePairs[] = {
                    {VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT,
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
                    {VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT,
                     VK_IMAGE_USAGE_SAMPLED_BIT},
                    {VK_FORMAT_FEATURE_TRANSFER_SRC_BIT,
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT},
                    {VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT},
            };
    VkFormatFeatureFlags tilingFeatures =
            (res.tiling == VK_IMAGE_TILING_OPTIMAL)
                    ? formatProperties.optimalTilingFeatures
                    : formatProperties.linearTilingFeatures;
    res.usageFlags = 0u;
    for (const auto& formatUsage : formatUsagePairs) {
        res.usageFlags |=
                (tilingFeatures & formatUsage.first) ? formatUsage.second : 0u;
    }
    res.createFlags = 0;

    res.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create the image. If external memory is supported, make it external.
    VkExternalMemoryImageCreateInfo extImageCi = {
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, 0,
        VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };

    VkExternalMemoryImageCreateInfo* extImageCiPtr = nullptr;

    if (sVkEmulation->deviceInfo.supportsExternalMemory) {
        extImageCiPtr = &extImageCi;
    }

    VkImageCreateInfo imageCi = {
         VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, extImageCiPtr,
         res.createFlags,
         res.type,
         res.format,
         res.extent,
         1, 1,
         VK_SAMPLE_COUNT_1_BIT,
         res.tiling,
         res.usageFlags,
         VK_SHARING_MODE_EXCLUSIVE, 0, nullptr,
         VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkResult createRes = vk->vkCreateImage(sVkEmulation->device, &imageCi,
                                           nullptr, &res.image);
    if (createRes != VK_SUCCESS) {
        LOG(VERBOSE) << "Failed to create Vulkan image for ColorBuffer "
                     << colorBufferHandle;
        return false;
    }

    bool use_dedicated = false;
    if (vk->vkGetImageMemoryRequirements2KHR) {
        VkMemoryDedicatedRequirements dedicated_reqs{
                VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, nullptr};
        VkMemoryRequirements2 reqs{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
                                   &dedicated_reqs};

        VkImageMemoryRequirementsInfo2 info{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
            nullptr,
            res.image
        };
        vk->vkGetImageMemoryRequirements2KHR(sVkEmulation->device, &info, &reqs);
        use_dedicated = dedicated_reqs.prefersDedicatedAllocation;
        res.memReqs = reqs.memoryRequirements;
    } else {
        vk->vkGetImageMemoryRequirements(sVkEmulation->device, res.image,
                                        &res.memReqs);
    }

    // Currently we only care about two memory properties: DEVICE_LOCAL
    // and HOST_VISIBLE; other memory properties specified in
    // rcSetColorBufferVulkanMode2() call will be ignored for now.
    memoryProperty = memoryProperty & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    res.memory.size = res.memReqs.size;

    // Determine memory type.
    if (memoryProperty) {
        res.memory.typeIndex = lastGoodTypeIndexWithMemoryProperties(
                res.memReqs.memoryTypeBits, memoryProperty);
    } else {
        res.memory.typeIndex = lastGoodTypeIndex(res.memReqs.memoryTypeBits);
    }

    LOG(VERBOSE) << "ColorBuffer " << colorBufferHandle
                 << ", allocation size and type index: " << res.memory.size
                 << ", " << res.memory.typeIndex
                 << ", allocated memory property: "
                 << sVkEmulation->deviceInfo.memProps
                            .memoryTypes[res.memory.typeIndex]
                            .propertyFlags
                 << ", requested memory property: " << memoryProperty;

    bool isHostVisible = memoryProperty & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    Optional<uint64_t> deviceAlignment =
            isHostVisible ? Optional(res.memReqs.alignment) : kNullopt;
    bool allocRes =
            allocExternalMemory(vk, &res.memory, true /*actuallyExternal*/,
                                deviceAlignment, 
                                use_dedicated? 
                                Optional<VkImage>(res.image): android::base::kNullopt);

    if (!allocRes) {
        LOG(VERBOSE) << "Failed to allocate ColorBuffer with Vulkan backing.";
        return false;
    }

    res.memory.pageOffset =
            reinterpret_cast<uint64_t>(res.memory.mappedPtr) % kPageSize;
    res.memory.bindOffset =
            res.memory.pageOffset ? kPageSize - res.memory.pageOffset : 0u;

    VkResult bindImageMemoryRes =
            vk->vkBindImageMemory(sVkEmulation->device, res.image,
                                  res.memory.memory, res.memory.bindOffset);

    if (bindImageMemoryRes != VK_SUCCESS) {
        fprintf(stderr, "%s: Failed to bind image memory. %d\n", __func__,
        bindImageMemoryRes);
        return false;
    }

    if (sVkEmulation->instanceSupportsMoltenVK) {
        sVkEmulation->getMTLTextureFunc(res.image, &res.mtlTexture);
        if (!res.mtlTexture) {
            fprintf(stderr, "%s: Failed to get MTLTexture.\n", __func__);
        }
#ifdef __APPLE__
        CFRetain(res.mtlTexture);
#endif
    }

    if (sVkEmulation->deviceInfo.supportsExternalMemory &&
        sVkEmulation->deviceInfo.glInteropSupported &&
        glCompatible &&
        FrameBuffer::getFB()->importMemoryToColorBuffer(
            dupExternalMemory(res.memory.exportedHandle),
            res.memory.size,
            false /* dedicated */,
            res.tiling == VK_IMAGE_TILING_LINEAR,
            vulkanOnly,
            colorBufferHandle)) {
        res.glExported = true;
    }

    if (exported) *exported = res.glExported;
    if (allocSize) *allocSize = res.memory.size;
    if (typeIndex) *typeIndex = res.memory.typeIndex;
    if (mappedPtr)
        *mappedPtr = res.memory.mappedPtr;

    sVkEmulation->colorBuffers[colorBufferHandle] = res;

    return true;
}

bool teardownVkColorBuffer(uint32_t colorBufferHandle) {
    if (!sVkEmulation || !sVkEmulation->live) return false;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBufferHandle);

    if (!infoPtr) return false;

    auto& info = *infoPtr;

    vk->vkDestroyImage(sVkEmulation->device, info.image, nullptr);
    freeExternalMemoryLocked(vk, &info.memory);

#ifdef __APPLE__
    if (info.mtlTexture) {
        CFRelease(info.mtlTexture);
    }
#endif

    sVkEmulation->colorBuffers.erase(colorBufferHandle);

    return true;
}

VkEmulation::ColorBufferInfo getColorBufferInfo(uint32_t colorBufferHandle) {
    VkEmulation::ColorBufferInfo res;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBufferHandle);

    if (!infoPtr) return res;

    res = *infoPtr;
    return res;
}

bool updateColorBufferFromVkImage(uint32_t colorBufferHandle) {
    if (!sVkEmulation || !sVkEmulation->live) return false;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBufferHandle);

    if (!infoPtr) {
        // Color buffer not found; this is usually OK.
        return false;
    }

    if (!infoPtr->image) {
        fprintf(stderr, "%s: error: ColorBuffer 0x%x has no VkImage\n", __func__, colorBufferHandle);
        return false;
    }

    if (infoPtr->glExported ||
        (infoPtr->vulkanMode == VkEmulation::VulkanMode::VulkanOnly) ||
        infoPtr->frameworkFormat != FrameworkFormat::FRAMEWORK_FORMAT_GL_COMPATIBLE) {
        // No sync needed if exported to GL or in Vulkan-only mode
        return true;
    }

    // Record our synchronization commands.
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr /* no inheritance info */,
    };

    vk->vkBeginCommandBuffer(
        sVkEmulation->commandBuffer,
        &beginInfo);

    // From the spec: If an application does not need the contents of a resource
    // to remain valid when transferring from one queue family to another, then
    // the ownership transfer should be skipped.

    // We definitely need to transition the image to
    // VK_TRANSFER_SRC_OPTIMAL and back.

    VkImageMemoryBarrier presentToTransferSrc = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
        0,
        VK_ACCESS_HOST_READ_BIT,
        infoPtr->currentLayout,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        infoPtr->image,
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1, 0, 1,
        },
    };

    vk->vkCmdPipelineBarrier(
        sVkEmulation->commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &presentToTransferSrc);

    infoPtr->currentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // Copy to staging buffer
    uint32_t bpp = 4; /* format always rgba8...not */
    switch (infoPtr->format) {
        case VK_FORMAT_R8_UNORM:
            bpp = 1;
            break;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            bpp = 2;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
            bpp = 3;
            break;
        default:
        case VK_FORMAT_R8G8B8A8_UNORM:
            bpp = 4;
            break;
    }
    VkBufferImageCopy region = {
        0 /* buffer offset */,
        infoPtr->extent.width,
        infoPtr->extent.height,
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 0, 1,
        },
        { 0, 0, 0 },
        infoPtr->extent,
    };

    vk->vkCmdCopyImageToBuffer(
        sVkEmulation->commandBuffer,
        infoPtr->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        sVkEmulation->staging.buffer,
        1, &region);

    vk->vkEndCommandBuffer(sVkEmulation->commandBuffer);

    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
        0, nullptr,
        nullptr,
        1, &sVkEmulation->commandBuffer,
        0, nullptr,
    };

    vk->vkQueueSubmit(
        sVkEmulation->queue,
        1, &submitInfo,
        sVkEmulation->commandBufferFence);

    static constexpr uint64_t ANB_MAX_WAIT_NS =
        5ULL * 1000ULL * 1000ULL * 1000ULL;

    vk->vkWaitForFences(
        sVkEmulation->device, 1, &sVkEmulation->commandBufferFence,
        VK_TRUE, ANB_MAX_WAIT_NS);
    vk->vkResetFences(
        sVkEmulation->device, 1, &sVkEmulation->commandBufferFence);

    VkMappedMemoryRange toInvalidate = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
        sVkEmulation->staging.memory.memory,
        0, VK_WHOLE_SIZE,
    };

    vk->vkInvalidateMappedMemoryRanges(
        sVkEmulation->device, 1, &toInvalidate);

    FrameBuffer::getFB()->
        replaceColorBufferContents(
            colorBufferHandle,
            sVkEmulation->staging.memory.mappedPtr,
            bpp * infoPtr->extent.width * infoPtr->extent.height);

    return true;
}

bool updateVkImageFromColorBuffer(uint32_t colorBufferHandle) {
    if (!sVkEmulation || !sVkEmulation->live) return false;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBufferHandle);

    if (!infoPtr) {
        // Color buffer not found; this is usually OK.
        return false;
    }

    if (infoPtr->frameworkFormat == FrameworkFormat::FRAMEWORK_FORMAT_GL_COMPATIBLE && (
        infoPtr->glExported ||
        infoPtr->vulkanMode == VkEmulation::VulkanMode::VulkanOnly)) {
        // No sync needed if exported to GL or in Vulkan-only mode
        return true;
    }

    size_t cbNumBytes = 0;
    bool readRes = FrameBuffer::getFB()->
        readColorBufferContents(
            colorBufferHandle, &cbNumBytes, nullptr);
    if (!readRes) {
        fprintf(stderr, "%s: Failed to read color buffer 0x%x\n",
                __func__, colorBufferHandle);
        return false;
    }

    if (cbNumBytes > sVkEmulation->staging.memory.size) {
        fprintf(stderr,
            "%s: Not enough space to read to staging buffer. "
            "Wanted: 0x%llx Have: 0x%llx\n", __func__,
            (unsigned long long)cbNumBytes,
            (unsigned long long)(sVkEmulation->staging.memory.size));
        return false;
    }

    readRes = FrameBuffer::getFB()->
        readColorBufferContents(
            colorBufferHandle, &cbNumBytes,
            sVkEmulation->staging.memory.mappedPtr);

    if (!readRes) {
        fprintf(stderr, "%s: Failed to read color buffer 0x%x (at glReadPixels)\n",
                __func__, colorBufferHandle);
        return false;
    }

    // Record our synchronization commands.
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr /* no inheritance info */,
    };

    vk->vkBeginCommandBuffer(
        sVkEmulation->commandBuffer,
        &beginInfo);

    // From the spec: If an application does not need the contents of a resource
    // to remain valid when transferring from one queue family to another, then
    // the ownership transfer should be skipped.

    // We definitely need to transition the image to
    // VK_TRANSFER_SRC_OPTIMAL and back.

    VkImageMemoryBarrier presentToTransferSrc = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
        0,
        VK_ACCESS_HOST_READ_BIT,
        infoPtr->currentLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        infoPtr->image,
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1, 0, 1,
        },
    };

    infoPtr->currentLayout =
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    vk->vkCmdPipelineBarrier(
        sVkEmulation->commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &presentToTransferSrc);

    // Copy to staging buffer
    std::vector<VkBufferImageCopy> regions;
    if (infoPtr->frameworkFormat == FrameworkFormat::FRAMEWORK_FORMAT_GL_COMPATIBLE) {
        regions.push_back({
            0 /* buffer offset */,
            infoPtr->extent.width,
            infoPtr->extent.height,
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 0, 1,
            },
            { 0, 0, 0 },
            infoPtr->extent,
        });
    } else {
        // YUV formats
        bool swapUV = infoPtr->frameworkFormat == FRAMEWORK_FORMAT_YV12;
        VkExtent3D subplaneExtent = {
            infoPtr->extent.width / 2,
            infoPtr->extent.height / 2,
            1
        };
        regions.push_back({
            0 /* buffer offset */,
            infoPtr->extent.width,
            infoPtr->extent.height,
            {
                VK_IMAGE_ASPECT_PLANE_0_BIT,
                0, 0, 1,
            },
            { 0, 0, 0 },
            infoPtr->extent,
        });
        regions.push_back({
            infoPtr->extent.width * infoPtr->extent.height /* buffer offset */,
            subplaneExtent.width,
            subplaneExtent.height,
            {
                swapUV ? VK_IMAGE_ASPECT_PLANE_2_BIT : VK_IMAGE_ASPECT_PLANE_1_BIT,
                0, 0, 1,
            },
            { 0, 0, 0 },
            subplaneExtent,
        });
        if (infoPtr->frameworkFormat == FRAMEWORK_FORMAT_YUV_420_888
            || infoPtr->frameworkFormat == FRAMEWORK_FORMAT_YV12) {
            regions.push_back({
                infoPtr->extent.width * infoPtr->extent.height
                    + subplaneExtent.width * subplaneExtent.height,
                subplaneExtent.width,
                subplaneExtent.height,
                {
                    swapUV ? VK_IMAGE_ASPECT_PLANE_1_BIT : VK_IMAGE_ASPECT_PLANE_2_BIT,
                    0, 0, 1,
                },
                { 0, 0, 0 },
                subplaneExtent,
            });
        }
    }

    vk->vkCmdCopyBufferToImage(
        sVkEmulation->commandBuffer,
        sVkEmulation->staging.buffer,
        infoPtr->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regions.size(), regions.data());

    vk->vkEndCommandBuffer(sVkEmulation->commandBuffer);

    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
        0, nullptr,
        nullptr,
        1, &sVkEmulation->commandBuffer,
        0, nullptr,
    };

    vk->vkQueueSubmit(
        sVkEmulation->queue,
        1, &submitInfo,
        sVkEmulation->commandBufferFence);

    static constexpr uint64_t ANB_MAX_WAIT_NS =
        5ULL * 1000ULL * 1000ULL * 1000ULL;

    vk->vkWaitForFences(
        sVkEmulation->device, 1, &sVkEmulation->commandBufferFence,
        VK_TRUE, ANB_MAX_WAIT_NS);
    vk->vkResetFences(
        sVkEmulation->device, 1, &sVkEmulation->commandBufferFence);

    VkMappedMemoryRange toInvalidate = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
        sVkEmulation->staging.memory.memory,
        0, VK_WHOLE_SIZE,
    };

    vk->vkInvalidateMappedMemoryRanges(
        sVkEmulation->device, 1, &toInvalidate);
    return true;
}

VK_EXT_MEMORY_HANDLE getColorBufferExtMemoryHandle(uint32_t colorBuffer) {
    if (!sVkEmulation || !sVkEmulation->live) return VK_EXT_MEMORY_HANDLE_INVALID;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBuffer);

    if (!infoPtr) {
        // Color buffer not found; this is usually OK.
        return VK_EXT_MEMORY_HANDLE_INVALID;
    }

    return infoPtr->memory.exportedHandle;
}

bool setColorBufferVulkanMode(uint32_t colorBuffer, uint32_t vulkanMode) {
    if (!sVkEmulation || !sVkEmulation->live) return VK_EXT_MEMORY_HANDLE_INVALID;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBuffer);

    if (!infoPtr) {
        return false;
    }

    infoPtr->vulkanMode = static_cast<VkEmulation::VulkanMode>(vulkanMode);

    return true;
}

MTLTextureRef getColorBufferMTLTexture(uint32_t colorBuffer) {
    if (!sVkEmulation || !sVkEmulation->live) return nullptr;

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->colorBuffers, colorBuffer);

    if (!infoPtr) {
        // Color buffer not found; this is usually OK.
        return nullptr;
    }

#ifdef __APPLE__
    CFRetain(infoPtr->mtlTexture);
#endif
    return infoPtr->mtlTexture;
}

int32_t mapGpaToBufferHandle(uint32_t bufferHandle,
                             uint64_t gpa,
                             uint64_t size) {
    if (!sVkEmulation || !sVkEmulation->live)
        return VK_ERROR_DEVICE_LOST;

    auto vk = sVkEmulation->dvk;

    AutoLock lock(sVkEmulationLock);

    VkEmulation::ExternalMemoryInfo* memoryInfoPtr = nullptr;

    auto colorBufferInfoPtr =
            android::base::find(sVkEmulation->colorBuffers, bufferHandle);
    if (colorBufferInfoPtr) {
        memoryInfoPtr = &colorBufferInfoPtr->memory;
    }
    auto bufferInfoPtr =
            android::base::find(sVkEmulation->buffers, bufferHandle);
    if (bufferInfoPtr) {
        memoryInfoPtr = &bufferInfoPtr->memory;
    }

    if (!memoryInfoPtr) {
        return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    }

    // memory should be already mapped to host.
    if (!memoryInfoPtr->mappedPtr) {
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    memoryInfoPtr->gpa = gpa;
    memoryInfoPtr->pageAlignedHva =
            reinterpret_cast<uint8_t*>(memoryInfoPtr->mappedPtr) +
            memoryInfoPtr->bindOffset;

    size_t rawSize = memoryInfoPtr->size + memoryInfoPtr->pageOffset;
    if (size && size < rawSize) {
        rawSize = size;
    }

    memoryInfoPtr->sizeToPage = ((rawSize + kPageSize - 1) >> kPageBits)
                                << kPageBits;

    LOG(VERBOSE) << "mapGpaToColorBuffer: hva = " << memoryInfoPtr->mappedPtr
                 << ", pageAlignedHva = " << memoryInfoPtr->pageAlignedHva
                 << " -> [ " << memoryInfoPtr->gpa << ", "
                 << memoryInfoPtr->gpa + memoryInfoPtr->sizeToPage << " ]";

    if (sVkEmulation->occupiedGpas.find(gpa) !=
        sVkEmulation->occupiedGpas.end()) {
        emugl::emugl_crash_reporter("FATAL: already mapped gpa 0x%lx! ", gpa);
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    get_emugl_vm_operations().mapUserBackedRam(
            gpa, memoryInfoPtr->pageAlignedHva, memoryInfoPtr->sizeToPage);

    sVkEmulation->occupiedGpas.insert(gpa);

    return memoryInfoPtr->pageOffset;
}

bool setupVkBuffer(uint32_t bufferHandle,
                   bool vulkanOnly,
                   uint32_t memoryProperty,
                   bool* exported,
                   VkDeviceSize* allocSize,
                   uint32_t* typeIndex) {
    if (vulkanOnly == false) {
        fprintf(stderr, "Data buffers should be vulkanOnly. Setup failed.\n");
        return false;
    }

    auto vk = sVkEmulation->dvk;
    auto fb = FrameBuffer::getFB();

    int size;
    if (!fb->getBufferInfo(bufferHandle, &size)) {
        return false;
    }

    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->buffers, bufferHandle);

    // Already setup
    if (infoPtr) {
        // Update the allocation size to what the host driver wanted, or we
        // might get VK_ERROR_OUT_OF_DEVICE_MEMORY and a host crash
        if (allocSize)
            *allocSize = infoPtr->memory.size;
        // Update the type index to what the host driver wanted, or we might
        // get VK_ERROR_DEVICE_LOST
        if (typeIndex)
            *typeIndex = infoPtr->memory.typeIndex;
        return true;
    }

    VkEmulation::BufferInfo res;

    res.handle = bufferHandle;

    res.size = size;
    res.usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    res.createFlags = 0;

    res.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create the image. If external memory is supported, make it external.
    VkExternalMemoryBufferCreateInfo extBufferCi = {
            VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO,
            0,
            VK_EXT_MEMORY_HANDLE_TYPE_BIT,
    };

    VkExternalMemoryBufferCreateInfo* extBufferCiPtr = nullptr;
    if (sVkEmulation->deviceInfo.supportsExternalMemory) {
        extBufferCiPtr = &extBufferCi;
    }

    VkBufferCreateInfo bufferCi = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            extBufferCiPtr,
            res.createFlags,
            res.size,
            res.usageFlags,
            res.sharingMode,
            /* queueFamilyIndexCount */ 0,
            /* pQueueFamilyIndices */ nullptr,
    };

    VkResult createRes = vk->vkCreateBuffer(sVkEmulation->device, &bufferCi,
                                            nullptr, &res.buffer);

    if (createRes != VK_SUCCESS) {
        LOG(VERBOSE) << "Failed to create Vulkan Buffer for Buffer "
                     << bufferHandle;
        return false;
    }


    bool use_dedicated = false;
    if (vk->vkGetImageMemoryRequirements2KHR) {
        VkMemoryDedicatedRequirements dedicated_reqs{
                VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, nullptr};
        VkMemoryRequirements2 reqs{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
                                   &dedicated_reqs};

        VkBufferMemoryRequirementsInfo2 info{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
            nullptr,
            res.buffer
        };
        vk->vkGetBufferMemoryRequirements2KHR(sVkEmulation->device, &info, &reqs);
        use_dedicated = dedicated_reqs.prefersDedicatedAllocation;
        res.memReqs = reqs.memoryRequirements;
    } else {
    vk->vkGetBufferMemoryRequirements(sVkEmulation->device, res.buffer,
                                      &res.memReqs);
    }


    // Currently we only care about two memory properties: DEVICE_LOCAL
    // and HOST_VISIBLE; other memory properties specified in
    // rcSetColorBufferVulkanMode2() call will be ignored for now.
    memoryProperty = memoryProperty & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    res.memory.size = res.memReqs.size;

    // Determine memory type.
    if (memoryProperty) {
        res.memory.typeIndex = lastGoodTypeIndexWithMemoryProperties(
                res.memReqs.memoryTypeBits, memoryProperty);
    } else {
        res.memory.typeIndex = lastGoodTypeIndex(res.memReqs.memoryTypeBits);
    }

    LOG(VERBOSE) << "Buffer " << bufferHandle
                 << "allocation size and type index: " << res.memory.size
                 << ", " << res.memory.typeIndex
                 << ", allocated memory property: "
                 << sVkEmulation->deviceInfo.memProps
                            .memoryTypes[res.memory.typeIndex]
                            .propertyFlags
                 << ", requested memory property: " << memoryProperty;

    bool isHostVisible = memoryProperty & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    Optional<uint64_t> deviceAlignment =
            isHostVisible ? Optional(res.memReqs.alignment) : kNullopt;
    bool allocRes = allocExternalMemory(
            vk, &res.memory, true /* actuallyExternal */, deviceAlignment,
                android::base::kNullopt,
                use_dedicated? Optional(res.buffer): android::base::kNullopt);

    if (!allocRes) {
        LOG(VERBOSE) << "Failed to allocate ColorBuffer with Vulkan backing.";
    }

    res.memory.pageOffset =
            reinterpret_cast<uint64_t>(res.memory.mappedPtr) % kPageSize;
    res.memory.bindOffset =
            res.memory.pageOffset ? kPageSize - res.memory.pageOffset : 0u;

    VkResult bindBufferMemoryRes = vk->vkBindBufferMemory(
            sVkEmulation->device, res.buffer, res.memory.memory, 0);

    if (bindBufferMemoryRes != VK_SUCCESS) {
        fprintf(stderr, "%s: Failed to bind buffer memory. %d\n", __func__,
                bindBufferMemoryRes);
        return bindBufferMemoryRes;
    }

    bool isHostVisibleMemory =
            memoryProperty & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    if (isHostVisibleMemory) {
        VkResult mapMemoryRes =
                vk->vkMapMemory(sVkEmulation->device, res.memory.memory, 0,
                                res.memory.size, {}, &res.memory.mappedPtr);

        if (mapMemoryRes != VK_SUCCESS) {
            fprintf(stderr, "%s: Failed to map image memory. %d\n", __func__,
                    mapMemoryRes);
            return false;
        }
    }

    res.glExported = false;
    if (exported)
        *exported = res.glExported;
    if (allocSize)
        *allocSize = res.memory.size;
    if (typeIndex)
        *typeIndex = res.memory.typeIndex;

    sVkEmulation->buffers[bufferHandle] = res;
    return allocRes;
}

bool teardownVkBuffer(uint32_t bufferHandle) {
    if (!sVkEmulation || !sVkEmulation->live)
        return false;

    auto vk = sVkEmulation->dvk;
    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->buffers, bufferHandle);
    if (!infoPtr)
        return false;
    auto& info = *infoPtr;

    vk->vkDestroyBuffer(sVkEmulation->device, info.buffer, nullptr);
    freeExternalMemoryLocked(vk, &info.memory);
    sVkEmulation->buffers.erase(bufferHandle);

    return true;
}

VK_EXT_MEMORY_HANDLE getBufferExtMemoryHandle(uint32_t bufferHandle) {
    if (!sVkEmulation || !sVkEmulation->live)
        return VK_EXT_MEMORY_HANDLE_INVALID;

    auto vk = sVkEmulation->dvk;
    AutoLock lock(sVkEmulationLock);

    auto infoPtr = android::base::find(sVkEmulation->buffers, bufferHandle);
    if (!infoPtr) {
        // Color buffer not found; this is usually OK.
        return VK_EXT_MEMORY_HANDLE_INVALID;
    }

    return infoPtr->memory.exportedHandle;
}

VkExternalMemoryHandleTypeFlags
transformExternalMemoryHandleTypeFlags_tohost(
    VkExternalMemoryHandleTypeFlags bits) {

    VkExternalMemoryHandleTypeFlags res = bits;

    // Transform Android/Fuchsia/Linux bits to host bits.
    if (bits & VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT) {
        res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    }

#ifdef _WIN32
    res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#endif

    if (bits & VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
        res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
        res |= VK_EXT_MEMORY_HANDLE_TYPE_BIT;
    }

    if (bits & VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA) {
        res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA;
        res |= VK_EXT_MEMORY_HANDLE_TYPE_BIT;
    }
    return res;
}

VkExternalMemoryHandleTypeFlags
transformExternalMemoryHandleTypeFlags_fromhost(
    VkExternalMemoryHandleTypeFlags hostBits,
    VkExternalMemoryHandleTypeFlags wantedGuestHandleType) {

    VkExternalMemoryHandleTypeFlags res = hostBits;

    if (res & VK_EXT_MEMORY_HANDLE_TYPE_BIT) {
        res &= ~VK_EXT_MEMORY_HANDLE_TYPE_BIT;
        res |= wantedGuestHandleType;
    }

#ifdef _WIN32
    res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    res &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#endif

    return res;
}

VkExternalMemoryProperties
transformExternalMemoryProperties_tohost(
    VkExternalMemoryProperties props) {
    VkExternalMemoryProperties res = props;
    res.exportFromImportedHandleTypes =
        transformExternalMemoryHandleTypeFlags_tohost(
            props.exportFromImportedHandleTypes);
    res.compatibleHandleTypes =
        transformExternalMemoryHandleTypeFlags_tohost(
            props.compatibleHandleTypes);
    return res;
}

VkExternalMemoryProperties
transformExternalMemoryProperties_fromhost(
    VkExternalMemoryProperties props,
    VkExternalMemoryHandleTypeFlags wantedGuestHandleType) {
    VkExternalMemoryProperties res = props;
    res.exportFromImportedHandleTypes =
        transformExternalMemoryHandleTypeFlags_fromhost(
            props.exportFromImportedHandleTypes,
            wantedGuestHandleType);
    res.compatibleHandleTypes =
        transformExternalMemoryHandleTypeFlags_fromhost(
            props.compatibleHandleTypes,
            wantedGuestHandleType);
    return res;
}

} // namespace goldfish_vk
