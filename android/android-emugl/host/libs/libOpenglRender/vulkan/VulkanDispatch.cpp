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
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"

#include "emugl/common/shared_library.h"

using android::base::LazyInstance;
using android::base::pj;
using android::base::System;

namespace emugl {

static void setIcdPath(const std::string& path) {
    System::get()->envSet("VK_ICD_FILENAMES", path);
}

static void initIcdPaths(bool forTesting) {
    if (forTesting) {
        auto res = pj(System::get()->getProgramDirectory(), "testlib64");
        setIcdPath(pj(res, "VkICD_mock_icd.json"));
    } else {
        // Assume there is a system loader in %PATH% or
        // %LD_LIBRARY_PATH%.
        auto icd = System::get()->envGet("ANDROID_EMU_VK_ICD");

        // Mac: Use gfx-rs libportability-icd by default,
        // and switch between that, its debug variant,
        // and MoltenVK depending on the environment variable setting.
#ifdef __APPLE__
        if (icd == "moltenvk") {
            setIcdPath(pj(System::get()->getProgramDirectory(), "lib64",
                          "vulkan", "MoltenVK_icd.json"));
        } else if (icd == "portability") {
            setIcdPath(pj(System::get()->getProgramDirectory(), "lib64",
                          "vulkan", "portability-macos.json"));
        } else if (icd == "portability-debug") {
            setIcdPath(pj(System::get()->getProgramDirectory(), "lib64",
                          "vulkan", "portability-macos-debug.json"));
        } else {
            setIcdPath(pj(System::get()->getProgramDirectory(), "lib64",
                          "vulkan", "portability-macos.json"));
        }
        // TODO: Once Swiftshader is working, set the ICD accordingly.
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
        return pj(System::get()->getProgramDirectory(), "testlib64",
                  VULKAN_LOADER_FILENAME);
    } else {

#ifdef __APPLE__
        // on Mac, the loader isn't in the system libraries.
        return pj(System::get()->getProgramDirectory(), "lib64", "vulkan",
                  VULKAN_LOADER_FILENAME);
#else
        return VULKAN_LOADER_FILENAME;
#endif
    }
}

class VulkanDispatchImpl {
public:
    VulkanDispatchImpl() = default;

    void setForTesting(bool forTesting) {
        mForTesting = forTesting;
    }

    void initialize();

    void* dlopen() {
        if (!mVulkanLoader) {
            auto loaderPath = getLoaderPath(mForTesting);
            mVulkanLoader = emugl::SharedLibrary::open(loaderPath.c_str());
        }
        return (void*)mVulkanLoader;
    }

    void* dlsym(void* lib, const char* name) {
        return (void*)((emugl::SharedLibrary*)(lib))->findSymbol(name);
    }

    VulkanDispatch* dispatch() { return &mDispatch; }

private:
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

void VulkanDispatchImpl::initialize() {
    if (mInitialized) return;

    initIcdPaths(mForTesting);

    goldfish_vk::init_vulkan_dispatch_from_system_loader(
            sVulkanDispatchDlOpen, 
            sVulkanDispatchDlSym,
            &mDispatch);

    mInitialized = true;
}

VulkanDispatch* vkDispatch(bool forTesting) {
    sDispatchImpl->setForTesting(forTesting);
    sDispatchImpl->initialize();
    return sDispatchImpl->dispatch();
}

}
