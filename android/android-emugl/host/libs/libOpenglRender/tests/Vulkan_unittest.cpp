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

#include <gtest/gtest.h>

#include <vulkan/vulkan.h>

#include "VulkanDispatch.h"

#include <sstream>
#include <string>

#ifdef _WIN32

#include <windows.h>

#else

#include <dlfcn.h>

#endif

namespace emugl {

static void* dlOpenFuncForTesting() {
#ifdef _WIN32
    return LoadLibraryW(L"vulkan-1.dll");
#else

#ifdef __APPLE__
    return dlopen("libvulkan.1.dylib", RTLD_NOW);
#else
    return dlopen("libvulkan.so", RTLD_NOW);
#endif

#endif
}

static void* dlSymFuncForTesting(void* lib, const char* sym) {
#ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)lib, sym);
#else
    return dlsym(lib, sym);
#endif
}

static std::string deviceTypeToString(VkPhysicalDeviceType type) {
#define DO_ENUM_RETURN_STRING(e) \
    case e: \
        return #e; \

    switch (type) {
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_OTHER)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_CPU)
        default:
            return "Unknown";
    }
}

static std::string queueFlagsToString(VkQueueFlags queueFlags) {
    std::stringstream ss;

    if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        ss << "VK_QUEUE_GRAPHICS_BIT | ";
    }
    if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
        ss << "VK_QUEUE_COMPUTE_BIT | ";
    }
    if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
        ss << "VK_QUEUE_TRANSFER_BIT | ";
    }
    if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        ss << "VK_QUEUE_SPARSE_BINDING_BIT | ";
    }
    if (queueFlags & VK_QUEUE_PROTECTED_BIT) {
        ss << "VK_QUEUE_PROTECTED_BIT";
    }

    return ss.str();
}

static std::string getPhysicalDevicePropertiesString(
    const VkPhysicalDeviceProperties& props) {

    std::stringstream ss;

    uint16_t apiMaj = (uint16_t)(props.apiVersion >> 22);
    uint16_t apiMin = (uint16_t)(0x003fffff & (props.apiVersion >> 12));
    uint16_t apiPatch = (uint16_t)(0x000007ff & (props.apiVersion));

    ss << "API version: " << apiMaj << "." << apiMin << "." << apiPatch << "\n";
    ss << "Driver version: " << std::hex << props.driverVersion << "\n";
    ss << "Vendor ID: " << std::hex << props.vendorID << "\n";
    ss << "Device ID: " << std::hex << props.deviceID << "\n";
    ss << "Device type: " << deviceTypeToString(props.deviceType) << "\n";
    ss << "Device name: " << props.deviceName << "\n";

    return ss.str();
}

static bool testInstanceGetter(const VulkanDispatch* vk, VkInstance* instance_out) {

    EXPECT_TRUE(vk->vkEnumerateInstanceExtensionProperties);
    EXPECT_TRUE(vk->vkCreateInstance);
    EXPECT_TRUE(vk->vkEnumeratePhysicalDevices);
    EXPECT_TRUE(vk->vkGetPhysicalDeviceProperties);
    EXPECT_TRUE(vk->vkGetPhysicalDeviceQueueFamilyProperties);

    uint32_t count;
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    fprintf(stderr, "%s: exts: %u\n", __func__, count);

    std::vector<VkExtensionProperties> props(count);
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

    for (uint32_t i = 0; i < count; i++) {
        fprintf(stderr, "%s: ext: %s\n", __func__, props[i].extensionName);
    }

    VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        0, nullptr,
    };

    VkInstance instance;

    EXPECT_EQ(VK_SUCCESS,
        vk->vkCreateInstance(
            &instanceCreateInfo, nullptr, &instance));

    uint32_t physicalDeviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;

    EXPECT_EQ(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, nullptr));

    physicalDevices.resize(physicalDeviceCount);

    EXPECT_EQ(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, physicalDevices.data()));

    std::vector<VkPhysicalDeviceProperties> physicalDeviceProps(physicalDeviceCount);

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        vk->vkGetPhysicalDeviceProperties(
            physicalDevices[i],
            physicalDeviceProps.data() + i);

        auto str = getPhysicalDevicePropertiesString(physicalDeviceProps[i]);
        fprintf(stderr, "%s\n", str.c_str());

        uint32_t queueFamilyCount;
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilies.data());

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueCount > 0) {
                auto flagsAsString =
                    queueFlagsToString(queueFamilies[j].queueFlags);

                fprintf(stderr, "%s: found %u @ family %u with caps: %s\n",
                        __func__,
                        queueFamilies[j].queueCount, j,
                        flagsAsString.c_str());
            }
        }
    }


    return false;
}

static bool testDeviceGetter(const VulkanDispatch* dispatch,
                             VkInstance instance,
                             VkPhysicalDevice* physDevice_out,
                             VkDevice* device_out) {
    return false;
}

TEST(Vulkan, Dispatch) {

    VulkanDispatch dispatch;
    goldfish_vk::init_vulkan_dispatch_from_system_loader(
        dlOpenFuncForTesting, dlSymFuncForTesting, testInstanceGetter, testDeviceGetter, &dispatch);

}

} // namespace emugl
