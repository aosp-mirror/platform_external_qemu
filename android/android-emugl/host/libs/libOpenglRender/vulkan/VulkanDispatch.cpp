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

static const char* getTestIcdFilename() {
#if defined(__APPLE__)
    return "libvk_swiftshader.dylib";
#elif defined(__linux__)
    return "libvk_swiftshader.so";
#elif defined(_WIN32) || defined(_MSC_VER)
    return "vk_swiftshader.dll";
#else
#error Host operating system not supported
#endif
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
                         << "Swiftshader ICD " << getTestIcdFilename();
            auto libPath = pj(System::get()->getProgramDirectory(), "lib64", "vulkan",
                              getTestIcdFilename());
            if (path_exists(libPath.c_str())) {
                LOG(VERBOSE) << "Swiftshader library exists";
            } else {
                LOG(VERBOSE) << "Swiftshader library doesn't exist, trying launcher path";
                libPath = pj(System::get()->getLauncherDirectory(), "lib64", "vulkan",
                             getTestIcdFilename());
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
            // Mac: Use MoltenVK by default unless GPU mode is set to
            // swiftshader.
#ifdef __APPLE__
            if (androidIcd == "swiftshader" ||
                emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER ||
                emugl::getRenderer() ==
                        SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths(
                        "vk_swiftshader_icd.json"));
                System::get()->envSet("ANDROID_EMU_VK_ICD", "swiftshader");
            } else {
                setIcdPath(icdJsonNameToProgramAndLauncherPaths(
                        "MoltenVK_icd.json"));
                System::get()->envSet("ANDROID_EMU_VK_ICD", "moltenvk");
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
        auto path = pj(directory, "lib64", "vulkan", VULKAN_LOADER_FILENAME);
        LOG(VERBOSE) << "Not in test environment. Using loader: " << path;
        return path;
#endif
    }
}

static std::string getMoltenVkPath(const std::string& directory, bool forTesting) {
#ifdef __APPLE__
    auto path = System::get()->envGet("ANDROID_EMU_VK_LOADER_PATH");
    if (!path.empty()) {
        return path;
    }
    auto androidIcd = System::get()->envGet("ANDROID_EMU_VK_ICD");

    // Skip loader when using MoltenVK as this gives us access to
    // VK_MVK_moltenvk, which is required for external memory support.
    if (!forTesting && androidIcd == "moltenvk") {
        auto path = pj(directory, "lib64", "vulkan", "libMoltenVK.dylib");
        LOG(VERBOSE) << "Skipping loader and using ICD directly: " << path;
        return path;
    }

#endif
    return "";
}

class SharedLibraries {
public:
    explicit SharedLibraries(size_t sizeLimit = 1): mSizeLimit(sizeLimit) {}

    size_t size() const { return mLibs.size(); }

    bool addLibrary(const std::string& path) {
        if (size() >= mSizeLimit) {
            dwarning("cannot add library %s: full", path.c_str());
            return false;
        }

        auto library = emugl::SharedLibrary::open(path.c_str());
        if (library) {
            mLibs.push_back(library);
            dinfo("added library %s", path.c_str());
            return true;
        } else {
            dwarning("cannot add library %s: failed", path.c_str());
            return false;
        }
    }

    ~SharedLibraries() = default;

    void* dlsym(const char* name) {
        for (const auto& lib : mLibs) {
            void* funcPtr = reinterpret_cast<void*>(lib->findSymbol(name));
            if (funcPtr) {
                return funcPtr;
            }
        }
        return nullptr;
    }

private:
    size_t mSizeLimit;
    std::vector<emugl::SharedLibrary*> mLibs;
};

static constexpr size_t getVulkanLibraryNumLimits() {
// macOS may have both Vulkan loader (for non MoltenVK-specific functions) and
// MoltenVK library (only for MoltenVK-specific vk...MVK functions) loaded at
// the same time. So there could be at most 2 libraries loaded. On other systems
// only one Vulkan loader is allowed.
#ifdef __APPLE__
    return 2;
#else
    return 1;
#endif
}

class VulkanDispatchImpl {
public:
    VulkanDispatchImpl(): mVulkanLibs(getVulkanLibraryNumLimits()) {}

    void initialize(bool forTesting);

    void* dlopen() {
        bool sandbox = System::get()->envGet("ANDROID_EMU_SANDBOX") == "1";

        if (mVulkanLibs.size() == 0) {
            if (sandbox) {
                bool success = mVulkanLibs.addLibrary(VULKAN_LOADER_FILENAME);
#ifdef __linux__
                if (!success) {
                    success = mVulkanLibs.addLibrary("libvulkan.so.1");
                }
#endif // __linux__
            } else {
                auto loaderPath = getLoaderPath(System::get()->getProgramDirectory(), mForTesting);
                bool success = mVulkanLibs.addLibrary(loaderPath);

                if (!success) {
                    loaderPath = getLoaderPath(System::get()->getLauncherDirectory(), mForTesting);
                    success = mVulkanLibs.addLibrary(loaderPath);
                }

#ifdef __linux__
                // On Linux, it might not be called libvulkan.so.
                // Try libvulkan.so.1 if that doesn't work.
                if (!success) {
                    loaderPath = pj(System::get()->getLauncherDirectory(), "lib64", "vulkan", "libvulkan.so.1");
                    success = mVulkanLibs.addLibrary(loaderPath);
                }
#endif // __linux__
#ifdef __APPLE__
                // On macOS it is possible that we are using MoltenVK as the
                // ICD. In that case we need to add MoltenVK libraries to
                // mSharedLibs to use MoltenVK-specific functions.
                auto mvkPath = getMoltenVkPath(System::get()->getProgramDirectory(), mForTesting);
                if (!mvkPath.empty()) {
                    success = mVulkanLibs.addLibrary(mvkPath);
                }

                if (!success) {
                    mvkPath = getMoltenVkPath(System::get()->getLauncherDirectory(), mForTesting);
                    if (!mvkPath.empty()) {
                        success = mVulkanLibs.addLibrary(mvkPath);
                    }
                }
#endif // __APPLE__
            }
        }
        return static_cast<void*>(&mVulkanLibs);
    }

    void* dlsym(void* lib, const char* name) {
        return (void*)((emugl::SharedLibraries*)(lib))->dlsym(name);
    }

    VulkanDispatch* dispatch() { return &mDispatch; }

private:
    Lock mLock;
    bool mForTesting = false;
    bool mInitialized = false;
    VulkanDispatch mDispatch;
    SharedLibraries mVulkanLibs;
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
