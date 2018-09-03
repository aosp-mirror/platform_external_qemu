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

#ifdef _WIN32

#include <windows.h>

#else

#include <dlfcn.h>

#endif

namespace emugl {

// export DYLD_LIBRARY_PATH=/Users/lfy/vulkansdk-macos-1.1.82.0/macOS/lib
// export VK_ICD_FILENAMES=/Users/lfy/vulkansdk-macos-1.1.82.0/macOS/etc/vulkan/icd.d/MoltenVK_icd.json

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

static bool testInstanceGetter(const VulkanDispatch* vk, VkInstance* instance_out) {
    if (vk->vkEnumerateInstanceExtensionProperties) {
        fprintf(stderr, "%s: can enumerate instance exts\n", __func__);

        uint32_t count;
        vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        fprintf(stderr, "%s: exts: %u\n", __func__, count);

        std::vector<VkExtensionProperties> props(count);
        vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

        for (uint32_t i = 0; i < count; i++) {
            fprintf(stderr, "%s: ext: %s\n", __func__, props[i].extensionName);
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
