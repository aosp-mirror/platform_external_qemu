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

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/synchronization/Lock.h"
#include "android/utils/path.h"

#include "emugl/common/shared_library.h"

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::pj;
using android::base::System;

namespace emugl {

static void setIcdPath(const std::string& path) {
    if (path_exists(path.c_str())) {
        LOG(VERBOSE) << "setIcdPath: path exists: " << path;
    } else {
        LOG(VERBOSE) << "setIcdPath: path doesn't exist: " << path;
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
    if (forTesting || androidIcd == "swiftshader") {
        auto res = pj(System::get()->getProgramDirectory(), "lib64", "vulkan");
        LOG(VERBOSE) << "In test environment or ICD set to swiftshader, using "
                        "Swiftshader ICD";
        auto libPath = pj(System::get()->getProgramDirectory(), "lib64", "vulkan", "libvk_swiftshader.so");;
        if (path_exists(libPath.c_str())) {
            LOG(VERBOSE) << "Swiftshader library exists";
        } else {
            LOG(VERBOSE) << "Swiftshader library doesn't";
        }
        setIcdPath(pj(res, "vk_swiftshader_icd.json"));
        System::get()->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
    } else {
        LOG(VERBOSE) << "Not in test environment. ICD (blank for default): ["
                     << androidIcd << "]";
        // Mac: Use gfx-rs libportability-icd by default,
        // and switch between that, its debug variant,
        // and MoltenVK depending on the environment variable setting.
#ifdef __APPLE__
        if (androidIcd == "moltenvk") {
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("MoltenVK_icd.json"));
        } else if (androidIcd == "portability") {
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos.json"));
        } else if (androidIcd == "portability-debug") {
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("portability-macos-debug.json"));
        } else if (androidIcd == "swiftshader") {
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
        } else {
            // go/ab likes swiftshader better
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
        }
#else
        // By default, on other platforms, just use whatever the system
        // is packing.
#endif
    }
}

#ifdef __APPLE__
#define VULKAN_LOADER_FILENAME "libvulkan.dylib"
#else
#ifdef _WIN32
#define VULKAN_LOADER_FILENAME "vulkan-1.dll"
#else
#define VULKAN_LOADER_FILENAME "libvulkan.so"
#endif

#endif
static std::string getLoaderPath(bool forTesting) {
    if (forTesting) {

        auto path = pj(System::get()->getProgramDirectory(), "testlib64",
                  VULKAN_LOADER_FILENAME);
        LOG(VERBOSE) << "In test environment or using Swiftshader. Using loader: " << path;
        return path;
    } else {
#ifdef _WIN32
        LOG(VERBOSE) << "Not in test environment. Using loader: " << VULKAN_LOADER_FILENAME;
        return VULKAN_LOADER_FILENAME;
#else
        auto path = pj(System::get()->getProgramDirectory(), "lib64", "vulkan",
                  VULKAN_LOADER_FILENAME);
        LOG(VERBOSE) << "Not in test environment. Using loader: " << path;
        return path;
#endif
    }
}

static std::string getBackupLoaderPath(bool forTesting) {
    auto androidIcd = System::get()->envGet("ANDROID_EMU_VK_ICD");
    if (forTesting || androidIcd == "mock") {
        return pj(System::get()->getLauncherDirectory(), "testlib64",
                  VULKAN_LOADER_FILENAME);
    } else {

#ifdef _WIN32
        LOG(VERBOSE) << "Not in test environment. Using loader: " << VULKAN_LOADER_FILENAME;
        return VULKAN_LOADER_FILENAME;
#else
        auto path = pj(System::get()->getLauncherDirectory(), "lib64", "vulkan",
                  VULKAN_LOADER_FILENAME);
        LOG(VERBOSE) << "Not in test environment. Using loader: " << path;
        return path;
#endif
    }
}

class VulkanDispatchImpl {
public:
    VulkanDispatchImpl() = default;

    void initialize(bool forTesting);

    void* dlopen() {
        if (!mVulkanLoader) {
            auto loaderPath = getLoaderPath(mForTesting);
            mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());

            if (!mVulkanLoader) {
                loaderPath = getBackupLoaderPath(mForTesting);
                mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());
            }
        }
#ifdef __linux__
        // On Linux, it might not be called libvulkan.so.
        // Try libvulkan.so.1 if that doesn't work.
        if (!mVulkanLoader) {
            auto altPath = pj(System::get()->getLauncherDirectory(),
                "lib64", "vulkan", "libvulkan.so.1");
            mVulkanLoader =
                emugl::SharedLibrary::open(altPath.c_str());
        }
#endif
        return (void*)mVulkanLoader;
    }

    void* dlsym(void* lib, const char* name) {
        return (void*)((emugl::SharedLibrary*)(lib))->findSymbol(name);
    }

    VulkanDispatch* dispatch() { return &mDispatch; }

private:
    Lock mLock;
    bool mForTesting = false;
    bool mInitialized = false;
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

void VulkanDispatchImpl::initialize(bool forTesting) {
    AutoLock lock(mLock);

    if (mInitialized) return;

    mForTesting = forTesting;
    initIcdPaths(mForTesting);

    goldfish_vk::init_vulkan_dispatch_from_system_loader(
            sVulkanDispatchDlOpen,
            sVulkanDispatchDlSym,
            &mDispatch);

    mInitialized = true;
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

}
