// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "host-common/opengl/emugl_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/files/Stream.h"
#include "android/utils/path.h"

#include "aemu/base/StringFormat.h"
#include "aemu/base/logging/Log.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/opengl/EmuglBackendList.h"
#include "android/opengl/gpuinfo.h"
#include "android/skin/backend-defs.h"
#include "host-common/crash-handler.h"
#include "host-common/feature_control.h"
#include "host-common/opengles.h"
#include "vulkan/vulkan.h"

#if defined(__APPLE__)
#if (VK_HEADER_VERSION > 216)
#include <vulkan/vulkan_beta.h>
#else
// Manually define MoltenVK related parts until we update the Vulkan headers
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME \
    "VK_KHR_portability_enumeration"
#endif
static const uint32_t VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR =
        0x00000001;
#endif
#endif

#ifndef _WIN32
#include <dlfcn.h>
#endif

typedef char      hw_bool_t;
typedef int       hw_int_t;
typedef int64_t   hw_disksize_t;
typedef char*     hw_string_t;
typedef double    hw_double_t;

/* these macros are used to define the fields of AndroidHwConfig
 * declared below
 */
#define   HWCFG_BOOL(n,s,d,a,t)       hw_bool_t      n;
#define   HWCFG_INT(n,s,d,a,t)        hw_int_t       n;
#define   HWCFG_STRING(n,s,d,a,t)     hw_string_t    n;
#define   HWCFG_DOUBLE(n,s,d,a,t)     hw_double_t    n;
#define   HWCFG_DISKSIZE(n,s,d,a,t)   hw_disksize_t  n;

typedef struct AndroidHwConfig {
#include "android/avd/hw-config-defs.h"
} AndroidHwConfig;

#define DEBUG 0

#if DEBUG
#define D(...)  dprint(__VA_ARGS__)
#else
#define D(...)  crashhandler_append_message_format(__VA_ARGS__)
#endif

using android::base::RunOptions;
using android::base::StringFormat;
using android::base::System;
using android::opengl::EmuglBackendList;
namespace fc = android::featurecontrol;

static EmuglBackendList* sBackendList = NULL;

static void resetBackendList(int bitness) {
    delete sBackendList;
    if (System::getEnvironmentVariable("ANDROID_EMUGL_FIXED_BACKEND_LIST") == "1") {
        std::vector<std::string> fixedBackendNames = {
            "swiftshader_indirect",
            "angle_indirect",
        };
        sBackendList = new EmuglBackendList(64, fixedBackendNames);
    } else {
        sBackendList = new EmuglBackendList(
                System::get()->getLauncherDirectory().c_str(), bitness);
    }
}

static bool stringVectorContains(const std::vector<std::string>& list,
                                 const char* value) {
    for (size_t n = 0; n < list.size(); ++n) {
        if (!strcmp(list[n].c_str(), value)) {
            return true;
        }
    }
    return false;
}

bool isHostGpuBlacklisted() {
    return async_query_host_gpu_blacklisted();
}

// Get a description of host GPU properties.
// Need to free after use.
emugl_host_gpu_prop_list emuglConfig_get_host_gpu_props() {
    const GpuInfoList& gpulist = globalGpuInfoList();
    emugl_host_gpu_prop_list res;
    res.num_gpus = gpulist.infos.size();
    res.props = new emugl_host_gpu_props[res.num_gpus];

    const std::vector<GpuInfo>& infos = gpulist.infos;
    for (int i = 0; i < res.num_gpus; i++) {
        res.props[i].make = strdup(infos[i].make.c_str());
        res.props[i].model = strdup(infos[i].model.c_str());
        res.props[i].device_id = strdup(infos[i].device_id.c_str());
        res.props[i].revision_id = strdup(infos[i].revision_id.c_str());
        res.props[i].version = strdup(infos[i].version.c_str());
        res.props[i].renderer = strdup(infos[i].renderer.c_str());
    }
    return res;
}

SelectedRenderer emuglConfig_get_renderer(const char* gpu_mode) {
    if (!gpu_mode) {
        return SELECTED_RENDERER_UNKNOWN;
    } else if (!strcmp(gpu_mode, "host") ||
        !strcmp(gpu_mode, "on")) {
        return SELECTED_RENDERER_HOST;
    } else if (!strcmp(gpu_mode, "off")) {
        return SELECTED_RENDERER_OFF;
    } else if (!strcmp(gpu_mode, "guest")) {
        return SELECTED_RENDERER_GUEST;
    } else if (!strcmp(gpu_mode, "mesa")) {
        return SELECTED_RENDERER_MESA;
    } else if (!strcmp(gpu_mode, "swiftshader")) {
        return SELECTED_RENDERER_SWIFTSHADER;
    } else if (!strcmp(gpu_mode, "angle")
            || !strcmp(gpu_mode, "swangle")) {
        return SELECTED_RENDERER_ANGLE;
    } else if (!strcmp(gpu_mode, "angle9")) {
        return SELECTED_RENDERER_ANGLE9;
    } else if (!strcmp(gpu_mode, "swiftshader_indirect")) {
        return SELECTED_RENDERER_SWIFTSHADER_INDIRECT;
    } else if (!strcmp(gpu_mode, "angle_indirect")
            || !strcmp(gpu_mode, "swangle_indirect")) {
        return SELECTED_RENDERER_ANGLE_INDIRECT;
    } else if (!strcmp(gpu_mode, "angle9_indirect")) {
        return SELECTED_RENDERER_ANGLE9_INDIRECT;
    } else if (!strcmp(gpu_mode, "error")) {
        return SELECTED_RENDERER_ERROR;
    } else {
        return SELECTED_RENDERER_UNKNOWN;
    }
}

static SelectedRenderer sCurrentRenderer =
    SELECTED_RENDERER_UNKNOWN;

SelectedRenderer emuglConfig_get_current_renderer() {
    return sCurrentRenderer;
}

static std::string sGpuOption;

const char* emuglConfig_get_user_gpu_option() {
    return sGpuOption.c_str();
}

const char* emuglConfig_renderer_to_string(SelectedRenderer renderer) {
    switch (renderer) {
        case SELECTED_RENDERER_UNKNOWN:
            return "(Unknown)";
        case SELECTED_RENDERER_HOST:
            return "Host";
        case SELECTED_RENDERER_OFF:
            return "Off";
        case SELECTED_RENDERER_GUEST:
            return "Guest";
        case SELECTED_RENDERER_MESA:
            return "Mesa";
        case SELECTED_RENDERER_SWIFTSHADER:
            return "Swiftshader";
        case SELECTED_RENDERER_ANGLE:
            return "Angle";
        case SELECTED_RENDERER_ANGLE9:
            return "Angle9";
        case SELECTED_RENDERER_SWIFTSHADER_INDIRECT:
            return "Swiftshader Indirect";
        case SELECTED_RENDERER_ANGLE_INDIRECT:
            return "Angle Indirect";
        case SELECTED_RENDERER_ANGLE9_INDIRECT:
            return "Angle9 Indirect";
        case SELECTED_RENDERER_ERROR:
            return "(Error)";
    }
    return "(Bad value)";
}

bool emuglConfig_current_renderer_supports_snapshot() {
    if (getConsoleAgents()->settings->hw()->hw_arc) {
        return sCurrentRenderer == SELECTED_RENDERER_OFF ||
               sCurrentRenderer == SELECTED_RENDERER_GUEST;
    }
    return sCurrentRenderer == SELECTED_RENDERER_HOST ||
           sCurrentRenderer == SELECTED_RENDERER_OFF ||
           sCurrentRenderer == SELECTED_RENDERER_GUEST ||
           sCurrentRenderer == SELECTED_RENDERER_ANGLE_INDIRECT ||
           sCurrentRenderer == SELECTED_RENDERER_SWIFTSHADER_INDIRECT;
}

void free_emugl_host_gpu_props(emugl_host_gpu_prop_list proplist) {
    for (int i = 0; i < proplist.num_gpus; i++) {
        free(proplist.props[i].make);
        free(proplist.props[i].model);
        free(proplist.props[i].device_id);
        free(proplist.props[i].revision_id);
        free(proplist.props[i].version);
        free(proplist.props[i].renderer);
    }
    delete [] proplist.props;
}

static void setCurrentRenderer(const char* gpuMode) {
    sCurrentRenderer = emuglConfig_get_renderer(gpuMode);
}

// Checks if the user enforced a specific GPU, it can be done via index or name.
// Otherwise try to find the best device with discrete GPU and high vulkan API
// level. Scoring of the devices is done by some implicit choices based on known
// driver quality, stability and performance issues of current GPUs. Only one
// Vulkan device is selected; this makes things simple for now, but we could
// consider utilizing multiple devices in use cases that make sense.
// TODO: avoid code duplication here and use a shared function or sync device id
// with gfxstream's getSelectedGpuIndex() function
struct DeviceSupportInfo {
    VkPhysicalDeviceProperties physdevProps;
    bool hasGraphicsQueueFamily;
};

int getSelectedGpuIndex(const std::vector<DeviceSupportInfo>& deviceInfos) {
    const int physdevCount = deviceInfos.size();
    if (physdevCount == 1) {
        return 0;
    }

    const char* EnvVarSelectGpu = "ANDROID_EMU_VK_SELECT_GPU";
    std::string enforcedGpuStr = android::base::getEnvironmentVariable(EnvVarSelectGpu);
    int enforceGpuIndex = -1;
    if (enforcedGpuStr.size()) {
        INFO("%s is set to %s", EnvVarSelectGpu, enforcedGpuStr.c_str());

        if (enforcedGpuStr[0] == '0') {
            enforceGpuIndex = 0;
        } else {
            enforceGpuIndex = (atoi(enforcedGpuStr.c_str()));
            if (enforceGpuIndex == 0) {
                // Could not convert to an integer, try searching with device name
                // Do the comparison case insensitive as vendor names don't have consistency
                enforceGpuIndex = -1;
                std::transform(enforcedGpuStr.begin(), enforcedGpuStr.end(), enforcedGpuStr.begin(),
                               [](unsigned char c) { return std::tolower(c); });

                for (int i = 0; i < physdevCount; ++i) {
                    std::string deviceName = std::string(deviceInfos[i].physdevProps.deviceName);
                    std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    INFO("Physical device [%d] = %s", i, deviceName.c_str());

                    if (deviceName.find(enforcedGpuStr) != std::string::npos) {
                        enforceGpuIndex = i;
                    }
                }
            }
        }

        if (enforceGpuIndex != -1 && enforceGpuIndex >= 0 && enforceGpuIndex < deviceInfos.size()) {
            INFO("Selecting GPU (%s) at index %d.",
                 deviceInfos[enforceGpuIndex].physdevProps.deviceName, enforceGpuIndex);
        } else {
            WARN("Could not select the GPU with ANDROID_EMU_VK_GPU_SELECT.");
            enforceGpuIndex = -1;
        }
    }

    if (enforceGpuIndex != -1) {
        return enforceGpuIndex;
    }

    // If there are multiple devices, and none of them are enforced to use,
    // score each device and select the best
    int selectedGpuIndex = 0;
    auto getDeviceScore = [](const DeviceSupportInfo& deviceInfo) {
        uint32_t deviceScore = 0;
        if (!deviceInfo.hasGraphicsQueueFamily) {
            // Not supporting graphics, cannot be used.
            return deviceScore;
        }

        // Matches the ordering in VkPhysicalDeviceType
        const uint32_t deviceTypeScoreTable[] = {
            100,   // VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
            1000,  // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
            2000,  // VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
            500,   // VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
            600,   // VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
        };

        // Prefer discrete GPUs, then integrated and then others..
        const int deviceType = deviceInfo.physdevProps.deviceType;
        deviceScore += deviceTypeScoreTable[deviceInfo.physdevProps.deviceType];

        // Prefer higher level of Vulkan API support, restrict version numbers to
        // common limits to ensure an always increasing scoring change
        const uint32_t major = VK_API_VERSION_MAJOR(deviceInfo.physdevProps.apiVersion);
        const uint32_t minor = VK_API_VERSION_MINOR(deviceInfo.physdevProps.apiVersion);
        const uint32_t patch = VK_API_VERSION_PATCH(deviceInfo.physdevProps.apiVersion);
        deviceScore += major * 5000 + std::min(minor, 10u) * 500 + std::min(patch, 400u);

        return deviceScore;
    };

    uint32_t maxScore = 0;
    for (int i = 0; i < physdevCount; ++i) {
        const uint32_t score = getDeviceScore(deviceInfos[i]);
        if (score > maxScore) {
            selectedGpuIndex = i;
            maxScore = score;
        }
    }

    return selectedGpuIndex;
}

static char* sVkVendor = nullptr;
static int sVkVersionMajor = 0;
static int sVkVersionMinor = 0;
static int sVkVersionPatch = 0;

void emuglConfig_get_vulkan_hardware_gpu(char** vendor,
                                         int* major,
                                         int* minor,
                                         int* patch) {
    if (!vendor || !major || !minor || !patch) {
        return;
    }
    if (sVkVendor) {
        *vendor = strdup(sVkVendor);
        *major = sVkVersionMajor;
        *minor = sVkVersionMinor;
        *patch = sVkVersionPatch;
        return;
    }
#if defined(_WIN32)
    const char* mylibname = "vulkan-1.dll";
#elif defined(__linux__)
    const char* mylibname = "libvulkan.so";
#elif defined(__APPLE__)
    const char* mylibname = "libvulkan.dylib";
#endif

#if defined(_WIN32)
    HMODULE library = LoadLibraryA(mylibname);
    if (!library) {
        dwarning("%s: cannot open vulkan lib %s\n", __func__, mylibname);
        return;
    }
    #define GET_VK_PROC(lib, name) reinterpret_cast<PFN_##name>(GetProcAddress(lib, #name));
#else
    const auto launcherDir =
            android::base::System::get()->getLauncherDirectory();
    const auto strlibname = android::base::PathUtils::join(launcherDir, "lib64",
                                                           "vulkan", mylibname);
    const char* fulllibname = strlibname.c_str();
    auto library = dlopen(fulllibname, RTLD_NOW);
    if (!library) {
        dwarning("%s: failed to open %s", __func__, fulllibname);
        return;
    } else {
        dprint("%s: successfully opened %s", __func__, fulllibname);
    }
    #define GET_VK_PROC(lib, name) reinterpret_cast<PFN_##name>(dlsym(lib, #name));
#endif

    auto* pvkCreateInstance = GET_VK_PROC(library, vkCreateInstance);
    auto* pvkEnumeratePhysicalDevices = GET_VK_PROC(library, vkEnumeratePhysicalDevices);
    auto* pvkGetPhysicalDeviceProperties = GET_VK_PROC(library, vkGetPhysicalDeviceProperties);
    auto* pvkGetPhysicalDeviceQueueFamilyProperties = GET_VK_PROC(library, vkGetPhysicalDeviceQueueFamilyProperties);
    auto* pvkDestroyInstance = GET_VK_PROC(library, vkDestroyInstance);
#undef GET_VK_PROC

    if (!pvkCreateInstance || !pvkEnumeratePhysicalDevices ||
        !pvkGetPhysicalDeviceProperties || !pvkDestroyInstance) {
        derror("failed to find vkCreateInstance vkEnumeratePhysicalDevices "
               "vkGetPhysicalDeviceProperties");
        return;
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DetectGpuInfo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "test_engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extNames;

#if defined(__APPLE__)
    // MoltenVK requires portability enumeratiion and extension enabled
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    extNames.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount = (uint32_t)extNames.size();
    createInfo.ppEnabledExtensionNames = extNames.data();

    VkInstance instance;

    VkResult result = pvkCreateInstance(&createInfo, 0, &instance);

    if (result != VK_SUCCESS) {
        derror("%s: Failed to create vulkan instance error code: %d\n",
                 __func__, result);
        return;
    }
    dprint("%s: Successfully created vulkan instance\n", __func__);

    uint32_t deviceCount = 0;
    result = pvkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS) {
        pvkDestroyInstance(instance, nullptr);
        derror("%s: Failed to query physical devices count %d\n", __func__,
                 result);
        return;
    }
    dprint("%s: Physical devices count is %d\n", __func__,
               (int)(deviceCount));

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result =
            pvkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    if (result != VK_SUCCESS) {
        pvkDestroyInstance(instance, nullptr);
        derror("%s: Failed to query physical devices %d\n", __func__, result);
        return;
    }

    std::vector<DeviceSupportInfo> deviceInfos(deviceCount);
    for (int i = 0; i < deviceCount; i++) {
        pvkGetPhysicalDeviceProperties(devices[i],
                                       &deviceInfos[i].physdevProps);

        deviceInfos[i].hasGraphicsQueueFamily = false;
        {
            uint32_t queueFamilyCount = 0;
            pvkGetPhysicalDeviceQueueFamilyProperties(
                    devices[i], &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilyProps(
                    queueFamilyCount);
            pvkGetPhysicalDeviceQueueFamilyProperties(
                    devices[i], &queueFamilyCount, queueFamilyProps.data());

            for (uint32_t j = 0; j < queueFamilyCount; ++j) {
                auto count = queueFamilyProps[j].queueCount;
                auto flags = queueFamilyProps[j].queueFlags;
                if (count > 0 && (flags & VK_QUEUE_GRAPHICS_BIT)) {
                    deviceInfos[i].hasGraphicsQueueFamily = true;
                    break;
                }
            }
        }
    }

    uint32_t selectedGpuIndex = getSelectedGpuIndex(deviceInfos);

    const auto& physicalProp  =deviceInfos[selectedGpuIndex].physdevProps;
    *vendor = strdup(physicalProp.deviceName);
    *major = VK_API_VERSION_MAJOR(physicalProp.apiVersion);
    *minor = VK_API_VERSION_MINOR(physicalProp.apiVersion);
    *patch = VK_API_VERSION_PATCH(physicalProp.apiVersion);

    if (*vendor) {
        dinfo("%s: Using gpu '%s' for vulkan support\n", __func__, *vendor);
        sVkVendor = strdup(*vendor);
        sVkVersionMajor = *major;
        sVkVersionMinor = *minor;
        sVkVersionPatch = *patch;
    }
    pvkDestroyInstance(instance, nullptr);
}

bool emuglConfig_init(EmuglConfig* config,
                      bool gpu_enabled,
                      const char* gpu_mode,
                      const char* gpu_option,
                      int bitness,
                      bool no_window,
                      bool blacklisted,
                      bool has_guest_renderer,
                      int uiPreferredBackend,
                      bool use_host_vulkan) {
    D("%s: blacklisted=%d has_guest_renderer=%d, mode: %s, option: %s\n",
      __FUNCTION__,
      blacklisted,
      has_guest_renderer,
      gpu_mode, gpu_option);

    // zero all fields first.
    memset(config, 0, sizeof(*config));

    bool host_set_in_hwconfig = false;
    bool has_auto_no_window = false;

    bool hasUiPreference = (enum WinsysPreferredGlesBackend)uiPreferredBackend != WINSYS_GLESBACKEND_PREFERENCE_AUTO;

    // The value of '-gpu <mode>' overrides both the hardware properties
    // and the UI setting, except if <mode> is 'auto'.
    if (gpu_option) {
        sGpuOption = gpu_option;
        if (!strcmp(gpu_option, "on") || !strcmp(gpu_option, "enable")) {
            gpu_enabled = true;
            if (!gpu_mode || !strcmp(gpu_mode, "auto")) {
                gpu_mode = "host";
            }
        } else if (!strcmp(gpu_option, "off") ||
                   !strcmp(gpu_option, "disable") ||
                   !strcmp(gpu_option, "guest")) {
            gpu_mode = gpu_option;
            gpu_enabled = false;
        } else if (!strcmp(gpu_option, "auto")){
            // Nothing to do, use gpu_mode set from
            // hardware properties instead.
        } else if  (!strcmp(gpu_option, "auto-no-window")) {
            // Nothing to do, use gpu_mode set from
            // hardware properties instead.
            has_auto_no_window = true;
        } else {
            gpu_enabled = true;
            gpu_mode = gpu_option;
        }
    } else {
        // Support "hw.gpu.mode=on" in config.ini
        if (gpu_enabled && gpu_mode && (
            !strcmp(gpu_mode, "on") ||
            !strcmp(gpu_mode, "enable") ||
            !strcmp(gpu_mode, "host"))) {
            gpu_enabled = true;
            gpu_mode = "host";
            host_set_in_hwconfig = true;
        }
        sGpuOption = gpu_mode;
    }

    if (gpu_mode &&
        (!strcmp(gpu_mode, "guest") ||
         !strcmp(gpu_mode, "off"))) {
        gpu_enabled = false;
    }

    if (!gpu_option && hasUiPreference) {
        gpu_enabled = true;
        gpu_mode = "auto";
    }

    if (!gpu_enabled) {
        config->enabled = false;
        snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
        snprintf(config->status, sizeof(config->status),
                 "GPU emulation is disabled");
        setCurrentRenderer(gpu_mode);
        return true;
    }

    if (!strcmp("swiftshader", gpu_mode)) {
        gpu_mode = "swiftshader_indirect";
    }

    if (!strcmp("swangle", gpu_mode)) {
        gpu_mode = "swangle_indirect";
    }

    if (!bitness) {
        bitness = System::get()->getProgramBitness();
    }

#if defined(__APPLE__) && defined(__arm64__)
    if (!strcmp("host", gpu_mode)) {
        use_host_vulkan = true;
    }
#endif

    config->bitness = bitness;
    resetBackendList(bitness);

    // For GPU mode in software rendering:
    // On Mac, both swiftshader and swangle will redirect to swangle.
    // On Linux, swiftshader will go to the old swiftshader, while swangle will
    // go to swangle.
    // On Windows, swiftshader will go to the old swiftshader, swangle is not
    // supported yet.
    bool force_swiftshader_to_swangle = false;
    const char* swiftshader_backend_name = "swiftshader";
    const char* swangle_backend_name = "angle";
#ifdef __APPLE__
    force_swiftshader_to_swangle = true;
#endif
#ifdef _WIN32
    swangle_backend_name = nullptr;
#endif

    // Check that the GPU mode is a valid value. 'auto' means determine
    // the best mode depending on the environment. Its purpose is to
    // enable 'swiftshader' mode automatically when NX or Chrome Remote Desktop
    // is detected.
    if (!strcmp(gpu_mode, "auto") || host_set_in_hwconfig) {
        // The default will be 'host' unless:
        // 1. NX or Chrome Remote Desktop is detected, or |no_window| is true.
        // 2. The user's host GPU is on the blacklist.
        std::string sessionType;
        if (System::get()->isRemoteSession(&sessionType)) {
            D("%s: %s session detected\n", __FUNCTION__, sessionType.c_str());
            if (swangle_backend_name &&
                    sBackendList->contains(swangle_backend_name)) {
                D("%s: 'swangle_indirect' mode auto-selected\n", __FUNCTION__);
                gpu_mode = "swangle_indirect";
            } else if (sBackendList->contains(swiftshader_backend_name)) {
                D("%s: 'swiftshader_indirect' mode auto-selected\n", __FUNCTION__);
                gpu_mode = "swiftshader_indirect";
            } else {
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled under %s without Swiftshader",
                        sessionType.c_str());
                setCurrentRenderer(gpu_mode);
                return true;
            }
            setCurrentRenderer(gpu_mode);
        }
        else if ((!has_auto_no_window && no_window) ||
                (blacklisted && !hasUiPreference)) {
            if (swangle_backend_name && stringVectorContains(sBackendList->names(),
                    swangle_backend_name)) {
                D("%s: Headless mode or blacklisted GPU driver, "
                  "using SwANGLE backend\n",
                  __FUNCTION__);
                gpu_mode = "swangle_indirect";
            } else if (stringVectorContains(sBackendList->names(),
                    swiftshader_backend_name)) {
                D("%s: Headless mode or blacklisted GPU driver, "
                  "using Swiftshader backend\n",
                  __FUNCTION__);
                gpu_mode = "swiftshader_indirect";
            } else if (!has_guest_renderer) {
                D("%s: Headless (-no-window) mode (or blacklisted GPU driver)"
                  " without Swiftshader, forcing '-gpu off'\n",
                  __FUNCTION__);
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is disabled (-no-window without Swiftshader)");
                setCurrentRenderer(gpu_mode);
                return true;
            } else {
                D("%s: Headless (-no-window) mode (or blacklisted GPU driver)"
                  ", using guest GPU backend\n",
                  __FUNCTION__);
                config->enabled = false;
                gpu_mode = "off";
                snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
                snprintf(config->status, sizeof(config->status),
                        "GPU emulation is in the guest");
                gpu_mode = "guest";
                setCurrentRenderer(gpu_mode);
                return true;
            }
        } else {
            switch (uiPreferredBackend) {
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE:
                    gpu_mode = "angle_indirect";
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_ANGLE9:
                    gpu_mode = "angle_indirect";
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_SWIFTSHADER:
                    if (force_swiftshader_to_swangle) {
                        gpu_mode = "swangle_indirect";
                    } else {
                        gpu_mode = "swiftshader_indirect";
                    }
                    break;
                case WINSYS_GLESBACKEND_PREFERENCE_NATIVEGL:
                    gpu_mode = "host";
#if defined(__APPLE__) && defined(__arm64__)
                    use_host_vulkan = true;
#endif
                    break;
                default:
                    gpu_mode = "host";
                    break;
            }
            D("%s: auto-selected %s based on conditions and UI preference %d\n",
              __func__, gpu_mode, uiPreferredBackend);
        }
    }

    config->use_host_vulkan = use_host_vulkan;

    // b/328275986: Turn off ANGLE because it breaks.
    bool force_swiftshader = (!strcmp("angle", gpu_mode) ||
                              !strcmp("angle_indirect", gpu_mode) ||
                              !strcmp("angle9", gpu_mode) ||
                              !strcmp("angle9_indirect", gpu_mode));
#ifdef _WIN32
    // Also turn off swangle_indirect mode on Windows
    force_swiftshader =
            force_swiftshader || (!strcmp("swangle_indirect", gpu_mode));
#endif
    if (force_swiftshader) {
        gpu_mode = "swiftshader_indirect";
    }

    const char* library_mode = gpu_mode;
    printf("library_mode %s gpu mode %s\n", library_mode, gpu_mode);
    if ((force_swiftshader_to_swangle && strstr(library_mode, "swiftshader"))
            || strstr(library_mode, "swangle")) {
        library_mode = "angle";
    }

    // 'host' is a special value corresponding to the default translation
    // to desktop GL, 'guest' does not use host-side emulation,
    // anything else must be checked against existing host-side backends.
    if (strcmp(gpu_mode, "host") != 0 && strcmp(gpu_mode, "guest") != 0) {
        const std::vector<std::string>& backends = sBackendList->names();
        if (!stringVectorContains(backends, library_mode)) {
            std::string error = StringFormat(
                "Invalid GPU mode '%s', use one of: host swiftshader_indirect. "
                "If you're already using one of those modes, "
                "the emulator installation may be corrupt. "
                "Please re-install the emulator.", gpu_mode);

            for (size_t n = 0; n < backends.size(); ++n) {
                error += " ";
                error += backends[n];
            }

            D("%s: Error: [%s]\n", __func__, error.c_str());
            derror("%s: %s", __func__, error);

            config->enabled = false;
            gpu_mode = "error";
            snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
            snprintf(config->status, sizeof(config->status), "%s",
                     error.c_str());
            setCurrentRenderer(gpu_mode);
            return false;
        }
    }

    if (strcmp(gpu_mode, "guest")) {
        config->enabled = true;
    }

    snprintf(config->backend, sizeof(config->backend), "%s", gpu_mode);
    snprintf(config->status, sizeof(config->status),
             "GPU emulation enabled using '%s' mode", gpu_mode);
    setCurrentRenderer(gpu_mode);

#if defined(__linux__)
    // todo: add the amd/intel gpu quirks
    if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
        char* vkVendor = nullptr;
        int vkMajor = 0;
        int vkMinor = 0;
        int vkPatch = 0;
        // bug: 324086743
        // we have to enable VulkanAllocateDeviceMemoryOnly
        // to work around the kvm+amdgpu driver bug
        // where kvm apparently error out with Bad Address
        emuglConfig_get_vulkan_hardware_gpu(&vkVendor, &vkMajor, &vkMinor,
                                            &vkPatch);
        bool isAMD = (vkVendor && strncmp("AMD", vkVendor, 3) == 0);
        bool isIntel = (vkVendor && strncmp("Intel", vkVendor, 5) == 0);
        if (isAMD) {
            feature_set_if_not_overridden(
                    kFeature_VulkanAllocateDeviceMemoryOnly, true);
            if (fc::isEnabled(fc::VulkanAllocateDeviceMemoryOnly)) {
                dinfo("Enabled VulkanAllocateDeviceMemoryOnly feature for "
                      "gpu "
                      "vendor %s on Linux\n",
                      vkVendor);
            }
        }
        if (isIntel || isAMD) {
            feature_set_if_not_overridden(kFeature_VulkanAllocateHostMemory,
                                          true);
            if (fc::isEnabled(fc::VulkanAllocateHostMemory)) {
                dinfo("Enabled VulkanAllocateHostMemory feature for "
                      "gpu "
                      "vendor %s on Linux\n",
                      vkVendor);
            }
        }

        if (vkVendor) {
            free(vkVendor);
        }
    }
#endif

    D("%s: %s\n", __func__, config->status);
    return true;
}

void emuglConfig_setupEnv(const EmuglConfig* config) {
    System* system = System::get();

    if (config->use_host_vulkan) {
#ifdef __APPLE__
        system->envSet("ANDROID_EMU_VK_ICD", "moltenvk");
#else
        system->envSet("ANDROID_EMU_VK_ICD", NULL);
#endif
    } else
#ifndef __APPLE__
    // Default to swiftshader vk on mac
    if  (sCurrentRenderer == SELECTED_RENDERER_SWIFTSHADER_INDIRECT
            || sCurrentRenderer == SELECTED_RENDERER_SWIFTSHADER
            || strstr(config->backend, "swangle"))
#endif
    {
        // Use Swiftshader vk icd if using swiftshader_indirect
        system->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
    }

    if (!config->enabled) {
        // There is no real GPU emulation. As a special case, define
        // SDL_RENDER_DRIVER to 'software' to ensure that the
        // software SDL renderer is being used. This allows one
        // to run with '-gpu off' under NX and Chrome Remote Desktop
        // properly.
        system->envSet("SDL_RENDER_DRIVER", "software");
        return;
    }

    bool use_swangle = strstr(config->backend, "swangle");
#ifdef __APPLE__
    use_swangle |= strstr(config->backend, "swiftshader") != nullptr;
#endif
    // $EXEC_DIR/<lib>/ is already added to the library search path by default,
    // since generic libraries are bundled there. We may need more though:
    resetBackendList(config->bitness);
    if (strcmp(config->backend, "host") != 0) {
        // If the backend is not 'host', we also need to add the
        // backend directory.
        std::string dir = sBackendList->getLibDirPath(
                use_swangle ? "angle" : config->backend);
        if (dir.size()) {
            D("Adding to the library search path: %s\n", dir.c_str());
            system->addLibrarySearchDir(dir);
        }
    }

    if (!strcmp(config->backend, "host")) {
        // Nothing more to do for the 'host' backend.
        return;
    }

    // Set ANGLE backend. This has no effect when not using ANGLE.
#if defined(__APPLE__)
    if (strstr(config->backend, "angle")) {
        system->envSet("ANGLE_DEFAULT_PLATFORM", "metal");
    }
#elif defined(__linux__)
    if (strstr(config->backend, "angle")) {
        system->envSet("ANGLE_DEFAULT_PLATFORM", "vulkan");
    }
#endif
    if (use_swangle) {
        system->envSet("ANGLE_DEFAULT_PLATFORM", "swiftshader");
    }

    if (!strcmp(config->backend, "angle_indirect")
            || !strcmp(config->backend, "swiftshader_indirect")
            || !strcmp(config->backend, "swangle_indirect")) {
        system->envSet("ANDROID_EGL_ON_EGL", "1");
        return;
    }

    // For now, EmuGL selects its own translation libraries for
    // EGL/GLES libraries, unless the following environment
    // variables are defined:
    //    ANDROID_EGL_LIB
    //    ANDROID_GLESv1_LIB
    //    ANDROID_GLESv2_LIB
    //
    // If a backend provides one of these libraries, use it.
    std::string lib;
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_EGL, &lib)) {
        system->envSet("ANDROID_EGL_LIB", lib);
    }
    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv1, &lib)) {
        system->envSet("ANDROID_GLESv1_LIB", lib);
    } else if (strcmp(config->backend, "mesa")) {
        derror("OpenGL backend '%s' without OpenGL ES 1.x library detected. "
                        "Using GLESv2 only.",
                        config->backend);
        // A GLESv1 lib is optional---we can deal with a GLESv2 only
        // backend by using CoreProfileEngine in the Translator.
    }

    if (sBackendList->getBackendLibPath(
            config->backend, EmuglBackendList::LIBRARY_GLESv2, &lib)) {
        system->envSet("ANDROID_GLESv2_LIB", lib);
    }

    if (!strcmp(config->backend, "mesa")) {
        dwarning("The Mesa software renderer is deprecated. ")
        dwarning("Use Swiftshader (-gpu swiftshader) for software rendering.");
        system->envSet("ANDROID_GL_LIB", "mesa");
        system->envSet("ANDROID_GL_SOFTWARE_RENDERER", "1");
    }
}
