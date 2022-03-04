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

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "DecompressionShaders.h"
#include "FrameBuffer.h"
#include "GLcommon/etc.h"
#include "VkAndroidNativeBuffer.h"
#include "VkCommonOperations.h"
#include "VkDecoderSnapshot.h"
#include "VkFormatUtils.h"
#include "VulkanDispatch.h"
#include "VulkanStream.h"
#include "android/base/ArraySize.h"
#include "android/base/Optional.h"
#include "android/base/containers/EntityManager.h"
#include "android/base/containers/HybridEntityManager.h"
#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/Stream.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/Tracing.h"
#include "android/utils/GfxstreamFatalError.h"
#include "common/goldfish_vk_marshaling.h"
#include "common/goldfish_vk_reserved_marshaling.h"
#include "common/goldfish_vk_deepcopy.h"
#include "common/goldfish_vk_dispatch.h"
#include "emugl/common/address_space_device_control_ops.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/feature_control.h"
#include "emugl/common/logging.h"
#include "emugl/common/vm_operations.h"
#include "vk_util.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

using android::base::arraySize;
using android::base::AutoLock;
using android::base::ConditionVariable;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::Optional;
using android::base::pj;
using android::base::System;

#define VKDGS_DEBUG 0

#if VKDGS_DEBUG
#define VKDGS_LOG(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define VKDGS_LOG(fmt,...)
#endif

namespace goldfish_vk {

// A list of extensions that should not be passed to the host driver.
// These will mainly include Vulkan features that we emulate ourselves.
static constexpr const char* const
kEmulatedExtensions[] = {
    "VK_ANDROID_external_memory_android_hardware_buffer",
    "VK_ANDROID_native_buffer",
    "VK_FUCHSIA_buffer_collection",
    "VK_FUCHSIA_external_memory",
    "VK_FUCHSIA_external_semaphore",
    "VK_FUCHSIA_buffer_collection_x",
    "VK_EXT_queue_family_foreign",
    "VK_KHR_external_memory",
    "VK_KHR_external_memory_capabilities",
    "VK_KHR_external_semaphore",
    "VK_KHR_external_semaphore_capabilities",
    "VK_KHR_external_semaphore_fd",
    "VK_KHR_external_semaphore_win32",
    "VK_KHR_external_fence_capabilities",
    "VK_KHR_external_fence",
    "VK_KHR_external_fence_fd",
    "VK_EXT_device_memory_report",
};

static constexpr uint32_t kMaxSafeVersion = VK_MAKE_VERSION(1, 1, 0);
static constexpr uint32_t kMinVersion = VK_MAKE_VERSION(1, 0, 0);

#define DEFINE_BOXED_HANDLE_TYPE_TAG(type) \
    Tag_##type, \

enum BoxedHandleTypeTag {
    Tag_Invalid = 0,
    GOLDFISH_VK_LIST_HANDLE_TYPES_BY_STAGE(DEFINE_BOXED_HANDLE_TYPE_TAG)
};

struct OrderMaintenanceInfo {
     uint32_t sequenceNumber = 0;
     Lock lock;
     ConditionVariable cv;

     uint32_t refcount = 1;

     void incRef() {
         __atomic_add_fetch(&refcount, 1, __ATOMIC_SEQ_CST);
     }

     bool decRef() {
         return 0 == __atomic_sub_fetch(&refcount, 1, __ATOMIC_SEQ_CST);
     }
};

static void acquireOrderMaintInfo(OrderMaintenanceInfo* ord) {
    if (!ord) return;
    ord->incRef();
}

static void releaseOrderMaintInfo(OrderMaintenanceInfo* ord) {
    if (!ord) return;
    if (ord->decRef()) delete ord;
}


template <class T>
class BoxedHandleManager {
public:

    // The hybrid entity manager uses a sequence lock to protect access to
    // a working set of 16000 handles, allowing us to avoid using a regular
    // lock for those. Performance is degraded when going over this number,
    // as it will then fall back to a std::map.
    //
    // We use 16000 as the max number of live handles to track; we don't
    // expect the system to go over 16000 total live handles, outside some
    // dEQP object management tests.
    using Store = android::base::HybridEntityManager<16000, uint64_t, T>;

    Lock lock;
    mutable Store store;
    std::unordered_map<uint64_t, uint64_t> reverseMap;
    struct DelayedRemove {
        uint64_t handle;
        std::function<void()> callback;
    };
    std::unordered_map<VkDevice, std::vector<DelayedRemove>> delayedRemoves;

    void clear() {
        reverseMap.clear();
        store.clear();
    }

    uint64_t add(const T& item, BoxedHandleTypeTag tag) {
        auto res = (uint64_t)store.add(item, (size_t)tag);
        AutoLock l(lock);
        reverseMap[(uint64_t)(item.underlying)] = res;
        return res;
    }

    uint64_t addFixed(uint64_t handle, const T& item, BoxedHandleTypeTag tag) {
        auto res = (uint64_t)store.addFixed(handle, item, (size_t)tag);
        AutoLock l(lock);
        reverseMap[(uint64_t)(item.underlying)] = res;
        return res;
    }

    void remove(uint64_t h) {
        auto item = get(h);
        if (item) {
            AutoLock l(lock);
            reverseMap.erase((uint64_t)(item->underlying));
        }
        store.remove(h);
    }

    void removeDelayed(uint64_t h, VkDevice device, std::function<void()> callback) {
        AutoLock l(lock);
        delayedRemoves[device].push_back({ h, callback });
    }

    void processDelayedRemovesGlobalStateLocked(VkDevice device) {
        AutoLock l(lock);
        auto it = delayedRemoves.find(device);
        if (it == delayedRemoves.end()) return;
        auto& delayedRemovesList = it->second;
        for (const auto& r : delayedRemovesList) {
            auto h = r.handle;
            // VkDecoderGlobalState is already locked when callback is called.
            auto funcGlobalStateLocked = r.callback;
            funcGlobalStateLocked();
            store.remove(h);
        }
        delayedRemovesList.clear();
        delayedRemoves.erase(it);
    }

    T* get(uint64_t h) {
        return (T*)store.get_const(h);
    }

    uint64_t getBoxedFromUnboxedLocked(uint64_t unboxed) {
        auto res = android::base::find(reverseMap, unboxed);
        if (!res) return 0;
        return *res;
    }
};

template <class T>
class DispatchableHandleInfo {
    public:
        T underlying;
        VulkanDispatch* dispatch = nullptr;
        bool ownDispatch = false;
        OrderMaintenanceInfo* ordMaintInfo = nullptr;
        VulkanMemReadingStream* readStream = nullptr;
};

static BoxedHandleManager<DispatchableHandleInfo<uint64_t>> sBoxedHandleManager;

struct ReadStreamRegistry {
     Lock mLock;

     std::vector<VulkanMemReadingStream*> freeStreams;

     ReadStreamRegistry() { freeStreams.reserve(100); };

     VulkanMemReadingStream* pop() {
         AutoLock lock(mLock);
         if (freeStreams.empty()) {
             return new VulkanMemReadingStream(0);
         } else {
            VulkanMemReadingStream* res = freeStreams.back();
            freeStreams.pop_back();
            return res;
         }
     }

     void push(VulkanMemReadingStream* stream) {
         AutoLock lock(mLock);
         freeStreams.push_back(stream);
     }
};

static ReadStreamRegistry sReadStreamRegistry;

class VkDecoderGlobalState::Impl {
public:
    Impl() :
        m_vk(emugl::vkDispatch()),
        m_emu(getGlobalVkEmulation()) {
        mSnapshotsEnabled =
            emugl::emugl_feature_is_enabled(
                android::featurecontrol::VulkanSnapshots);
        mVkCleanupEnabled = System::get()->envGet("ANDROID_EMU_VK_NO_CLEANUP") != "1";
        mLogging = System::get()->envGet("ANDROID_EMU_VK_LOG_CALLS") == "1";
        mVerbosePrints = System::get()->envGet("ANDROID_EMUGL_VERBOSE") == "1";
        if (get_emugl_address_space_device_control_ops().control_get_hw_funcs &&
            get_emugl_address_space_device_control_ops().control_get_hw_funcs()) {
            mUseOldMemoryCleanupPath = 0 == get_emugl_address_space_device_control_ops().control_get_hw_funcs()->getPhysAddrStartLocked();
        }

        if (mVerbosePrints) {
            fprintf(stderr, "%s: memory cleanup path is old? %d\n", __func__, mUseOldMemoryCleanupPath);
            fprintf(stderr, "%s: vk cleanup enabled? %d\n", __func__, mVkCleanupEnabled);
        }

        mGuestUsesAngle =
            emugl::emugl_feature_is_enabled(
                android::featurecontrol::GuestUsesAngle);
    }

    ~Impl() = default;

    // Resets all internal tracking info.
    // Assumes that the heavyweight cleanup operations
    // have already happened.
    void clear() {
        mInstanceInfo.clear();
        mPhysdevInfo.clear();
        mDeviceInfo.clear();
        mImageInfo.clear();
        mImageViewInfo.clear();
        mSamplerInfo.clear();
        mCmdBufferInfo.clear();
        mCmdPoolInfo.clear();

        mDeviceToPhysicalDevice.clear();
        mPhysicalDeviceToInstance.clear();
        mQueueInfo.clear();
        mBufferInfo.clear();
        mMapInfo.clear();
        mSemaphoreInfo.clear();
        mFenceInfo.clear();
#ifdef _WIN32
        mSemaphoreId = 1;
        mExternalSemaphoresById.clear();
#endif
        mDescriptorUpdateTemplateInfo.clear();

        mCreatedHandlesForSnapshotLoad.clear();
        mCreatedHandlesForSnapshotLoadIndex = 0;

        sBoxedHandleManager.clear();
    }

    bool snapshotsEnabled() const {
        return mSnapshotsEnabled;
    }

    bool vkCleanupEnabled() const {
        return mVkCleanupEnabled;
    }

    void save(android::base::Stream* stream) {
        snapshot()->save(stream);
    }

    void load(android::base::Stream* stream) {
        // assume that we already destroyed all instances
        // from FrameBuffer's onLoad method.

        // destroy all current internal data structures
        clear();

        // this part will replay in the decoder
        snapshot()->load(stream);
    }

    void lock() {
        mLock.lock();
    }

    void unlock() {
        mLock.unlock();
    }

    size_t setCreatedHandlesForSnapshotLoad(const unsigned char* buffer) {
        size_t consumed = 0;

        if (!buffer) return consumed;

        uint32_t bufferSize = *(uint32_t*)buffer;

        consumed += 4;

        uint32_t handleCount = bufferSize / 8;
        VKDGS_LOG("incoming handle count: %u", handleCount);

        uint64_t* handles = (uint64_t*)(buffer + 4);

        mCreatedHandlesForSnapshotLoad.clear();
        mCreatedHandlesForSnapshotLoadIndex = 0;

        for (uint32_t i = 0; i < handleCount; ++i) {
            VKDGS_LOG("handle to load: 0x%llx", (unsigned long long)(uintptr_t)handles[i]);
            mCreatedHandlesForSnapshotLoad.push_back(handles[i]);
            consumed += 8;
        }

        return consumed;
    }

    void clearCreatedHandlesForSnapshotLoad() {
        mCreatedHandlesForSnapshotLoad.clear();
        mCreatedHandlesForSnapshotLoadIndex = 0;
    }

    VkResult on_vkEnumerateInstanceVersion(
            android::base::BumpPool* pool,
            uint32_t* pApiVersion) {
        if (m_vk->vkEnumerateInstanceVersion) {
            VkResult res = m_vk->vkEnumerateInstanceVersion(pApiVersion);

            if (*pApiVersion > kMaxSafeVersion) {
                *pApiVersion = kMaxSafeVersion;
            }

            return res;
        }
        *pApiVersion = kMinVersion;
        return VK_SUCCESS;
    }

    VkResult on_vkCreateInstance(
            android::base::BumpPool* pool,
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {

        std::vector<const char*> finalExts =
            filteredExtensionNames(
                    pCreateInfo->enabledExtensionCount,
                    pCreateInfo->ppEnabledExtensionNames);

        // Create higher version instance whenever it is possible.
        uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
        if (pCreateInfo->pApplicationInfo) {
            apiVersion = pCreateInfo->pApplicationInfo->apiVersion;
        }
        if (m_vk->vkEnumerateInstanceVersion) {
            uint32_t instanceVersion;
            VkResult result =
                m_vk->vkEnumerateInstanceVersion(&instanceVersion);
            if (result == VK_SUCCESS &&
                instanceVersion >= VK_MAKE_VERSION(1, 1, 0)) {
                apiVersion = instanceVersion;
            }
        }

        VkInstanceCreateInfo createInfoFiltered;
        deepcopy_VkInstanceCreateInfo(pool,
                                      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                      pCreateInfo, &createInfoFiltered);

        createInfoFiltered.enabledExtensionCount =
                static_cast<uint32_t>(finalExts.size());
        createInfoFiltered.ppEnabledExtensionNames = finalExts.data();
        if (createInfoFiltered.pApplicationInfo != nullptr) {
            const_cast<VkApplicationInfo*>(createInfoFiltered.pApplicationInfo)
                    ->apiVersion = apiVersion;
        }

        // remove VkDebugReportCallbackCreateInfoEXT from the chain.
        auto* curr = reinterpret_cast<vk_struct_common*>(&createInfoFiltered);
        while (curr != nullptr) {
            if (curr->pNext != nullptr &&
                curr->pNext->sType ==
                        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT) {
                curr->pNext = curr->pNext->pNext;
            }
            curr = curr->pNext;
        }

        // bug: 155795731 (see below)
        AutoLock lock(mLock);

        VkResult res = m_vk->vkCreateInstance(&createInfoFiltered, pAllocator, pInstance);

        if (res != VK_SUCCESS) return res;

        // bug: 155795731 we should protect vkCreateInstance in the driver too
        // because, at least w/ tcmalloc, there is a flaky crash on loading its
        // procs
        //
        // AutoLock lock(mLock);

        // TODO: bug 129484301
        get_emugl_vm_operations().setSkipSnapshotSave(
                !emugl::emugl_feature_is_enabled(
                    android::featurecontrol::VulkanSnapshots));

        InstanceInfo info;
        info.apiVersion = apiVersion;
        for (uint32_t i = 0; i < createInfoFiltered.enabledExtensionCount;
                ++i) {
            info.enabledExtensionNames.push_back(
                    createInfoFiltered.ppEnabledExtensionNames[i]);
        }

        // Box it up
        VkInstance boxed = new_boxed_VkInstance(*pInstance, nullptr, true /* own dispatch */);
        init_vulkan_dispatch_from_instance(
                m_vk, *pInstance,
                dispatch_VkInstance(boxed));
        info.boxed = boxed;

        if (m_emu->instanceSupportsMoltenVK) {
            if (!m_vk->vkSetMTLTextureMVK) {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                        "Cannot find vkSetMTLTextureMVK";
            }
        }

        mInstanceInfo[*pInstance] = info;

        *pInstance = (VkInstance)info.boxed;

        auto fb = FrameBuffer::getFB();
        if (!fb) return res;

        if (vkCleanupEnabled()) {
          fb->registerProcessCleanupCallback(
              unbox_VkInstance(boxed),
              [this, boxed] {

                vkDestroyInstanceImpl(
                    unbox_VkInstance(boxed),
                    nullptr);
              });
        }

        return res;
    }

    void vkDestroyInstanceImpl(VkInstance instance, const VkAllocationCallbacks* pAllocator) {

        // Do delayed removes out of the lock, but get the list of devices to destroy inside the lock.
        {
            AutoLock lock(mLock);
            std::vector<VkDevice> devicesToDestroy;

            for (auto it : mDeviceToPhysicalDevice) {
                auto otherInstance = android::base::find(mPhysicalDeviceToInstance, it.second);
                if (!otherInstance) continue;
                if (instance == *otherInstance) {
                    devicesToDestroy.push_back(it.first);
                }
            }

            for (auto device: devicesToDestroy) {
                sBoxedHandleManager.processDelayedRemovesGlobalStateLocked(
                        device);
            }
        }


        AutoLock lock(mLock);

        teardownInstanceLocked(instance);

        m_vk->vkDestroyInstance(instance, pAllocator);

        auto it = mPhysicalDeviceToInstance.begin();

        while (it != mPhysicalDeviceToInstance.end()) {
            if (it->second == instance) {
                it = mPhysicalDeviceToInstance.erase(it);
            } else {
                ++it;
            }
        }

        auto instInfo = android::base::find(mInstanceInfo, instance);
        delete_VkInstance(instInfo->boxed);
        mInstanceInfo.erase(instance);
    }

    void on_vkDestroyInstance(
            android::base::BumpPool* pool,
            VkInstance boxed_instance,
            const VkAllocationCallbacks* pAllocator) {

        auto instance = unbox_VkInstance(boxed_instance);

        vkDestroyInstanceImpl(instance, pAllocator);

        auto fb = FrameBuffer::getFB();
        if (!fb) return;

        fb->unregisterProcessCleanupCallback(instance);
    }

    VkResult on_vkEnumeratePhysicalDevices(android::base::BumpPool* pool,
                                           VkInstance boxed_instance,
                                           uint32_t* physicalDeviceCount,
                                           VkPhysicalDevice* physicalDevices) {
        auto instance = unbox_VkInstance(boxed_instance);
        auto vk = dispatch_VkInstance(boxed_instance);

        uint32_t physicalDevicesSize = 0;
        if (physicalDeviceCount) {
            physicalDevicesSize = *physicalDeviceCount;
        }

        uint32_t actualPhysicalDeviceCount;
        auto res = vk->vkEnumeratePhysicalDevices(
            instance, &actualPhysicalDeviceCount, nullptr);
        if (res != VK_SUCCESS) {
            return res;
        }
        std::vector<VkPhysicalDevice> validPhysicalDevices(
            actualPhysicalDeviceCount);
        res = vk->vkEnumeratePhysicalDevices(
            instance, &actualPhysicalDeviceCount, validPhysicalDevices.data());
        if (res != VK_SUCCESS) return res;

        AutoLock lock(mLock);

        if (m_emu->instanceSupportsExternalMemoryCapabilities) {


                validPhysicalDevices.erase(
                        std::remove_if(
                                validPhysicalDevices.begin(),
                                validPhysicalDevices.end(),
                                [instance, vk,
                                 this](VkPhysicalDevice physicalDevice) {
                                    // We can get the device UUID.
                                    VkPhysicalDeviceIDPropertiesKHR idProps = {
                                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR,
                                            nullptr,
                                    };
                                    VkPhysicalDeviceProperties2KHR propsWithId = {
                                            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR,
                                            &idProps,
                                    };

                                    VkPhysicalDeviceProperties properties;
                                    vk->vkGetPhysicalDeviceProperties(
                                            physicalDevice, &properties);
                                    PFN_vkGetPhysicalDeviceProperties2KHR
                                            getPhysdevProps2Func = nullptr;
                                    if (properties.apiVersion >=
                                        VK_MAKE_VERSION(1, 1, 0)) {
                                        getPhysdevProps2Func =
                                                vk->vkGetPhysicalDeviceProperties2;
                                        if (!getPhysdevProps2Func) {
                                            getPhysdevProps2Func =
                                                    (PFN_vkGetPhysicalDeviceProperties2KHR)(vk->vkGetInstanceProcAddr(
                                                            instance,
                                                            "vkGetPhysicalDeviceProperties2"));
                                        }
                                    } else if (hasInstanceExtension(
                                                       instance,
                                                       "VK_KHR_get_physical_device_properties2")) {
                                        getPhysdevProps2Func =
                                                vk->vkGetPhysicalDeviceProperties2KHR;
                                        if (!getPhysdevProps2Func) {
                                            getPhysdevProps2Func =
                                                    (PFN_vkGetPhysicalDeviceProperties2KHR)(vk->vkGetInstanceProcAddr(
                                                            instance,
                                                            "vkGetPhysicalDeviceProperties2KHR"));
                                        }
                                    }
                                    if (!getPhysdevProps2Func) {
                                        return false;
                                    }
                                    getPhysdevProps2Func(physicalDevice,
                                                         &propsWithId);

                                    // Remove those devices whose UUIDs don't
                                    // match the one in VkCommonOperations.
                                    return memcmp(m_emu->deviceInfo.idProps
                                                          .deviceUUID,
                                                  idProps.deviceUUID,
                                                  VK_UUID_SIZE) != 0;
                                }),
                        validPhysicalDevices.end());
        } else {
            // If we don't support ID properties then just advertise only the
            // first physical device.
            fprintf(stderr,
                    "%s: device id properties not supported, using first "
                    "physical device\n",
                    __func__);
        }
        if (!validPhysicalDevices.empty()) {
            validPhysicalDevices.erase(std::next(validPhysicalDevices.begin()),
                                       validPhysicalDevices.end());
        }

        if (physicalDeviceCount) {
            *physicalDeviceCount = validPhysicalDevices.size();
        }

        auto emu = getGlobalVkEmulation();

        if (physicalDeviceCount && physicalDevices) {
            // Box them up
            for (uint32_t i = 0;
                 i < std::min(*physicalDeviceCount, physicalDevicesSize); ++i) {
                mPhysicalDeviceToInstance[validPhysicalDevices[i]] = instance;

                auto& physdevInfo = mPhysdevInfo[validPhysicalDevices[i]];

                physdevInfo.boxed = new_boxed_VkPhysicalDevice(
                    validPhysicalDevices[i], vk,
                    false /* does not own dispatch */);

                vk->vkGetPhysicalDeviceProperties(validPhysicalDevices[i],
                                                  &physdevInfo.props);

                if (physdevInfo.props.apiVersion > kMaxSafeVersion) {
                    physdevInfo.props.apiVersion = kMaxSafeVersion;
                }

                vk->vkGetPhysicalDeviceMemoryProperties(
                    validPhysicalDevices[i], &physdevInfo.memoryProperties);

                uint32_t queueFamilyPropCount = 0;

                vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    validPhysicalDevices[i], &queueFamilyPropCount, nullptr);

                physdevInfo.queueFamilyProperties.resize(
                    (size_t)queueFamilyPropCount);

                vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    validPhysicalDevices[i], &queueFamilyPropCount,
                    physdevInfo.queueFamilyProperties.data());

                physicalDevices[i] = (VkPhysicalDevice)physdevInfo.boxed;
            }
            if (physicalDevicesSize < *physicalDeviceCount) {
                res = VK_INCOMPLETE;
            }
        }

        return res;
    }

    void on_vkGetPhysicalDeviceFeatures(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceFeatures* pFeatures) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
        pFeatures->textureCompressionETC2 = true;
        pFeatures->textureCompressionASTC_LDR |= kEmulateAstc;

#ifdef __APPLE__
        // MoltenVK does not support geometryShader, and dEQP expects it to be
        // supported if multiViewport is supported.
        if (!pFeatures->geometryShader) {
            pFeatures->multiViewport = false;
        }
#endif  // __APPLE__
    }

    void on_vkGetPhysicalDeviceFeatures2(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceFeatures2* pFeatures) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo =
            android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        auto instanceInfo = android::base::find(mInstanceInfo, instance);
        if (!instanceInfo) return;

        if (instanceInfo->apiVersion >= VK_MAKE_VERSION(1, 1, 0) &&
            physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceFeatures2KHR(physicalDevice, pFeatures);
        } else {
            // No instance extension, fake it!!!!
            if (pFeatures->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceFeatures2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, 0, };
            vk->vkGetPhysicalDeviceFeatures(physicalDevice, &pFeatures->features);
        }

        pFeatures->features.textureCompressionETC2 = true;
        pFeatures->features.textureCompressionASTC_LDR |= kEmulateAstc;

#ifdef __APPLE__
        // MoltenVK does not support geometryShader, and dEQP expects it to be
        // supported if multiViewport is supported.
        if (!pFeatures->features.geometryShader) {
            pFeatures->features.multiViewport = false;
        }
#endif  // __APPLE__
    }

    VkResult on_vkGetPhysicalDeviceImageFormatProperties(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkImageType type,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkImageCreateFlags flags,
            VkImageFormatProperties* pImageFormatProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        bool emulatedEtc2 = needEmulatedEtc2(physicalDevice, vk);
        bool emulatedAstc = needEmulatedAstc(physicalDevice, vk);
        if (emulatedEtc2 || emulatedAstc) {
            CompressedImageInfo cmpInfo = createCompressedImageInfo(format);
            if (cmpInfo.isCompressed && ((emulatedEtc2 && cmpInfo.isEtc2) ||
                                         (emulatedAstc && cmpInfo.isAstc))) {
                if (!supportEmulatedCompressedImageFormatProperty(
                    format, type, tiling, usage, flags)) {
                    memset(pImageFormatProperties, 0, sizeof(VkImageFormatProperties));
                    return VK_ERROR_FORMAT_NOT_SUPPORTED;
                }
                flags &= ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                format = cmpInfo.sizeCompFormat;
            }
        }
        return vk->vkGetPhysicalDeviceImageFormatProperties(
                physicalDevice, format, type, tiling, usage, flags,
                pImageFormatProperties);
    }

    VkResult on_vkGetPhysicalDeviceImageFormatProperties2(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
            VkImageFormatProperties2* pImageFormatProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        VkPhysicalDeviceImageFormatInfo2 imageFormatInfo;
        bool emulatedEtc2 = needEmulatedEtc2(physicalDevice, vk);
        bool emulatedAstc = needEmulatedAstc(physicalDevice, vk);
        if (emulatedEtc2 || emulatedAstc) {
            CompressedImageInfo cmpInfo =
                createCompressedImageInfo(pImageFormatInfo->format);
            if (cmpInfo.isCompressed && ((emulatedEtc2 && cmpInfo.isEtc2) ||
                                         (emulatedAstc && cmpInfo.isAstc))) {
                if (!supportEmulatedCompressedImageFormatProperty(
                    pImageFormatInfo->format, pImageFormatInfo->type, pImageFormatInfo->tiling,
                    pImageFormatInfo->usage, pImageFormatInfo->flags)) {
                    memset(&pImageFormatProperties->imageFormatProperties, 0,
                        sizeof(VkImageFormatProperties));
                    return VK_ERROR_FORMAT_NOT_SUPPORTED;
                }
                imageFormatInfo = *pImageFormatInfo;
                pImageFormatInfo = &imageFormatInfo;
                imageFormatInfo.flags &= ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
                imageFormatInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
                imageFormatInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
                imageFormatInfo.format = cmpInfo.sizeCompFormat;
            }
        }
        AutoLock lock(mLock);

        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        VkResult res = VK_ERROR_INITIALIZATION_FAILED;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        auto instanceInfo = android::base::find(mInstanceInfo, instance);
        if (!instanceInfo) {
            return res;
        }

        if (instanceInfo->apiVersion >= VK_MAKE_VERSION(1, 1, 0) &&
            physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            res = vk->vkGetPhysicalDeviceImageFormatProperties2(
                physicalDevice, pImageFormatInfo, pImageFormatProperties);
        } else if (hasInstanceExtension(
                    instance,
                    "VK_KHR_get_physical_device_properties2")) {
            res = vk->vkGetPhysicalDeviceImageFormatProperties2KHR(
                physicalDevice, pImageFormatInfo, pImageFormatProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pImageFormatProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceFeatures2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pImageFormatProperties = {
                VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
                0,
            };
            res = vk->vkGetPhysicalDeviceImageFormatProperties(
                physicalDevice, pImageFormatInfo->format,
                pImageFormatInfo->type, pImageFormatInfo->tiling,
                pImageFormatInfo->usage, pImageFormatInfo->flags,
                &pImageFormatProperties->imageFormatProperties);
        }

        const VkPhysicalDeviceExternalImageFormatInfo* extImageFormatInfo =
            vk_find_struct<VkPhysicalDeviceExternalImageFormatInfo>(pImageFormatInfo);
        VkExternalImageFormatProperties* extImageFormatProps =
            vk_find_struct<VkExternalImageFormatProperties>(pImageFormatProperties);

        // Only allow dedicated allocations for external images.
        if (extImageFormatInfo && extImageFormatProps) {
            extImageFormatProps->externalMemoryProperties.externalMemoryFeatures |=
                VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;
        }

        return res;
    }

    void on_vkGetPhysicalDeviceFormatProperties(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkFormatProperties* pFormatProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);
        getPhysicalDeviceFormatPropertiesCore<VkFormatProperties>(
                [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                    VkFormatProperties* pFormatProperties) {
                vk->vkGetPhysicalDeviceFormatProperties(
                        physicalDevice, format, pFormatProperties);
                },
                vk, physicalDevice, format, pFormatProperties);
    }

    void on_vkGetPhysicalDeviceFormatProperties2(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkFormat format,
            VkFormatProperties2* pFormatProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo)
            return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        auto instanceInfo = android::base::find(mInstanceInfo, instance);
        if (!instanceInfo) return;

        if (instanceInfo->apiVersion >= VK_MAKE_VERSION(1, 1, 0) &&
            physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties2>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                        VkFormatProperties2* pFormatProperties) {
                    vk->vkGetPhysicalDeviceFormatProperties2(
                            physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format, pFormatProperties);
        } else if (hasInstanceExtension(
                    instance,
                    "VK_KHR_get_physical_device_properties2")) {
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties2>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                        VkFormatProperties2* pFormatProperties) {
                    vk->vkGetPhysicalDeviceFormatProperties2KHR(
                            physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format, pFormatProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pFormatProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "vkGetPhysicalDeviceFormatProperties2 without having "
                        "enabled the extension!!!!11111\n",
                        __func__);
            }
            pFormatProperties->sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
            getPhysicalDeviceFormatPropertiesCore<VkFormatProperties>(
                    [vk](VkPhysicalDevice physicalDevice, VkFormat format,
                        VkFormatProperties* pFormatProperties) {
                    vk->vkGetPhysicalDeviceFormatProperties(
                            physicalDevice, format, pFormatProperties);
                    },
                    vk, physicalDevice, format,
                    &pFormatProperties->formatProperties);
        }
    }

    void on_vkGetPhysicalDeviceProperties(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceProperties* pProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceProperties(
                physicalDevice, pProperties);

        if (pProperties->apiVersion > kMaxSafeVersion) {
            pProperties->apiVersion = kMaxSafeVersion;
        }
    }

    void on_vkGetPhysicalDeviceProperties2(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceProperties2* pProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        AutoLock lock(mLock);

        auto physdevInfo =
            android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        auto instanceInfo = android::base::find(mInstanceInfo, instance);
        if (!instanceInfo) return;

        if (instanceInfo->apiVersion >= VK_MAKE_VERSION(1, 1, 0) &&
            physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceProperties2KHR(physicalDevice, pProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceProperties2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, 0, };
            vk->vkGetPhysicalDeviceProperties(physicalDevice, &pProperties->properties);
        }

        if (pProperties->properties.apiVersion > kMaxSafeVersion) {
            pProperties->properties.apiVersion = kMaxSafeVersion;
        }
    }

    void on_vkGetPhysicalDeviceMemoryProperties(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceMemoryProperties* pMemoryProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        vk->vkGetPhysicalDeviceMemoryProperties(
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

            if (!emugl::emugl_feature_is_enabled(
                        android::featurecontrol::GLDirectMem) &&
                !emugl::emugl_feature_is_enabled(
                        android::featurecontrol::VirtioGpuNext)) {
                pMemoryProperties->memoryTypes[i].propertyFlags =
                    pMemoryProperties->memoryTypes[i].propertyFlags &
                    ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }
    }

    void on_vkGetPhysicalDeviceMemoryProperties2(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        auto physdevInfo =
            android::base::find(mPhysdevInfo, physicalDevice);
        if (!physdevInfo) return;

        auto instance = mPhysicalDeviceToInstance[physicalDevice];
        auto instanceInfo = android::base::find(mInstanceInfo, instance);
        if (!instanceInfo) return;

        if (instanceInfo->apiVersion >= VK_MAKE_VERSION(1, 1, 0) &&
            physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) {
            vk->vkGetPhysicalDeviceMemoryProperties2(physicalDevice, pMemoryProperties);
        } else if (hasInstanceExtension(instance, "VK_KHR_get_physical_device_properties2")) {
            vk->vkGetPhysicalDeviceMemoryProperties2KHR(physicalDevice, pMemoryProperties);
        } else {
            // No instance extension, fake it!!!!
            if (pMemoryProperties->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkPhysicalDeviceMemoryProperties2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }
            *pMemoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, 0, };
            vk->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &pMemoryProperties->memoryProperties);
        }

        // Pick a max heap size that will work around
        // drivers that give bad suggestions (such as 0xFFFFFFFFFFFFFFFF for the heap size)
        // plus won't break the bank on 32-bit userspace.
        static constexpr VkDeviceSize kMaxSafeHeapSize =
            2ULL * 1024ULL * 1024ULL * 1024ULL;

        for (uint32_t i = 0; i < pMemoryProperties->memoryProperties.memoryTypeCount; ++i) {
            uint32_t heapIndex = pMemoryProperties->memoryProperties.memoryTypes[i].heapIndex;
            auto& heap = pMemoryProperties->memoryProperties.memoryHeaps[heapIndex];

            if (heap.size > kMaxSafeHeapSize) {
                heap.size = kMaxSafeHeapSize;
            }

            if (!emugl::emugl_feature_is_enabled(
                        android::featurecontrol::GLDirectMem) &&
                !emugl::emugl_feature_is_enabled(
                        android::featurecontrol::VirtioGpuNext)) {
                pMemoryProperties->memoryProperties.memoryTypes[i].propertyFlags =
                    pMemoryProperties->memoryProperties.memoryTypes[i].propertyFlags &
                    ~(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }
    }

    VkResult on_vkEnumerateDeviceExtensionProperties(
        android::base::BumpPool* pool,
        VkPhysicalDevice boxed_physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        std::vector<VkExtensionProperties> properties;
        uint32_t propertyCount = 0u;
        VkResult result = vk->vkEnumerateDeviceExtensionProperties(
            physicalDevice, pLayerName, &propertyCount, nullptr);
        if (result != VK_SUCCESS) {
            return result;
        }

        properties.resize(propertyCount);
        result = vk->vkEnumerateDeviceExtensionProperties(
            physicalDevice, pLayerName, &propertyCount, properties.data());
        if (result != VK_SUCCESS) {
            return result;
        }

        // If MoltenVK is supported on host, we need to ensure that we include
        // VK_MVK_moltenvk extenstion in returned properties.
        if (m_emu->instanceSupportsMoltenVK &&
            std::find_if(properties.begin(), properties.end(),
                         [](const VkExtensionProperties& props) {
                             return strcmp(props.extensionName,
                                           VK_MVK_MOLTENVK_EXTENSION_NAME) == 0;
                         }) == properties.end()) {
            VkExtensionProperties mvk_props;
            strncpy(mvk_props.extensionName, VK_MVK_MOLTENVK_EXTENSION_NAME,
                    sizeof(mvk_props.extensionName));
            mvk_props.specVersion = VK_MVK_MOLTENVK_SPEC_VERSION;
            properties.push_back(mvk_props);
        }

        if (std::find_if(
                    properties.begin(), properties.end(),
                    [](const VkExtensionProperties& props) {
                        return strcmp(props.extensionName,
                                      VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME) ==
                               0;
                    }) == properties.end()) {
            VkExtensionProperties property;
            strncpy(property.extensionName,
                    VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
                    sizeof(property.extensionName));
            property.specVersion = 2;
            properties.push_back(property);
        }

        if (*pPropertyCount == 0) {
            *pPropertyCount = properties.size();
        } else {
            memcpy(pProperties, properties.data(), *pPropertyCount * sizeof(VkExtensionProperties));
        }

        return VK_SUCCESS;
    }

    VkResult on_vkCreateDevice(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {

        if (mLogging) {
            fprintf(stderr, "%s: begin\n", __func__);
        }

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        std::vector<const char*> finalExts =
            filteredExtensionNames(
                    pCreateInfo->enabledExtensionCount,
                    pCreateInfo->ppEnabledExtensionNames);

        // Run the underlying API call, filtering extensions.
        VkDeviceCreateInfo createInfoFiltered = *pCreateInfo;
        bool emulateTextureEtc2 = false;
        bool emulateTextureAstc = false;
        VkPhysicalDeviceFeatures featuresFiltered;

        if (pCreateInfo->pEnabledFeatures) {
            featuresFiltered = *pCreateInfo->pEnabledFeatures;
            if (featuresFiltered.textureCompressionETC2) {
                if (needEmulatedEtc2(physicalDevice, vk)) {
                    emulateTextureEtc2 = true;
                    featuresFiltered.textureCompressionETC2 = false;
                }
            }
            if (featuresFiltered.textureCompressionASTC_LDR) {
                if (needEmulatedAstc(physicalDevice, vk)) {
                    emulateTextureAstc = true;
                    featuresFiltered.textureCompressionASTC_LDR = false;
                }
            }
            createInfoFiltered.pEnabledFeatures = &featuresFiltered;
        }

        vk_foreach_struct(ext, pCreateInfo->pNext) {
            switch (ext->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2:
                    if (needEmulatedEtc2(physicalDevice, vk)) {
                        emulateTextureEtc2 = true;
                        VkPhysicalDeviceFeatures2* features2 =
                            (VkPhysicalDeviceFeatures2*)ext;
                        features2->features.textureCompressionETC2 = false;
                    }
                    if (needEmulatedAstc(physicalDevice, vk)) {
                        emulateTextureAstc = true;
                        VkPhysicalDeviceFeatures2* features2 =
                                (VkPhysicalDeviceFeatures2*)ext;
                        features2->features.textureCompressionASTC_LDR = false;
                    }
                    break;
                default:
                    break;
            }
        }

        createInfoFiltered.enabledExtensionCount = (uint32_t)finalExts.size();
        createInfoFiltered.ppEnabledExtensionNames = finalExts.data();

        // bug: 155795731 (see below)
        if (mLogging) {
            fprintf(stderr, "%s: acquire lock\n", __func__);
        }

        AutoLock lock(mLock);

        if (mLogging) {
            fprintf(stderr, "%s: got lock, calling host\n", __func__);
        }

        VkResult result =
            vk->vkCreateDevice(
                    physicalDevice, &createInfoFiltered, pAllocator, pDevice);

        if (mLogging) {
            fprintf(stderr, "%s: host returned. result: %d\n", __func__, result);
        }

        if (result != VK_SUCCESS) return result;

        if (mLogging) {
            fprintf(stderr, "%s: track the new device (begin)\n", __func__);
        }

        // bug: 155795731 we should protect vkCreateDevice in the driver too
        // because, at least w/ tcmalloc, there is a flaky crash on loading its
        // procs
        //
        // AutoLock lock(mLock);

        mDeviceToPhysicalDevice[*pDevice] = physicalDevice;

        // Fill out information about the logical device here.
        auto& deviceInfo = mDeviceInfo[*pDevice];
        deviceInfo.physicalDevice = physicalDevice;
        deviceInfo.emulateTextureEtc2 = emulateTextureEtc2;
        deviceInfo.emulateTextureAstc = emulateTextureAstc;

        for (uint32_t i = 0; i < createInfoFiltered.enabledExtensionCount;
                ++i) {
            deviceInfo.enabledExtensionNames.push_back(
                createInfoFiltered.ppEnabledExtensionNames[i]);
        }

        // First, get the dispatch table.
        VkDevice boxed = new_boxed_VkDevice(*pDevice, nullptr, true /* own dispatch */);

        if (mLogging) {
            fprintf(stderr, "%s: init vulkan dispatch from device\n", __func__);
        }

        init_vulkan_dispatch_from_device(
                vk, *pDevice,
                dispatch_VkDevice(boxed));

        if (mLogging) {
            fprintf(stderr, "%s: init vulkan dispatch from device (end)\n", __func__);
        }

        deviceInfo.boxed = boxed;

        // Next, get information about the queue families used by this device.
        std::unordered_map<uint32_t, uint32_t> queueFamilyIndexCounts;
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            const auto& queueCreateInfo =
                pCreateInfo->pQueueCreateInfos[i];
            // Check only queues created with flags = 0 in VkDeviceQueueCreateInfo.
            auto flags = queueCreateInfo.flags;
            if (flags) continue;
            uint32_t queueFamilyIndex = queueCreateInfo.queueFamilyIndex;
            uint32_t queueCount = queueCreateInfo.queueCount;
            queueFamilyIndexCounts[queueFamilyIndex] = queueCount;
        }

        auto it = mPhysdevInfo.find(physicalDevice);

        for (auto it : queueFamilyIndexCounts) {
            auto index = it.first;
            auto count = it.second;
            auto& queues = deviceInfo.queues[index];
            for (uint32_t i = 0; i < count; ++i) {
                VkQueue queueOut;

                if (mLogging) {
                    fprintf(stderr, "%s: get device queue (begin)\n", __func__);
                }

                vk->vkGetDeviceQueue(
                        *pDevice, index, i, &queueOut);

                if (mLogging) {
                    fprintf(stderr, "%s: get device queue (end)\n", __func__);
                }

                queues.push_back(queueOut);
                mQueueInfo[queueOut].device = *pDevice;
                mQueueInfo[queueOut].queueFamilyIndex = index;

                auto boxed = new_boxed_VkQueue(queueOut, dispatch_VkDevice(deviceInfo.boxed), false /* does not own dispatch */);
                mQueueInfo[queueOut].boxed = boxed;
                mQueueInfo[queueOut].lock = new Lock;
            }
        }

        // Box the device.
        *pDevice = (VkDevice)deviceInfo.boxed;

        if (mLogging) {
            fprintf(stderr, "%s: (end)\n", __func__);
        }

        return VK_SUCCESS;
    }

    void on_vkGetDeviceQueue(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            uint32_t queueFamilyIndex,
            uint32_t queueIndex,
            VkQueue* pQueue) {

        auto device = unbox_VkDevice(boxed_device);

        AutoLock lock(mLock);

        *pQueue = VK_NULL_HANDLE;

        auto deviceInfo = android::base::find(mDeviceInfo, device);
        if (!deviceInfo) return;

        const auto& queues =
            deviceInfo->queues;

        const auto queueList =
            android::base::find(queues, queueFamilyIndex);

        if (!queueList) return;
        if (queueIndex >= queueList->size()) return;

        VkQueue unboxedQueue = (*queueList)[queueIndex];

        auto queueInfo = android::base::find(mQueueInfo, unboxedQueue);

        if (!queueInfo) return;

        *pQueue = (VkQueue)queueInfo->boxed;
    }

    void destroyDeviceLocked(VkDevice device, const VkAllocationCallbacks* pAllocator) {
        auto it = mDeviceInfo.find(device);
        if (it == mDeviceInfo.end()) return;

        auto eraseIt = mQueueInfo.begin();
        for(; eraseIt != mQueueInfo.end();) {
            if (eraseIt->second.device == device) {
                delete eraseIt->second.lock;
                delete_VkQueue(eraseIt->second.boxed);
                eraseIt = mQueueInfo.erase(eraseIt);
            } else {
                ++eraseIt;
            }
        }

        // Run the underlying API call.
        m_vk->vkDestroyDevice(device, pAllocator);

        delete_VkDevice(it->second.boxed);
    }

    void on_vkDestroyDevice(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);

        AutoLock lock(mLock);

        sBoxedHandleManager.processDelayedRemovesGlobalStateLocked(device);
        destroyDeviceLocked(device, pAllocator);

        mDeviceInfo.erase(device);
        mDeviceToPhysicalDevice.erase(device);
    }

    VkResult on_vkCreateBuffer(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkBufferCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkBuffer* pBuffer) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
            vk->vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            auto& bufInfo = mBufferInfo[*pBuffer];
            bufInfo.device = device;
            bufInfo.size = pCreateInfo->size;
            bufInfo.vk = vk;
            *pBuffer = new_boxed_non_dispatchable_VkBuffer(*pBuffer);
        }

        return result;
    }

    void on_vkDestroyBuffer(
            android::base::BumpPool* pool,
            VkDevice boxed_device, VkBuffer buffer,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyBuffer(device, buffer, pAllocator);

        AutoLock lock(mLock);
        mBufferInfo.erase(buffer);
    }

    void setBufferMemoryBindInfoLocked(
            VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        auto it = mBufferInfo.find(buffer);
        if (it == mBufferInfo.end()) {
            return;
        }
        it->second.memory = memory;
        it->second.memoryOffset = memoryOffset;
    }

    VkResult on_vkBindBufferMemory(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkBuffer buffer,
            VkDeviceMemory memory,
            VkDeviceSize memoryOffset) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
            vk->vkBindBufferMemory(device, buffer, memory, memoryOffset);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            setBufferMemoryBindInfoLocked(buffer, memory, memoryOffset);
        }
        return result;
    }

    VkResult on_vkBindBufferMemory2(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            uint32_t bindInfoCount,
            const VkBindBufferMemoryInfo* pBindInfos) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
            vk->vkBindBufferMemory2(device, bindInfoCount, pBindInfos);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            for (uint32_t i = 0; i < bindInfoCount; ++i) {
                setBufferMemoryBindInfoLocked(
                        pBindInfos[i].buffer,
                        pBindInfos[i].memory,
                        pBindInfos[i].memoryOffset);
            }
        }

        return result;
    }

    VkResult on_vkBindBufferMemory2KHR(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            uint32_t bindInfoCount,
            const VkBindBufferMemoryInfo* pBindInfos) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
            vk->vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);

        if (result == VK_SUCCESS) {
            AutoLock lock(mLock);
            for (uint32_t i = 0; i < bindInfoCount; ++i) {
                setBufferMemoryBindInfoLocked(
                        pBindInfos[i].buffer,
                        pBindInfos[i].memory,
                        pBindInfos[i].memoryOffset);
            }
        }

        return result;

    }

    VkResult on_vkCreateImage(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkImageCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkImage* pImage) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        CompressedImageInfo cmpInfo = {};
        VkImageCreateInfo& sizeCompInfo = cmpInfo.sizeCompImgCreateInfo;
        VkImageCreateInfo decompInfo;
        if (deviceInfoIt->second.needEmulatedDecompression(
                    pCreateInfo->format)) {
            cmpInfo = createCompressedImageInfo(pCreateInfo->format);
            cmpInfo.imageType = pCreateInfo->imageType;
            cmpInfo.extent = pCreateInfo->extent;
            cmpInfo.mipLevels = pCreateInfo->mipLevels;
            cmpInfo.layerCount = pCreateInfo->arrayLayers;
            sizeCompInfo = *pCreateInfo;
            sizeCompInfo.format = cmpInfo.sizeCompFormat;
            sizeCompInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            sizeCompInfo.flags &=
                    ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
            sizeCompInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            // Each block is 4x4 in ETC2 compressed texture
            sizeCompInfo.extent.width = (sizeCompInfo.extent.width +
                                         cmpInfo.compressedBlockWidth - 1) /
                                        cmpInfo.compressedBlockWidth;
            sizeCompInfo.extent.height = (sizeCompInfo.extent.height +
                                          cmpInfo.compressedBlockHeight - 1) /
                                         cmpInfo.compressedBlockHeight;
            sizeCompInfo.mipLevels = 1;
            if (pCreateInfo->queueFamilyIndexCount) {
                cmpInfo.sizeCompImgQueueFamilyIndices.assign(
                        pCreateInfo->pQueueFamilyIndices,
                        pCreateInfo->pQueueFamilyIndices +
                                pCreateInfo->queueFamilyIndexCount);
                sizeCompInfo.pQueueFamilyIndices =
                        cmpInfo.sizeCompImgQueueFamilyIndices.data();
            }
            decompInfo = *pCreateInfo;
            decompInfo.format = cmpInfo.decompFormat;
            decompInfo.flags &=
                    ~VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_KHR;
            decompInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            decompInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
            pCreateInfo = &decompInfo;
        }
        cmpInfo.device = device;

        AndroidNativeBufferInfo anbInfo;
        const VkNativeBufferANDROID* nativeBufferANDROID =
            vk_find_struct<VkNativeBufferANDROID>(pCreateInfo);

        VkResult createRes = VK_SUCCESS;

        if (nativeBufferANDROID) {

            auto memProps = memPropsOfDeviceLocked(device);

            createRes =
                prepareAndroidNativeBufferImage(
                        vk, device, pCreateInfo, nativeBufferANDROID, pAllocator,
                        memProps, &anbInfo);
            if (createRes == VK_SUCCESS) {
                *pImage = anbInfo.image;
            }
        } else {
            createRes =
                vk->vkCreateImage(device, pCreateInfo, pAllocator, pImage);
        }

        if (createRes != VK_SUCCESS) return createRes;

        if (deviceInfoIt->second.needEmulatedDecompression(cmpInfo)) {
            cmpInfo.decompImg = *pImage;
            createSizeCompImages(vk, &cmpInfo);
        }

        auto& imageInfo = mImageInfo[*pImage];
        imageInfo.anbInfo = anbInfo;
        imageInfo.cmpInfo = cmpInfo;

        *pImage = new_boxed_non_dispatchable_VkImage(*pImage);

        return createRes;
    }

    void on_vkDestroyImage(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkImage image,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(image);

        if (it == mImageInfo.end()) return;

        auto info = it->second;

        if (info.anbInfo.image) {
            teardownAndroidNativeBufferImage(vk, &info.anbInfo);
        } else {
            if (info.cmpInfo.isCompressed) {
                CompressedImageInfo& cmpInfo = info.cmpInfo;
                if (image != cmpInfo.decompImg) {
                    vk->vkDestroyImage(device, info.cmpInfo.decompImg, nullptr);
                }
                for (const auto& image : cmpInfo.sizeCompImgs) {
                    vk->vkDestroyImage(device, image, nullptr);
                }
                vk->vkDestroyDescriptorSetLayout(
                        device, cmpInfo.decompDescriptorSetLayout, nullptr);
                vk->vkDestroyDescriptorPool(
                        device, cmpInfo.decompDescriptorPool, nullptr);
                vk->vkDestroyShaderModule(device, cmpInfo.decompShader,
                        nullptr);
                vk->vkDestroyPipelineLayout(
                        device, cmpInfo.decompPipelineLayout, nullptr);
                vk->vkDestroyPipeline(device, cmpInfo.decompPipeline, nullptr);
                for (const auto& imageView : cmpInfo.sizeCompImageViews) {
                    vk->vkDestroyImageView(device, imageView, nullptr);
                }
                for (const auto& imageView : cmpInfo.decompImageViews) {
                    vk->vkDestroyImageView(device, imageView, nullptr);
                }
            }
            vk->vkDestroyImage(device, image, pAllocator);
        }
        mImageInfo.erase(image);
    }

    VkResult on_vkBindImageMemory(android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkImage image,
            VkDeviceMemory memory,
            VkDeviceSize memoryOffset) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        VkResult result =
            vk->vkBindImageMemory(device, image, memory, memoryOffset);
        if (VK_SUCCESS != result) {
            return result;
        }
        AutoLock lock(mLock);
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        auto mapInfoIt = mMapInfo.find(memory);
        if (mapInfoIt == mMapInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        if (mapInfoIt->second.mtlTexture) {
            result = m_vk->vkSetMTLTextureMVK(image, mapInfoIt->second.mtlTexture);
            if (result != VK_SUCCESS) {
                fprintf(stderr, "vkSetMTLTexture failed\n");
                return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
        }
        if (!deviceInfoIt->second.emulateTextureEtc2 &&
            !deviceInfoIt->second.emulateTextureAstc) {
            return VK_SUCCESS;
        }
        auto imageInfoIt = mImageInfo.find(image);
        if (imageInfoIt == mImageInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        CompressedImageInfo& cmp = imageInfoIt->second.cmpInfo;
        if (!deviceInfoIt->second.needEmulatedDecompression(cmp)) {
            return VK_SUCCESS;
        }
        for (size_t i = 0; i < cmp.sizeCompImgs.size(); i++) {
            result = vk->vkBindImageMemory(device, cmp.sizeCompImgs[i], memory,
                    memoryOffset + cmp.memoryOffsets[i]);
        }

        return VK_SUCCESS;
    }

    VkResult on_vkCreateImageView(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkImageViewCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkImageView* pView) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!pCreateInfo) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        AutoLock lock(mLock);
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        auto imageInfoIt = mImageInfo.find(pCreateInfo->image);
        if (imageInfoIt == mImageInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        VkImageViewCreateInfo createInfo;
        bool needEmulatedAlpha = false;
        if (deviceInfoIt->second.emulateTextureEtc2 ||
            deviceInfoIt->second.emulateTextureAstc) {
            CompressedImageInfo cmpInfo = createCompressedImageInfo(
                    pCreateInfo->format
                    );
            if (deviceInfoIt->second.needEmulatedDecompression(cmpInfo)) {
                if (imageInfoIt->second.cmpInfo.decompImg) {
                    createInfo = *pCreateInfo;
                    createInfo.format = cmpInfo.decompFormat;
                    needEmulatedAlpha = cmpInfo.needEmulatedAlpha();
                    createInfo.image = imageInfoIt->second.cmpInfo.decompImg;
                    pCreateInfo = &createInfo;
                }
            } else if (deviceInfoIt->second.needEmulatedDecompression(
                               imageInfoIt->second.cmpInfo)) {
                // Size compatible image view
                createInfo = *pCreateInfo;
                createInfo.format = cmpInfo.sizeCompFormat;
                needEmulatedAlpha = false;
                createInfo.image =
                    imageInfoIt->second.cmpInfo.sizeCompImgs
                    [pCreateInfo->subresourceRange.baseMipLevel];
                createInfo.subresourceRange.baseMipLevel = 0;
                pCreateInfo = &createInfo;
            }
        }
        if (imageInfoIt->second.anbInfo.externallyBacked) {
            createInfo = *pCreateInfo;
            createInfo.format = imageInfoIt->second.anbInfo.vkFormat;
            pCreateInfo = &createInfo;
        }

        VkResult result =
            vk->vkCreateImageView(device, pCreateInfo, pAllocator, pView);
        if (result != VK_SUCCESS) {
            return result;
        }

        auto& imageViewInfo = mImageViewInfo[*pView];
        imageViewInfo.needEmulatedAlpha = needEmulatedAlpha;

        *pView = new_boxed_non_dispatchable_VkImageView(*pView);

        return result;
    }

    void on_vkDestroyImageView(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkImageView imageView,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyImageView(device, imageView, pAllocator);
        AutoLock lock(mLock);
        mImageViewInfo.erase(imageView);
    }

    VkResult on_vkCreateSampler(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkSamplerCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSampler* pSampler) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        VkResult result =
            vk->vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        auto& samplerInfo = mSamplerInfo[*pSampler];
        samplerInfo.createInfo = *pCreateInfo;
        // We emulate RGB with RGBA for some compressed textures, which does not
        // handle translarent border correctly.
        samplerInfo.needEmulatedAlpha =
            (pCreateInfo->addressModeU ==
             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
             pCreateInfo->addressModeV ==
             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
             pCreateInfo->addressModeW ==
             VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) &&
            (pCreateInfo->borderColor ==
             VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK ||
             pCreateInfo->borderColor ==
             VK_BORDER_COLOR_INT_TRANSPARENT_BLACK);

        *pSampler = new_boxed_non_dispatchable_VkSampler(*pSampler);

        return result;
    }

    void on_vkDestroySampler(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkSampler sampler,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        vk->vkDestroySampler(device, sampler, pAllocator);
        AutoLock lock(mLock);
        const auto& samplerInfoIt = mSamplerInfo.find(sampler);
        if (samplerInfoIt != mSamplerInfo.end()) {
            if (samplerInfoIt->second.emulatedborderSampler != VK_NULL_HANDLE) {
                vk->vkDestroySampler(
                        device, samplerInfoIt->second.emulatedborderSampler,
                        nullptr);
            }
            mSamplerInfo.erase(samplerInfoIt);
        }
    }

    VkResult on_vkCreateSemaphore(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkSemaphoreCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSemaphore* pSemaphore) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkSemaphoreCreateInfo localCreateInfo = vk_make_orphan_copy(*pCreateInfo);
        vk_struct_chain_iterator structChainIter = vk_make_chain_iterator(&localCreateInfo);

        VkSemaphoreTypeCreateInfoKHR localSemaphoreTypeCreateInfo;
        if (const VkSemaphoreTypeCreateInfoKHR* semaphoreTypeCiPtr =
                    vk_find_struct<VkSemaphoreTypeCreateInfoKHR>(pCreateInfo);
            semaphoreTypeCiPtr) {
            localSemaphoreTypeCreateInfo =
                    vk_make_orphan_copy(*semaphoreTypeCiPtr);
            vk_append_struct(&structChainIter, &localSemaphoreTypeCreateInfo);
        }

        const VkExportSemaphoreCreateInfoKHR* exportCiPtr =
            vk_find_struct<VkExportSemaphoreCreateInfoKHR>(pCreateInfo);
        VkExportSemaphoreCreateInfoKHR localSemaphoreCreateInfo;

        if (exportCiPtr) {
            localSemaphoreCreateInfo = vk_make_orphan_copy(*exportCiPtr);

#ifdef _WIN32
            if (localSemaphoreCreateInfo.handleTypes) {
                localSemaphoreCreateInfo.handleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
            }
#endif

            vk_append_struct(&structChainIter, &localSemaphoreCreateInfo);
        }

        VkResult res = vk->vkCreateSemaphore(device, &localCreateInfo, pAllocator, pSemaphore);

        if (res != VK_SUCCESS) return res;

        *pSemaphore = new_boxed_non_dispatchable_VkSemaphore(*pSemaphore);

        return res;
    }

    VkResult on_vkCreateFence(android::base::BumpPool* pool,
                              VkDevice boxed_device,
                              const VkFenceCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkFence* pFence) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult res =
                vk->vkCreateFence(device, pCreateInfo, pAllocator, pFence);
        if (res != VK_SUCCESS) {
            return res;
        }

        {
            AutoLock lock(mLock);

            DCHECK(mFenceInfo.find(*pFence) == mFenceInfo.end());
            // Create FenceInfo for *pFence.
            auto& fenceInfo = mFenceInfo[*pFence];
            fenceInfo.device = device;
            fenceInfo.vk = vk;

            *pFence = new_boxed_non_dispatchable_VkFence(*pFence);
            fenceInfo.boxed = *pFence;
        }

        return res;
    }

    VkResult on_vkResetFences(android::base::BumpPool* pool,
                              VkDevice boxed_device,
                              uint32_t fenceCount,
                              const VkFence* pFences) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        std::vector<VkFence> cleanedFences;
        for (uint32_t i = 0; i < fenceCount; ++i) {
            if (pFences[i] != VK_NULL_HANDLE)
                cleanedFences.push_back(pFences[i]);
        }

        VkResult res = vk->vkResetFences(device, (uint32_t)cleanedFences.size(), cleanedFences.data());

        if (res != VK_SUCCESS) {
            return res;
        }

        // Reset all fences' states to kNotWaitable.
        {
            AutoLock lock(mLock);
            for (uint32_t i = 0; i < fenceCount; i++) {
                DCHECK(mFenceInfo.find(pFences[i]) != mFenceInfo.end());
                mFenceInfo[pFences[i]].state = FenceInfo::State::kNotWaitable;
            }
        }
        return VK_SUCCESS;
    }

    VkResult on_vkImportSemaphoreFdKHR(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

#ifdef _WIN32
        AutoLock lock(mLock);

        auto infoPtr = android::base::find(
                mSemaphoreInfo, mExternalSemaphoresById[pImportSemaphoreFdInfo->fd]);

        if (!infoPtr) {
            return VK_ERROR_INVALID_EXTERNAL_HANDLE;
        }

        VK_EXT_MEMORY_HANDLE handle =
            dupExternalMemory(infoPtr->externalHandle);

        VkImportSemaphoreWin32HandleInfoKHR win32ImportInfo = {
            VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR, 0,
            pImportSemaphoreFdInfo->semaphore,
            pImportSemaphoreFdInfo->flags,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
            handle, L"",
        };

        return vk->vkImportSemaphoreWin32HandleKHR(
                device, &win32ImportInfo);
#else
        VkImportSemaphoreFdInfoKHR importInfo = *pImportSemaphoreFdInfo;
        importInfo.fd = dup(pImportSemaphoreFdInfo->fd);
        return vk->vkImportSemaphoreFdKHR(device, &importInfo);
#endif
    }

    VkResult on_vkGetSemaphoreFdKHR(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
            int* pFd) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
#ifdef _WIN32
        VkSemaphoreGetWin32HandleInfoKHR getWin32 = {
            VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR, 0,
            pGetFdInfo->semaphore,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT,
        };
        VK_EXT_MEMORY_HANDLE handle;
        VkResult result = vk->vkGetSemaphoreWin32HandleKHR(device, &getWin32, &handle);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        mSemaphoreInfo[pGetFdInfo->semaphore].externalHandle = handle;
        int nextId = genSemaphoreId();
        mExternalSemaphoresById[nextId] = pGetFdInfo->semaphore;
        *pFd = nextId;
#else
        VkResult result = vk->vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);

        mSemaphoreInfo[pGetFdInfo->semaphore].externalHandle = *pFd;
        // No next id; its already an fd
#endif
        return result;
    }

    void on_vkDestroySemaphore(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkSemaphore semaphore,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

#ifndef _WIN32
        AutoLock lock(mLock);
        const auto& ite = mSemaphoreInfo.find(semaphore);
        if (ite != mSemaphoreInfo.end() &&
                (ite->second.externalHandle != VK_EXT_MEMORY_HANDLE_INVALID)) {
            close(ite->second.externalHandle);
        }
#endif
        vk->vkDestroySemaphore(device, semaphore, pAllocator);
    }

    void on_vkDestroyFence(android::base::BumpPool* pool,
                           VkDevice boxed_device,
                           VkFence fence,
                           const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        {
            AutoLock lock(mLock);
            mFenceInfo.erase(fence);
        }

        vk->vkDestroyFence(device, fence, pAllocator);
    }

    VkResult on_vkCreateDescriptorSetLayout(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorSetLayout* pSetLayout) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto res =
            vk->vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);

        if (res == VK_SUCCESS) {
            AutoLock lock(mLock);
            auto& info = mDescriptorSetLayoutInfo[*pSetLayout];
            info.device = device;
            *pSetLayout = new_boxed_non_dispatchable_VkDescriptorSetLayout(*pSetLayout);
            info.boxed = *pSetLayout;

            info.createInfo = *pCreateInfo;
            for (uint32_t i = 0; i < pCreateInfo->bindingCount; ++i) {
                info.bindings.push_back(pCreateInfo->pBindings[i]);
            }
        }

        return res;
    }

    void on_vkDestroyDescriptorSetLayout(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        VkDescriptorSetLayout descriptorSetLayout,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);

        AutoLock lock(mLock);
        mDescriptorSetLayoutInfo.erase(descriptorSetLayout);
    }

    VkResult on_vkCreateDescriptorPool(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkDescriptorPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorPool* pDescriptorPool) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto res =
            vk->vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);

        if (res == VK_SUCCESS) {
            AutoLock lock(mLock);
            auto& info = mDescriptorPoolInfo[*pDescriptorPool];
            info.device = device;
            *pDescriptorPool = new_boxed_non_dispatchable_VkDescriptorPool(*pDescriptorPool);
            info.boxed = *pDescriptorPool;
            info.createInfo = *pCreateInfo;
            info.maxSets = pCreateInfo->maxSets;
            info.usedSets = 0;

            for (uint32_t i = 0; i < pCreateInfo->poolSizeCount; ++i) {
                DescriptorPoolInfo::PoolState state;
                state.type = pCreateInfo->pPoolSizes[i].type;
                state.descriptorCount = pCreateInfo->pPoolSizes[i].descriptorCount;
                state.used = 0;
                info.pools.push_back(state);
            }

            if (emugl::emugl_feature_is_enabled(android::featurecontrol::VulkanBatchedDescriptorSetUpdate)) {
                for (uint32_t i = 0; i < pCreateInfo->maxSets; ++i) {
                    info.poolIds.push_back((uint64_t)new_boxed_non_dispatchable_VkDescriptorSet(VK_NULL_HANDLE));
                }
            }
        }

        return res;
    }

    void cleanupDescriptorPoolAllocedSetsLocked(VkDescriptorPool descriptorPool, bool isDestroy = false) {
        auto info = android::base::find(mDescriptorPoolInfo, descriptorPool);

        if (!info) return;

        for (auto it : info->allocedSetsToBoxed) {
            auto unboxedSet = it.first;
            auto boxedSet = it.second;
            mDescriptorSetInfo.erase(unboxedSet);
            if (!emugl::emugl_feature_is_enabled(android::featurecontrol::VulkanBatchedDescriptorSetUpdate)) {
                delete_VkDescriptorSet(boxedSet);
            }
        }

        if (emugl::emugl_feature_is_enabled(android::featurecontrol::VulkanBatchedDescriptorSetUpdate)) {
            if (isDestroy) {
                for (auto poolId : info->poolIds) {
                    delete_VkDescriptorSet((VkDescriptorSet)poolId);
                }
            } else {
                for (auto poolId : info->poolIds) {
                    auto handleInfo = sBoxedHandleManager.get(poolId);
                    if (handleInfo) handleInfo->underlying = reinterpret_cast<uint64_t>(VK_NULL_HANDLE);
                }
            }
        }

        info->usedSets = 0;
        info->allocedSetsToBoxed.clear();

        for (auto& pool : info->pools) {
            pool.used = 0;
        }
    }

    void on_vkDestroyDescriptorPool(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        VkDescriptorPool descriptorPool,
        const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorPool(device, descriptorPool, pAllocator);

        AutoLock lock(mLock);
        cleanupDescriptorPoolAllocedSetsLocked(descriptorPool, true /* destroy */);
        mDescriptorPoolInfo.erase(descriptorPool);
    }

    VkResult on_vkResetDescriptorPool(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        VkDescriptorPool descriptorPool,
        VkDescriptorPoolResetFlags flags) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto res = vk->vkResetDescriptorPool(device, descriptorPool, flags);

        if (res == VK_SUCCESS) {
            AutoLock lock(mLock);
            cleanupDescriptorPoolAllocedSetsLocked(descriptorPool);
        }

        return res;
    }

    void initDescriptorSetInfoLocked(
        VkDescriptorPool pool,
        VkDescriptorSetLayout setLayout,
        uint64_t boxedDescriptorSet,
        VkDescriptorSet descriptorSet) {

        auto poolInfo = android::base::find(mDescriptorPoolInfo, pool);

        auto setLayoutInfo =
            android::base::find(mDescriptorSetLayoutInfo, setLayout);

        auto& setInfo = mDescriptorSetInfo[descriptorSet];

        setInfo.pool = pool;
        setInfo.bindings = setLayoutInfo->bindings;

        poolInfo->allocedSetsToBoxed[descriptorSet] = (VkDescriptorSet)boxedDescriptorSet;
        applyDescriptorSetAllocationLocked(*poolInfo, setInfo.bindings);
    }

    VkResult on_vkAllocateDescriptorSets(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkDescriptorSetAllocateInfo* pAllocateInfo,
        VkDescriptorSet* pDescriptorSets) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto allocValidationRes = validateDescriptorSetAllocLocked(pAllocateInfo);
        if (allocValidationRes != VK_SUCCESS) return allocValidationRes;

        auto res = vk->vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

        if (res == VK_SUCCESS) {

            auto poolInfo = android::base::find(mDescriptorPoolInfo, pAllocateInfo->descriptorPool);
            if (!poolInfo) return res;

            for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; ++i) {
                auto unboxed = pDescriptorSets[i];
                pDescriptorSets[i] = new_boxed_non_dispatchable_VkDescriptorSet(pDescriptorSets[i]);
                initDescriptorSetInfoLocked(
                    pAllocateInfo->descriptorPool,
                    pAllocateInfo->pSetLayouts[i],
                    (uint64_t)(pDescriptorSets[i]),
                    unboxed);
            }
        }

        return res;
    }

    VkResult on_vkFreeDescriptorSets(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto res = vk->vkFreeDescriptorSets(
            device, descriptorPool,
            descriptorSetCount, pDescriptorSets);

        if (res == VK_SUCCESS) {
            AutoLock lock(mLock);

            for (uint32_t i = 0; i < descriptorSetCount; ++i) {
                auto setInfo = android::base::find(
                    mDescriptorSetInfo, pDescriptorSets[i]);

                if (!setInfo) continue;

                auto poolInfo =
                    android::base::find(
                        mDescriptorPoolInfo, setInfo->pool);

                if (!poolInfo) continue;

                removeDescriptorSetAllocationLocked(*poolInfo, setInfo->bindings);

                auto descSetAllocedEntry =
                    android::base::find(
                        poolInfo->allocedSetsToBoxed, pDescriptorSets[i]);

                if (!descSetAllocedEntry) continue;

                auto handleInfo = sBoxedHandleManager.get((uint64_t)*descSetAllocedEntry);
                if (handleInfo) {
                    if (emugl::emugl_feature_is_enabled(android::featurecontrol::VulkanBatchedDescriptorSetUpdate)) {
                        handleInfo->underlying = reinterpret_cast<uint64_t>(VK_NULL_HANDLE);
                    } else {
                        delete_VkDescriptorSet(*descSetAllocedEntry);
                    }
                }

                poolInfo->allocedSetsToBoxed.erase(pDescriptorSets[i]);

                mDescriptorSetInfo.erase(pDescriptorSets[i]);
            }
        }

        return res;
    }

    void on_vkUpdateDescriptorSets(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            uint32_t descriptorWriteCount,
            const VkWriteDescriptorSet* pDescriptorWrites,
            uint32_t descriptorCopyCount,
            const VkCopyDescriptorSet* pDescriptorCopies) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        on_vkUpdateDescriptorSetsImpl(pool, vk, device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }

    void on_vkUpdateDescriptorSetsImpl(
            android::base::BumpPool* pool,
            VulkanDispatch* vk,
            VkDevice device,
            uint32_t descriptorWriteCount,
            const VkWriteDescriptorSet* pDescriptorWrites,
            uint32_t descriptorCopyCount,
            const VkCopyDescriptorSet* pDescriptorCopies) {

        bool needEmulateWriteDescriptor = false;
        // c++ seems to allow for 0-size array allocation
        std::unique_ptr<bool[]> descriptorWritesNeedDeepCopy(
                new bool[descriptorWriteCount]);
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& descriptorWrite = pDescriptorWrites[i];
            descriptorWritesNeedDeepCopy[i] = false;
            if (descriptorWrite.descriptorType !=
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                continue;
            }
            for (uint32_t j = 0; j < descriptorWrite.descriptorCount; j++) {
                const VkDescriptorImageInfo& imageInfo =
                    descriptorWrite.pImageInfo[j];
                const auto& viewIt = mImageViewInfo.find(imageInfo.imageView);
                if (viewIt == mImageViewInfo.end()) {
                    continue;
                }
                const auto& samplerIt = mSamplerInfo.find(imageInfo.sampler);
                if (samplerIt == mSamplerInfo.end()) {
                    continue;
                }
                if (viewIt->second.needEmulatedAlpha &&
                        samplerIt->second.needEmulatedAlpha) {
                    needEmulateWriteDescriptor = true;
                    descriptorWritesNeedDeepCopy[i] = true;
                    break;
                }
            }
        }
        if (!needEmulateWriteDescriptor) {
            vk->vkUpdateDescriptorSets(device, descriptorWriteCount,
                    pDescriptorWrites, descriptorCopyCount,
                    pDescriptorCopies);
            return;
        }
        std::list<std::unique_ptr<VkDescriptorImageInfo[]>> imageInfoPool;
        std::unique_ptr<VkWriteDescriptorSet[]> descriptorWrites(
                new VkWriteDescriptorSet[descriptorWriteCount]);
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& srcDescriptorWrite =
                pDescriptorWrites[i];
            VkWriteDescriptorSet& dstDescriptorWrite = descriptorWrites[i];
            // Shallow copy first
            dstDescriptorWrite = srcDescriptorWrite;
            if (!descriptorWritesNeedDeepCopy[i]) {
                continue;
            }
            // Deep copy
            assert(dstDescriptorWrite.descriptorType ==
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            imageInfoPool.emplace_back(
                    new VkDescriptorImageInfo[dstDescriptorWrite
                    .descriptorCount]);
            VkDescriptorImageInfo* imageInfos = imageInfoPool.back().get();
            memcpy(imageInfos, srcDescriptorWrite.pImageInfo,
                    dstDescriptorWrite.descriptorCount *
                    sizeof(VkDescriptorImageInfo));
            dstDescriptorWrite.pImageInfo = imageInfos;
            for (uint32_t j = 0; j < dstDescriptorWrite.descriptorCount; j++) {
                VkDescriptorImageInfo& imageInfo = imageInfos[j];
                const auto& viewIt = mImageViewInfo.find(imageInfo.imageView);
                if (viewIt == mImageViewInfo.end()) {
                    continue;
                }
                const auto& samplerIt = mSamplerInfo.find(imageInfo.sampler);
                if (samplerIt == mSamplerInfo.end()) {
                    continue;
                }
                if (viewIt->second.needEmulatedAlpha &&
                        samplerIt->second.needEmulatedAlpha) {
                    SamplerInfo& samplerInfo = samplerIt->second;
                    if (samplerInfo.emulatedborderSampler == VK_NULL_HANDLE) {
                        // create the emulated sampler
                        VkSamplerCreateInfo createInfo = samplerInfo.createInfo;
                        switch (createInfo.borderColor) {
                            case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
                                createInfo.borderColor =
                                    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                                break;
                            case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
                                createInfo.borderColor =
                                    VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                                break;
                            default:
                                break;
                        }
                        vk->vkCreateSampler(device, &createInfo, nullptr,
                                &samplerInfo.emulatedborderSampler);
                    }
                    imageInfo.sampler = samplerInfo.emulatedborderSampler;
                }
            }
        }
        vk->vkUpdateDescriptorSets(device, descriptorWriteCount,
                descriptorWrites.get(), descriptorCopyCount,
                pDescriptorCopies);
    }

    void on_vkCmdCopyImage(android::base::BumpPool* pool,
                           VkCommandBuffer boxed_commandBuffer,
                           VkImage srcImage,
                           VkImageLayout srcImageLayout,
                           VkImage dstImage,
                           VkImageLayout dstImageLayout,
                           uint32_t regionCount,
                           const VkImageCopy* pRegions) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        AutoLock lock(mLock);
        auto srcIt = mImageInfo.find(srcImage);
        if (srcIt == mImageInfo.end()) {
            return;
        }
        auto dstIt = mImageInfo.find(dstImage);
        if (dstIt == mImageInfo.end()) {
            return;
        }
        VkDevice device = srcIt->second.cmpInfo.device;
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        bool needEmulatedSrc = deviceInfoIt->second.needEmulatedDecompression(
                srcIt->second.cmpInfo);
        bool needEmulatedDst = deviceInfoIt->second.needEmulatedDecompression(
                dstIt->second.cmpInfo);
        if (!needEmulatedSrc && !needEmulatedDst) {
            vk->vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout,
                               dstImage, dstImageLayout, regionCount, pRegions);
            return;
        }
        VkImage srcImageMip = srcImage;
        VkImage dstImageMip = dstImage;
        for (uint32_t r = 0; r < regionCount; r++) {
            VkImageCopy region = pRegions[r];
            if (needEmulatedSrc) {
                uint32_t mipLevel = region.srcSubresource.mipLevel;
                uint32_t compressedBlockWidth =
                        srcIt->second.cmpInfo.compressedBlockWidth;
                uint32_t compressedBlockHeight =
                        srcIt->second.cmpInfo.compressedBlockHeight;
                srcImageMip = srcIt->second.cmpInfo.sizeCompImgs[mipLevel];
                region.srcSubresource.mipLevel = 0;
                region.srcOffset.x /= compressedBlockWidth;
                region.srcOffset.y /= compressedBlockHeight;
                uint32_t width =
                        srcIt->second.cmpInfo.sizeCompMipmapWidth(mipLevel);
                uint32_t height =
                        srcIt->second.cmpInfo.sizeCompMipmapHeight(mipLevel);
                // region.extent uses pixel size for source image
                region.extent.width =
                        (region.extent.width + compressedBlockWidth - 1) /
                        compressedBlockWidth;
                region.extent.height =
                        (region.extent.height + compressedBlockHeight - 1) /
                        compressedBlockHeight;
                region.extent.width = std::min(region.extent.width, width);
                region.extent.height = std::min(region.extent.height, height);
            }
            if (needEmulatedDst) {
                uint32_t compressedBlockWidth =
                        dstIt->second.cmpInfo.compressedBlockWidth;
                uint32_t compressedBlockHeight =
                        dstIt->second.cmpInfo.compressedBlockHeight;
                uint32_t mipLevel = region.dstSubresource.mipLevel;
                dstImageMip = dstIt->second.cmpInfo.sizeCompImgs[mipLevel];
                region.dstSubresource.mipLevel = 0;
                region.dstOffset.x /= compressedBlockWidth;
                region.dstOffset.y /= compressedBlockHeight;
            }
            vk->vkCmdCopyImage(commandBuffer, srcImageMip, srcImageLayout,
                               dstImageMip, dstImageLayout, 1, &region);
        }
    }

    void on_vkCmdCopyImageToBuffer(android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkImage srcImage,
            VkImageLayout srcImageLayout,
            VkBuffer dstBuffer,
            uint32_t regionCount,
            const VkBufferImageCopy* pRegions) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(srcImage);
        if (it == mImageInfo.end()) {
            return;
        }
        auto bufferInfoIt = mBufferInfo.find(dstBuffer);
        if (bufferInfoIt == mBufferInfo.end()) {
            return;
        }
        VkDevice device = bufferInfoIt->second.device;
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        if (!deviceInfoIt->second.needEmulatedDecompression(
                    it->second.cmpInfo)) {
            vk->vkCmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout,
                    dstBuffer, regionCount, pRegions);
            return;
        }
        CompressedImageInfo& cmp = it->second.cmpInfo;
        for (uint32_t r = 0; r < regionCount; r++) {
            VkBufferImageCopy region;
            region = pRegions[r];
            uint32_t mipLevel = region.imageSubresource.mipLevel;
            region.imageSubresource.mipLevel = 0;
            region.bufferRowLength /= cmp.compressedBlockWidth;
            region.bufferImageHeight /= cmp.compressedBlockHeight;
            region.imageOffset.x /= cmp.compressedBlockWidth;
            region.imageOffset.y /= cmp.compressedBlockHeight;
            uint32_t width = cmp.sizeCompMipmapWidth(mipLevel);
            uint32_t height = cmp.sizeCompMipmapHeight(mipLevel);
            region.imageExtent.width =
                    (region.imageExtent.width + cmp.compressedBlockWidth - 1) /
                    cmp.compressedBlockWidth;
            region.imageExtent.height = (region.imageExtent.height +
                                         cmp.compressedBlockHeight - 1) /
                                        cmp.compressedBlockHeight;
            region.imageExtent.width =
                std::min(region.imageExtent.width, width);
            region.imageExtent.height =
                std::min(region.imageExtent.height, height);
            vk->vkCmdCopyImageToBuffer(commandBuffer,
                    cmp.sizeCompImgs[mipLevel],
                    srcImageLayout, dstBuffer, 1, &region);
        }
    }

    void on_vkGetImageMemoryRequirements(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkImage image,
            VkMemoryRequirements* pMemoryRequirements) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        vk->vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
        AutoLock lock(mLock);
        updateImageMemorySizeLocked(device, image, pMemoryRequirements);
    }

    void on_vkGetImageMemoryRequirements2(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkImageMemoryRequirementsInfo2* pInfo,
            VkMemoryRequirements2* pMemoryRequirements) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        AutoLock lock(mLock);

        auto physicalDevice = mDeviceToPhysicalDevice[device];
        auto physdevInfo = android::base::find(mPhysdevInfo, physicalDevice);

        if (!physdevInfo) {
            // If this fails, we crash, as we assume that the memory properties
            // map should have the info.
            emugl::emugl_crash_reporter(
                    "FATAL: Could not get image memory requirement for "
                    "VkPhysicalDevice");
        }

        if ((physdevInfo->props.apiVersion >= VK_MAKE_VERSION(1, 1, 0)) &&
            vk->vkGetImageMemoryRequirements2) {
            vk->vkGetImageMemoryRequirements2(device, pInfo,
                    pMemoryRequirements);
        } else if (hasDeviceExtension(device,
                    "VK_KHR_get_memory_requirements2")) {
            vk->vkGetImageMemoryRequirements2KHR(device, pInfo,
                    pMemoryRequirements);
        } else {
            if (pInfo->pNext) {
                fprintf(stderr,
                        "%s: Warning: Trying to use extension struct in "
                        "VkMemoryRequirements2 without having enabled "
                        "the extension!!!!11111\n",
                        __func__);
            }

            vk->vkGetImageMemoryRequirements(
                    device, pInfo->image,
                    &pMemoryRequirements->memoryRequirements);
        }
        updateImageMemorySizeLocked(device, pInfo->image,
                &pMemoryRequirements->memoryRequirements);
    }

    void on_vkCmdCopyBufferToImage(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkBuffer srcBuffer,
            VkImage dstImage,
            VkImageLayout dstImageLayout,
            uint32_t regionCount,
            const VkBufferImageCopy* pRegions) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        AutoLock lock(mLock);
        auto it = mImageInfo.find(dstImage);
        if (it == mImageInfo.end()) return;
        auto bufferInfoIt = mBufferInfo.find(srcBuffer);
        if (bufferInfoIt == mBufferInfo.end()) {
            return;
        }
        VkDevice device = bufferInfoIt->second.device;
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        if (!deviceInfoIt->second.needEmulatedDecompression(
                    it->second.cmpInfo)) {
            vk->vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage,
                    dstImageLayout, regionCount, pRegions);
            return;
        }
        auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
        if (cmdBufferInfoIt == mCmdBufferInfo.end()) {
            return;
        }
        CompressedImageInfo& cmp = it->second.cmpInfo;
        for (uint32_t r = 0; r < regionCount; r++) {
            VkBufferImageCopy dstRegion;
            dstRegion = pRegions[r];
            uint32_t mipLevel = dstRegion.imageSubresource.mipLevel;
            dstRegion.imageSubresource.mipLevel = 0;
            dstRegion.bufferRowLength /= cmp.compressedBlockWidth;
            dstRegion.bufferImageHeight /= cmp.compressedBlockHeight;
            dstRegion.imageOffset.x /= cmp.compressedBlockWidth;
            dstRegion.imageOffset.y /= cmp.compressedBlockHeight;
            uint32_t width = cmp.sizeCompMipmapWidth(mipLevel);
            uint32_t height = cmp.sizeCompMipmapHeight(mipLevel);
            dstRegion.imageExtent.width = (dstRegion.imageExtent.width +
                                           cmp.compressedBlockWidth - 1) /
                                          cmp.compressedBlockWidth;
            dstRegion.imageExtent.height = (dstRegion.imageExtent.height +
                                            cmp.compressedBlockHeight - 1) /
                                           cmp.compressedBlockHeight;
            dstRegion.imageExtent.width =
                std::min(dstRegion.imageExtent.width, width);
            dstRegion.imageExtent.height =
                std::min(dstRegion.imageExtent.height, height);
            vk->vkCmdCopyBufferToImage(commandBuffer, srcBuffer,
                    cmp.sizeCompImgs[mipLevel],
                    dstImageLayout, 1, &dstRegion);
        }
    }

    inline void convertQueueFamilyForeignToExternal(uint32_t* queueFamilyIndexPtr) {
        if (*queueFamilyIndexPtr == VK_QUEUE_FAMILY_FOREIGN_EXT) {
            *queueFamilyIndexPtr = VK_QUEUE_FAMILY_EXTERNAL;
        }
    }

    inline void convertQueueFamilyForeignToExternal_VkBufferMemoryBarrier(VkBufferMemoryBarrier* barrier) {
        convertQueueFamilyForeignToExternal(&barrier->srcQueueFamilyIndex);
        convertQueueFamilyForeignToExternal(&barrier->dstQueueFamilyIndex);
    }

    inline void convertQueueFamilyForeignToExternal_VkImageMemoryBarrier(VkImageMemoryBarrier* barrier) {
        convertQueueFamilyForeignToExternal(&barrier->srcQueueFamilyIndex);
        convertQueueFamilyForeignToExternal(&barrier->dstQueueFamilyIndex);
    }

    void on_vkCmdPipelineBarrier(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        for (uint32_t i = 0; i < bufferMemoryBarrierCount; ++i) {
            convertQueueFamilyForeignToExternal_VkBufferMemoryBarrier(
                ((VkBufferMemoryBarrier*)pBufferMemoryBarriers) + i);
        }

        for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i) {
            convertQueueFamilyForeignToExternal_VkImageMemoryBarrier(
                ((VkImageMemoryBarrier*)pImageMemoryBarriers) + i);
        }

        if (imageMemoryBarrierCount == 0) {
            vk->vkCmdPipelineBarrier(
                    commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                    memoryBarrierCount, pMemoryBarriers,
                    bufferMemoryBarrierCount, pBufferMemoryBarriers,
                    imageMemoryBarrierCount, pImageMemoryBarriers);
            return;
        }
        AutoLock lock(mLock);
        auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
        if (cmdBufferInfoIt == mCmdBufferInfo.end()) {
            return;
        }
        auto deviceInfoIt = mDeviceInfo.find(cmdBufferInfoIt->second.device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return;
        }
        if (!deviceInfoIt->second.emulateTextureEtc2 &&
            !deviceInfoIt->second.emulateTextureAstc) {
            vk->vkCmdPipelineBarrier(
                    commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                    memoryBarrierCount, pMemoryBarriers,
                    bufferMemoryBarrierCount, pBufferMemoryBarriers,
                    imageMemoryBarrierCount, pImageMemoryBarriers);
            return;
        }
        // Add barrier for decompressed image
        std::vector<VkImageMemoryBarrier> persistentImageBarriers;
        bool needRebind = false;
        for (uint32_t i = 0; i < imageMemoryBarrierCount; i++) {
            const VkImageMemoryBarrier& srcBarrier = pImageMemoryBarriers[i];
            auto image = srcBarrier.image;
            auto it = mImageInfo.find(image);
            if (it == mImageInfo.end() ||
                !deviceInfoIt->second.needEmulatedDecompression(
                        it->second.cmpInfo)) {
                persistentImageBarriers.push_back(srcBarrier);
                continue;
            }
            uint32_t baseMipLevel = srcBarrier.subresourceRange.baseMipLevel;
            uint32_t levelCount = srcBarrier.subresourceRange.levelCount;
            VkImageMemoryBarrier decompBarrier = srcBarrier;
            decompBarrier.image = it->second.cmpInfo.decompImg;
            VkImageMemoryBarrier sizeCompBarrierTemplate = srcBarrier;
            sizeCompBarrierTemplate.subresourceRange.baseMipLevel = 0;
            sizeCompBarrierTemplate.subresourceRange.levelCount = 1;
            std::vector<VkImageMemoryBarrier> sizeCompBarriers(
                    srcBarrier.subresourceRange.levelCount,
                    sizeCompBarrierTemplate);
            for (uint32_t j = 0; j < levelCount; j++) {
                sizeCompBarriers[j].image =
                    it->second.cmpInfo.sizeCompImgs[baseMipLevel + j];
            }

            // TODO: should we use image layout or access bit?
            if (srcBarrier.oldLayout == 0 ||
                    (srcBarrier.newLayout != VK_IMAGE_LAYOUT_GENERAL &&
                     srcBarrier.newLayout !=
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
                // TODO: might only need to push one of them?
                persistentImageBarriers.push_back(decompBarrier);
                persistentImageBarriers.insert(persistentImageBarriers.end(),
                        sizeCompBarriers.begin(),
                        sizeCompBarriers.end());
                continue;
            }
            if (srcBarrier.newLayout !=
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                    srcBarrier.newLayout != VK_IMAGE_LAYOUT_GENERAL) {
                dwarning("unexpected usage to transfer "
                        "compressed image layout from %d to %d",
                        srcBarrier.oldLayout, srcBarrier.newLayout);
            }

            VkResult result = it->second.cmpInfo.initDecomp(
                    vk, cmdBufferInfoIt->second.device, image);
            if (result != VK_SUCCESS) {
                dwarning(" texture decompression failed");
                continue;
            }

            std::vector<VkImageMemoryBarrier> currImageBarriers;
            currImageBarriers.reserve(sizeCompBarriers.size() + 1);
            currImageBarriers.insert(currImageBarriers.end(),
                    sizeCompBarriers.begin(),
                    sizeCompBarriers.end());
            for (auto& barrier : currImageBarriers) {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            currImageBarriers.push_back(decompBarrier);
            {
                VkImageMemoryBarrier& barrier = currImageBarriers[levelCount];
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            vk->vkCmdPipelineBarrier(commandBuffer, srcStageMask,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                    nullptr, 0, nullptr,
                    currImageBarriers.size(),
                    currImageBarriers.data());
            it->second.cmpInfo.cmdDecompress(
                    vk, commandBuffer, dstStageMask, decompBarrier.newLayout,
                    decompBarrier.dstAccessMask, baseMipLevel, levelCount,
                    srcBarrier.subresourceRange.baseArrayLayer,
                    srcBarrier.subresourceRange.layerCount);
            needRebind = true;

            for (uint32_t j = 0; j < currImageBarriers.size(); j++) {
                VkImageMemoryBarrier& barrier = currImageBarriers[j];
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = srcBarrier.dstAccessMask;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = srcBarrier.newLayout;
            }
            {
                VkImageMemoryBarrier& barrier = currImageBarriers[levelCount];
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = srcBarrier.dstAccessMask;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = srcBarrier.newLayout;
            }

            vk->vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                    dstStageMask,                          // dstStageMask
                    0,                                     // dependencyFlags
                    0,                                     // memoryBarrierCount
                    nullptr,                               // pMemoryBarriers
                    0,                         // bufferMemoryBarrierCount
                    nullptr,                   // pBufferMemoryBarriers
                    currImageBarriers.size(),  // imageMemoryBarrierCount
                    currImageBarriers.data()   // pImageMemoryBarriers
                    );
        }
        if (needRebind && cmdBufferInfoIt->second.computePipeline) {
            // Recover pipeline bindings
            vk->vkCmdBindPipeline(commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    cmdBufferInfoIt->second.computePipeline);
            if (cmdBufferInfoIt->second.descriptorSets.size() > 0) {
                vk->vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        cmdBufferInfoIt->second.descriptorLayout,
                        cmdBufferInfoIt->second.firstSet,
                        cmdBufferInfoIt->second.descriptorSets.size(),
                        cmdBufferInfoIt->second.descriptorSets.data(), 0,
                        nullptr);
            }
        }
        if (memoryBarrierCount || bufferMemoryBarrierCount ||
                !persistentImageBarriers.empty()) {
            vk->vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask,
                    dependencyFlags, memoryBarrierCount,
                    pMemoryBarriers, bufferMemoryBarrierCount,
                    pBufferMemoryBarriers,
                    persistentImageBarriers.size(),
                    persistentImageBarriers.data());
        }
    }

    bool mapHostVisibleMemoryToGuestPhysicalAddressLocked(
            VulkanDispatch* vk,
            VkDevice device,
            VkDeviceMemory memory,
            uint64_t physAddr) {

        if (!emugl::emugl_feature_is_enabled(
                    android::featurecontrol::GLDirectMem) &&
            !emugl::emugl_feature_is_enabled(
                    android::featurecontrol::VirtioGpuNext)) {
            emugl::emugl_crash_reporter(
                    "FATAL: Tried to use direct mapping "
                    "while GLDirectMem is not enabled!");
        }

        auto info = android::base::find(mMapInfo, memory);

        if (!info) return false;

        info->guestPhysAddr = physAddr;

#if defined(__APPLE__) && defined(__aarch64__)
        constexpr size_t kPageBits = 14;
#else
        constexpr size_t kPageBits = 12;
#endif
        constexpr size_t kPageSize = 1u << kPageBits;
        constexpr size_t kPageOffsetMask = kPageSize - 1;

        uintptr_t addr = reinterpret_cast<uintptr_t>(info->ptr);
        uintptr_t pageOffset = addr & kPageOffsetMask;

        info->pageAlignedHva =
            reinterpret_cast<void*>(addr - pageOffset);
        info->sizeToPage =
            ((info->size + pageOffset + kPageSize - 1) >>
             kPageBits) << kPageBits;

        if (mLogging) {
            fprintf(stderr, "%s: map: %p, %p -> [0x%llx 0x%llx]\n", __func__,
                    info->ptr,
                    info->pageAlignedHva,
                    (unsigned long long)info->guestPhysAddr,
                    (unsigned long long)info->guestPhysAddr + info->sizeToPage);
        }

        info->directMapped = true;
        uint64_t gpa = info->guestPhysAddr;
        void* hva = info->pageAlignedHva;
        size_t sizeToPage = info->sizeToPage;

        AutoLock occupiedGpasLock(mOccupiedGpasLock);

        auto existingMemoryInfo =
            android::base::find(mOccupiedGpas, gpa);

        if (existingMemoryInfo) {

            dwarning("%s: already mapped gpa 0x%llx, replacing",
                    __func__,
                    (unsigned long long)gpa);

            get_emugl_vm_operations().unmapUserBackedRam(
                existingMemoryInfo->gpa,
                existingMemoryInfo->sizeToPage);

            mOccupiedGpas.erase(gpa);
        }

        get_emugl_vm_operations().mapUserBackedRam(
            gpa, hva, sizeToPage);

        if (mVerbosePrints) {
            fprintf(stderr, "VERBOSE:%s: registering gpa 0x%llx to mOccupiedGpas\n", __func__,
                    (unsigned long long)gpa);
        }

        mOccupiedGpas[gpa] = {
            vk,
            device,
            memory,
            gpa,
            sizeToPage,
        };

        if (!mUseOldMemoryCleanupPath) {

            if (mVerbosePrints) {
                fprintf(stderr, "%s: Registering deallocation callback for host-visible memory at gpa 0x%llx\n", __func__,
                        (unsigned long long)gpa);
            }

            get_emugl_address_space_device_control_ops().register_deallocation_callback(
                this, gpa, [](void* thisPtr, uint64_t gpa) {
                Impl* implPtr = (Impl*)thisPtr;
                implPtr->unmapMemoryAtGpaIfExists(gpa);
            });
        }

        return true;
    }

    // Only call this from the address space device deallocation operation's
    // context, or it's possible that the guest/host view of which gpa's are
    // occupied goes out of sync.
    void unmapMemoryAtGpaIfExists(uint64_t gpa) {
        AutoLock lock(mOccupiedGpasLock);

        if (mVerbosePrints) {
            fprintf(stderr, "VERBOSE:%s: deallocation callback for gpa 0x%llx\n", __func__,
                    (unsigned long long)gpa);
        }

        auto existingMemoryInfo =
            android::base::find(mOccupiedGpas, gpa);

        if (!existingMemoryInfo) return;

        get_emugl_vm_operations().unmapUserBackedRam(
            existingMemoryInfo->gpa,
            existingMemoryInfo->sizeToPage);

        mOccupiedGpas.erase(gpa);
    }

    VkResult on_vkAllocateMemory(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkMemoryAllocateInfo* pAllocateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDeviceMemory* pMemory) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!pAllocateInfo) return VK_ERROR_INITIALIZATION_FAILED;

        VkMemoryAllocateInfo localAllocInfo = vk_make_orphan_copy(*pAllocateInfo);
        vk_struct_chain_iterator structChainIter = vk_make_chain_iterator(&localAllocInfo);

        VkMemoryAllocateFlagsInfo allocFlagsInfo;
        VkMemoryOpaqueCaptureAddressAllocateInfo opaqueCaptureAddressAllocInfo;
        VkExportMemoryAllocateInfo exportAllocInfo;
        VkImportColorBufferGOOGLE importCbInfo;
        VkImportPhysicalAddressGOOGLE importPhysAddrInfo;
        VkMemoryDedicatedAllocateInfo dedicatedAllocInfo;

        // Add buffer device address capture structs
        const VkMemoryAllocateFlagsInfo* allocFlagsInfoPtr =
            vk_find_struct<VkMemoryAllocateFlagsInfo>(pAllocateInfo);
        const VkMemoryOpaqueCaptureAddressAllocateInfo* opaqueCaptureAddressAllocInfoPtr =
            vk_find_struct<VkMemoryOpaqueCaptureAddressAllocateInfo>(pAllocateInfo);

        if (allocFlagsInfoPtr) {
            allocFlagsInfo = *allocFlagsInfoPtr;
            vk_append_struct(&structChainIter, &allocFlagsInfo);
        }

        if (opaqueCaptureAddressAllocInfoPtr) {
            opaqueCaptureAddressAllocInfo = *opaqueCaptureAddressAllocInfoPtr;
            vk_append_struct(&structChainIter, &opaqueCaptureAddressAllocInfo);
        }

        // handle type should already be converted in unmarshaling
        const VkExportMemoryAllocateInfo* exportAllocInfoPtr =
            vk_find_struct<VkExportMemoryAllocateInfo>(pAllocateInfo);

        if (exportAllocInfoPtr) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER)) <<
                "Export allocs are to be handled on the guest side / VkCommonOperations.";
        }

        const VkMemoryDedicatedAllocateInfo* dedicatedAllocInfoPtr =
            vk_find_struct<VkMemoryDedicatedAllocateInfo>(pAllocateInfo);
        VkMemoryDedicatedAllocateInfo localDedicatedAllocInfo;

        if (dedicatedAllocInfoPtr) {
            localDedicatedAllocInfo = vk_make_orphan_copy(*dedicatedAllocInfoPtr);
            vk_append_struct(&structChainIter, &localDedicatedAllocInfo);
        }

        const VkImportPhysicalAddressGOOGLE* importPhysAddrInfoPtr =
            vk_find_struct<VkImportPhysicalAddressGOOGLE>(pAllocateInfo);

        if (importPhysAddrInfoPtr) {
            // TODO: Implement what happens on importing a physical address:
            // 1 - perform action of vkMapMemoryIntoAddressSpaceGOOGLE if
            //     host visible
            // 2 - create color buffer, setup Vk for it,
            //     and associate it with the physical address
        }

        const VkImportColorBufferGOOGLE* importCbInfoPtr =
            vk_find_struct<VkImportColorBufferGOOGLE>(pAllocateInfo);
        const VkImportBufferGOOGLE* importBufferInfoPtr =
                vk_find_struct<VkImportBufferGOOGLE>(pAllocateInfo);

#ifdef _WIN32
        VkImportMemoryWin32HandleInfoKHR importInfo {
            VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, 0,
                VK_EXT_MEMORY_HANDLE_TYPE_BIT,
                VK_EXT_MEMORY_HANDLE_INVALID, L"",
        };
#else
        VkImportMemoryFdInfoKHR importInfo {
            VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, 0,
                VK_EXT_MEMORY_HANDLE_TYPE_BIT,
                VK_EXT_MEMORY_HANDLE_INVALID,
        };
#endif

        AutoLock lock(mLock);

        auto physdev = android::base::find(mDeviceToPhysicalDevice, device);

        if (!physdev) {
            // User app gave an invalid VkDevice,
            // but we don't really want to crash here.
            // We should allow invalid apps.
            return VK_ERROR_DEVICE_LOST;
        }

        auto physdevInfo = android::base::find(mPhysdevInfo, *physdev);

        if (!physdevInfo) {
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
        if (localAllocInfo.memoryTypeIndex >=
            physdevInfo->memoryProperties.memoryTypeCount) {
            // Continue allowing invalid behavior.
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        }

        VkMemoryPropertyFlags memoryPropertyFlags =
                physdevInfo->memoryProperties
                        .memoryTypes[localAllocInfo.memoryTypeIndex]
                        .propertyFlags;

        lock.unlock();

        void* mappedPtr = nullptr;
        if (importCbInfoPtr) {
            bool vulkanOnly = mGuestUsesAngle;

            // Ensure color buffer has Vulkan backing.
            setupVkColorBuffer(importCbInfoPtr->colorBuffer,
                               vulkanOnly,
                               0u /* memoryProperty */, nullptr,
                               // Modify the allocation size and type index
                               // to suit the resulting image memory size.
                               &localAllocInfo.allocationSize,
                               &localAllocInfo.memoryTypeIndex, &mappedPtr);

            if (!vulkanOnly) {
                updateVkImageFromColorBuffer(importCbInfoPtr->colorBuffer);
            }

            if (m_emu->instanceSupportsExternalMemoryCapabilities) {
                VK_EXT_MEMORY_HANDLE cbExtMemoryHandle =
                    getColorBufferExtMemoryHandle(importCbInfoPtr->colorBuffer);

                if (cbExtMemoryHandle == VK_EXT_MEMORY_HANDLE_INVALID) {
                    fprintf(stderr,
                            "%s: VK_ERROR_OUT_OF_DEVICE_MEMORY: "
                            "colorBuffer 0x%x does not have Vulkan external memory backing\n", __func__,
                            importCbInfoPtr->colorBuffer);
                    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
                }

                cbExtMemoryHandle = dupExternalMemory(cbExtMemoryHandle);

#ifdef _WIN32
                importInfo.handle = cbExtMemoryHandle;
#else
                importInfo.fd = cbExtMemoryHandle;
#endif
                vk_append_struct(&structChainIter, &importInfo);
            }
        }

        if (importBufferInfoPtr) {
            // Ensure buffer has Vulkan backing.
            setupVkBuffer(importBufferInfoPtr->buffer,
                          true /* Buffers are Vulkan only */,
                          0u /* memoryProperty */, nullptr,
                          // Modify the allocation size and type index
                          // to suit the resulting image memory size.
                          &localAllocInfo.allocationSize,
                          &localAllocInfo.memoryTypeIndex);

            if (m_emu->instanceSupportsExternalMemoryCapabilities) {
                VK_EXT_MEMORY_HANDLE bufferExtMemoryHandle =
                        getBufferExtMemoryHandle(importBufferInfoPtr->buffer);

                if (bufferExtMemoryHandle == VK_EXT_MEMORY_HANDLE_INVALID) {
                    fprintf(stderr,
                            "%s: VK_ERROR_OUT_OF_DEVICE_MEMORY: "
                            "buffer 0x%x does not have Vulkan external memory "
                            "backing\n",
                            __func__, importBufferInfoPtr->buffer);
                    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
                }

                bufferExtMemoryHandle =
                        dupExternalMemory(bufferExtMemoryHandle);

#ifdef _WIN32
                importInfo.handle = bufferExtMemoryHandle;
#else
                importInfo.fd = bufferExtMemoryHandle;
#endif
                vk_append_struct(&structChainIter, &importInfo);
            }
        }

        VkResult result =
            vk->vkAllocateMemory(device, &localAllocInfo, pAllocator, pMemory);

        if (result != VK_SUCCESS) {
            return result;
        }

        lock.lock();

        mMapInfo[*pMemory] = MappedMemoryInfo();
        auto& mapInfo = mMapInfo[*pMemory];
        mapInfo.size = localAllocInfo.allocationSize;
        mapInfo.device = device;
        if (importCbInfoPtr && m_emu->instanceSupportsMoltenVK) {
            mapInfo.mtlTexture = getColorBufferMTLTexture(importCbInfoPtr->colorBuffer);
        }

        bool hostVisible =
                memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        if (!hostVisible) {
            *pMemory = new_boxed_non_dispatchable_VkDeviceMemory(*pMemory);
            return result;
        }

        if (mappedPtr) {
            mapInfo.needUnmap = false;
            mapInfo.ptr = mappedPtr;
        } else {
            mapInfo.needUnmap = true;
            VkResult mapResult = vk->vkMapMemory(device, *pMemory, 0,
                                                 mapInfo.size, 0, &mapInfo.ptr);
            if (mapResult != VK_SUCCESS) {
                return VK_ERROR_OUT_OF_HOST_MEMORY;
            }
        }

        *pMemory = new_boxed_non_dispatchable_VkDeviceMemory(*pMemory);

        return result;
    }

    void freeMemoryLocked(
            VulkanDispatch* vk,
            VkDevice device,
            VkDeviceMemory memory,
            const VkAllocationCallbacks* pAllocator) {

        auto info = android::base::find(mMapInfo, memory);

        if (!info) {
            // Invalid usage.
            return;
        }

#ifdef __APPLE__
        if (info->mtlTexture) {
            CFRelease(info->mtlTexture);
            info->mtlTexture = nullptr;
        }
#endif

        if (info->directMapped) {

            // if direct mapped, we leave it up to the guest address space driver
            // to control the unmapping of kvm slot on the host side
            // in order to avoid situations where
            //
            // 1. we try to unmap here and deadlock
            //
            // 2. unmapping at the wrong time (possibility of a parallel call
            // to unmap vs. address space allocate and mapMemory leading to
            // mapping the same gpa twice)
            if (mUseOldMemoryCleanupPath) {
                unmapMemoryAtGpaIfExists(info->guestPhysAddr);
            }
        }

        if (info->virtioGpuMapped) {
            if (mLogging) {
                fprintf(stderr, "%s: unmap hostmem %p id 0x%llx\n", __func__,
                        info->ptr,
                        (unsigned long long)info->hostmemId);
            }

            get_emugl_vm_operations().hostmemUnregister(info->hostmemId);
        }

        if (info->needUnmap && info->ptr) {
            vk->vkUnmapMemory(device, memory);
        }

        vk->vkFreeMemory(device, memory, pAllocator);

        mMapInfo.erase(memory);
    }

    void on_vkFreeMemory(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkDeviceMemory memory,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        freeMemoryLocked(vk, device, memory, pAllocator);
    }

    VkResult on_vkMapMemory(
            android::base::BumpPool* pool,
            VkDevice,
            VkDeviceMemory memory,
            VkDeviceSize offset,
            VkDeviceSize size,
            VkMemoryMapFlags flags,
            void** ppData) {

        AutoLock lock(mLock);
        return on_vkMapMemoryLocked(0, memory, offset, size, flags, ppData);
    }
    VkResult on_vkMapMemoryLocked(VkDevice,
            VkDeviceMemory memory,
            VkDeviceSize offset,
            VkDeviceSize size,
            VkMemoryMapFlags flags,
            void** ppData) {
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

    void on_vkUnmapMemory(
            android::base::BumpPool* pool,
            VkDevice, VkDeviceMemory) {
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

    bool usingDirectMapping() const {
        return emugl::emugl_feature_is_enabled(
                android::featurecontrol::GLDirectMem) ||
               emugl::emugl_feature_is_enabled(
                android::featurecontrol::VirtioGpuNext);
    }

    HostFeatureSupport getHostFeatureSupport() const {
        HostFeatureSupport res;

        if (!m_vk) return res;

        auto emu = getGlobalVkEmulation();

        res.supportsVulkan = emu && emu->live;

        if (!res.supportsVulkan) return res;

        const auto& props = emu->deviceInfo.physdevProps;

        res.supportsVulkan1_1 =
            props.apiVersion >= VK_API_VERSION_1_1;
        res.supportsExternalMemory =
            emu->deviceInfo.supportsExternalMemory;
        res.useDeferredCommands =
            emu->useDeferredCommands;
        res.useCreateResourcesWithRequirements =
            emu->useCreateResourcesWithRequirements;

        res.apiVersion = props.apiVersion;
        res.driverVersion = props.driverVersion;
        res.deviceID = props.deviceID;
        res.vendorID = props.vendorID;
        return res;
    }

    bool hasInstanceExtension(VkInstance instance, const std::string& name) {
        auto info = android::base::find(mInstanceInfo, instance);
        if (!info) return false;

        for (const auto& enabledName : info->enabledExtensionNames) {
            if (name == enabledName) return true;
        }

        return false;
    }

    bool hasDeviceExtension(VkDevice device, const std::string& name) {
        auto info = android::base::find(mDeviceInfo, device);
        if (!info) return false;

        for (const auto& enabledName : info->enabledExtensionNames) {
            if (name == enabledName) return true;
        }

        return false;
    }

    // VK_ANDROID_native_buffer
    VkResult on_vkGetSwapchainGrallocUsageANDROID(
            android::base::BumpPool* pool,
            VkDevice,
            VkFormat format,
            VkImageUsageFlags imageUsage,
            int* grallocUsage) {
        getGralloc0Usage(format, imageUsage, grallocUsage);
        return VK_SUCCESS;
    }

    VkResult on_vkGetSwapchainGrallocUsage2ANDROID(
            android::base::BumpPool* pool,
            VkDevice,
            VkFormat format,
            VkImageUsageFlags imageUsage,
            VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
            uint64_t* grallocConsumerUsage,
            uint64_t* grallocProducerUsage) {
        getGralloc1Usage(format, imageUsage, swapchainImageUsage,
                grallocConsumerUsage,
                grallocProducerUsage);
        return VK_SUCCESS;
    }

    VkResult on_vkAcquireImageANDROID(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkImage image,
            int nativeFenceFd,
            VkSemaphore semaphore,
            VkFence fence) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto imageInfo = android::base::find(mImageInfo, image);
        if (!imageInfo) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        VkQueue defaultQueue;
        uint32_t defaultQueueFamilyIndex;
        if (!getDefaultQueueForDeviceLocked(
                    device, &defaultQueue, &defaultQueueFamilyIndex)) {
            fprintf(stderr, "%s: cant get the default q\n", __func__);
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        AndroidNativeBufferInfo* anbInfo = &imageInfo->anbInfo;

        return
            setAndroidNativeImageSemaphoreSignaled(
                    vk, device,
                    defaultQueue, defaultQueueFamilyIndex,
                    semaphore, fence, anbInfo);
    }

    VkResult on_vkQueueSignalReleaseImageANDROID(
            android::base::BumpPool* pool,
            VkQueue boxed_queue,
            uint32_t waitSemaphoreCount,
            const VkSemaphore* pWaitSemaphores,
            VkImage image,
            int* pNativeFenceFd) {

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        AutoLock lock(mLock);

        auto queueFamilyIndex = queueFamilyIndexOfQueueLocked(queue);

        if (!queueFamilyIndex) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto imageInfo = android::base::find(mImageInfo, image);
        AndroidNativeBufferInfo* anbInfo = &imageInfo->anbInfo;

        return
            syncImageToColorBuffer(
                    vk,
                    *queueFamilyIndex,
                    queue,
                    waitSemaphoreCount, pWaitSemaphores,
                    pNativeFenceFd, anbInfo);
    }

    VkResult on_vkMapMemoryIntoAddressSpaceGOOGLE(
            android::base::BumpPool* pool,
            VkDevice boxed_device, VkDeviceMemory memory, uint64_t* pAddress) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!emugl::emugl_feature_is_enabled(
                    android::featurecontrol::GLDirectMem)) {
            emugl::emugl_crash_reporter(
                    "FATAL: Tried to use direct mapping "
                    "while GLDirectMem is not enabled!");
        }

        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

        if (mLogging) {
            fprintf(stderr, "%s: deviceMemory: 0x%llx pAddress: 0x%llx\n", __func__,
                    (unsigned long long)memory,
                    (unsigned long long)(*pAddress));
        }

        if (!mapHostVisibleMemoryToGuestPhysicalAddressLocked(
                    vk, device, memory, *pAddress)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if (!info) return VK_ERROR_INITIALIZATION_FAILED;

        *pAddress = (uint64_t)(uintptr_t)info->ptr;

        return VK_SUCCESS;
    }

    VkResult on_vkGetMemoryHostAddressInfoGOOGLE(
            android::base::BumpPool* pool,
            VkDevice boxed_device, VkDeviceMemory memory,
            uint64_t* pAddress, uint64_t* pSize, uint64_t* pHostmemId) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);

        auto info = android::base::find(mMapInfo, memory);

        if (!info) return VK_ERROR_OUT_OF_HOST_MEMORY;

        uint64_t hva = (uint64_t)(uintptr_t)(info->ptr);
        uint64_t size = (uint64_t)(uintptr_t)(info->size);

#if defined(__APPLE__) && defined(__aarch64__)
        constexpr size_t kPageBits = 14;
#else
        constexpr size_t kPageBits = 12;
#endif
        constexpr size_t kPageSize = 1u << kPageBits;
        constexpr size_t kPageOffsetMask = kPageSize - 1;

        uint64_t pageOffset = hva & kPageOffsetMask;
        uint64_t sizeToPage =
            ((size + pageOffset + kPageSize - 1) >>
             kPageBits) << kPageBits;

        auto id =
            get_emugl_vm_operations().hostmemRegister(
                    (uint64_t)(uintptr_t)(info->ptr),
                    (uint64_t)(uintptr_t)(info->size),
                    // No fixed registration
                    false, 0);

        *pAddress = hva & (0xfff); // Don't expose exact hva to guest
        *pSize = sizeToPage;
        *pHostmemId = id;

        info->virtioGpuMapped = true;
        info->hostmemId = id;

        fprintf(stderr, "%s: hva, size, sizeToPage: %p 0x%llx 0x%llx id 0x%llx\n", __func__,
                info->ptr, (unsigned long long)(info->size),
                (unsigned long long)(sizeToPage),
                (unsigned long long)(*pHostmemId));
        return VK_SUCCESS;
    }

    VkResult on_vkFreeMemorySyncGOOGLE(
        android::base::BumpPool* pool,
        VkDevice boxed_device, VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {

        on_vkFreeMemory(pool, boxed_device, memory, pAllocator);

        return VK_SUCCESS;
    }


    VkResult on_vkRegisterImageColorBufferGOOGLE(
            android::base::BumpPool* pool,
            VkDevice device, VkImage image, uint32_t colorBuffer) {

        (void)image;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkRegisterBufferColorBufferGOOGLE(
            android::base::BumpPool* pool,
            VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {

        (void)buffer;

        bool success = setupVkColorBuffer(colorBuffer);

        return success ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    VkResult on_vkAllocateCommandBuffers(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result = vk->vkAllocateCommandBuffers(
                device, pAllocateInfo, pCommandBuffers);

        if (result != VK_SUCCESS) {
            return result;
        }

        AutoLock lock(mLock);
        for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
            mCmdBufferInfo[pCommandBuffers[i]] = CommandBufferInfo();
            mCmdBufferInfo[pCommandBuffers[i]].device = device;
            mCmdBufferInfo[pCommandBuffers[i]].cmdPool =
                pAllocateInfo->commandPool;
            auto boxed = new_boxed_VkCommandBuffer(pCommandBuffers[i], vk, false /* does not own dispatch */);
            mCmdBufferInfo[pCommandBuffers[i]].boxed = boxed;
            pCommandBuffers[i] = (VkCommandBuffer)boxed;
        }
        return result;
    }

    VkResult on_vkCreateCommandPool(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkCommandPoolCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkCommandPool* pCommandPool) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result = vk->vkCreateCommandPool(device, pCreateInfo,
                pAllocator, pCommandPool);
        if (result != VK_SUCCESS) {
            return result;
        }
        AutoLock lock(mLock);
        mCmdPoolInfo[*pCommandPool] = CommandPoolInfo();
        auto& cmdPoolInfo = mCmdPoolInfo[*pCommandPool];
        cmdPoolInfo.device = device;

        *pCommandPool = new_boxed_non_dispatchable_VkCommandPool(*pCommandPool);
        cmdPoolInfo.boxed = *pCommandPool;

        return result;
    }

    void on_vkDestroyCommandPool(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkCommandPool commandPool,
            const VkAllocationCallbacks* pAllocator) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyCommandPool(device, commandPool, pAllocator);
        AutoLock lock(mLock);
        const auto ite = mCmdPoolInfo.find(commandPool);
        if (ite != mCmdPoolInfo.end()) {
            removeCommandBufferInfo(ite->second.cmdBuffers);
            mCmdPoolInfo.erase(ite);
        }
    }

    VkResult on_vkResetCommandPool(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkCommandPool commandPool,
            VkCommandPoolResetFlags flags) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        VkResult result =
            vk->vkResetCommandPool(device, commandPool, flags);
        if (result != VK_SUCCESS) {
            return result;
        }
        return result;
    }

    void on_vkCmdExecuteCommands(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            uint32_t commandBufferCount,
            const VkCommandBuffer* pCommandBuffers) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        vk->vkCmdExecuteCommands(commandBuffer, commandBufferCount,
                pCommandBuffers);
        AutoLock lock(mLock);
        CommandBufferInfo& cmdBuffer = mCmdBufferInfo[commandBuffer];
        cmdBuffer.subCmds.insert(cmdBuffer.subCmds.end(),
                pCommandBuffers, pCommandBuffers + commandBufferCount);
    }

    VkResult on_vkQueueSubmit(
            android::base::BumpPool* pool,
            VkQueue boxed_queue,
            uint32_t submitCount,
            const VkSubmitInfo* pSubmits,
            VkFence fence) {

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        AutoLock lock(mLock);

        {
            auto queueInfo = android::base::find(mQueueInfo, queue);
            if (queueInfo) {
                VkDevice device = queueInfo->device;
                sBoxedHandleManager.processDelayedRemovesGlobalStateLocked(
                        queueInfo->device);
            }
        }

        for (uint32_t i = 0; i < submitCount; i++) {
            const VkSubmitInfo& submit = pSubmits[i];
            for (uint32_t c = 0; c < submit.commandBufferCount; c++) {
                executePreprocessRecursive(0, submit.pCommandBuffers[c]);
            }
        }

        auto queueInfo = android::base::find(mQueueInfo, queue);
        if (!queueInfo) return VK_SUCCESS;
        Lock* ql = queueInfo->lock;
        lock.unlock();

        AutoLock qlock(*ql);

        auto result = vk->vkQueueSubmit(queue, submitCount, pSubmits, fence);

        // After vkQueueSubmit is called, we can signal the conditional variable
        // in FenceInfo, so that other threads (e.g. SyncThread) can call
        // waitForFence() on this fence.
        lock.lock();
        auto fenceInfo = mFenceInfo.find(fence);
        if (fenceInfo != mFenceInfo.end()) {
            fenceInfo->second.state = FenceInfo::State::kWaitable;
            fenceInfo->second.lock.lock();
            fenceInfo->second.cv.signalAndUnlock(&fenceInfo->second.lock);
        }
        lock.unlock();

        return result;
    }

    VkResult on_vkQueueWaitIdle(
            android::base::BumpPool* pool,
            VkQueue boxed_queue) {

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        if (!queue) return VK_SUCCESS;

        AutoLock lock(mLock);
        auto queueInfo = android::base::find(mQueueInfo, queue);
        if (!queueInfo) return VK_SUCCESS;
        Lock* ql = queueInfo->lock;
        lock.unlock();

        AutoLock qlock(*ql);
        return vk->vkQueueWaitIdle(queue);
    }

    VkResult on_vkResetCommandBuffer(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkCommandBufferResetFlags flags) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        VkResult result = vk->vkResetCommandBuffer(commandBuffer, flags);
        if (VK_SUCCESS == result) {
            AutoLock lock(mLock);
            mCmdBufferInfo[commandBuffer].preprocessFuncs.clear();
            mCmdBufferInfo[commandBuffer].subCmds.clear();
            mCmdBufferInfo[commandBuffer].computePipeline = 0;
            mCmdBufferInfo[commandBuffer].firstSet = 0;
            mCmdBufferInfo[commandBuffer].descriptorLayout = 0;
            mCmdBufferInfo[commandBuffer].descriptorSets.clear();
        }
        return result;
    }

    void on_vkFreeCommandBuffers(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkCommandPool commandPool,
            uint32_t commandBufferCount,
            const VkCommandBuffer* pCommandBuffers) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        if (!device) return;
        vk->vkFreeCommandBuffers(device, commandPool, commandBufferCount,
                pCommandBuffers);
        AutoLock lock(mLock);
        for (uint32_t i = 0; i < commandBufferCount; i++) {
            const auto& cmdBufferInfoIt =
                mCmdBufferInfo.find(pCommandBuffers[i]);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                const auto& cmdPoolInfoIt =
                    mCmdPoolInfo.find(cmdBufferInfoIt->second.cmdPool);
                if (cmdPoolInfoIt != mCmdPoolInfo.end()) {
                    cmdPoolInfoIt->second.cmdBuffers.erase(pCommandBuffers[i]);
                }
                // Done in decoder
                // delete_VkCommandBuffer(cmdBufferInfoIt->second.boxed);
                mCmdBufferInfo.erase(cmdBufferInfoIt);
            }
        }
    }

    void on_vkGetPhysicalDeviceExternalSemaphoreProperties(
            android::base::BumpPool* pool,
            VkPhysicalDevice boxed_physicalDevice,
            const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
            VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {

        auto physicalDevice = unbox_VkPhysicalDevice(boxed_physicalDevice);
        auto vk = dispatch_VkPhysicalDevice(boxed_physicalDevice);

        if (!physicalDevice) {
            return;
        }
        // Cannot forward this call to driver because nVidia linux driver crahses on it.
        switch (pExternalSemaphoreInfo->handleType) {
            case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT:
                pExternalSemaphoreProperties->exportFromImportedHandleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
                pExternalSemaphoreProperties->compatibleHandleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
                pExternalSemaphoreProperties->externalSemaphoreFeatures =
                    VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                    VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT;
                return;
            case VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT:
                pExternalSemaphoreProperties->exportFromImportedHandleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
                pExternalSemaphoreProperties->compatibleHandleTypes =
                    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
                pExternalSemaphoreProperties->externalSemaphoreFeatures =
                    VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                    VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT;
                return;
            default:
                break;
        }

        pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
        pExternalSemaphoreProperties->compatibleHandleTypes = 0;
        pExternalSemaphoreProperties->externalSemaphoreFeatures = 0;
    }

    VkResult on_vkCreateDescriptorUpdateTemplate(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto descriptorUpdateTemplateInfo =
            calcLinearizedDescriptorUpdateTemplateInfo(pCreateInfo);

        VkResult res = vk->vkCreateDescriptorUpdateTemplate(
                device, &descriptorUpdateTemplateInfo.createInfo,
                pAllocator, pDescriptorUpdateTemplate);

        if (res == VK_SUCCESS) {
            registerDescriptorUpdateTemplate(
                    *pDescriptorUpdateTemplate,
                    descriptorUpdateTemplateInfo);
            *pDescriptorUpdateTemplate = new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(*pDescriptorUpdateTemplate);
        }

        return res;
    }

    VkResult on_vkCreateDescriptorUpdateTemplateKHR(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        auto descriptorUpdateTemplateInfo =
            calcLinearizedDescriptorUpdateTemplateInfo(pCreateInfo);

        VkResult res = vk->vkCreateDescriptorUpdateTemplateKHR(
                device, &descriptorUpdateTemplateInfo.createInfo,
                pAllocator, pDescriptorUpdateTemplate);

        if (res == VK_SUCCESS) {
            registerDescriptorUpdateTemplate(
                    *pDescriptorUpdateTemplate,
                    descriptorUpdateTemplateInfo);
            *pDescriptorUpdateTemplate = new_boxed_non_dispatchable_VkDescriptorUpdateTemplate(*pDescriptorUpdateTemplate);
        }

        return res;
    }

    void on_vkDestroyDescriptorUpdateTemplate(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkDescriptorUpdateTemplate descriptorUpdateTemplate,
            const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorUpdateTemplate(
                device, descriptorUpdateTemplate, pAllocator);

        unregisterDescriptorUpdateTemplate(descriptorUpdateTemplate);
    }

    void on_vkDestroyDescriptorUpdateTemplateKHR(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkDescriptorUpdateTemplate descriptorUpdateTemplate,
            const VkAllocationCallbacks* pAllocator) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkDestroyDescriptorUpdateTemplateKHR(
                device, descriptorUpdateTemplate, pAllocator);

        unregisterDescriptorUpdateTemplate(descriptorUpdateTemplate);
    }

    void on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
            android::base::BumpPool* pool,
            VkDevice boxed_device,
            VkDescriptorSet descriptorSet,
            VkDescriptorUpdateTemplate descriptorUpdateTemplate,
            uint32_t imageInfoCount,
            uint32_t bufferInfoCount,
            uint32_t bufferViewCount,
            const uint32_t* pImageInfoEntryIndices,
            const uint32_t* pBufferInfoEntryIndices,
            const uint32_t* pBufferViewEntryIndices,
            const VkDescriptorImageInfo* pImageInfos,
            const VkDescriptorBufferInfo* pBufferInfos,
            const VkBufferView* pBufferViews) {

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        AutoLock lock(mLock);
        auto info = android::base::find(
                mDescriptorUpdateTemplateInfo,
                descriptorUpdateTemplate);

        if (!info) return;

        memcpy(info->data.data() + info->imageInfoStart,
                pImageInfos,
                imageInfoCount * sizeof(VkDescriptorImageInfo));
        memcpy(info->data.data() + info->bufferInfoStart,
                pBufferInfos,
                bufferInfoCount * sizeof(VkDescriptorBufferInfo));
        memcpy(info->data.data() + info->bufferViewStart,
                pBufferViews,
                bufferViewCount * sizeof(VkBufferView));

        vk->vkUpdateDescriptorSetWithTemplate(
                device, descriptorSet, descriptorUpdateTemplate,
                info->data.data());
    }

    void hostSyncCommandBuffer(
        const char* tag,
        VkCommandBuffer boxed_commandBuffer,
        uint32_t needHostSync,
        uint32_t sequenceNumber) {

        auto nextDeadline = []() {
            return System::get()->getUnixTimeUs() + 1; // 1 ms
        };

        auto timeoutDeadline = System::get()->getUnixTimeUs() + 5000000; // 5 s

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);

        OrderMaintenanceInfo* order = ordmaint_VkCommandBuffer(boxed_commandBuffer);
        if (!order) return;

        AutoLock lock(order->lock);

        if (needHostSync) {
            while ((sequenceNumber - __atomic_load_n(&order->sequenceNumber, __ATOMIC_ACQUIRE) != 1) &&
                    sequenceNumber > __atomic_load_n(&order->sequenceNumber, __ATOMIC_ACQUIRE)) {
                auto waitUntilUs = nextDeadline();
                order->cv.timedWait(
                    &order->lock, waitUntilUs);

                if (timeoutDeadline < System::get()->getUnixTimeUs()) {
                    fprintf(stderr, "%s: warning: sync timed out! curr %u info %u\n", __func__, sequenceNumber, order->sequenceNumber);
                    break;
                }
            }
        }

        __atomic_fetch_add(&order->sequenceNumber, 1, __ATOMIC_RELEASE);
        order->cv.broadcast();
        releaseOrderMaintInfo(order);
    }

    void on_vkCommandBufferHostSyncGOOGLE(
            android::base::BumpPool* pool,
            VkCommandBuffer commandBuffer,
            uint32_t needHostSync,
            uint32_t sequenceNumber) {
        this->hostSyncCommandBuffer("hostSync", commandBuffer, needHostSync, sequenceNumber);
    }

    void hostSyncQueue(
        const char* tag,
        VkQueue boxed_queue,
        uint32_t needHostSync,
        uint32_t sequenceNumber) {

        auto nextDeadline = []() {
            return System::get()->getUnixTimeUs() + 1; // 1 ms
        };

        auto timeoutDeadline = System::get()->getUnixTimeUs() + 5000000; // 5 s

        auto commandBuffer = unbox_VkQueue(boxed_queue);

        OrderMaintenanceInfo* order = ordmaint_VkQueue(boxed_queue);
        if (!order) return;

        AutoLock lock(order->lock);

        if (needHostSync) {
            while ((sequenceNumber - __atomic_load_n(&order->sequenceNumber, __ATOMIC_ACQUIRE) != 1) &&
                    sequenceNumber > __atomic_load_n(&order->sequenceNumber, __ATOMIC_ACQUIRE)) {
                auto waitUntilUs = nextDeadline();
                order->cv.timedWait(
                    &order->lock, waitUntilUs);

                if (timeoutDeadline < System::get()->getUnixTimeUs()) {
                    fprintf(stderr, "%s: warning: sync timed out! curr %u info %u\n", __func__, sequenceNumber, order->sequenceNumber);
                    break;
                }
            }
        }

        __atomic_fetch_add(&order->sequenceNumber, 1, __ATOMIC_RELEASE);
        order->cv.broadcast();
        releaseOrderMaintInfo(order);
    }

    void on_vkQueueHostSyncGOOGLE(
        android::base::BumpPool* pool,
        VkQueue queue,
        uint32_t needHostSync,
        uint32_t sequenceNumber) {
        this->hostSyncQueue("hostSyncQueue", queue, needHostSync, sequenceNumber);
    }


    VkResult on_vkCreateImageWithRequirementsGOOGLE(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage,
        VkMemoryRequirements* pMemoryRequirements) {

        if (pMemoryRequirements) {
            memset(pMemoryRequirements, 0, sizeof(*pMemoryRequirements));
        }

        VkResult imageCreateRes =
            on_vkCreateImage(pool, boxed_device, pCreateInfo, pAllocator, pImage);

        if (imageCreateRes != VK_SUCCESS) {
            return imageCreateRes;
        }

        on_vkGetImageMemoryRequirements(
            pool, boxed_device,
            unbox_VkImage(*pImage),
            pMemoryRequirements);

        return imageCreateRes;
    }

    VkResult on_vkCreateBufferWithRequirementsGOOGLE(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkBufferCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkBuffer* pBuffer,
        VkMemoryRequirements* pMemoryRequirements) {

        if (pMemoryRequirements) {
            memset(pMemoryRequirements, 0, sizeof(*pMemoryRequirements));
        }

        VkResult bufferCreateRes =
            on_vkCreateBuffer(pool, boxed_device, pCreateInfo, pAllocator, pBuffer);

        if (bufferCreateRes != VK_SUCCESS) {
            return bufferCreateRes;
        }

        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);

        vk->vkGetBufferMemoryRequirements(
            device, unbox_VkBuffer(*pBuffer), pMemoryRequirements);

        return bufferCreateRes;
    }


    VkResult on_vkBeginCommandBuffer(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        VkResult result = vk->vkBeginCommandBuffer(commandBuffer, pBeginInfo);
        if (result != VK_SUCCESS) {
            return result;
        }
        // TODO: Check VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT?
        AutoLock lock(mLock);
        mCmdBufferInfo[commandBuffer].preprocessFuncs.clear();
        mCmdBufferInfo[commandBuffer].subCmds.clear();
        return VK_SUCCESS;
    }

    VkResult on_vkBeginCommandBufferAsyncGOOGLE(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            const VkCommandBufferBeginInfo* pBeginInfo) {
        return this->on_vkBeginCommandBuffer(
            pool, boxed_commandBuffer, pBeginInfo);
    }

    void on_vkEndCommandBufferAsyncGOOGLE(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer) {

        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);

        vk->vkEndCommandBuffer(commandBuffer);
    }

    void on_vkResetCommandBufferAsyncGOOGLE(
            android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkCommandBufferResetFlags flags) {
        on_vkResetCommandBuffer(pool, boxed_commandBuffer, flags);
    }

    void on_vkCmdBindPipeline(android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkPipelineBindPoint pipelineBindPoint,
            VkPipeline pipeline) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        vk->vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
        if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
            AutoLock lock(mLock);
            auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
                    cmdBufferInfoIt->second.computePipeline = pipeline;
                }
            }
        }
    }

    void on_vkCmdBindDescriptorSets(android::base::BumpPool* pool,
            VkCommandBuffer boxed_commandBuffer,
            VkPipelineBindPoint pipelineBindPoint,
            VkPipelineLayout layout,
            uint32_t firstSet,
            uint32_t descriptorSetCount,
            const VkDescriptorSet* pDescriptorSets,
            uint32_t dynamicOffsetCount,
            const uint32_t* pDynamicOffsets) {
        auto commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        auto vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        vk->vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout,
                firstSet, descriptorSetCount,
                pDescriptorSets, dynamicOffsetCount,
                pDynamicOffsets);
        if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
            AutoLock lock(mLock);
            auto cmdBufferInfoIt = mCmdBufferInfo.find(commandBuffer);
            if (cmdBufferInfoIt != mCmdBufferInfo.end()) {
                cmdBufferInfoIt->second.descriptorLayout = layout;
                if (descriptorSetCount) {
                    cmdBufferInfoIt->second.firstSet = firstSet;
                    cmdBufferInfoIt->second.descriptorSets.assign(
                            pDescriptorSets,
                            pDescriptorSets + descriptorSetCount);
                }
            }
        }
    }

    VkResult on_vkCreateRenderPass(android::base::BumpPool* pool,
                                   VkDevice boxed_device,
                                   const VkRenderPassCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkRenderPass* pRenderPass) {
        auto device = unbox_VkDevice(boxed_device);
        auto vk = dispatch_VkDevice(boxed_device);
        VkRenderPassCreateInfo createInfo;
        bool needReformat = false;
        AutoLock lock(mLock);

        auto deviceInfoIt = mDeviceInfo.find(device);
        if (deviceInfoIt == mDeviceInfo.end()) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        if (deviceInfoIt->second.emulateTextureEtc2 ||
            deviceInfoIt->second.emulateTextureAstc) {
            for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
                if (deviceInfoIt->second.needEmulatedDecompression(
                            pCreateInfo->pAttachments[i].format)) {
                    needReformat = true;
                    break;
                }
            }
        }
        std::vector<VkAttachmentDescription> attachments;
        if (needReformat) {
            createInfo = *pCreateInfo;
            attachments.assign(
                    pCreateInfo->pAttachments,
                    pCreateInfo->pAttachments + pCreateInfo->attachmentCount);
            createInfo.pAttachments = attachments.data();
            for (auto& attachment : attachments) {
                attachment.format = getDecompFormat(attachment.format);
            }
            pCreateInfo = &createInfo;
        }
        VkResult res = vk->vkCreateRenderPass(device, pCreateInfo, pAllocator,
                                              pRenderPass);
        if (res != VK_SUCCESS) {
            return res;
        }

        *pRenderPass = new_boxed_non_dispatchable_VkRenderPass(*pRenderPass);

        return res;
    }

    VkResult on_vkQueueBindSparse(
        android::base::BumpPool* pool,
        VkQueue boxed_queue,
        uint32_t bindInfoCount,
        const VkBindSparseInfo* pBindInfo, VkFence fence) {

        // If pBindInfo contains VkTimelineSemaphoreSubmitInfo, then it's
        // possible the host driver isn't equipped to deal with them yet.  To
        // work around this, send empty vkQueueSubmits before and after the
        // call to vkQueueBindSparse that contain the right values for
        // wait/signal semaphores and contains the user's
        // VkTimelineSemaphoreSubmitInfo structure, following the *submission
        // order* implied by the indices of pBindInfo.

        // TODO: Detect if we are running on a driver that supports timeline
        // semaphore signal/wait operations in vkQueueBindSparse
        const bool needTimelineSubmitInfoWorkaround = true;

        bool hasTimelineSemaphoreSubmitInfo = false;

        for (uint32_t i = 0; i < bindInfoCount; ++i) {
            const VkTimelineSemaphoreSubmitInfoKHR* tsSi =
                vk_find_struct<VkTimelineSemaphoreSubmitInfoKHR>(pBindInfo + i);
            if (tsSi) {
                hasTimelineSemaphoreSubmitInfo = true;
            }
        }

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        if (!hasTimelineSemaphoreSubmitInfo) {
            (void)pool;
            return vk->vkQueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
        } else {
            std::vector<VkPipelineStageFlags> waitDstStageMasks;
            VkTimelineSemaphoreSubmitInfoKHR currTsSi = {
                VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, 0,
                0, nullptr,
                0, nullptr,
            };

            VkSubmitInfo currSi = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, &currTsSi,
                0, nullptr,
                nullptr,
                0, nullptr, // No commands
                0, nullptr,
            };

            VkBindSparseInfo currBi;

            VkResult res;

            for (uint32_t i = 0; i < bindInfoCount; ++i) {
                const VkTimelineSemaphoreSubmitInfoKHR* tsSi =
                    vk_find_struct<VkTimelineSemaphoreSubmitInfoKHR>(pBindInfo + i);
                if (!tsSi) {
                    res = vk->vkQueueBindSparse(queue, 1, pBindInfo + i, fence);
                    if (VK_SUCCESS != res) return res;
                    continue;
                }

                currTsSi.waitSemaphoreValueCount = tsSi->waitSemaphoreValueCount;
                currTsSi.pWaitSemaphoreValues = tsSi->pWaitSemaphoreValues;
                currTsSi.signalSemaphoreValueCount = 0;
                currTsSi.pSignalSemaphoreValues = nullptr;

                currSi.waitSemaphoreCount = pBindInfo[i].waitSemaphoreCount;
                currSi.pWaitSemaphores = pBindInfo[i].pWaitSemaphores;
                waitDstStageMasks.resize(pBindInfo[i].waitSemaphoreCount, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
                currSi.pWaitDstStageMask = waitDstStageMasks.data();

                currSi.signalSemaphoreCount = 0;
                currSi.pSignalSemaphores = nullptr;

                res = vk->vkQueueSubmit(queue, 1, &currSi, nullptr);
                if (VK_SUCCESS != res) return res;

                currBi = pBindInfo[i];

                vk_struct_chain_remove(tsSi, &currBi);

                currBi.waitSemaphoreCount = 0;
                currBi.pWaitSemaphores = nullptr;
                currBi.signalSemaphoreCount = 0;
                currBi.pSignalSemaphores = nullptr;

                res = vk->vkQueueBindSparse(queue, 1, &currBi, nullptr);
                if (VK_SUCCESS != res) return res;

                currTsSi.waitSemaphoreValueCount = 0;
                currTsSi.pWaitSemaphoreValues = nullptr;
                currTsSi.signalSemaphoreValueCount = tsSi->signalSemaphoreValueCount;
                currTsSi.pSignalSemaphoreValues = tsSi->pSignalSemaphoreValues;

                currSi.waitSemaphoreCount = 0;
                currSi.pWaitSemaphores = nullptr;
                currSi.signalSemaphoreCount = pBindInfo[i].signalSemaphoreCount;
                currSi.pSignalSemaphores = pBindInfo[i].pSignalSemaphores;

                res = vk->vkQueueSubmit(queue, 1, &currSi, i == bindInfoCount - 1 ? fence : nullptr);
                if (VK_SUCCESS != res) return res;
            }

            return VK_SUCCESS;
        }
    }

    void on_vkGetLinearImageLayoutGOOGLE(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        VkFormat format,
        VkDeviceSize* pOffset,
        VkDeviceSize* pRowPitchAlignment) {
        if (mPerFormatLinearImageProperties.find(format) ==
            mPerFormatLinearImageProperties.end()) {
            VkDeviceSize offset = 0u;
            VkDeviceSize rowPitchAlignment = UINT_MAX;

            for (uint32_t width = 64; width <= 256; width++) {
                LinearImageCreateInfo linearImageCreateInfo = {
                        .extent =
                                {
                                        .width = width,
                                        .height = 64,
                                        .depth = 1,
                                },
                        .format = format,
                        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                };

                VkDeviceSize currOffset = 0u;
                VkDeviceSize currRowPitchAlignment = UINT_MAX;

                VkImageCreateInfo defaultVkImageCreateInfo =
                        linearImageCreateInfo.toDefaultVk();
                on_vkGetLinearImageLayout2GOOGLE(
                        pool, boxed_device, &defaultVkImageCreateInfo,
                        &currOffset, &currRowPitchAlignment);

                offset = currOffset;
                rowPitchAlignment =
                        std::min(currRowPitchAlignment, rowPitchAlignment);
            }
            mPerFormatLinearImageProperties[format] = LinearImageProperties{
                    .offset = offset,
                    .rowPitchAlignment = rowPitchAlignment,
            };
        }

        if (pOffset) {
            *pOffset = mPerFormatLinearImageProperties[format].offset;
        }
        if (pRowPitchAlignment) {
            *pRowPitchAlignment =
                    mPerFormatLinearImageProperties[format].rowPitchAlignment;
        }
    }

    void on_vkGetLinearImageLayout2GOOGLE(android::base::BumpPool* pool,
                                          VkDevice boxed_device,
                                          const VkImageCreateInfo* pCreateInfo,
                                          VkDeviceSize* pOffset,
                                          VkDeviceSize* pRowPitchAlignment) {
        LinearImageCreateInfo linearImageCreateInfo = {
                .extent = pCreateInfo->extent,
                .format = pCreateInfo->format,
                .usage = pCreateInfo->usage,
        };
        if (mLinearImageProperties.find(linearImageCreateInfo) ==
            mLinearImageProperties.end()) {
            auto device = unbox_VkDevice(boxed_device);
            auto vk = dispatch_VkDevice(boxed_device);

            VkImageSubresource subresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .arrayLayer = 0,
            };

            VkImage image;
            VkSubresourceLayout subresourceLayout;

            VkImageCreateInfo defaultVkImageCreateInfo =
                    linearImageCreateInfo.toDefaultVk();
            VkResult result = vk->vkCreateImage(
                    device, &defaultVkImageCreateInfo, nullptr, &image);
            if (result != VK_SUCCESS) {
                LOG(ERROR) << "vkCreateImage failed. size: ("
                           << linearImageCreateInfo.extent.width << "x"
                           << linearImageCreateInfo.extent.height << ")"
                           << " result: " << result;
                return;
            }
            vk->vkGetImageSubresourceLayout(device, image, &subresource,
                                            &subresourceLayout);
            vk->vkDestroyImage(device, image, nullptr);

            VkDeviceSize offset = subresourceLayout.offset;
            uint64_t rowPitch = subresourceLayout.rowPitch;
            VkDeviceSize rowPitchAlignment = rowPitch & (~rowPitch + 1);

            mLinearImageProperties[linearImageCreateInfo] = {
                    .offset = offset,
                    .rowPitchAlignment = rowPitchAlignment,
            };
        }

        if (pOffset != nullptr) {
            *pOffset = mLinearImageProperties[linearImageCreateInfo].offset;
        }
        if (pRowPitchAlignment != nullptr) {
            *pRowPitchAlignment = mLinearImageProperties[linearImageCreateInfo]
                                          .rowPitchAlignment;
        }
    }

#include "VkSubDecoder.cpp"

    void on_vkQueueFlushCommandsGOOGLE(
        android::base::BumpPool* pool,
        VkQueue queue,
        VkCommandBuffer boxed_commandBuffer,
        VkDeviceSize dataSize,
        const void* pData) {
        (void)queue;

        VkCommandBuffer commandBuffer = unbox_VkCommandBuffer(boxed_commandBuffer);
        VulkanDispatch* vk = dispatch_VkCommandBuffer(boxed_commandBuffer);
        VulkanMemReadingStream* readStream = readstream_VkCommandBuffer(boxed_commandBuffer);
        subDecode(readStream, vk, boxed_commandBuffer, commandBuffer, dataSize, pData);
    }

    VkDescriptorSet getOrAllocateDescriptorSetFromPoolAndId(
        VulkanDispatch* vk,
        VkDevice device,
        VkDescriptorPool pool,
        VkDescriptorSetLayout setLayout,
        uint64_t poolId,
        uint32_t pendingAlloc,
        bool* didAlloc) {

        auto poolInfo = android::base::find(mDescriptorPoolInfo, pool);
        if (!poolInfo) {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "descriptor pool " << pool << " not found ";
        }

        DispatchableHandleInfo<uint64_t>* setHandleInfo = sBoxedHandleManager.get(poolId);

        if (setHandleInfo->underlying) {
            if (pendingAlloc) {
                VkDescriptorSet allocedSet;
                vk->vkFreeDescriptorSets(device, pool, 1, (VkDescriptorSet*)(&setHandleInfo->underlying));
                VkDescriptorSetAllocateInfo dsAi = {
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 0,
                    pool,
                    1,
                    &setLayout,
                };
                vk->vkAllocateDescriptorSets(device, &dsAi, &allocedSet);
                setHandleInfo->underlying = (uint64_t)allocedSet;
                initDescriptorSetInfoLocked(pool, setLayout, poolId, allocedSet);
                *didAlloc = true;
                return allocedSet;
            } else {
                *didAlloc = false;
                return (VkDescriptorSet)(setHandleInfo->underlying);
            }
        } else {
            if (pendingAlloc) {
                VkDescriptorSet allocedSet;
                VkDescriptorSetAllocateInfo dsAi = {
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 0,
                    pool,
                    1,
                    &setLayout,
                };
                vk->vkAllocateDescriptorSets(device, &dsAi, &allocedSet);
                setHandleInfo->underlying = (uint64_t)allocedSet;
                initDescriptorSetInfoLocked(pool, setLayout, poolId, allocedSet);
                *didAlloc = true;
                return allocedSet;
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                        << "descriptor pool " << pool << " wanted to get set with id 0x" <<
                        std::hex << poolId;
                return nullptr;
            }
        }
    }

    void on_vkQueueCommitDescriptorSetUpdatesGOOGLE(
        android::base::BumpPool* pool,
        VkQueue boxed_queue,
        uint32_t descriptorPoolCount,
        const VkDescriptorPool* pDescriptorPools,
        uint32_t descriptorSetCount,
        const VkDescriptorSetLayout* pDescriptorSetLayouts,
        const uint64_t* pDescriptorSetPoolIds,
        const uint32_t* pDescriptorSetWhichPool,
        const uint32_t* pDescriptorSetPendingAllocation,
        const uint32_t* pDescriptorWriteStartingIndices,
        uint32_t pendingDescriptorWriteCount,
        const VkWriteDescriptorSet* pPendingDescriptorWrites) {

        AutoLock lock(mLock);

        VkDevice device;

        auto queue = unbox_VkQueue(boxed_queue);
        auto vk = dispatch_VkQueue(boxed_queue);

        auto queueInfo = android::base::find(mQueueInfo, queue);
        if (queueInfo) {
            device = queueInfo->device;
        } else {
            GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                << "queue " << queue << "(boxed: " << boxed_queue << ") with no device registered";
        }

        std::vector<VkDescriptorSet> setsToUpdate(descriptorSetCount, nullptr);

        bool didAlloc = false;

        for (uint32_t i = 0; i < descriptorSetCount; ++i) {
            uint64_t poolId = pDescriptorSetPoolIds[i];
            uint32_t whichPool = pDescriptorSetWhichPool[i];
            uint32_t pendingAlloc = pDescriptorSetPendingAllocation[i];
            bool didAllocThisTime;
            setsToUpdate[i] = getOrAllocateDescriptorSetFromPoolAndId(
                vk, device,
                pDescriptorPools[whichPool],
                pDescriptorSetLayouts[i],
                poolId,
                pendingAlloc,
                &didAllocThisTime);

            if (didAllocThisTime) didAlloc = true;
        }

        if (didAlloc) {

            std::vector<VkWriteDescriptorSet> writeDescriptorSetsForHostDriver(pendingDescriptorWriteCount);
            memcpy(writeDescriptorSetsForHostDriver.data(), pPendingDescriptorWrites, pendingDescriptorWriteCount * sizeof(VkWriteDescriptorSet));

            for (uint32_t i = 0; i < descriptorSetCount; ++i) {
                uint32_t writeStartIndex = pDescriptorWriteStartingIndices[i];
                uint32_t writeEndIndex;
                if (i == descriptorSetCount - 1) {
                    writeEndIndex = pendingDescriptorWriteCount;
                } else {
                    writeEndIndex = pDescriptorWriteStartingIndices[i + 1];
                }

                for (uint32_t j = writeStartIndex; j < writeEndIndex; ++j) {
                    writeDescriptorSetsForHostDriver[j].dstSet = setsToUpdate[i];
                }
            }
            this->on_vkUpdateDescriptorSetsImpl(
                pool, vk, device,
                (uint32_t)writeDescriptorSetsForHostDriver.size(),
                writeDescriptorSetsForHostDriver.data(), 0, nullptr);
        } else {
            this->on_vkUpdateDescriptorSetsImpl(
                pool, vk, device,
                pendingDescriptorWriteCount,
                pPendingDescriptorWrites,
                0, nullptr);
        }
    }

    void on_vkCollectDescriptorPoolIdsGOOGLE(
            android::base::BumpPool* pool,
            VkDevice device,
            VkDescriptorPool descriptorPool,
            uint32_t* pPoolIdCount,
            uint64_t* pPoolIds) {

        AutoLock lock(mLock);
        auto& info = mDescriptorPoolInfo[descriptorPool];
        *pPoolIdCount = (uint32_t)info.poolIds.size();

        if (pPoolIds) {
            for (uint32_t i = 0; i < info.poolIds.size(); ++i) {
                pPoolIds[i] = info.poolIds[i];
            }
        }
    }

    VkResult waitForFence(VkFence boxed_fence, uint64_t timeout) {
        AutoLock lock(mLock);

        VkFence fence = unbox_VkFence(boxed_fence);
        if (fence == VK_NULL_HANDLE || mFenceInfo.find(fence) == mFenceInfo.end()) {
            // No fence, could be a semaphore.
            // TODO: Async wait for semaphores
            return VK_SUCCESS;
        }

        // Vulkan specs require fences of vkQueueSubmit to be *externally
        // synchronized*, i.e. we cannot submit a queue while waiting for the
        // fence in another thread. For threads that call this function, they
        // have to wait until a vkQueueSubmit() using this fence is called
        // before calling vkWaitForFences(). So we use a conditional variable
        // and mutex for thread synchronization.
        //
        // See:
        // https://www.khronos.org/registry/vulkan/specs/1.2/html/vkspec.html#fundamentals-threadingbehavior
        // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/519

        const VkDevice device = mFenceInfo[fence].device;
        const VulkanDispatch* vk = mFenceInfo[fence].vk;
        auto& fenceLock = mFenceInfo[fence].lock;
        auto& cv = mFenceInfo[fence].cv;
        lock.unlock();

        fenceLock.lock();
        cv.wait(&fenceLock, [this, fence] {
            AutoLock lock(mLock);
            if (mFenceInfo[fence].state == FenceInfo::State::kWaitable) {
                mFenceInfo[fence].state = FenceInfo::State::kWaiting;
                return true;
            }
            return false;
        });
        fenceLock.unlock();

        return vk->vkWaitForFences(device, /* fenceCount */ 1u, &fence,
                                   /* waitAll */ false, timeout);
    }

    VkResult getFenceStatus(VkFence boxed_fence) {
        AutoLock lock(mLock);

        VkFence fence = unbox_VkFence(boxed_fence);
        if (fence == VK_NULL_HANDLE || mFenceInfo.find(fence) == mFenceInfo.end()) {
            // No fence, could be a semaphore.
            // TODO: Async get status for semaphores
            return VK_SUCCESS;
        }

        const VkDevice device = mFenceInfo[fence].device;
        const VulkanDispatch* vk = mFenceInfo[fence].vk;
        lock.unlock();

        return vk->vkGetFenceStatus(device, fence);
    }

#define GUEST_EXTERNAL_MEMORY_HANDLE_TYPES                                \
    (VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID | \
     VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA)

    // Transforms

    void transformImpl_VkExternalMemoryProperties_tohost(
            const VkExternalMemoryProperties* props, uint32_t count) {
        VkExternalMemoryProperties* mut =
            (VkExternalMemoryProperties*)props;
        for (uint32_t i = 0; i < count; ++i) {
            mut[i] = transformExternalMemoryProperties_tohost(mut[i]);
        }
    }
    void transformImpl_VkExternalMemoryProperties_fromhost(
            const VkExternalMemoryProperties* props, uint32_t count) {
        VkExternalMemoryProperties* mut =
            (VkExternalMemoryProperties*)props;
        for (uint32_t i = 0; i < count; ++i) {
            mut[i] = transformExternalMemoryProperties_fromhost(mut[i], GUEST_EXTERNAL_MEMORY_HANDLE_TYPES);
        }
    }

#define DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(type, field) \
    void transformImpl_##type##_tohost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].field = (VkExternalMemoryHandleTypeFlagBits) \
            transformExternalMemoryHandleTypeFlags_tohost( \
                    mut[i].field); \
        } \
    } \
    void transformImpl_##type##_fromhost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].field = (VkExternalMemoryHandleTypeFlagBits) \
            transformExternalMemoryHandleTypeFlags_fromhost( \
                    mut[i].field, GUEST_EXTERNAL_MEMORY_HANDLE_TYPES); \
        } \
    } \

#define DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(type) \
    void transformImpl_##type##_tohost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].externalMemoryProperties = transformExternalMemoryProperties_tohost( \
                    mut[i].externalMemoryProperties); \
        } \
    } \
    void transformImpl_##type##_fromhost(const type* props, uint32_t count) { \
        type* mut = (type*)props; \
        for (uint32_t i = 0; i < count; ++i) { \
            mut[i].externalMemoryProperties = transformExternalMemoryProperties_fromhost( \
                    mut[i].externalMemoryProperties, GUEST_EXTERNAL_MEMORY_HANDLE_TYPES); \
        } \
    } \

    DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkPhysicalDeviceExternalImageFormatInfo, handleType)
        DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkPhysicalDeviceExternalBufferInfo, handleType)
        DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExternalMemoryImageCreateInfo, handleTypes)
        DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExternalMemoryBufferCreateInfo, handleTypes)
        DEFINE_EXTERNAL_HANDLE_TYPE_TRANSFORM(VkExportMemoryAllocateInfo, handleTypes)
        DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(VkExternalImageFormatProperties)
        DEFINE_EXTERNAL_MEMORY_PROPERTIES_TRANSFORM(VkExternalBufferProperties)


    uint64_t newGlobalHandle(const DispatchableHandleInfo<uint64_t>& item, BoxedHandleTypeTag typeTag) {
        if (!mCreatedHandlesForSnapshotLoad.empty() &&
                (mCreatedHandlesForSnapshotLoad.size() - mCreatedHandlesForSnapshotLoadIndex > 0)) {
            auto handle = mCreatedHandlesForSnapshotLoad[mCreatedHandlesForSnapshotLoadIndex];
            VKDGS_LOG("use handle: %p", handle);
            ++mCreatedHandlesForSnapshotLoadIndex;
            auto res = sBoxedHandleManager.addFixed(handle, item, typeTag);
            return res;
        } else {
            return sBoxedHandleManager.add(item, typeTag);
        }
    }

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_IMPL(type) \
    type new_boxed_##type(type underlying, VulkanDispatch* dispatch, bool ownDispatch) { \
        DispatchableHandleInfo<uint64_t> item; \
        item.underlying = (uint64_t)underlying; \
        item.dispatch = dispatch ? dispatch : new VulkanDispatch; \
        item.ownDispatch = ownDispatch; \
        item.ordMaintInfo = new OrderMaintenanceInfo; \
        item.readStream = nullptr; \
        auto res = (type)newGlobalHandle(item, Tag_##type); \
        return res; \
    } \
    void delete_##type(type boxed) { \
        if (!boxed) return; \
        auto elt = sBoxedHandleManager.get( \
            (uint64_t)(uintptr_t)boxed); \
        if (!elt) return; \
        releaseOrderMaintInfo(elt->ordMaintInfo); \
        if (elt->readStream) { \
            sReadStreamRegistry.push(elt->readStream); \
            elt->readStream = nullptr; \
        } \
        sBoxedHandleManager.remove((uint64_t)boxed); \
    } \
    type unbox_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) return VK_NULL_HANDLE; \
        return (type)elt->underlying; \
    } \
    OrderMaintenanceInfo* ordmaint_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) return 0; \
        auto info = elt->ordMaintInfo; \
        if (!info) return 0; \
        acquireOrderMaintInfo(info); return info; \
    } \
    VulkanMemReadingStream* readstream_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) return 0; \
        auto stream = elt->readStream; \
        if (!stream) { \
            stream = sReadStreamRegistry.pop(); \
            elt->readStream = stream; \
        } \
        return stream; \
    } \
    type unboxed_to_boxed_##type(type unboxed) { \
        AutoLock lock(sBoxedHandleManager.lock); \
        return (type)sBoxedHandleManager.getBoxedFromUnboxedLocked( \
                (uint64_t)(uintptr_t)unboxed); \
    } \
    VulkanDispatch* dispatch_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: err not found boxed %p\n", __func__, boxed); return nullptr; } \
        return elt->dispatch; \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_IMPL(type) \
    type new_boxed_non_dispatchable_##type(type underlying) { \
        DispatchableHandleInfo<uint64_t> item; \
        item.underlying = (uint64_t)underlying; \
        auto res = (type)newGlobalHandle(item, Tag_##type); \
        return res; \
    } \
    void delayed_delete_##type(type boxed, VkDevice device, std::function<void()> callback) { \
        sBoxedHandleManager.removeDelayed((uint64_t)boxed, device, callback); \
    } \
    void delete_##type(type boxed) { \
        sBoxedHandleManager.remove((uint64_t)boxed); \
    } \
    type unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        AutoLock lock(sBoxedHandleManager.lock); \
        return (type)sBoxedHandleManager.getBoxedFromUnboxedLocked( \
                (uint64_t)(uintptr_t)unboxed); \
    } \
    type unbox_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: unbox %p failed, not found\n", __func__, boxed); abort(); return VK_NULL_HANDLE; } \
        return (type)elt->underlying; \
    } \

    GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_IMPL)
        GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_IMPL)

        VkDecoderSnapshot* snapshot() { return &mSnapshot; }

private:
    static const bool kEmulateAstc = true;
    bool isEmulatedExtension(const char* name) const {
        for (auto emulatedExt : kEmulatedExtensions) {
            if (!strcmp(emulatedExt, name)) return true;
        }
        return false;
    }

    bool supportEmulatedCompressedImageFormatProperty(
        VkFormat compressedFormat,
        VkImageType type,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkImageCreateFlags flags
    ) {
        // BUG: 139193497
        return !(usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            && !(type == VK_IMAGE_TYPE_1D);
    }

    std::vector<const char*>
        filteredExtensionNames(
                uint32_t count, const char* const* extNames) {
            std::vector<const char*> res;
            for (uint32_t i = 0; i < count; ++i) {
                auto extName = extNames[i];
                if (!isEmulatedExtension(extName)) {
                    res.push_back(extName);
                }
                if (m_emu->instanceSupportsMoltenVK) {
                    continue;
                }
                if (!strcmp("VK_KHR_external_memory_capabilities", extName)) {
                    res.push_back("VK_KHR_external_memory_capabilities");
                }
                if (!strcmp("VK_KHR_external_memory", extName)) {
                    res.push_back("VK_KHR_external_memory");
                }
                if (!strcmp("VK_KHR_external_semaphore_capabilities", extName)) {
                    res.push_back("VK_KHR_external_semaphore_capabilities");
                }
                if (!strcmp("VK_KHR_external_semaphore", extName)) {
                    res.push_back("VK_KHR_external_semaphore");
                }
                if (!strcmp("VK_ANDROID_external_memory_android_hardware_buffer", extName) ||
                    !strcmp("VK_FUCHSIA_external_memory", extName)) {
#ifdef _WIN32
                    res.push_back("VK_KHR_external_memory_win32");
#else
                    res.push_back("VK_KHR_external_memory_fd");
#endif
                }
                // External semaphore maps to the win32 version on windows,
                // continues with external semaphore fd on non-windows
                if (!strcmp("VK_KHR_external_semaphore_fd", extName)) {
#ifdef _WIN32
                    res.push_back("VK_KHR_external_semaphore_win32");
#else
                    res.push_back("VK_KHR_external_semaphore_fd");
#endif
                }
            }
            return res;
        }

    VkPhysicalDeviceMemoryProperties* memPropsOfDeviceLocked(VkDevice device) {
        auto physdev = android::base::find(mDeviceToPhysicalDevice, device);
        if (!physdev) return nullptr;

        auto physdevInfo = android::base::find(mPhysdevInfo, *physdev);
        if (!physdevInfo) return nullptr;

        return &physdevInfo->memoryProperties;
    }

    Optional<uint32_t> queueFamilyIndexOfQueueLocked(VkQueue queue) {
        auto info = android::base::find(mQueueInfo, queue);
        if (!info) return {};

        return info->queueFamilyIndex;
    }

    bool getDefaultQueueForDeviceLocked(
            VkDevice device, VkQueue* queue, uint32_t* queueFamilyIndex) {

        auto deviceInfo = android::base::find(mDeviceInfo, device);
        if (!deviceInfo) return false;

        auto zeroIt = deviceInfo->queues.find(0);
        if (zeroIt == deviceInfo->queues.end() ||
                zeroIt->second.size() == 0) {
            // Get the first queue / queueFamilyIndex
            // that does show up.
            for (auto it : deviceInfo->queues) {
                auto index = it.first;
                for (auto deviceQueue : it.second) {
                    *queue = deviceQueue;
                    *queueFamilyIndex = index;
                    return true;
                }
            }
            // Didn't find anything, fail.
            return false;
        } else {
            // Use queue family index 0.
            *queue = zeroIt->second[0];
            *queueFamilyIndex = 0;
            return true;
        }

        return false;
    }

    static SpvFileEntry loadDecompressionShaderSource(const char* filename) {
        size_t numDecompressionShaderFileEntries = arraySize(sDecompressionShaderFileEntries);

        for (size_t i = 0; i < numDecompressionShaderFileEntries; ++i) {
            if (!strcmp(filename, sDecompressionShaderFileEntries[i].name)) {
                return sDecompressionShaderFileEntries[i];
            }
        }

        SpvFileEntry invalid = {
            filename, nullptr, 0,
        };
        dwarning("shader source open failed! %s",
                filename);
        return invalid;
    }

    struct CompressedImageInfo {
        bool isCompressed = false;
        bool isEtc2 = false;
        bool isAstc = false;
        VkDevice device = 0;
        VkFormat compFormat;  // The compressed format
        VkImageType imageType;
        VkImageCreateInfo sizeCompImgCreateInfo;
        std::vector<uint32_t> sizeCompImgQueueFamilyIndices;
        VkFormat sizeCompFormat;  // Size compatible format
        VkDeviceSize alignment = 0;
        std::vector<VkDeviceSize> memoryOffsets = {};
        std::vector<VkImage> sizeCompImgs;  // Size compatible images
        VkFormat decompFormat =
            VK_FORMAT_R8G8B8A8_UNORM;  // Decompressed format
        VkImage decompImg = 0;  // Decompressed image
        VkExtent3D extent;
        uint32_t compressedBlockWidth = 1;
        uint32_t compressedBlockHeight = 1;
        uint32_t layerCount;
        uint32_t mipLevels = 1;
        uint32_t mipmapWidth(uint32_t level) {
            return std::max<uint32_t>(extent.width >> level, 1);
        }
        uint32_t mipmapHeight(uint32_t level) {
            return std::max<uint32_t>(extent.height >> level, 1);
        }
        uint32_t mipmapDepth(uint32_t level) {
            return std::max<uint32_t>(extent.depth >> level, 1);
        }
        uint32_t sizeCompMipmapWidth(uint32_t level) {
            return (mipmapWidth(level) + compressedBlockWidth - 1) /
                   compressedBlockWidth;
        }
        uint32_t sizeCompMipmapHeight(uint32_t level) {
            if (imageType != VK_IMAGE_TYPE_1D) {
                return (mipmapHeight(level) + compressedBlockHeight - 1) /
                       compressedBlockHeight;
            } else {
                return 1;
            }
        }
        uint32_t sizeCompMipmapDepth(uint32_t level) {
            return mipmapDepth(level);
        }
        VkDeviceSize decompPixelSize() {
            return getLinearFormatPixelSize(decompFormat);
        }
        bool needEmulatedAlpha() {
            if (!isCompressed) {
                return false;
            }
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                    return true;
                default:
                    return false;
            }
        }

        struct Etc2PushConstant {
            uint32_t compFormat;
            uint32_t baseLayer;
        };

        struct AstcPushConstant {
            uint32_t blockSize[2];
            uint32_t compFormat;
            uint32_t baseLayer;
            uint32_t sRGB;
            uint32_t smallBlock;
        };
        VkDescriptorSetLayout decompDescriptorSetLayout = 0;
        VkDescriptorPool decompDescriptorPool = 0;
        std::vector<VkDescriptorSet> decompDescriptorSets = {};
        VkShaderModule decompShader = 0;
        VkPipelineLayout decompPipelineLayout = 0;
        VkPipeline decompPipeline = 0;
        std::vector<VkImageView> sizeCompImageViews = {};
        std::vector<VkImageView> decompImageViews = {};

        static VkImageView createDefaultImageView(
                goldfish_vk::VulkanDispatch* vk,
                VkDevice device,
                VkImage image,
                VkFormat format,
                VkImageType imageType,
                uint32_t mipLevel,
                uint32_t layerCount) {
            VkImageViewCreateInfo imageViewInfo = {};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = image;
            switch (imageType) {
                case VK_IMAGE_TYPE_1D:
                    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
                    break;
                case VK_IMAGE_TYPE_2D:
                    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    break;
                case VK_IMAGE_TYPE_3D:
                    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
                    break;
                default:
                    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
                    break;
            }
            imageViewInfo.format = format;
            imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
            imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
            imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
            imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
            imageViewInfo.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = mipLevel;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = layerCount;
            VkImageView imageView;
            if (VK_SUCCESS != vk->vkCreateImageView(device, &imageViewInfo,
                        nullptr, &imageView)) {
                fprintf(stderr, "Warning: %s %s:%d failure\n", __func__,
                        __FILE__, __LINE__);
                return 0;
            }
            return imageView;
        }

        VkResult initDecomp(goldfish_vk::VulkanDispatch* vk,
                VkDevice device,
                VkImage image) {
            if (decompPipeline != 0) {
                return VK_SUCCESS;
            }
            // TODO: release resources on failure

#define _RETURN_ON_FAILURE(cmd)                                                \
            {                                                                          \
                VkResult result = cmd;                                                 \
                if (VK_SUCCESS != result) {                                            \
                    fprintf(stderr, "Warning: %s %s:%d vulkan failure %d\n", __func__, \
                            __FILE__, __LINE__, result);                               \
                    return (result);                                                   \
                }                                                                      \
            }

            std::string shaderSrcFileName;
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                    shaderSrcFileName = "Etc2RGB8_";
                    break;
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                    shaderSrcFileName = "Etc2RGBA8_";
                    break;
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                    shaderSrcFileName = "EacR11Unorm_";
                    break;
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                    shaderSrcFileName = "EacR11Snorm_";
                    break;
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                    shaderSrcFileName = "EacRG11Unorm_";
                    break;
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    shaderSrcFileName = "EacRG11Snorm_";
                    break;
                case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                    shaderSrcFileName = "Astc_";
                    break;
                default:
                    shaderSrcFileName = "Etc2RGB8_";
                    break;
            }
            if (imageType == VK_IMAGE_TYPE_1D) {
                shaderSrcFileName += "1DArray.spv";
            } else if (imageType == VK_IMAGE_TYPE_3D) {
                shaderSrcFileName += "3D.spv";
            } else {
                shaderSrcFileName += "2DArray.spv";
            }

            SpvFileEntry shaderSource = VkDecoderGlobalState::Impl::loadDecompressionShaderSource(
                    shaderSrcFileName.c_str());

            if (!shaderSource.size)  {
                return VK_ERROR_OUT_OF_HOST_MEMORY;
            }

            VkShaderModuleCreateInfo shaderInfo = {};
            shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderInfo.codeSize = shaderSource.size;
            // DecompressionShaders.h declares everything as aligned to 4 bytes,
            // so it is safe to cast
            shaderInfo.pCode =
                reinterpret_cast<const uint32_t*>(shaderSource.base);
            _RETURN_ON_FAILURE(vk->vkCreateShaderModule(
                        device, &shaderInfo, nullptr, &decompShader));

            VkDescriptorSetLayoutBinding dsLayoutBindings[] = {
                {
                    0,                                 // bindings
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
                    1,                            // descriptorCount
                    VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
                    0,                            // pImmutableSamplers
                },
                {
                    1,                                 // bindings
                    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  // descriptorType
                    1,                            // descriptorCount
                    VK_SHADER_STAGE_COMPUTE_BIT,  // stageFlags
                    0,                            // pImmutableSamplers
                },
            };
            VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
            dsLayoutInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dsLayoutInfo.bindingCount = sizeof(dsLayoutBindings) /
                sizeof(VkDescriptorSetLayoutBinding);
            dsLayoutInfo.pBindings = dsLayoutBindings;
            _RETURN_ON_FAILURE(vk->vkCreateDescriptorSetLayout(
                        device, &dsLayoutInfo, nullptr,
                        &decompDescriptorSetLayout));

            VkDescriptorPoolSize poolSize[1] = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 * mipLevels},
            };
            VkDescriptorPoolCreateInfo dsPoolInfo = {};
            dsPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            dsPoolInfo.flags =
                VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            dsPoolInfo.maxSets = mipLevels;
            dsPoolInfo.poolSizeCount = 1;
            dsPoolInfo.pPoolSizes = poolSize;
            _RETURN_ON_FAILURE(vk->vkCreateDescriptorPool(
                        device, &dsPoolInfo, nullptr, &decompDescriptorPool));
            std::vector<VkDescriptorSetLayout> layouts(
                    mipLevels, decompDescriptorSetLayout);

            VkDescriptorSetAllocateInfo dsInfo = {};
            dsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsInfo.descriptorPool = decompDescriptorPool;
            dsInfo.descriptorSetCount = mipLevels;
            dsInfo.pSetLayouts = layouts.data();
            decompDescriptorSets.resize(mipLevels);
            _RETURN_ON_FAILURE(vk->vkAllocateDescriptorSets(
                        device, &dsInfo, decompDescriptorSets.data()));

            VkPushConstantRange pushConstant = {};
            pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            pushConstant.offset = 0;
            if (isEtc2) {
                pushConstant.size = sizeof(Etc2PushConstant);
            } else if (isAstc) {
                pushConstant.size = sizeof(AstcPushConstant);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &decompDescriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
            _RETURN_ON_FAILURE(
                    vk->vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                        nullptr, &decompPipelineLayout));

            VkComputePipelineCreateInfo computePipelineInfo = {};
            computePipelineInfo.sType =
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineInfo.stage.sType =
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            computePipelineInfo.stage.module = decompShader;
            computePipelineInfo.stage.pName = "main";
            computePipelineInfo.layout = decompPipelineLayout;
            _RETURN_ON_FAILURE(vk->vkCreateComputePipelines(
                        device, 0, 1, &computePipelineInfo, nullptr,
                        &decompPipeline));

            VkFormat intermediateFormat = decompFormat;
            switch (compFormat) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                    intermediateFormat = VK_FORMAT_R8G8B8A8_UINT;
                    break;
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    intermediateFormat = decompFormat;
                    break;
                case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                    intermediateFormat = VK_FORMAT_R8G8B8A8_UINT;
                    break;
                default:
                    intermediateFormat = decompFormat;
                    break;  // should not happen
            }

            sizeCompImageViews.resize(mipLevels);
            decompImageViews.resize(mipLevels);
            VkDescriptorImageInfo sizeCompDescriptorImageInfo[1] = {{}};
            sizeCompDescriptorImageInfo[0].sampler = 0;
            sizeCompDescriptorImageInfo[0].imageLayout =
                VK_IMAGE_LAYOUT_GENERAL;

            VkDescriptorImageInfo decompDescriptorImageInfo[1] = {{}};
            decompDescriptorImageInfo[0].sampler = 0;
            decompDescriptorImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            VkWriteDescriptorSet writeDescriptorSets[2] = {{}, {}};
            writeDescriptorSets[0].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[0].dstBinding = 0;
            writeDescriptorSets[0].descriptorCount = 1;
            writeDescriptorSets[0].descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[0].pImageInfo = sizeCompDescriptorImageInfo;

            writeDescriptorSets[1].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[1].dstBinding = 1;
            writeDescriptorSets[1].descriptorCount = 1;
            writeDescriptorSets[1].descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            writeDescriptorSets[1].pImageInfo = decompDescriptorImageInfo;

            for (uint32_t i = 0; i < mipLevels; i++) {
                sizeCompImageViews[i] = createDefaultImageView(
                        vk, device, sizeCompImgs[i], sizeCompFormat, imageType,
                        0, layerCount);
                decompImageViews[i] = createDefaultImageView(
                        vk, device, decompImg, intermediateFormat, imageType, i,
                        layerCount);
                sizeCompDescriptorImageInfo[0].imageView =
                    sizeCompImageViews[i];
                decompDescriptorImageInfo[0].imageView = decompImageViews[i];
                writeDescriptorSets[0].dstSet = decompDescriptorSets[i];
                writeDescriptorSets[1].dstSet = decompDescriptorSets[i];
                vk->vkUpdateDescriptorSets(device, 2, writeDescriptorSets, 0,
                        nullptr);
            }
            return VK_SUCCESS;
        }

        void cmdDecompress(goldfish_vk::VulkanDispatch* vk,
                VkCommandBuffer commandBuffer,
                VkPipelineStageFlags dstStageMask,
                VkImageLayout newLayout,
                VkAccessFlags dstAccessMask,
                uint32_t baseMipLevel,
                uint32_t levelCount,
                uint32_t baseLayer,
                uint32_t _layerCount) {
            vk->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    decompPipeline);
            int dispatchZ = _layerCount;

            if (isEtc2) {
                Etc2PushConstant pushConstant = {compFormat, baseLayer};
                if (extent.depth > 1) {
                    // 3D texture
                    pushConstant.baseLayer = 0;
                    dispatchZ = extent.depth;
                }
                vk->vkCmdPushConstants(commandBuffer, decompPipelineLayout,
                                       VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                       sizeof(pushConstant), &pushConstant);
            } else if (isAstc) {
                uint32_t srgb = false;
                uint32_t smallBlock = false;
                switch (compFormat) {
                    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                        srgb = true;
                        break;
                    default:
                        break;
                }
                switch (compFormat) {
                    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                        smallBlock = true;
                        break;
                    default:
                        break;
                }
                AstcPushConstant pushConstant = {
                        {compressedBlockWidth, compressedBlockHeight},
                        compFormat,
                        baseLayer,
                        srgb,
                        smallBlock,
                };
                if (extent.depth > 1) {
                    // 3D texture
                    pushConstant.baseLayer = 0;
                    dispatchZ = extent.depth;
                }
                vk->vkCmdPushConstants(commandBuffer, decompPipelineLayout,
                                       VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                       sizeof(pushConstant), &pushConstant);
            }
            for (uint32_t i = baseMipLevel; i < baseMipLevel + levelCount;
                    i++) {
                vk->vkCmdBindDescriptorSets(
                        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        decompPipelineLayout, 0, 1,
                        decompDescriptorSets.data() + i, 0, nullptr);

                vk->vkCmdDispatch(commandBuffer, sizeCompMipmapWidth(i),
                                  sizeCompMipmapHeight(i), dispatchZ);
            }
        }
    };

    void createSizeCompImages(goldfish_vk::VulkanDispatch* vk,
            CompressedImageInfo* cmpInfo) {
        if (cmpInfo->sizeCompImgs.size() > 0) {
            return;
        }
        VkDevice device = cmpInfo->device;
        uint32_t mipLevels = cmpInfo->mipLevels;
        cmpInfo->sizeCompImgs.resize(mipLevels);
        VkImageCreateInfo imageInfo = cmpInfo->sizeCompImgCreateInfo;
        imageInfo.mipLevels = 1;
        for (uint32_t i = 0; i < mipLevels; i++) {
            imageInfo.extent.width = cmpInfo->sizeCompMipmapWidth(i);
            imageInfo.extent.height = cmpInfo->sizeCompMipmapHeight(i);
            imageInfo.extent.depth = cmpInfo->sizeCompMipmapDepth(i);
            VkDevice device = cmpInfo->device;
            vk->vkCreateImage(device, &imageInfo, nullptr,
                    cmpInfo->sizeCompImgs.data() + i);
        }

        VkPhysicalDevice physicalDevice = mDeviceInfo[device].physicalDevice;
        int32_t memIdx = -1;
        VkDeviceSize& alignment = cmpInfo->alignment;
        std::vector<VkDeviceSize> memSizes(mipLevels);
        VkDeviceSize decompImageSize = 0;
        {
            VkMemoryRequirements memRequirements;
            vk->vkGetImageMemoryRequirements(device, cmpInfo->decompImg,
                    &memRequirements);
            memIdx = findProperties(physicalDevice,
                    memRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (memIdx < 0) {
                fprintf(stderr, "Error: cannot find memory property!\n");
                return;
            }
            alignment = std::max(alignment, memRequirements.alignment);
            decompImageSize = memRequirements.size;
        }
        for (size_t i = 0; i < mipLevels; i++) {
            VkMemoryRequirements memRequirements;
            vk->vkGetImageMemoryRequirements(device, cmpInfo->sizeCompImgs[i],
                    &memRequirements);
            alignment = std::max(alignment, memRequirements.alignment);
            memSizes[i] = memRequirements.size;
        }
        auto& memoryOffsets = cmpInfo->memoryOffsets;
        memoryOffsets.resize(mipLevels + 1);
        {
            VkDeviceSize alignedSize = decompImageSize;
            if (alignment != 0) {
                alignedSize =
                    (alignedSize + alignment - 1) / alignment * alignment;
            }
            memoryOffsets[0] = alignedSize;
        }
        for (size_t i = 0; i < cmpInfo->sizeCompImgs.size(); i++) {
            VkDeviceSize alignedSize = memSizes[i];
            if (alignment != 0) {
                alignedSize =
                    (alignedSize + alignment - 1) / alignment * alignment;
            }
            memoryOffsets[i + 1] = memoryOffsets[i] + alignedSize;
        }
    }

    void updateImageMemorySizeLocked(
            VkDevice device,
            VkImage image,
            VkMemoryRequirements* pMemoryRequirements) {
        auto deviceInfoIt = mDeviceInfo.find(device);
        if (!deviceInfoIt->second.emulateTextureEtc2 &&
            !deviceInfoIt->second.emulateTextureAstc) {
            return;
        }
        auto it = mImageInfo.find(image);
        if (it == mImageInfo.end()) {
            return;
        }
        CompressedImageInfo& cmpInfo = it->second.cmpInfo;
        if (!deviceInfoIt->second.needEmulatedDecompression(cmpInfo)) {
            return;
        }
        pMemoryRequirements->alignment =
            std::max(pMemoryRequirements->alignment, cmpInfo.alignment);
        pMemoryRequirements->size += cmpInfo.memoryOffsets[cmpInfo.mipLevels];
    }

    static bool needEmulatedEtc2(VkPhysicalDevice physicalDevice,
            goldfish_vk::VulkanDispatch* vk) {
        VkPhysicalDeviceFeatures feature;
        vk->vkGetPhysicalDeviceFeatures(physicalDevice, &feature);
        return !feature.textureCompressionETC2;
    }

    static bool needEmulatedAstc(VkPhysicalDevice physicalDevice,
                                 goldfish_vk::VulkanDispatch* vk) {
        if (!kEmulateAstc) {
            return false;
        }
        VkPhysicalDeviceFeatures feature;
        vk->vkGetPhysicalDeviceFeatures(physicalDevice, &feature);
        return !feature.textureCompressionASTC_LDR;
    }

    static ETC2ImageFormat getEtc2Format(VkFormat fmt) {
        switch (fmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                return EtcRGB8;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return EtcRGBA8;
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                return EtcRGB8A1;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                return EtcR11;
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return EtcSignedR11;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                return EtcRG11;
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return EtcSignedRG11;
            default:
                fprintf(stderr,
                        "TODO: unsupported compressed texture format %d\n",
                        fmt);
                return EtcRGB8;
        }
    }

    static VkFormat getDecompFormat(VkFormat compFmt) {
        switch (compFmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                return VK_FORMAT_R16_UNORM;
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return VK_FORMAT_R16_SNORM;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                return VK_FORMAT_R16G16_UNORM;
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return VK_FORMAT_R16G16_SNORM;
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                return VK_FORMAT_R8G8B8A8_SRGB;
            default:
                return compFmt;
        }
    }

    VkFormat getSizeCompFormat(VkFormat compFmt) {
        switch (compFmt) {
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                return VK_FORMAT_R16G16B16A16_UINT;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                return VK_FORMAT_R32G32B32A32_UINT;
            case VK_FORMAT_EAC_R11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                return VK_FORMAT_R32G32_UINT;
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                return VK_FORMAT_R32G32B32A32_UINT;
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                return VK_FORMAT_R32G32B32A32_UINT;
            default:
                return compFmt;
        }
    }

    bool isEtc2Compatible(VkFormat compFmt1, VkFormat compFmt2) {
        const VkFormat kCmpSets[][2] = {
            {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
                VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
            {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
                VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
            {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
                VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
            {VK_FORMAT_EAC_R11_UNORM_BLOCK, VK_FORMAT_EAC_R11_SNORM_BLOCK},
            {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
                VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
        };
        if (compFmt1 == compFmt2) {
            return true;
        }
        for (auto& cmpSet : kCmpSets) {
            if (compFmt1 == cmpSet[0] || compFmt1 == cmpSet[1]) {
                return compFmt2 == cmpSet[0] || compFmt2 == cmpSet[1];
            }
        }
        return false;
    }
    CompressedImageInfo createCompressedImageInfo(VkFormat compFmt) {
        CompressedImageInfo cmpInfo;
        cmpInfo.compFormat = compFmt;
        cmpInfo.decompFormat = getDecompFormat(compFmt);
        cmpInfo.sizeCompFormat = getSizeCompFormat(compFmt);
        cmpInfo.isCompressed = (cmpInfo.decompFormat != compFmt);

        if (cmpInfo.isCompressed) {
            switch (compFmt) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    cmpInfo.compressedBlockWidth = 4;
                    cmpInfo.compressedBlockHeight = 4;
                    cmpInfo.isEtc2 = true;
                    break;
                case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 4;
                    cmpInfo.compressedBlockHeight = 4;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 5;
                    cmpInfo.compressedBlockHeight = 4;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 5;
                    cmpInfo.compressedBlockHeight = 5;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 6;
                    cmpInfo.compressedBlockHeight = 5;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 6;
                    cmpInfo.compressedBlockHeight = 6;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 8;
                    cmpInfo.compressedBlockHeight = 5;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 8;
                    cmpInfo.compressedBlockHeight = 6;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 8;
                    cmpInfo.compressedBlockHeight = 8;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 10;
                    cmpInfo.compressedBlockHeight = 5;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 10;
                    cmpInfo.compressedBlockHeight = 6;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 10;
                    cmpInfo.compressedBlockHeight = 8;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 10;
                    cmpInfo.compressedBlockHeight = 10;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 12;
                    cmpInfo.compressedBlockHeight = 10;
                    cmpInfo.isAstc = true;
                    break;
                case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                    cmpInfo.compressedBlockWidth = 12;
                    cmpInfo.compressedBlockHeight = 12;
                    cmpInfo.isAstc = true;
                    break;
                default:
                    break;
            }
        }

        return cmpInfo;
    }

    static const VkFormatFeatureFlags kEmulatedEtc2BufferFeatureMask =
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
        VK_FORMAT_FEATURE_BLIT_SRC_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

    void maskFormatPropertiesForEmulatedEtc2(
            VkFormatProperties* pFormatProperties) {
        pFormatProperties->bufferFeatures &= kEmulatedEtc2BufferFeatureMask;
    }

    void maskFormatPropertiesForEmulatedEtc2(
            VkFormatProperties2* pFormatProperties) {
        pFormatProperties->formatProperties.bufferFeatures &=
            kEmulatedEtc2BufferFeatureMask;
    }

    template <class VkFormatProperties1or2>
        void getPhysicalDeviceFormatPropertiesCore(
                std::function<
                void(VkPhysicalDevice, VkFormat, VkFormatProperties1or2*)>
                getPhysicalDeviceFormatPropertiesFunc,
                goldfish_vk::VulkanDispatch* vk,
                VkPhysicalDevice physicalDevice,
                VkFormat format,
                VkFormatProperties1or2* pFormatProperties) {
            switch (format) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: {
                    if (!needEmulatedEtc2(physicalDevice, vk)) {
                        // Hardware supported ETC2
                        getPhysicalDeviceFormatPropertiesFunc(
                                physicalDevice, format, pFormatProperties);
                        return;
                    }
                    // Emulate ETC formats
                    CompressedImageInfo cmpInfo =
                            createCompressedImageInfo(format);
                    getPhysicalDeviceFormatPropertiesFunc(physicalDevice,
                                                          cmpInfo.decompFormat,
                                                          pFormatProperties);
                    maskFormatPropertiesForEmulatedEtc2(pFormatProperties);
                    break;
                }
                case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: {
                    if (!needEmulatedAstc(physicalDevice, vk)) {
                        // Hardware supported ETC2
                        getPhysicalDeviceFormatPropertiesFunc(
                                physicalDevice, format, pFormatProperties);
                        return;
                    }
                    // Emulate ETC formats
                    CompressedImageInfo cmpInfo =
                            createCompressedImageInfo(format);
                    getPhysicalDeviceFormatPropertiesFunc(physicalDevice,
                                                          cmpInfo.decompFormat,
                                                          pFormatProperties);
                    maskFormatPropertiesForEmulatedEtc2(pFormatProperties);
                    break;
                }
                default:
                    getPhysicalDeviceFormatPropertiesFunc(
                            physicalDevice, format, pFormatProperties);
                    break;
            }
        }


    void executePreprocessRecursive(int level, VkCommandBuffer cmdBuffer) {
        auto cmdBufferIt = mCmdBufferInfo.find(cmdBuffer);
        if (cmdBufferIt == mCmdBufferInfo.end()) {
            return;
        }
        for (const auto& func : cmdBufferIt->second.preprocessFuncs) {
            func();
        }
        // TODO: fix
        // for (const auto& subCmd : cmdBufferIt->second.subCmds) {
        // executePreprocessRecursive(level + 1, subCmd);
        // }
    }

    void teardownInstanceLocked(VkInstance instance) {

        std::vector<VkDevice> devicesToDestroy;
        std::vector<VulkanDispatch*> devicesToDestroyDispatches;

        for (auto it : mDeviceToPhysicalDevice) {
            auto otherInstance = android::base::find(mPhysicalDeviceToInstance, it.second);
            if (!otherInstance) continue;

            if (instance == *otherInstance) {
                devicesToDestroy.push_back(it.first);
                devicesToDestroyDispatches.push_back(
                        dispatch_VkDevice(
                            mDeviceInfo[it.first].boxed));
            }
        }

        for (uint32_t i = 0; i < devicesToDestroy.size(); ++i) {
            // https://bugs.chromium.org/p/chromium/issues/detail?id=1074600
            // it's important to idle the device before destroying it!
            devicesToDestroyDispatches[i]->vkDeviceWaitIdle(devicesToDestroy[i]);
            {
                std::vector<VkDeviceMemory> toDestroy;
                auto it = mMapInfo.begin();
                while (it != mMapInfo.end()) {
                    if (it->second.device == devicesToDestroy[i]) {
                        toDestroy.push_back(it->first);
                    }
                    ++it;
                }

                for (auto mem: toDestroy) {
                    freeMemoryLocked(devicesToDestroyDispatches[i],
                            devicesToDestroy[i],
                            mem, nullptr);
                }
            }

            // Free all command buffers and command pools
            {
                std::vector<VkCommandBuffer> toDestroy;
                std::vector<VkCommandPool> toDestroyPools;
                auto it = mCmdBufferInfo.begin();
                while (it != mCmdBufferInfo.end()) {
                    if (it->second.device == devicesToDestroy[i]) {
                        toDestroy.push_back(it->first);
                        toDestroyPools.push_back(it->second.cmdPool);
                    }
                    ++it;
                }

                for (int j = 0; j < toDestroy.size(); ++j) {
                    devicesToDestroyDispatches[i]->vkFreeCommandBuffers(devicesToDestroy[i], toDestroyPools[j], 1, &toDestroy[j]);
                    VkCommandBuffer boxed = unboxed_to_boxed_VkCommandBuffer(toDestroy[j]);
                    delete_VkCommandBuffer(boxed);
                    mCmdBufferInfo.erase(toDestroy[j]);
                }
            }

            {
                std::vector<VkCommandPool> toDestroy;
                std::vector<VkCommandPool> toDestroyBoxed;
                auto it = mCmdPoolInfo.begin();
                while (it != mCmdPoolInfo.end()) {
                    if (it->second.device == devicesToDestroy[i]) {
                        toDestroy.push_back(it->first);
                        toDestroyBoxed.push_back(it->second.boxed);
                    }
                    ++it;
                }

                for (int j = 0; j < toDestroy.size(); ++j) {
                    devicesToDestroyDispatches[i]->vkDestroyCommandPool(devicesToDestroy[i], toDestroy[j], 0);
                    delete_VkCommandPool(toDestroyBoxed[j]);
                    mCmdPoolInfo.erase(toDestroy[j]);
                }
            }

            {
                std::vector<VkDescriptorPool> toDestroy;
                std::vector<VkDescriptorPool> toDestroyBoxed;
                auto it = mDescriptorPoolInfo.begin();
                while (it != mDescriptorPoolInfo.end()) {
                    if (it->second.device == devicesToDestroy[i]) {
                        toDestroy.push_back(it->first);
                        toDestroyBoxed.push_back(it->second.boxed);
                    }
                    ++it;
                }

                for (int j = 0; j < toDestroy.size(); ++j) {
                    devicesToDestroyDispatches[i]->vkDestroyDescriptorPool(devicesToDestroy[i], toDestroy[j], 0);
                    delete_VkDescriptorPool(toDestroyBoxed[j]);
                    mDescriptorPoolInfo.erase(toDestroy[j]);
                }
            }

            {
                std::vector<VkDescriptorSetLayout> toDestroy;
                std::vector<VkDescriptorSetLayout> toDestroyBoxed;
                auto it = mDescriptorSetLayoutInfo.begin();
                while (it != mDescriptorSetLayoutInfo.end()) {
                    if (it->second.device == devicesToDestroy[i]) {
                        toDestroy.push_back(it->first);
                        toDestroyBoxed.push_back(it->second.boxed);
                    }
                    ++it;
                }

                for (int j = 0; j < toDestroy.size(); ++j) {
                    devicesToDestroyDispatches[i]->vkDestroyDescriptorSetLayout(devicesToDestroy[i], toDestroy[j], 0);
                    delete_VkDescriptorSetLayout(toDestroyBoxed[j]);
                    mDescriptorSetLayoutInfo.erase(toDestroy[j]);
                }
            }
        }

        for (uint32_t i = 0; i < devicesToDestroy.size(); ++i) {
            destroyDeviceLocked(devicesToDestroy[i], nullptr);
            mDeviceInfo.erase(devicesToDestroy[i]);
            mDeviceToPhysicalDevice.erase(devicesToDestroy[i]);
        }
    }

    typedef std::function<void()> PreprocessFunc;
    struct CommandBufferInfo {
        std::vector<PreprocessFunc> preprocessFuncs = {};
        std::vector<VkCommandBuffer> subCmds = {};
        VkDevice device = 0;
        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer boxed = nullptr;
        VkPipeline computePipeline = 0;
        uint32_t firstSet = 0;
        VkPipelineLayout descriptorLayout = 0;
        std::vector<VkDescriptorSet> descriptorSets;
        uint32_t sequenceNumber = 0;
    };

    struct CommandPoolInfo {
        VkDevice device = 0;
        VkCommandPool boxed = 0;
        std::unordered_set<VkCommandBuffer> cmdBuffers = {};
    };

    void removeCommandBufferInfo(
            const std::unordered_set<VkCommandBuffer>& cmdBuffers) {
        for (const auto& cmdBuffer : cmdBuffers) {
            mCmdBufferInfo.erase(cmdBuffer);
        }
    }

    bool isDescriptorTypeImageInfo(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
            (descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
            (descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
            (descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
            (descType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    }

    bool isDescriptorTypeBufferInfo(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
            (descType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
            (descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
            (descType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    }

    bool isDescriptorTypeBufferView(VkDescriptorType descType) {
        return (descType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
            (descType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
    }

    struct DescriptorUpdateTemplateInfo {
        VkDescriptorUpdateTemplateCreateInfo createInfo;
        std::vector<VkDescriptorUpdateTemplateEntry>
            linearizedTemplateEntries;
        // Preallocated pData
        std::vector<uint8_t> data;
        size_t imageInfoStart;
        size_t bufferInfoStart;
        size_t bufferViewStart;
    };

    DescriptorUpdateTemplateInfo calcLinearizedDescriptorUpdateTemplateInfo(
            const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo) {

        DescriptorUpdateTemplateInfo res;
        res.createInfo = *pCreateInfo;

        size_t numImageInfos = 0;
        size_t numBufferInfos = 0;
        size_t numBufferViews = 0;

        for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; ++i) {
            const auto& entry = pCreateInfo->pDescriptorUpdateEntries[i];
            auto type = entry.descriptorType;
            auto count = entry.descriptorCount;
            if (isDescriptorTypeImageInfo(type)) {
                numImageInfos += count;
            } else if (isDescriptorTypeBufferInfo(type)) {
                numBufferInfos += count;
            } else if (isDescriptorTypeBufferView(type)) {
                numBufferViews += count;
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                    << "unknown descriptor type 0x" << std::hex << type;
            }
        }

        size_t imageInfoBytes = numImageInfos * sizeof(VkDescriptorImageInfo);
        size_t bufferInfoBytes = numBufferInfos * sizeof(VkDescriptorBufferInfo);
        size_t bufferViewBytes = numBufferViews * sizeof(VkBufferView);

        res.data.resize(imageInfoBytes + bufferInfoBytes + bufferViewBytes);
        res.imageInfoStart = 0;
        res.bufferInfoStart = imageInfoBytes;
        res.bufferViewStart = imageInfoBytes + bufferInfoBytes;

        size_t imageInfoCount = 0;
        size_t bufferInfoCount = 0;
        size_t bufferViewCount = 0;

        for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; ++i) {

            const auto& entry = pCreateInfo->pDescriptorUpdateEntries[i];
            VkDescriptorUpdateTemplateEntry entryForHost = entry;

            auto type = entry.descriptorType;
            auto count = entry.descriptorCount;

            if (isDescriptorTypeImageInfo(type)) {
                entryForHost.offset =
                    res.imageInfoStart +
                    imageInfoCount * sizeof(VkDescriptorImageInfo);
                entryForHost.stride = sizeof(VkDescriptorImageInfo);
                ++imageInfoCount;
            } else if (isDescriptorTypeBufferInfo(type)) {
                entryForHost.offset =
                    res.bufferInfoStart +
                    bufferInfoCount * sizeof(VkDescriptorBufferInfo);
                entryForHost.stride = sizeof(VkDescriptorBufferInfo);
                ++bufferInfoCount;
            } else if (isDescriptorTypeBufferView(type)) {
                entryForHost.offset =
                    res.bufferViewStart +
                    bufferViewCount * sizeof(VkBufferView);
                entryForHost.stride = sizeof(VkBufferView);
                ++bufferViewCount;
            } else {
                GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
                    << "unknown descriptor type 0x" << std::hex << type;
            }

            res.linearizedTemplateEntries.push_back(entryForHost);
        }

        res.createInfo.pDescriptorUpdateEntries =
            res.linearizedTemplateEntries.data();

        return res;
    }

    void registerDescriptorUpdateTemplate(
            VkDescriptorUpdateTemplate descriptorUpdateTemplate,
            const DescriptorUpdateTemplateInfo& info) {
        AutoLock lock(mLock);
        mDescriptorUpdateTemplateInfo[descriptorUpdateTemplate] = info;
    }

    void unregisterDescriptorUpdateTemplate(
            VkDescriptorUpdateTemplate descriptorUpdateTemplate) {
        AutoLock lock(mLock);
        mDescriptorUpdateTemplateInfo.erase(descriptorUpdateTemplate);
    }

    // Returns the momory property index when succeeds; returns -1 when fails.
    int32_t findProperties(VkPhysicalDevice physicalDevice,
            uint32_t memoryTypeBitsRequirement,
            VkMemoryPropertyFlags requiredProperties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        auto ivk = dispatch_VkInstance(
                mInstanceInfo[mPhysicalDeviceToInstance[physicalDevice]].boxed);

        ivk->vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                &memProperties);

        const uint32_t memoryCount = memProperties.memoryTypeCount;
        for (uint32_t memoryIndex = 0; memoryIndex < memoryCount;
                ++memoryIndex) {
            const uint32_t memoryTypeBits = (1 << memoryIndex);
            const bool isRequiredMemoryType =
                memoryTypeBitsRequirement & memoryTypeBits;

            const VkMemoryPropertyFlags properties =
                memProperties.memoryTypes[memoryIndex].propertyFlags;
            const bool hasRequiredProperties =
                (properties & requiredProperties) == requiredProperties;

            if (isRequiredMemoryType && hasRequiredProperties)
                return static_cast<int32_t>(memoryIndex);
        }

        // failed to find memory type
        return -1;
    }

    VulkanDispatch* m_vk;
    VkEmulation* m_emu;
    bool mSnapshotsEnabled = false;
    bool mVkCleanupEnabled = true;
    bool mLogging = false;
    bool mVerbosePrints = false;
    bool mUseOldMemoryCleanupPath = false;
    bool mGuestUsesAngle = false;

    Lock mLock;
    ConditionVariable mCvWaitSequenceNumber;

    // We always map the whole size on host.
    // This makes it much easier to implement
    // the memory map API.
    struct MappedMemoryInfo {
        // This indicates whether the VkDecoderGlobalState needs to clean up
        // and unmap the mapped memory; only the owner of the mapped memory
        // should call unmap.
        bool needUnmap = false;
        // When ptr is null, it means the VkDeviceMemory object
        // was not allocated with the HOST_VISIBLE property.
        void* ptr = nullptr;
        VkDeviceSize size;
        // GLDirectMem info
        bool directMapped = false;
        bool virtioGpuMapped = false;
        uint64_t guestPhysAddr = 0;
        void* pageAlignedHva = nullptr;
        uint64_t sizeToPage = 0;
        uint64_t hostmemId = 0;
        VkDevice device = VK_NULL_HANDLE;
        MTLTextureRef mtlTexture = nullptr;
    };

    struct InstanceInfo {
        std::vector<std::string> enabledExtensionNames;
        uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
        VkInstance boxed = nullptr;
    };

    struct PhysicalDeviceInfo {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkPhysicalDevice boxed = nullptr;
    };

    struct DeviceInfo {
        std::unordered_map<uint32_t, std::vector<VkQueue>> queues;
        std::vector<std::string> enabledExtensionNames;
        bool emulateTextureEtc2 = false;
        bool emulateTextureAstc = false;
        VkPhysicalDevice physicalDevice;
        VkDevice boxed = nullptr;
        bool needEmulatedDecompression(const CompressedImageInfo& imageInfo) {
            return imageInfo.isCompressed &&
                   ((imageInfo.isEtc2 && emulateTextureEtc2) ||
                    (imageInfo.isAstc && emulateTextureAstc));
        }
        bool needEmulatedDecompression(VkFormat format) {
            switch (format) {
                case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
                case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
                case VK_FORMAT_EAC_R11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11_SNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
                case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
                    return emulateTextureEtc2;
                case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
                case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
                case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
                case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
                case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
                    return emulateTextureAstc;
                default:
                    return false;
            }
        }
    };

    struct QueueInfo {
        Lock* lock = nullptr;
        VkDevice device;
        uint32_t queueFamilyIndex;
        VkQueue boxed = nullptr;
        uint32_t sequenceNumber = 0;
    };

    struct BufferInfo {
        VkDevice device;
        VkDeviceMemory memory = 0;
        VkDeviceSize memoryOffset = 0;
        VkDeviceSize size;
        // For compressed texture emulation
        VulkanDispatch* vk = nullptr;
    };

    struct ImageInfo {
        AndroidNativeBufferInfo anbInfo;
        CompressedImageInfo cmpInfo;
    };

    struct ImageViewInfo {
        bool needEmulatedAlpha = false;
    };

    struct SamplerInfo {
        bool needEmulatedAlpha = false;
        VkSamplerCreateInfo createInfo = {};
        VkSampler emulatedborderSampler = VK_NULL_HANDLE;
    };

    struct FenceInfo {
        VkDevice device = VK_NULL_HANDLE;
        VkFence boxed = VK_NULL_HANDLE;
        VulkanDispatch* vk = nullptr;

        android::base::StaticLock lock;
        android::base::ConditionVariable cv;

        enum class State {
            kWaitable,
            kNotWaitable,
            kWaiting,
        };
        State state = State::kNotWaitable;
    };

    struct SemaphoreInfo {
        int externalHandleId = 0;
        VK_EXT_MEMORY_HANDLE externalHandle =
            VK_EXT_MEMORY_HANDLE_INVALID;
    };

    struct DescriptorSetLayoutInfo  {
        VkDevice device = 0;
        VkDescriptorSetLayout boxed = 0;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    struct DescriptorPoolInfo {
        VkDevice device = 0;
        VkDescriptorPool boxed = 0;
        struct PoolState {
            VkDescriptorType type;
            uint32_t descriptorCount;
            uint32_t used;
        };

        VkDescriptorPoolCreateInfo createInfo;
        uint32_t maxSets;
        uint32_t usedSets;
        std::vector<PoolState> pools;

        std::unordered_map<VkDescriptorSet, VkDescriptorSet> allocedSetsToBoxed;
        std::vector<uint64_t> poolIds;
    };

    struct DescriptorSetInfo {
        VkDescriptorPool pool;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    bool isBindingFeasibleForAlloc(const DescriptorPoolInfo::PoolState& poolState, const VkDescriptorSetLayoutBinding& binding) {
        if (binding.descriptorCount && (poolState.type != binding.descriptorType)) {
            return false;
        }

        uint32_t availDescriptorCount =
            poolState.descriptorCount - poolState.used;

        if (availDescriptorCount < binding.descriptorCount) {
            return false;
        }

        return true;
    }

    bool isBindingFeasibleForFree(const DescriptorPoolInfo::PoolState& poolState, const VkDescriptorSetLayoutBinding& binding) {
        if (poolState.type != binding.descriptorType) return false;
        if (poolState.used < binding.descriptorCount) return false;
        return true;
    }

    void allocBindingFeasible(
        const VkDescriptorSetLayoutBinding& binding,
        DescriptorPoolInfo::PoolState& poolState) {
        poolState.used += binding.descriptorCount;
    }

    void freeBindingFeasible(
        const VkDescriptorSetLayoutBinding& binding,
        DescriptorPoolInfo::PoolState& poolState) {
        poolState.used -= binding.descriptorCount;
    }

    VkResult validateDescriptorSetAllocLocked(const VkDescriptorSetAllocateInfo* pAllocateInfo) {
        auto poolInfo = android::base::find(mDescriptorPoolInfo, pAllocateInfo->descriptorPool);
        if (!poolInfo) return VK_ERROR_INITIALIZATION_FAILED;

        // Check the number of sets available.
        auto setsAvailable = poolInfo->maxSets - poolInfo->usedSets;

        if (setsAvailable < pAllocateInfo->descriptorSetCount) {
            return VK_ERROR_OUT_OF_POOL_MEMORY;
        }

        // Perform simulated allocation and error out with
        // VK_ERROR_OUT_OF_POOL_MEMORY if it fails.
        std::vector<DescriptorPoolInfo::PoolState> poolCopy =
            poolInfo->pools;

        for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; ++i) {
            auto setLayoutInfo = android::base::find(mDescriptorSetLayoutInfo, pAllocateInfo->pSetLayouts[i]);
            if (!setLayoutInfo) return VK_ERROR_INITIALIZATION_FAILED;

            for (const auto& binding : setLayoutInfo->bindings) {
                bool success = false;
                for (auto& pool : poolCopy) {
                    if (!isBindingFeasibleForAlloc(pool, binding)) continue;

                    success = true;
                    allocBindingFeasible(binding, pool);
                    break;
                }

                if (!success) {
                    return VK_ERROR_OUT_OF_POOL_MEMORY;
                }
            }
        }
        return VK_SUCCESS;
    }

    void applyDescriptorSetAllocationLocked(DescriptorPoolInfo& poolInfo, const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        ++poolInfo.usedSets;
        for (const auto& binding : bindings) {
            for (auto& pool : poolInfo.pools) {
                if (!isBindingFeasibleForAlloc(pool, binding)) continue;
                allocBindingFeasible(binding, pool);
                break;
            }
        }
    }

    void removeDescriptorSetAllocationLocked(DescriptorPoolInfo& poolInfo, const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        --poolInfo.usedSets;
        for (const auto& binding : bindings) {
            for (auto& pool : poolInfo.pools) {
                if (!isBindingFeasibleForFree(pool, binding)) continue;
                freeBindingFeasible(binding, pool);
                break;
            }
        }
    }

    template <class T>
    class NonDispatchableHandleInfo {
    public:
        T underlying;
    };

    std::unordered_map<VkInstance, InstanceInfo>
        mInstanceInfo;
    std::unordered_map<VkPhysicalDevice, PhysicalDeviceInfo>
        mPhysdevInfo;
    std::unordered_map<VkDevice, DeviceInfo>
        mDeviceInfo;
    std::unordered_map<VkImage, ImageInfo>
        mImageInfo;
    std::unordered_map<VkImageView, ImageViewInfo> mImageViewInfo;
    std::unordered_map<VkSampler, SamplerInfo> mSamplerInfo;
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> mCmdBufferInfo;
    std::unordered_map<VkCommandPool, CommandPoolInfo> mCmdPoolInfo;
    // TODO: release CommandBufferInfo when a command pool is reset/released

    // Back-reference to the physical device associated with a particular
    // VkDevice, and the VkDevice corresponding to a VkQueue.
    std::unordered_map<VkDevice, VkPhysicalDevice> mDeviceToPhysicalDevice;
    std::unordered_map<VkPhysicalDevice, VkInstance> mPhysicalDeviceToInstance;

    std::unordered_map<VkQueue, QueueInfo> mQueueInfo;
    std::unordered_map<VkBuffer, BufferInfo> mBufferInfo;

    std::unordered_map<VkDeviceMemory, MappedMemoryInfo> mMapInfo;

    std::unordered_map<VkSemaphore, SemaphoreInfo> mSemaphoreInfo;
    std::unordered_map<VkFence, FenceInfo> mFenceInfo;

    std::unordered_map<VkDescriptorSetLayout, DescriptorSetLayoutInfo> mDescriptorSetLayoutInfo;
    std::unordered_map<VkDescriptorPool, DescriptorPoolInfo> mDescriptorPoolInfo;
    std::unordered_map<VkDescriptorSet, DescriptorSetInfo> mDescriptorSetInfo;

#ifdef _WIN32
    int mSemaphoreId = 1;
    int genSemaphoreId() {
        if (mSemaphoreId == -1) {
            mSemaphoreId = 1;
        }
        int res = mSemaphoreId;
        ++mSemaphoreId;
        return res;
    }
    std::unordered_map<int, VkSemaphore> mExternalSemaphoresById;
#endif
    std::unordered_map<VkDescriptorUpdateTemplate, DescriptorUpdateTemplateInfo> mDescriptorUpdateTemplateInfo;

    VkDecoderSnapshot mSnapshot;

    std::vector<uint64_t> mCreatedHandlesForSnapshotLoad;
    size_t mCreatedHandlesForSnapshotLoadIndex = 0;

    Lock mOccupiedGpasLock;
    // Back-reference to the VkDeviceMemory that is occupying a particular
    // guest physical address
    struct OccupiedGpaInfo {
        VulkanDispatch* vk;
        VkDevice device;
        VkDeviceMemory memory;
        uint64_t gpa;
        size_t sizeToPage;
    };
    std::unordered_map<uint64_t, OccupiedGpaInfo> mOccupiedGpas;

    struct LinearImageCreateInfo {
        VkExtent3D extent;
        VkFormat format;
        VkImageUsageFlags usage;

        VkImageCreateInfo toDefaultVk() const {
            return VkImageCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = {},
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = format,
                    .extent = extent,
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .tiling = VK_IMAGE_TILING_LINEAR,
                    .usage = usage,
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = 0,
                    .pQueueFamilyIndices = nullptr,
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
        }

        struct Hash {
            std::size_t operator()(const LinearImageCreateInfo& ci) const {
                std::size_t s = 0;
                // Magic number used in boost::hash_combine().
                constexpr size_t kHashMagic = 0x9e3779b9;
                s ^= std::hash<uint32_t>{}(ci.extent.width) + kHashMagic +
                     (s << 6) + (s >> 2);
                s ^= std::hash<uint32_t>{}(ci.extent.height) + kHashMagic +
                     (s << 6) + (s >> 2);
                s ^= std::hash<uint32_t>{}(ci.extent.depth) + kHashMagic +
                     (s << 6) + (s >> 2);
                s ^= std::hash<VkFormat>{}(ci.format) + kHashMagic + (s << 6) +
                     (s >> 2);
                s ^= std::hash<VkImageUsageFlags>{}(ci.usage) + kHashMagic +
                     (s << 6) + (s >> 2);
                return s;
            }
        };
    };

    friend bool operator==(const LinearImageCreateInfo& a,
                           const LinearImageCreateInfo& b) {
        return a.extent.width == b.extent.width &&
               a.extent.height == b.extent.height &&
               a.extent.depth == b.extent.depth && a.format == b.format &&
               a.usage == b.usage;
    }

    struct LinearImageProperties {
        VkDeviceSize offset;
        VkDeviceSize rowPitchAlignment;
    };

    // TODO(liyl): Remove after removing the old vkGetLinearImageLayoutGOOGLE.
    std::unordered_map<VkFormat, LinearImageProperties>
            mPerFormatLinearImageProperties;

    std::unordered_map<LinearImageCreateInfo,
                       LinearImageProperties,
                       LinearImageCreateInfo::Hash>
            mLinearImageProperties;
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

// Snapshots
bool VkDecoderGlobalState::snapshotsEnabled() const {
    return mImpl->snapshotsEnabled();
}

bool VkDecoderGlobalState::vkCleanupEnabled() const {
    return mImpl->vkCleanupEnabled();
}

void VkDecoderGlobalState::save(android::base::Stream* stream) {
    mImpl->save(stream);
}

void VkDecoderGlobalState::load(android::base::Stream* stream) {
    mImpl->load(stream);
}

void VkDecoderGlobalState::lock() {
    mImpl->lock();
}

void VkDecoderGlobalState::unlock() {
    mImpl->unlock();
}

size_t VkDecoderGlobalState::setCreatedHandlesForSnapshotLoad(const unsigned char* buffer) {
    return mImpl->setCreatedHandlesForSnapshotLoad(buffer);
}

void VkDecoderGlobalState::clearCreatedHandlesForSnapshotLoad() {
    mImpl->clearCreatedHandlesForSnapshotLoad();
}

VkResult VkDecoderGlobalState::on_vkEnumerateInstanceVersion(
    android::base::BumpPool* pool,
    uint32_t* pApiVersion) {
    return mImpl->on_vkEnumerateInstanceVersion(pool, pApiVersion);
}

VkResult VkDecoderGlobalState::on_vkCreateInstance(
    android::base::BumpPool* pool,
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    return mImpl->on_vkCreateInstance(pool, pCreateInfo, pAllocator, pInstance);
}

void VkDecoderGlobalState::on_vkDestroyInstance(
    android::base::BumpPool* pool,
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyInstance(pool, instance, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkEnumeratePhysicalDevices(
    android::base::BumpPool* pool,
    VkInstance instance,
    uint32_t* physicalDeviceCount,
    VkPhysicalDevice* physicalDevices) {
    return mImpl->on_vkEnumeratePhysicalDevices(pool, instance, physicalDeviceCount, physicalDevices);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures(pool, physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(pool, physicalDevice, pFeatures);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFeatures2KHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures2KHR* pFeatures) {
    mImpl->on_vkGetPhysicalDeviceFeatures2(pool, physicalDevice, pFeatures);
}

VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties(
            pool, physicalDevice, format, type, tiling, usage, flags,
            pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            pool, physicalDevice, pImageFormatInfo, pImageFormatProperties);
}
VkResult VkDecoderGlobalState::on_vkGetPhysicalDeviceImageFormatProperties2KHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
    VkImageFormatProperties2* pImageFormatProperties) {
    return mImpl->on_vkGetPhysicalDeviceImageFormatProperties2(
            pool, physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties(pool, physicalDevice, format,
                                                  pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(pool, physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceFormatProperties2KHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties2* pFormatProperties) {
    mImpl->on_vkGetPhysicalDeviceFormatProperties2(pool, physicalDevice, format,
                                                   pFormatProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceProperties2KHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties2* pProperties) {
    mImpl->on_vkGetPhysicalDeviceProperties2(pool, physicalDevice, pProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties(
        pool, physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        pool, physicalDevice, pMemoryProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceMemoryProperties2KHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties2* pMemoryProperties) {
    mImpl->on_vkGetPhysicalDeviceMemoryProperties2(
        pool, physicalDevice, pMemoryProperties);
}

VkResult VkDecoderGlobalState::on_vkEnumerateDeviceExtensionProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) {
    return mImpl->on_vkEnumerateDeviceExtensionProperties(
        pool, physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult VkDecoderGlobalState::on_vkCreateDevice(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    return mImpl->on_vkCreateDevice(pool, physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void VkDecoderGlobalState::on_vkGetDeviceQueue(
    android::base::BumpPool* pool,
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue) {
    mImpl->on_vkGetDeviceQueue(pool, device, queueFamilyIndex, queueIndex, pQueue);
}

void VkDecoderGlobalState::on_vkDestroyDevice(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDevice(pool, device, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateBuffer(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer) {
    return mImpl->on_vkCreateBuffer(pool, device, pCreateInfo, pAllocator, pBuffer);
}

void VkDecoderGlobalState::on_vkDestroyBuffer(
    android::base::BumpPool* pool,
    VkDevice device,
    VkBuffer buffer,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyBuffer(pool, device, buffer, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory(
    android::base::BumpPool* pool,
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset) {
    return mImpl->on_vkBindBufferMemory(pool, device, buffer, memory, memoryOffset);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2(
    android::base::BumpPool* pool,
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2(pool, device, bindInfoCount, pBindInfos);
}

VkResult VkDecoderGlobalState::on_vkBindBufferMemory2KHR(
    android::base::BumpPool* pool,
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos) {
    return mImpl->on_vkBindBufferMemory2KHR(pool, device, bindInfoCount, pBindInfos);
}

VkResult VkDecoderGlobalState::on_vkCreateImage(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage* pImage) {
    return mImpl->on_vkCreateImage(pool, device, pCreateInfo, pAllocator, pImage);
}

void VkDecoderGlobalState::on_vkDestroyImage(
    android::base::BumpPool* pool,
    VkDevice device,
    VkImage image,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImage(pool, device, image, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkBindImageMemory(android::base::BumpPool* pool,
                                                    VkDevice device,
                                                    VkImage image,
                                                    VkDeviceMemory memory,
                                                    VkDeviceSize memoryOffset) {
    return mImpl->on_vkBindImageMemory(pool, device, image, memory,
                                       memoryOffset);
}

VkResult VkDecoderGlobalState::on_vkCreateImageView(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView) {
    return mImpl->on_vkCreateImageView(pool, device, pCreateInfo, pAllocator, pView);
}

void VkDecoderGlobalState::on_vkDestroyImageView(
    android::base::BumpPool* pool,
    VkDevice device,
    VkImageView imageView,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyImageView(pool, device, imageView, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateSampler(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSampler* pSampler) {
    return mImpl->on_vkCreateSampler(pool, device, pCreateInfo, pAllocator, pSampler);
}

void VkDecoderGlobalState::on_vkDestroySampler(
    android::base::BumpPool* pool,
    VkDevice device,
    VkSampler sampler,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroySampler(pool, device, sampler, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateSemaphore(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore) {
    return mImpl->on_vkCreateSemaphore(pool, device, pCreateInfo, pAllocator, pSemaphore);
}

VkResult VkDecoderGlobalState::on_vkImportSemaphoreFdKHR(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) {
    return mImpl->on_vkImportSemaphoreFdKHR(pool, device, pImportSemaphoreFdInfo);
}

VkResult VkDecoderGlobalState::on_vkGetSemaphoreFdKHR(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
    int* pFd) {
    return mImpl->on_vkGetSemaphoreFdKHR(pool, device, pGetFdInfo, pFd);
}

void VkDecoderGlobalState::on_vkDestroySemaphore(
    android::base::BumpPool* pool,
    VkDevice device,
    VkSemaphore semaphore,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroySemaphore(pool, device, semaphore, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateFence(
        android::base::BumpPool* pool,
        VkDevice device,
        const VkFenceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkFence* pFence) {
    return mImpl->on_vkCreateFence(pool, device, pCreateInfo, pAllocator,
                                   pFence);
}

VkResult VkDecoderGlobalState::on_vkResetFences(android::base::BumpPool* pool,
                                                VkDevice device,
                                                uint32_t fenceCount,
                                                const VkFence* pFences) {
    return mImpl->on_vkResetFences(pool, device, fenceCount, pFences);
}

void VkDecoderGlobalState::on_vkDestroyFence(
        android::base::BumpPool* pool,
        VkDevice device,
        VkFence fence,
        const VkAllocationCallbacks* pAllocator) {
    return mImpl->on_vkDestroyFence(pool, device, fence, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateDescriptorSetLayout(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pSetLayout) {
    return mImpl->on_vkCreateDescriptorSetLayout(pool, device, pCreateInfo, pAllocator, pSetLayout);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorSetLayout(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorSetLayout(pool, device, descriptorSetLayout, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkCreateDescriptorPool(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorPool* pDescriptorPool) {
    return mImpl->on_vkCreateDescriptorPool(pool, device, pCreateInfo, pAllocator, pDescriptorPool);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorPool(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDescriptorPool descriptorPool,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorPool(
        pool, device, descriptorPool, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkResetDescriptorPool(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorPoolResetFlags flags) {
    return mImpl->on_vkResetDescriptorPool(
        pool, device, descriptorPool, flags);
}

VkResult VkDecoderGlobalState::on_vkAllocateDescriptorSets(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet* pDescriptorSets) {
    return mImpl->on_vkAllocateDescriptorSets(
        pool, device, pAllocateInfo, pDescriptorSets);
}

VkResult VkDecoderGlobalState::on_vkFreeDescriptorSets(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets) {
    return mImpl->on_vkFreeDescriptorSets(
        pool, device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void VkDecoderGlobalState::on_vkUpdateDescriptorSets(
    android::base::BumpPool* pool,
    VkDevice device,
    uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies) {
    mImpl->on_vkUpdateDescriptorSets(pool, device, descriptorWriteCount,
                                     pDescriptorWrites, descriptorCopyCount,
                                     pDescriptorCopies);
}

void VkDecoderGlobalState::on_vkCmdCopyBufferToImage(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    VkBuffer srcBuffer,
    VkImage dstImage,
    VkImageLayout dstImageLayout,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions) {
    mImpl->on_vkCmdCopyBufferToImage(pool, commandBuffer, srcBuffer, dstImage,
            dstImageLayout, regionCount, pRegions);
}

void VkDecoderGlobalState::on_vkCmdCopyImage(android::base::BumpPool* pool,
                                             VkCommandBuffer commandBuffer,
                                             VkImage srcImage,
                                             VkImageLayout srcImageLayout,
                                             VkImage dstImage,
                                             VkImageLayout dstImageLayout,
                                             uint32_t regionCount,
                                             const VkImageCopy* pRegions) {
    mImpl->on_vkCmdCopyImage(pool, commandBuffer, srcImage, srcImageLayout,
                             dstImage, dstImageLayout, regionCount, pRegions);
}
void VkDecoderGlobalState::on_vkCmdCopyImageToBuffer(
        android::base::BumpPool* pool,
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkBuffer dstBuffer,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions) {
    mImpl->on_vkCmdCopyImageToBuffer(pool, commandBuffer, srcImage,
                                     srcImageLayout, dstBuffer, regionCount,
                                     pRegions);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements(
        android::base::BumpPool* pool,
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements(pool, device, image,
                                           pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements2(
        android::base::BumpPool* pool,
        VkDevice device,
        const VkImageMemoryRequirementsInfo2* pInfo,
        VkMemoryRequirements2* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements2(pool, device, pInfo,
                                            pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkGetImageMemoryRequirements2KHR(
        android::base::BumpPool* pool,
        VkDevice device,
        const VkImageMemoryRequirementsInfo2* pInfo,
        VkMemoryRequirements2* pMemoryRequirements) {
    mImpl->on_vkGetImageMemoryRequirements2(pool, device, pInfo,
                                            pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkCmdPipelineBarrier(
        android::base::BumpPool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const VkImageMemoryBarrier* pImageMemoryBarriers) {
    mImpl->on_vkCmdPipelineBarrier(
            pool, commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
            memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
            pBufferMemoryBarriers, imageMemoryBarrierCount,
            pImageMemoryBarriers);
}

VkResult VkDecoderGlobalState::on_vkAllocateMemory(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory) {
    return mImpl->on_vkAllocateMemory(pool, device, pAllocateInfo, pAllocator, pMemory);
}

void VkDecoderGlobalState::on_vkFreeMemory(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkFreeMemory(pool, device, memory, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkMapMemory(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkMemoryMapFlags flags,
    void** ppData) {
    return mImpl->on_vkMapMemory(pool, device, memory, offset, size, flags, ppData);
}

void VkDecoderGlobalState::on_vkUnmapMemory(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDeviceMemory memory) {
    mImpl->on_vkUnmapMemory(pool, device, memory);
}

uint8_t* VkDecoderGlobalState::getMappedHostPointer(VkDeviceMemory memory) {
    return mImpl->getMappedHostPointer(memory);
}

VkDeviceSize VkDecoderGlobalState::getDeviceMemorySize(VkDeviceMemory memory) {
    return mImpl->getDeviceMemorySize(memory);
}

bool VkDecoderGlobalState::usingDirectMapping() const {
    return mImpl->usingDirectMapping();
}

VkDecoderGlobalState::HostFeatureSupport
VkDecoderGlobalState::getHostFeatureSupport() const {
    return mImpl->getHostFeatureSupport();
}

// VK_ANDROID_native_buffer
VkResult VkDecoderGlobalState::on_vkGetSwapchainGrallocUsageANDROID(
    android::base::BumpPool* pool,
    VkDevice device,
    VkFormat format,
    VkImageUsageFlags imageUsage,
    int* grallocUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsageANDROID(
        pool, device, format, imageUsage, grallocUsage);
}

VkResult VkDecoderGlobalState::on_vkGetSwapchainGrallocUsage2ANDROID(
    android::base::BumpPool* pool,
    VkDevice device,
    VkFormat format,
    VkImageUsageFlags imageUsage,
    VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
    uint64_t* grallocConsumerUsage,
    uint64_t* grallocProducerUsage) {
    return mImpl->on_vkGetSwapchainGrallocUsage2ANDROID(
        pool, device, format, imageUsage,
        swapchainImageUsage,
        grallocConsumerUsage,
        grallocProducerUsage);
}

VkResult VkDecoderGlobalState::on_vkAcquireImageANDROID(
    android::base::BumpPool* pool,
    VkDevice device,
    VkImage image,
    int nativeFenceFd,
    VkSemaphore semaphore,
    VkFence fence) {
    return mImpl->on_vkAcquireImageANDROID(
        pool, device, image, nativeFenceFd, semaphore, fence);
}

VkResult VkDecoderGlobalState::on_vkQueueSignalReleaseImageANDROID(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    VkImage image,
    int* pNativeFenceFd) {
    return mImpl->on_vkQueueSignalReleaseImageANDROID(
        pool, queue, waitSemaphoreCount, pWaitSemaphores,
        image, pNativeFenceFd);
}

// VK_GOOGLE_gfxstream
VkResult VkDecoderGlobalState::on_vkMapMemoryIntoAddressSpaceGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device, VkDeviceMemory memory, uint64_t* pAddress) {
    return mImpl->on_vkMapMemoryIntoAddressSpaceGOOGLE(
        pool, device, memory, pAddress);
}
VkResult VkDecoderGlobalState::on_vkGetMemoryHostAddressInfoGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device, VkDeviceMemory memory,
    uint64_t* pAddress, uint64_t* pSize, uint64_t* pHostmemId) {
    return mImpl->on_vkGetMemoryHostAddressInfoGOOGLE(
        pool, device, memory, pAddress, pSize, pHostmemId);
}

// VK_GOOGLE_gfxstream
VkResult VkDecoderGlobalState::on_vkFreeMemorySyncGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDeviceMemory memory,
    const VkAllocationCallbacks* pAllocator) {
    return mImpl->on_vkFreeMemorySyncGOOGLE(pool, device, memory, pAllocator);
}


// VK_GOOGLE_color_buffer
VkResult VkDecoderGlobalState::on_vkRegisterImageColorBufferGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device, VkImage image, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterImageColorBufferGOOGLE(
        pool, device, image, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkRegisterBufferColorBufferGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device, VkBuffer buffer, uint32_t colorBuffer) {
    return mImpl->on_vkRegisterBufferColorBufferGOOGLE(
        pool, device, buffer, colorBuffer);
}

VkResult VkDecoderGlobalState::on_vkAllocateCommandBuffers(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkAllocateCommandBuffers(pool, device, pAllocateInfo,
                                              pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkCreateCommandPool(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool) {
    return mImpl->on_vkCreateCommandPool(pool, device, pCreateInfo, pAllocator,
                                         pCommandPool);
}

void VkDecoderGlobalState::on_vkDestroyCommandPool(
    android::base::BumpPool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyCommandPool(pool, device, commandPool, pAllocator);
}

VkResult VkDecoderGlobalState::on_vkResetCommandPool(
    android::base::BumpPool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    VkCommandPoolResetFlags flags) {
    return mImpl->on_vkResetCommandPool(pool, device, commandPool, flags);
}

void VkDecoderGlobalState::on_vkCmdExecuteCommands(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkCmdExecuteCommands(pool, commandBuffer, commandBufferCount,
                                          pCommandBuffers);
}

VkResult VkDecoderGlobalState::on_vkQueueSubmit(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence) {
    return mImpl->on_vkQueueSubmit(pool, queue, submitCount, pSubmits, fence);
}

VkResult VkDecoderGlobalState::on_vkQueueWaitIdle(
        android::base::BumpPool* pool,
        VkQueue queue) {
    return mImpl->on_vkQueueWaitIdle(pool, queue);
}

VkResult VkDecoderGlobalState::on_vkResetCommandBuffer(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags) {
    return mImpl->on_vkResetCommandBuffer(pool, commandBuffer, flags);
}

void VkDecoderGlobalState::on_vkFreeCommandBuffers(
    android::base::BumpPool* pool,
    VkDevice device,
    VkCommandPool commandPool,
    uint32_t commandBufferCount,
    const VkCommandBuffer* pCommandBuffers) {
    return mImpl->on_vkFreeCommandBuffers(pool, device, commandPool,
                                          commandBufferCount, pCommandBuffers);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceExternalSemaphoreProperties(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {
    return mImpl->on_vkGetPhysicalDeviceExternalSemaphoreProperties(
            pool, physicalDevice, pExternalSemaphoreInfo,
            pExternalSemaphoreProperties);
}

void VkDecoderGlobalState::on_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(
    android::base::BumpPool* pool,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceExternalSemaphoreInfo* pExternalSemaphoreInfo,
    VkExternalSemaphoreProperties* pExternalSemaphoreProperties) {
    return mImpl->on_vkGetPhysicalDeviceExternalSemaphoreProperties(
            pool, physicalDevice, pExternalSemaphoreInfo,
            pExternalSemaphoreProperties);
}

// Descriptor update templates
VkResult VkDecoderGlobalState::on_vkCreateDescriptorUpdateTemplate(
        android::base::BumpPool* pool,
    VkDevice boxed_device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
    return mImpl->on_vkCreateDescriptorUpdateTemplate(
        pool, boxed_device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VkResult VkDecoderGlobalState::on_vkCreateDescriptorUpdateTemplateKHR(
        android::base::BumpPool* pool,
    VkDevice boxed_device,
    const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
    return mImpl->on_vkCreateDescriptorUpdateTemplateKHR(
        pool, boxed_device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorUpdateTemplate(
        android::base::BumpPool* pool,
    VkDevice boxed_device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorUpdateTemplate(
        pool, boxed_device, descriptorUpdateTemplate, pAllocator);
}

void VkDecoderGlobalState::on_vkDestroyDescriptorUpdateTemplateKHR(
        android::base::BumpPool* pool,
    VkDevice boxed_device,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    const VkAllocationCallbacks* pAllocator) {
    mImpl->on_vkDestroyDescriptorUpdateTemplateKHR(
        pool, boxed_device, descriptorUpdateTemplate, pAllocator);
}

void VkDecoderGlobalState::on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
        android::base::BumpPool* pool,
    VkDevice boxed_device,
    VkDescriptorSet descriptorSet,
    VkDescriptorUpdateTemplate descriptorUpdateTemplate,
    uint32_t imageInfoCount,
    uint32_t bufferInfoCount,
    uint32_t bufferViewCount,
    const uint32_t* pImageInfoEntryIndices,
    const uint32_t* pBufferInfoEntryIndices,
    const uint32_t* pBufferViewEntryIndices,
    const VkDescriptorImageInfo* pImageInfos,
    const VkDescriptorBufferInfo* pBufferInfos,
    const VkBufferView* pBufferViews) {
    mImpl->on_vkUpdateDescriptorSetWithTemplateSizedGOOGLE(
        pool, boxed_device,
        descriptorSet,
        descriptorUpdateTemplate,
        imageInfoCount,
        bufferInfoCount,
        bufferViewCount,
        pImageInfoEntryIndices,
        pBufferInfoEntryIndices,
        pBufferViewEntryIndices,
        pImageInfos,
        pBufferInfos,
        pBufferViews);
}

VkResult VkDecoderGlobalState::on_vkBeginCommandBuffer(
        android::base::BumpPool* pool,
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo) {
    return mImpl->on_vkBeginCommandBuffer(pool, commandBuffer, pBeginInfo);
}

void VkDecoderGlobalState::on_vkBeginCommandBufferAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    const VkCommandBufferBeginInfo* pBeginInfo) {
    mImpl->on_vkBeginCommandBuffer(pool, commandBuffer, pBeginInfo);
}

void VkDecoderGlobalState::on_vkEndCommandBufferAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer) {
    mImpl->on_vkEndCommandBufferAsyncGOOGLE(
        pool, commandBuffer);
}

void VkDecoderGlobalState::on_vkResetCommandBufferAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    VkCommandBufferResetFlags flags) {
    mImpl->on_vkResetCommandBufferAsyncGOOGLE(
        pool, commandBuffer, flags);
}

void VkDecoderGlobalState::on_vkCommandBufferHostSyncGOOGLE(
    android::base::BumpPool* pool,
    VkCommandBuffer commandBuffer,
    uint32_t needHostSync,
    uint32_t sequenceNumber) {
    mImpl->hostSyncCommandBuffer("hostSync", commandBuffer, needHostSync, sequenceNumber);
}

VkResult VkDecoderGlobalState::on_vkCreateImageWithRequirementsGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImage* pImage,
    VkMemoryRequirements* pMemoryRequirements) {
    return mImpl->on_vkCreateImageWithRequirementsGOOGLE(
            pool, device, pCreateInfo, pAllocator, pImage, pMemoryRequirements);
}

VkResult VkDecoderGlobalState::on_vkCreateBufferWithRequirementsGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer,
    VkMemoryRequirements* pMemoryRequirements) {
    return mImpl->on_vkCreateBufferWithRequirementsGOOGLE(
            pool, device, pCreateInfo, pAllocator, pBuffer, pMemoryRequirements);
}

void VkDecoderGlobalState::on_vkCmdBindPipeline(
        android::base::BumpPool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint pipelineBindPoint,
        VkPipeline pipeline) {
    mImpl->on_vkCmdBindPipeline(pool, commandBuffer, pipelineBindPoint,
                                pipeline);
}

void VkDecoderGlobalState::on_vkCmdBindDescriptorSets(
        android::base::BumpPool* pool,
        VkCommandBuffer commandBuffer,
        VkPipelineBindPoint pipelineBindPoint,
        VkPipelineLayout layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets) {
    mImpl->on_vkCmdBindDescriptorSets(pool, commandBuffer, pipelineBindPoint,
                                      layout, firstSet, descriptorSetCount,
                                      pDescriptorSets, dynamicOffsetCount,
                                      pDynamicOffsets);
}

VkResult VkDecoderGlobalState::on_vkCreateRenderPass(
        android::base::BumpPool* pool,
        VkDevice boxed_device,
        const VkRenderPassCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass) {
    return mImpl->on_vkCreateRenderPass(pool, boxed_device, pCreateInfo,
                                        pAllocator, pRenderPass);
}

void VkDecoderGlobalState::on_vkQueueHostSyncGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t needHostSync,
    uint32_t sequenceNumber) {
    mImpl->hostSyncQueue("hostSyncQueue", queue, needHostSync, sequenceNumber);
}

void VkDecoderGlobalState::on_vkQueueSubmitAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence) {
    mImpl->on_vkQueueSubmit(pool, queue, submitCount, pSubmits, fence);
}

void VkDecoderGlobalState::on_vkQueueWaitIdleAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue) {
    mImpl->on_vkQueueWaitIdle(pool, queue);
}

void VkDecoderGlobalState::on_vkQueueBindSparseAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t bindInfoCount,
    const VkBindSparseInfo* pBindInfo, VkFence fence) {
    mImpl->on_vkQueueBindSparse(pool, queue, bindInfoCount, pBindInfo, fence);
}

void VkDecoderGlobalState::on_vkGetLinearImageLayoutGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device,
    VkFormat format,
    VkDeviceSize* pOffset,
    VkDeviceSize* pRowPitchAlignment) {
    mImpl->on_vkGetLinearImageLayoutGOOGLE(pool, device, format, pOffset, pRowPitchAlignment);
}

void VkDecoderGlobalState::on_vkGetLinearImageLayout2GOOGLE(
        android::base::BumpPool* pool,
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        VkDeviceSize* pOffset,
        VkDeviceSize* pRowPitchAlignment) {
    mImpl->on_vkGetLinearImageLayout2GOOGLE(pool, device, pCreateInfo, pOffset,
                                            pRowPitchAlignment);
}

void VkDecoderGlobalState::on_vkQueueFlushCommandsGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    VkCommandBuffer commandBuffer,
    VkDeviceSize dataSize,
    const void* pData) {
    mImpl->on_vkQueueFlushCommandsGOOGLE(pool, queue, commandBuffer, dataSize, pData);
}

void VkDecoderGlobalState::on_vkQueueCommitDescriptorSetUpdatesGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t descriptorPoolCount,
    const VkDescriptorPool* pDescriptorPools,
    uint32_t descriptorSetCount,
    const VkDescriptorSetLayout* pDescriptorSetLayouts,
    const uint64_t* pDescriptorSetPoolIds,
    const uint32_t* pDescriptorSetWhichPool,
    const uint32_t* pDescriptorSetPendingAllocation,
    const uint32_t* pDescriptorWriteStartingIndices,
    uint32_t pendingDescriptorWriteCount,
    const VkWriteDescriptorSet* pPendingDescriptorWrites) {
    mImpl->on_vkQueueCommitDescriptorSetUpdatesGOOGLE(pool, queue, descriptorPoolCount, pDescriptorPools, descriptorSetCount, pDescriptorSetLayouts, pDescriptorSetPoolIds, pDescriptorSetWhichPool, pDescriptorSetPendingAllocation, pDescriptorWriteStartingIndices, pendingDescriptorWriteCount, pPendingDescriptorWrites);
}

void VkDecoderGlobalState::on_vkCollectDescriptorPoolIdsGOOGLE(
    android::base::BumpPool* pool,
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t* pPoolIdCount,
    uint64_t* pPoolIds) {
    mImpl->on_vkCollectDescriptorPoolIdsGOOGLE(pool, device, descriptorPool, pPoolIdCount, pPoolIds);
}

VkResult VkDecoderGlobalState::on_vkQueueBindSparse(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t bindInfoCount,
    const VkBindSparseInfo* pBindInfo, VkFence fence) {
    return mImpl->on_vkQueueBindSparse(pool, queue, bindInfoCount, pBindInfo, fence);
}

void VkDecoderGlobalState::on_vkQueueSignalReleaseImageANDROIDAsyncGOOGLE(
    android::base::BumpPool* pool,
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    VkImage image) {
    int fenceFd;
    mImpl->on_vkQueueSignalReleaseImageANDROID(
        pool, queue, waitSemaphoreCount, pWaitSemaphores,
        image, &fenceFd);
}

VkResult VkDecoderGlobalState::waitForFence(VkFence boxed_fence,
                                            uint64_t timeout) {
    return mImpl->waitForFence(boxed_fence, timeout);
}

VkResult VkDecoderGlobalState::getFenceStatus(VkFence boxed_fence) {
    return mImpl->getFenceStatus(boxed_fence);
}

void VkDecoderGlobalState::deviceMemoryTransform_tohost(
    VkDeviceMemory* memory, uint32_t memoryCount,
    VkDeviceSize* offset, uint32_t offsetCount,
    VkDeviceSize* size, uint32_t sizeCount,
    uint32_t* typeIndex, uint32_t typeIndexCount,
    uint32_t* typeBits, uint32_t typeBitsCount) {
    // Not used currently
    (void)memory; (void)memoryCount;
    (void)offset; (void)offsetCount;
    (void)size; (void)sizeCount;
    (void)typeIndex; (void)typeIndexCount;
    (void)typeBits; (void)typeBitsCount;
}

void VkDecoderGlobalState::deviceMemoryTransform_fromhost(
    VkDeviceMemory* memory, uint32_t memoryCount,
    VkDeviceSize* offset, uint32_t offsetCount,
    VkDeviceSize* size, uint32_t sizeCount,
    uint32_t* typeIndex, uint32_t typeIndexCount,
    uint32_t* typeBits, uint32_t typeBitsCount) {
    // Not used currently
    (void)memory; (void)memoryCount;
    (void)offset; (void)offsetCount;
    (void)size; (void)sizeCount;
    (void)typeIndex; (void)typeIndexCount;
    (void)typeBits; (void)typeBitsCount;
}

VkDecoderSnapshot* VkDecoderGlobalState::snapshot() {
    return mImpl->snapshot();
}

#define DEFINE_TRANSFORMED_TYPE_IMPL(type) \
    void VkDecoderGlobalState::transformImpl_##type##_tohost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_tohost(val, count); \
    } \
    void VkDecoderGlobalState::transformImpl_##type##_fromhost(const type* val, uint32_t count) { \
        mImpl->transformImpl_##type##_fromhost(val, count); \
    } \

LIST_TRANSFORMED_TYPES(DEFINE_TRANSFORMED_TYPE_IMPL)

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEF(type) \
    type VkDecoderGlobalState::new_boxed_##type(type underlying, VulkanDispatch* dispatch, bool ownDispatch) { \
        return mImpl->new_boxed_##type(underlying, dispatch, ownDispatch); \
    } \
    void VkDecoderGlobalState::delete_##type(type boxed) { \
        mImpl->delete_##type(boxed); \
    } \
    type VkDecoderGlobalState::unbox_##type(type boxed) { \
        return mImpl->unbox_##type(boxed); \
    } \
    type VkDecoderGlobalState::unboxed_to_boxed_##type(type unboxed) { \
        return mImpl->unboxed_to_boxed_##type(unboxed); \
    } \
    VulkanDispatch* VkDecoderGlobalState::dispatch_##type(type boxed) { \
        return mImpl->dispatch_##type(boxed); \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEF(type) \
    type VkDecoderGlobalState::new_boxed_non_dispatchable_##type(type underlying) { \
        return mImpl->new_boxed_non_dispatchable_##type(underlying); \
    } \
    void VkDecoderGlobalState::delete_##type(type boxed) { \
        mImpl->delete_##type(boxed); \
    } \
    type VkDecoderGlobalState::unbox_##type(type boxed) { \
        return mImpl->unbox_##type(boxed); \
    } \
    type VkDecoderGlobalState::unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        return mImpl->unboxed_to_boxed_non_dispatchable_##type(unboxed); \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_API_DEF)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_API_DEF)

#define DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DEF(type) \
    type unbox_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) return VK_NULL_HANDLE; \
        return (type)elt->underlying; \
    } \
    VulkanDispatch* dispatch_##type(type boxed) { \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: err not found boxed %p\n", __func__, boxed); return nullptr; } \
        return elt->dispatch; \
    } \
    void delete_##type(type boxed) { \
        if (!boxed) return; \
        auto elt = sBoxedHandleManager.get( \
            (uint64_t)(uintptr_t)boxed); \
        if (!elt) return; \
        releaseOrderMaintInfo(elt->ordMaintInfo); \
        if (elt->readStream) { \
            sReadStreamRegistry.push(elt->readStream); \
            elt->readStream = nullptr; \
        } \
        sBoxedHandleManager.remove((uint64_t)boxed); \
    } \
    type unboxed_to_boxed_##type(type unboxed) { \
        AutoLock lock(sBoxedHandleManager.lock); \
        return (type)sBoxedHandleManager.getBoxedFromUnboxedLocked( \
                (uint64_t)(uintptr_t)unboxed); \
    } \

#define DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DEF(type) \
    type new_boxed_non_dispatchable_##type(type underlying) { \
        return VkDecoderGlobalState::get()->new_boxed_non_dispatchable_##type(underlying); \
    } \
    void delete_##type(type boxed) { \
        if (!boxed) return; \
        sBoxedHandleManager.remove((uint64_t)boxed); \
    } \
    void delayed_delete_##type(type boxed, VkDevice device, std::function<void()> callback) { \
        sBoxedHandleManager.removeDelayed((uint64_t)boxed, device, callback); \
    } \
    type unbox_##type(type boxed) { \
        if (!boxed) return boxed; \
        auto elt = sBoxedHandleManager.get( \
                (uint64_t)(uintptr_t)boxed); \
        if (!elt) { fprintf(stderr, "%s: unbox %p failed, not found\n", __func__, boxed); abort(); return VK_NULL_HANDLE; } \
        return (type)elt->underlying; \
    } \
    type unboxed_to_boxed_non_dispatchable_##type(type unboxed) { \
        AutoLock lock(sBoxedHandleManager.lock); \
        return (type)sBoxedHandleManager.getBoxedFromUnboxedLocked( \
                (uint64_t)(uintptr_t)unboxed); \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_DISPATCHABLE_HANDLE_GLOBAL_API_DEF)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(DEFINE_BOXED_NON_DISPATCHABLE_HANDLE_GLOBAL_API_DEF)

void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::setup(android::base::BumpPool* pool, uint64_t** bufPtr) {
    mPool = pool;
    mPreserveBufPtr = bufPtr;
}

void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::allocPreserve(size_t count) {
    *mPreserveBufPtr = (uint64_t*)mPool->alloc(count * sizeof(uint64_t));
}

#define BOXED_DISPATCHABLE_HANDLE_UNWRAP_AND_DELETE_PRESERVE_BOXED_IMPL(type_name) \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_##type_name(type_name* handles, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handles[i]); \
            if (handles[i]) { handles[i] = VkDecoderGlobalState::get()->unbox_##type_name(handles[i]); } else { handles[i] = nullptr; } ; \
        } \
    } \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_##type_name##_u64(const type_name* handles, uint64_t* handle_u64s, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handle_u64s[i]); \
            if (handles[i]) { handle_u64s[i] = (uint64_t)VkDecoderGlobalState::get()->unbox_##type_name(handles[i]); } else { handle_u64s[i] = 0; } \
        } \
    } \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_u64_##type_name(const uint64_t* handle_u64s, type_name* handles, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handle_u64s[i]); \
            if (handle_u64s[i]) { handles[i] = VkDecoderGlobalState::get()->unbox_##type_name((type_name)(uintptr_t)handle_u64s[i]); } else { handles[i] = nullptr; } \
        } \
    } \

#define BOXED_NON_DISPATCHABLE_HANDLE_UNWRAP_AND_DELETE_PRESERVE_BOXED_IMPL(type_name) \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_##type_name(type_name* handles, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handles[i]); \
            if (handles[i]) { auto boxed = handles[i]; handles[i] = VkDecoderGlobalState::get()->unbox_##type_name(handles[i]); delete_##type_name(boxed); } else { handles[i] = nullptr; }; \
        } \
    } \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_##type_name##_u64(const type_name* handles, uint64_t* handle_u64s, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handle_u64s[i]); \
            if (handles[i]) { auto boxed = handles[i]; handle_u64s[i] = (uint64_t)VkDecoderGlobalState::get()->unbox_##type_name(handles[i]); delete_##type_name(boxed); } else { handle_u64s[i] = 0; } \
        } \
    } \
    void BoxedHandleUnwrapAndDeletePreserveBoxedMapping::mapHandles_u64_##type_name(const uint64_t* handle_u64s, type_name* handles, size_t count) { \
        allocPreserve(count); \
        for (size_t i = 0; i < count; ++i) { \
            (*mPreserveBufPtr)[i] = (uint64_t)(handle_u64s[i]); \
            if (handle_u64s[i]) { auto boxed = (type_name)(uintptr_t)handle_u64s[i]; handles[i] = VkDecoderGlobalState::get()->unbox_##type_name((type_name)(uintptr_t)handle_u64s[i]); delete_##type_name(boxed); } else { handles[i] = nullptr; } \
        } \
    } \

GOLDFISH_VK_LIST_DISPATCHABLE_HANDLE_TYPES(BOXED_DISPATCHABLE_HANDLE_UNWRAP_AND_DELETE_PRESERVE_BOXED_IMPL)
GOLDFISH_VK_LIST_NON_DISPATCHABLE_HANDLE_TYPES(BOXED_NON_DISPATCHABLE_HANDLE_UNWRAP_AND_DELETE_PRESERVE_BOXED_IMPL)

}  // namespace goldfish_vk
