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

#include "VulkanDispatch.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include "emugl/common/crash_reporter.h"

#include <unordered_map>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData)
{
    fprintf(stderr, "dbg report: %s\n", pMessage);
    return VK_FALSE;
}

class VkDecoderGlobalState::Impl {
public:
    Impl() : m_vk(emugl::vkDispatch()),
             mEnableDebugReport(android::base::System::get()->envGet("ANDROID_EMU_VK_ENABLE_DEBUG_REPORT") == "1") { }
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
            mDevices[*pDevice] = physicalDevice;
        }

        VkPhysicalDeviceMemoryProperties props;
        m_vk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &props);

        {
            AutoLock lock(mLock);
            mPhysicalDeviceMemoryProperties[physicalDevice] = props;
        }

        return result;
    }

    void on_vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {

        AutoLock lock(mLock);
        mDevices.erase(device);

        // Run the underlying API call.
        m_vk->vkDestroyDevice(device, pAllocator);
    }

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


    VkResult on_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkInstance* pInstance) {

        std::vector<const char*> editedExts;

        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i) {
            editedExts.push_back(pCreateInfo->ppEnabledExtensionNames[i]);
        }

        if (mEnableDebugReport) {
            editedExts.push_back("VK_EXT_debug_report");
        }

        VkInstanceCreateInfo local_createInfo = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            pCreateInfo->pNext,
            pCreateInfo->flags,
            pCreateInfo->pApplicationInfo,
            pCreateInfo->enabledLayerCount,
            pCreateInfo->ppEnabledLayerNames,
            (uint32_t)editedExts.size(),
            editedExts.data(),
        };

        VkResult res = m_vk->vkCreateInstance(&local_createInfo, pAllocator, pInstance);

        if (mEnableDebugReport && res == VK_SUCCESS) {

            VkDebugReportCallbackCreateInfoEXT callback1 = {
                VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
                NULL,
                VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_DEBUG_BIT_EXT,
                &MyDebugReportCallback,
                NULL
            };

            VkDebugReportCallbackEXT callback;
            if (!mCreateDebugReportCallback) {
                mCreateDebugReportCallback =
                    (PFN_vkCreateDebugReportCallbackEXT)
                    m_vk->vkGetInstanceProcAddr(*pInstance, "vkCreateDebugReportCallbackEXT");
                mDestroyDebugReportCallback =
                    (PFN_vkDestroyDebugReportCallbackEXT)
                    m_vk->vkGetInstanceProcAddr(*pInstance, "vkDestroyDebugReportCallbackEXT");
            }
            mCreateDebugReportCallback(*pInstance, &callback1, nullptr, &callback);

            AutoLock lock(mLock);
            mDebugReportCallbacks[*pInstance] = callback;
        }

        return res;
    }

    void on_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
        AutoLock lock(mLock);
        if (mEnableDebugReport) {
            auto it = mDebugReportCallbacks.find(instance);
            if (it != mDebugReportCallbacks.end()) {
                mDestroyDebugReportCallback(instance, it->second, nullptr);
            }
        }
        m_vk->vkDestroyInstance(instance, pAllocator);
    }

private:
    // We always map the whole size on host.
    // This makes it much easier to implement
    // the memory map API.
    struct MappedMemoryInfo {
        // When ptr is null, it means the VkDeviceMemory object
        // was not allocated with the HOST_VISIBLE property.
        void* ptr = nullptr;
        VkDeviceSize size;
    };

    VulkanDispatch* m_vk;

    // Enable debug report if user specified the environment variable.
    bool mEnableDebugReport = false;
    PFN_vkCreateDebugReportCallbackEXT mCreateDebugReportCallback = 0;
    PFN_vkDestroyDebugReportCallbackEXT mDestroyDebugReportCallback = 0;

    Lock mLock;

    // Back-reference to the physical device associated
    // with a particular VkDevice.
    std::unordered_map<VkDevice, VkPhysicalDevice> mDevices;
    // Keep the physical device memory properties around.
    std::unordered_map<VkPhysicalDevice, VkPhysicalDeviceMemoryProperties>
        mPhysicalDeviceMemoryProperties;

    std::unordered_map<VkDeviceMemory, MappedMemoryInfo> mMapInfo;

    std::unordered_map<VkInstance, VkDebugReportCallbackEXT> mDebugReportCallbacks;
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

VkResult VkDecoderGlobalState::on_vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
    return mImpl->on_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

void VkDecoderGlobalState::on_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyInstance(instance, pAllocator);
}
