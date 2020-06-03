// Copyright (C) 2018 The Android Open Source Project
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

#include "VulkanDispatch.h"

#include "VkCommonOperations.h"

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/synchronization/Lock.h"
#include "android/utils/path.h"

#include "emugl/common/misc.h"
#include "emugl/common/shared_library.h"

#include <memory>
#include <vector>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::pj;
using android::base::System;

namespace emugl {

static void setIcdPath(const std::string& path) {
    if (path_exists(path.c_str())) {
        LOG(ERROR) << "setIcdPath: path exists: " << path;
    } else {
        LOG(ERROR) << "setIcdPath: path doesn't exist: " << path;
    }
    System::get()->envSet("VK_ICD_FILENAMES", path);
}

static std::string icdJsonNameToProgramAndLauncherPaths(
        const std::string& icdFilename) {

    std::string suffix = pj("lib64", "vulkan", icdFilename);

    return pj(System::get()->getProgramDirectory(), suffix) + ":" +
           pj(System::get()->getLauncherDirectory(), suffix);
}

static void initIcdPaths(bool forTesting) {
    auto androidIcd = System::get()->envGet("ANDROID_EMU_VK_ICD");
    if (System::get()->envGet("ANDROID_EMU_SANDBOX") == "1") {
        // Rely on user to set VK_ICD_FILENAMES
        return;
    } else {
        if (forTesting || androidIcd == "swiftshader") {
            auto res = pj(System::get()->getProgramDirectory(), "lib64", "vulkan");
            LOG(ERROR) << "In test environment or ICD set to swiftshader, using "
                            "Swiftshader ICD";
            auto libPath = pj(System::get()->getProgramDirectory(), "lib64", "vulkan", "libvk_swiftshader.so");;
            if (path_exists(libPath.c_str())) {
                LOG(ERROR) << "Swiftshader library exists";
            } else {
                LOG(ERROR) << "Swiftshader library doesn't exist, trying launcher path";
                libPath = pj(System::get()->getLauncherDirectory(), "lib64", "vulkan", "libvk_swiftshader.so");;
                if (path_exists(libPath.c_str())) {
                    LOG(ERROR) << "Swiftshader library found in launcher path";
                } else {
                    LOG(ERROR) << "Swiftshader library not found in program nor launcher path";
                }
            }
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
            System::get()->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
        } else {
            LOG(ERROR) << "Not in test environment. ICD (blank for default): ["
                         << androidIcd << "]";
            // Mac: Use MoltenVK by default unless GPU mode is set to swiftshader,
            // and switch between that and gfx-rs libportability-icd depending on
            // the environment variable setting.
    #ifdef __APPLE__
            if (androidIcd == "portability") {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos.json"));
            } else if (androidIcd == "portability-debug") {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos-debug.json"));
            } else {
                if (androidIcd == "swiftshader" ||
                    emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER ||
                    emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
                    setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
                    System::get()->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
                } else {
                    setIcdPath(icdJsonNameToProgramAndLauncherPaths("MoltenVK_icd.json"));
                    System::get()->envSet("ANDROID_EMU_VK_ICD", "moltenvk");
                }
            }
#else
            // By default, on other platforms, just use whatever the system
            // is packing.
#endif
        }
    }
}

#ifdef __APPLE__
#define VULKAN_LOADER_FILENAME "libvulkan.dylib"
#else
#ifdef _WIN32
#define VULKAN_LOADER_FILENAME "vulkan-1.dll"
#else
// linux
#define VULKAN_LOADER_FILENAME "libvulkan.so"
#define VULKAN_LOADER_FILENAME_WITH_SUFFIX "libvulkan.so.1"
#endif

#endif
static std::string getLoaderPath(
    const std::string& directory,
    bool forTesting,
    bool useSystemLoader,
    bool useSystemLoaderWithSuffix) {

    (void)useSystemLoader;
    (void)useSystemLoaderWithSuffix;

    auto path = System::get()->envGet("ANDROID_EMU_VK_LOADER_PATH");
    if (!path.empty()) {
        return path;
    }
    auto androidIcd = System::get()->envGet("ANDROID_EMU_VK_ICD");
    if (forTesting || androidIcd == "mock") {
        auto path = pj(directory, "testlib64", VULKAN_LOADER_FILENAME);
        LOG(ERROR) << "In test environment or using Swiftshader. Using loader: " << path;
        return path;
    } else {
#ifdef _WIN32
        LOG(ERROR) << "Not in test environment. Using loader: " << VULKAN_LOADER_FILENAME;
        return VULKAN_LOADER_FILENAME;
#else
#ifdef __APPLE__
        // Skip loader when using MoltenVK as this gives us access to
        // VK_MVK_moltenvk, which is required for external memory support.
        if (androidIcd == "moltenvk") {
            auto path = pj(directory, "lib64", "vulkan", "libMoltenVK.dylib");
            LOG(ERROR) << "Skipping loader and using ICD directly: " << path;
            return path;
        }
#else
        // linux
        if (useSystemLoader) {
            if (useSystemLoaderWithSuffix) {
                LOG(ERROR) << "Not in test environment. Using system loader w/ suffix: " << VULKAN_LOADER_FILENAME_WITH_SUFFIX;
                return VULKAN_LOADER_FILENAME_WITH_SUFFIX;
            } else {
                LOG(ERROR) << "Not in test environment. Using system loader: " << VULKAN_LOADER_FILENAME;
                return VULKAN_LOADER_FILENAME;
            }
        }

#endif
        auto path = pj(directory, "lib64", "vulkan", VULKAN_LOADER_FILENAME);
        LOG(ERROR) << "Not in test environment. Using loader: " << path;
        return path;
#endif
    }
}

class VulkanDispatchImpl {
public:
    VulkanDispatchImpl() = default;

    void initialize(bool forTesting);

    void* dlopen() {
        bool sandbox = System::get()->envGet("ANDROID_EMU_SANDBOX") == "1";

        if (!mVulkanLoader) {
            if (sandbox) {
                mVulkanLoader = emugl::SharedLibrary::open(VULKAN_LOADER_FILENAME);
            } else {
                auto loaderPath = getLoaderPath(System::get()->getProgramDirectory(), mForTesting, mPreferSystemLoader, false /* no suffix  on linux */);
                mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());

                if (!mVulkanLoader) {
                    fprintf(stderr, "%s: using suffix system laoder? mPreferSystemLoader %d\n", __func__, mPreferSystemLoader);
                    loaderPath = getLoaderPath(System::get()->getLauncherDirectory(), mForTesting, mPreferSystemLoader, true /* suffix on linux */);
                    mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());
                }
            }
        }
#ifdef __linux__
        // On Linux, it might not be called libvulkan.so.
        // Try libvulkan.so.1 if that doesn't work.
        if (!mVulkanLoader) {
            if (sandbox) {
                mVulkanLoader =
                    emugl::SharedLibrary::open("libvulkan.so.1");
            } else {
                auto altPath = pj(System::get()->getLauncherDirectory(),
                    "lib64", "vulkan", "libvulkan.so.1");
                mVulkanLoader =
                    emugl::SharedLibrary::open(altPath.c_str());
            }
        }
#endif
        return (void*)mVulkanLoader;
    }

    void* dlsym(void* lib, const char* name) {
        return (void*)((emugl::SharedLibrary*)(lib))->findSymbol(name);
    }

    VulkanDispatch* dispatch() { return &mDispatch; }

    void setPreferSystemLoader(bool prefer) {
        mPreferSystemLoader = prefer;
    }

private:
    Lock mLock;
    bool mForTesting = false;
    bool mInitialized = false;
#ifdef _WIN32
    bool mPreferSystemLoader = true;
#else
    bool mPreferSystemLoader = false;
#endif
    VulkanDispatch mDispatch;
    emugl::SharedLibrary* mVulkanLoader = nullptr;
};

static LazyInstance<VulkanDispatchImpl> sDispatchImpl =
    LAZY_INSTANCE_INIT;

static void* sVulkanDispatchDlOpen()  {
    return sDispatchImpl->dlopen();
}

static void* sVulkanDispatchDlSym(void* lib, const char* sym) {
    return sDispatchImpl->dlsym(lib, sym);
}

static bool extensionsSupported(
    const std::vector<VkExtensionProperties>& currentProps,
    const std::vector<const char*>& wantedExtNames) {

    std::vector<bool> foundExts(wantedExtNames.size(), false);

    for (uint32_t i = 0; i < currentProps.size(); ++i) {
        LOG(ERROR) << "has extension: " << currentProps[i].extensionName;
        for (size_t j = 0; j < wantedExtNames.size(); ++j) {
            if (!strcmp(wantedExtNames[j], currentProps[i].extensionName)) {
                foundExts[j] = true;
            }
        }
    }

    for (size_t i = 0; i < wantedExtNames.size(); ++i) {
        bool found = foundExts[i];
        LOG(ERROR) << "needed extension: " << wantedExtNames[i]
                     << " found: " << found;
        if (!found) {
            LOG(ERROR) << wantedExtNames[i] << " not found, bailing.";
            return false;
        }
    }

    return true;
}

static int sCalculateLoaderScore(VulkanDispatch* vk) {
    int score = 0;

    auto gvk = vk;

    std::vector<const char*> externalMemoryInstanceExtNames = {
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_get_physical_device_properties2",
    };

    std::vector<const char*> moltenVKInstanceExtNames = {
        "VK_MVK_moltenvk",
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
    bool moltenVKSupported =
        extensionsSupported(exts, moltenVKInstanceExtNames);

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
        instCi.enabledExtensionCount =
            moltenVKInstanceExtNames.size();
        instCi.ppEnabledExtensionNames =
            moltenVKInstanceExtNames.data();
   }

    VkInstance instance;
    VkResult res = gvk->vkCreateInstance(
        &instCi, nullptr, &instance);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create Vulkan instance.";
        score -= 1000;
        return score;
    }

    // Create instance level dispatch.
    auto ivk = std::make_unique<VulkanDispatch>();
    init_vulkan_dispatch_from_instance(
        vk, instance, ivk.get());

    if (!vulkan_dispatch_check_instance_VK_VERSION_1_0(ivk.get())) {
        LOG(ERROR) << "Vulkan 1.0 APIs missing from this loader";
        score -= 1000;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    if (ivk->vkEnumerateInstanceVersion) {
        uint32_t instanceVersion;
        VkResult enumInstanceRes = ivk->vkEnumerateInstanceVersion(&instanceVersion);
        if (enumInstanceRes >= VK_MAKE_VERSION(1, 1, 0)) {
            if (!vulkan_dispatch_check_instance_VK_VERSION_1_1(ivk.get())) {
                LOG(ERROR) << "Vulkan 1.1 APIs missing from this loader (instance)";
                score -= 1000;
                gvk->vkDestroyInstance(instance, nullptr);
                return score;
            }
        }
        // Vulkan >= 1.2
        if (enumInstanceRes >= VK_MAKE_VERSION(1, 2, 0)) {
            score += 1;
        }
    } else {
        LOG(ERROR) << "vkEnumerateInstanceVersion not found, probably not 1.1";
        score -= 100;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    if (ivk->vkEnumeratePhysicalDevices) {
        uint32_t physdevCount = 0;
        ivk->vkEnumeratePhysicalDevices(instance, &physdevCount,
                                       nullptr);
        score += physdevCount;
        if (physdevCount == 0) {
            LOG(ERROR) << "No Vulkan physical devices found";
            score -= 1000;
            gvk->vkDestroyInstance(instance, nullptr);
            return score;
        }
    } else {
        LOG(ERROR) << "vkEnumeratePhysicalDevices not found";
        score -= 1000;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    uint32_t physdevCount = 0;
    ivk->vkEnumeratePhysicalDevices(instance, &physdevCount,
                                   nullptr);
    std::vector<VkPhysicalDevice> physdevs(physdevCount);
    ivk->vkEnumeratePhysicalDevices(instance, &physdevCount,
                                   physdevs.data());

    LOG(ERROR) << "Found " << physdevCount << " Vulkan physical devices.";

    if (physdevCount == 0) {
        LOG(ERROR) << "No physical devices available.";
        score -= 10000;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    std::vector<goldfish_vk::VkEmulation::DeviceSupportInfo> deviceInfos(physdevCount);

    for (int i = 0; i < physdevCount; ++i) {
        ivk->vkGetPhysicalDeviceProperties(physdevs[i],
                                           &deviceInfos[i].physdevProps);

        LOG(ERROR) << "Considering Vulkan physical device " << i << ": "
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

        if (externalMemoryCapabilitiesSupported) {
            deviceInfos[i].supportsExternalMemory = extensionsSupported(
                    deviceExts, externalMemoryDeviceExtNames);
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
                LOG(ERROR) << "Graphics queue family index: " << j;
            }

            if (hasComputeQueueFamily) {
                deviceInfos[i].computeQueueFamilyIndices.push_back(j);
                LOG(ERROR) << "Compute queue family index: " << j;
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

    VkPhysicalDevice physdev = physdevs[maxScoringIndex];
    auto bestDeviceInfo = deviceInfos[maxScoringIndex];

    if (!bestDeviceInfo.hasGraphicsQueueFamily) {
        LOG(ERROR) << "No Vulkan devices with graphics queues found.";
        score -= 10000;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    auto deviceVersion = bestDeviceInfo.physdevProps.apiVersion;

    LOG(ERROR) << "Vulkan device found: "
                 << bestDeviceInfo.physdevProps.deviceName;
    LOG(ERROR) << "Version: "
                 << VK_VERSION_MAJOR(deviceVersion) << "." << VK_VERSION_MINOR(deviceVersion) << "." << VK_VERSION_PATCH(deviceVersion);
    LOG(ERROR) << "Has graphics queue? "
                 << bestDeviceInfo.hasGraphicsQueueFamily;
    LOG(ERROR) << "Has external memory support? "
                 << bestDeviceInfo.supportsExternalMemory;
    LOG(ERROR) << "Has compute queue? "
                 << bestDeviceInfo.hasComputeQueueFamily;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo dqCi = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
        bestDeviceInfo.graphicsQueueFamilyIndices[0],
        1, &priority,
    };

    uint32_t selectedDeviceExtensionCount = 0;
    const char* const* selectedDeviceExtensionNames = nullptr;

    if (bestDeviceInfo.supportsExternalMemory) {
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

    VkDevice device;
    ivk->vkCreateDevice(physdev, &dCi, nullptr,
                        &device);

    if (res != VK_SUCCESS) {
        LOG(ERROR) << "Failed to create Vulkan device.";
        score -= 10000;
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    // device created; populate dispatch table
    auto dvk = std::make_unique<VulkanDispatch>();
    init_vulkan_dispatch_from_device(
        ivk.get(), device, dvk.get());

    // Check if the dispatch table has everything 1.1 related
    if (!vulkan_dispatch_check_device_VK_VERSION_1_0(dvk.get())) {
        LOG(ERROR) << "Warning: Vulkan 1.0 APIs missing from device.";
        score -= 10000;
        ivk->vkDestroyDevice(device, nullptr);
        gvk->vkDestroyInstance(instance, nullptr);
        return score;
    }

    if (deviceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
        if (!vulkan_dispatch_check_device_VK_VERSION_1_1(dvk.get())) {
            LOG(ERROR) << "Warning: Vulkan 1.1 APIs missing from device.";
            score -= 10000;
            ivk->vkDestroyDevice(device, nullptr);
            gvk->vkDestroyInstance(instance, nullptr);
            return score;
        }
    }

    ivk->vkDestroyDevice(device, nullptr);
    gvk->vkDestroyInstance(instance, nullptr);
    return score;
}

void VulkanDispatchImpl::initialize(bool forTesting) {
    AutoLock lock(mLock);

    if (mInitialized) return;

    mForTesting = forTesting;
    initIcdPaths(mForTesting);

#ifdef __linux__

    sDispatchImpl->setPreferSystemLoader(false);
    goldfish_vk::init_vulkan_dispatch_from_system_loader(
            sVulkanDispatchDlOpen,
            sVulkanDispatchDlSym,
            &mDispatch);
    if (!mForTesting) {

        fprintf(stderr, "%s: prebuilt loader?\n", __func__);
        int scorePrebuiltLoader = sCalculateLoaderScore(&mDispatch);

        fprintf(stderr, "%s: system laoder loader?\n", __func__);
        sDispatchImpl->setPreferSystemLoader(true);
        goldfish_vk::init_vulkan_dispatch_from_system_loader(
                sVulkanDispatchDlOpen,
                sVulkanDispatchDlSym,
                &mDispatch);

        int scoreSystemLoader = sCalculateLoaderScore(&mDispatch);

        fprintf(stderr, "%s: prebuilt loader score: %d system loader score: %d********************************************************************************\n", __func__,
                scorePrebuiltLoader, scoreSystemLoader);

        if (scoreSystemLoader < scorePrebuiltLoader) {
            fprintf(stderr, "%s: warning, reverting back to prebuilt loader********************************************************************************\n", __func__);
            sDispatchImpl->setPreferSystemLoader(false);
            goldfish_vk::init_vulkan_dispatch_from_system_loader(
                    sVulkanDispatchDlOpen,
                    sVulkanDispatchDlSym,
                    &mDispatch);
        } else {
            fprintf(stderr, "%s: system loader might just work, lets try it********************************************************************************\n", __func__);
        }

    }
    mInitialized = true;
#else
    goldfish_vk::init_vulkan_dispatch_from_system_loader(
            sVulkanDispatchDlOpen,
            sVulkanDispatchDlSym,
            &mDispatch);

    mInitialized = true;
#endif
}

VulkanDispatch* vkDispatch(bool forTesting) {
    sDispatchImpl->initialize(forTesting);
    return sDispatchImpl->dispatch();
}

bool vkDispatchValid(const VulkanDispatch* vk) {
    return vk->vkEnumerateInstanceExtensionProperties != nullptr ||
           vk->vkGetInstanceProcAddr != nullptr ||
           vk->vkGetDeviceProcAddr != nullptr;
}

int vkDispatchScore(const VulkanDispatch* vk) {
}

} // namespace emugl
