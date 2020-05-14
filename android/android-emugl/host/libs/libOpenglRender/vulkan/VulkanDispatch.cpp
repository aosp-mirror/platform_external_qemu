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

#include "emugl/common/misc.h"
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
    if (System::get()->envGet("ANDROID_EMU_SANDBOX") == "1") {
        // Rely on user to set VK_ICD_FILENAMES
        return;
    } else {
        if (forTesting || androidIcd == "swiftshader") {
            auto res = pj(System::get()->getProgramDirectory(), "lib64", "vulkan");
            LOG(VERBOSE) << "In test environment or ICD set to swiftshader, using "
                            "Swiftshader ICD";
            auto libPath = pj(System::get()->getProgramDirectory(), "lib64", "vulkan", "libvk_swiftshader.so");;
            if (path_exists(libPath.c_str())) {
                LOG(VERBOSE) << "Swiftshader library exists";
            } else {
                LOG(VERBOSE) << "Swiftshader library doesn't exist, trying launcher path";
                libPath = pj(System::get()->getLauncherDirectory(), "lib64", "vulkan", "libvk_swiftshader.so");;
                if (path_exists(libPath.c_str())) {
                    LOG(VERBOSE) << "Swiftshader library found in launcher path";
                } else {
                    LOG(VERBOSE) << "Swiftshader library not found in program nor launcher path";
                }
            }
            setIcdPath(icdJsonNameToProgramAndLauncherPaths("vk_swiftshader_icd.json"));
            System::get()->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
        } else {
            LOG(VERBOSE) << "Not in test environment. ICD (blank for default): ["
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
#define VULKAN_LOADER_FILENAME "libvulkan.so"
#endif

#endif
static std::string getLoaderPath(const std::string& directory, bool forTesting) {
    auto path = System::get()->envGet("ANDROID_EMU_VK_LOADER_PATH");
    if (!path.empty()) {
        return path;
    }
    auto androidIcd = System::get()->envGet("ANDROID_EMU_VK_ICD");
    if (forTesting || androidIcd == "mock") {
        auto path = pj(directory, "testlib64", VULKAN_LOADER_FILENAME);
        LOG(VERBOSE) << "In test environment or using Swiftshader. Using loader: " << path;
        return path;
    } else {
#ifdef _WIN32
        LOG(VERBOSE) << "Not in test environment. Using loader: " << VULKAN_LOADER_FILENAME;
        return VULKAN_LOADER_FILENAME;
#else
#ifdef __APPLE__
        // Skip loader when using MoltenVK as this gives us access to
        // VK_MVK_moltenvk, which is required for external memory support.
        if (androidIcd == "moltenvk") {
            auto path = pj(directory, "lib64", "vulkan", "libMoltenVK.dylib");
            LOG(VERBOSE) << "Skipping loader and using ICD directly: " << path;
            return path;
        }
#endif
        auto path = pj(directory, "lib64", "vulkan", VULKAN_LOADER_FILENAME);
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
        bool sandbox = System::get()->envGet("ANDROID_EMU_SANDBOX") == "1";

        if (!mVulkanLoader) {
            mVulkanLoader = emugl::SharedLibrary::open(VULKAN_LOADER_FILENAME);
        }
#ifdef __linux__
        // On Linux, it might not be called libvulkan.so.
        // Try libvulkan.so.1 if that doesn't work.
        if (!mVulkanLoader) {
            mVulkanLoader =
                emugl::SharedLibrary::open("libvulkan.so.1");
        }
#endif

        if (sandbox) return (void*)mVulkanLoader;

        // If we still don't have a loader and are not in sandbox mode,
        // use the prebuilt loader still.
#ifndef __APPLE__ // We always expect this on macOS
        printf("%s: using prebuilt Vulkan loader\n", __func__);
#endif
        if (!mVulkanLoader) {
            auto loaderPath = getLoaderPath(System::get()->getProgramDirectory(), mForTesting);
            mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());

            if (!mVulkanLoader) {
                loaderPath = getLoaderPath(System::get()->getLauncherDirectory(), mForTesting);
                mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());
            }
        }

        if (!mVulkanLoader) {
            auto altPath = pj(System::get()->getLauncherDirectory(),
                "lib64", "vulkan", "libvulkan.so.1");
            mVulkanLoader =
                emugl::SharedLibrary::open(altPath.c_str());
        }

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
