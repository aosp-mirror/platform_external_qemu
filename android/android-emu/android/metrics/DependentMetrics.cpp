// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/metrics/DependentMetrics.h"

#include <stdlib.h>     // for free
#include <sys/stat.h>   // for stat, st_mtime
#include <cstdint>      // for int64_t, uin...
#include <functional>   // for __base
#include <iosfwd>       // for string
#include <memory>       // for shared_ptr
#include <string>       // for basic_string
#include <string_view>
#include <type_traits>  // for remove_extent_t
#include <utility>      // for pair
#include <vector>       // for vector

#include "render-utils/Renderer.h"                       // for RendererPtr
#include "android/CommonReportedInfo.h"                  // for setDetails
#include "android/avd/info.h"                            // for avdInfo_getA...
#include "android/avd/util.h"                            // for AVD_ANDROID_...
#include "aemu/base/Optional.h"                       // for Optional
#include "aemu/base/StringFormat.h"                   // for StringFormat

#include "aemu/base/Uuid.h"                           // for Uuid
#include "aemu/base/async/ThreadLooper.h"             // for ThreadLooper
#include "aemu/base/files/IniFile.h"                  // for IniFile
#include "aemu/base/files/PathUtils.h"                // for PathUtils
#include "aemu/base/memory/ScopedPtr.h"               // for ScopedCPtr
#include "android/base/system/System.h"                  // for System, Syst...
#include "android/cmdline-definitions.h"                 // for AndroidOptions
#include "android/console.h"                             // for getConsoleAg...
#include "android/cpu_accelerator.h"                     // for ANDROID_CPU_...
#include "android/emulation/CpuAccelerator.h"            // for GetCpuInfo
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/emulation/control/globals_agent.h"     // for QAndroidGlob...
#include "host-common/FeatureControl.h"       // for getDisabledO...
#include "host-common/Features.h"             // for Feature, Pla...
#include "android/metrics/MetricsEngine.h"               // for MetricsEngine
#include "android/metrics/MetricsReporter.h"             // for MetricsReporter
#include "android/metrics/PerfStatReporter.h"            // for PerfStatRepo...
#include "android/metrics/PeriodicReporter.h"            // for PeriodicRepo...
#include "android/metrics/StudioConfig.h"                // for UpdateChannel
#include "android/metrics/metrics.h"                     // for MetricsStopR...
#include "host-common/opengl/emugl_config.h"                 // for emuglConfig_...
#include "android/opengl/gpuinfo.h"                      // for GpuInfo, glo...
#include "host-common/opengles.h"                            // for android_getO...
#include "android/utils/debug.h"                         // for VERBOSE_PRINT
#include "android/utils/file_data.h"                     // for (anonymous)
#include "android/utils/file_io.h"                       // for android_stat
#include "android/utils/x86_cpuid.h"                     // for android_get_...
#include "studio_stats.pb.h"                             // for EmulatorFeat...

using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::System;
using android::metrics::MetricsEngine;
using android::metrics::MetricsReporter;
using android::metrics::PeriodicReporter;

static android_studio::EmulatorDetails::GuestCpuArchitecture
toClearcutLogGuestArch(const char* hw_cpu_arch) {
    using android_studio::EmulatorDetails;

    static constexpr std::pair<std::string_view,
                               EmulatorDetails::GuestCpuArchitecture>
            map[] = {
                    {"x86", EmulatorDetails::X86},
                    {"x86_64", EmulatorDetails::X86_64},
                    {"arm", EmulatorDetails::ARM},
                    {"arm64", EmulatorDetails::ARM_64},
                    {"mips", EmulatorDetails::MIPS},
                    {"mips64", EmulatorDetails::MIPS_64},
            };

    for (const auto& pair : map) {
        if (pair.first == hw_cpu_arch) {
            return pair.second;
        }
    }
    return EmulatorDetails::UNKNOWN_GUEST_CPU_ARCHITECTURE;
}

static android_studio::ProductDetails::CpuArchitecture
toClearcutLogCpuArchitecture(int bitness) {
    using android_studio::ProductDetails;
#ifdef __aarch64__
    return ProductDetails::ARM;
#else
    switch (bitness) {
        case 32:
            return ProductDetails::X86;
        case 64:
            return ProductDetails::X86_64;
        default:
            return ProductDetails::UNKNOWN_CPU_ARCHITECTURE;
    }
#endif
}

static android_studio::ProductDetails::SoftwareLifeCycleChannel
toClearcutLogUpdateChannel(android::studio::UpdateChannel channel) {
    using android::studio::UpdateChannel;
    using android_studio::ProductDetails;
    switch (channel) {
        case UpdateChannel::Stable:
            return ProductDetails::STABLE;
        case UpdateChannel::Beta:
            return ProductDetails::BETA;
        case UpdateChannel::Dev:
            return ProductDetails::DEV;
        case UpdateChannel::Canary:
            return ProductDetails::CANARY;
        default:
            return ProductDetails::UNKNOWN_LIFE_CYCLE_CHANNEL;
    }
}

static android_studio::EmulatorDetails::EmulatorRenderer
toClearcutLogEmulatorRenderer(SelectedRenderer renderer) {
    // As of now, the enum values are exactly the same. Watch out for changes!
    return static_cast<android_studio::EmulatorDetails::EmulatorRenderer>(
            static_cast<int>(renderer));
}

static void fillGuestGlMetrics(android_studio::AndroidStudioEvent* event) {
    char* glVendor = nullptr;
    char* glRenderer = nullptr;
    char* glVersion = nullptr;
    // This call is only sensible after android_startOpenglesRenderer()
    // has been called.
    android_getOpenglesHardwareStrings(&glVendor, &glRenderer, &glVersion);
    if (glVendor) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_vendor(
                glVendor);
        free(glVendor);
    }
    if (glRenderer) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_renderer(
                glRenderer);
        free(glRenderer);
    }
    if (glVersion) {
        event->mutable_emulator_details()->mutable_guest_gl()->set_version(
                glVersion);
        free(glVersion);
    }
}

static Optional<int64_t> getAvdCreationTimeSec(AvdInfo* avd) {
    const char* avdContentPath = avdInfo_getContentPath(avd);
    struct stat st;
    if (avdContentPath) {
        if (const auto createTime =
                    System::get()->pathCreationTime(avdContentPath)) {
            return *createTime / 1000000;
        }
        // We were either unable to get the creation time, or this is Linux/really
        // old Mac and the system libs don't support creation times at all. Let's
        // use modification time for some rarely updated file in the avd,
        // e.g. <content_path>/data directory, or the root .ini.
        if (!android_stat(
                    PathUtils::join(avdContentPath, "data").c_str(),
                    &st)) {
            return st.st_mtime;
        }
    }
    const char* rootIniPath = avdInfo_getRootIniPath(avd);
    if (rootIniPath && !android_stat(rootIniPath, &st)) {
        return st.st_mtime;
    }
    return {};
}

static void fillAvdFileInfo(
        android_studio::EmulatorAvdInfo* avdInfo,
        android_studio::EmulatorAvdFile::EmulatorAvdFileKind kind,
        const char* path,
        bool isCustom) {
    const auto file = avdInfo->add_files();
    file->set_kind(kind);
    System::FileSize size;
    if (System::get()->pathFileSize(path, &size)) {
        file->set_size(size);
    }
    if (auto creationTime = System::get()->pathCreationTime(path)) {
        file->set_creation_timestamp(*creationTime / 1000000);
    }
    file->set_location(isCustom ? android_studio::EmulatorAvdFile::CUSTOM
                                : android_studio::EmulatorAvdFile::STANDARD);
}

static android_studio::EmulatorAvdInfo::EmulatorAvdProperty
toClearcutLogAvdProperty(AvdFlavor flavor) {
    switch (flavor) {
        case AVD_PHONE:
            return android_studio::EmulatorAvdInfo::PHONE_AVD;
        case AVD_DESKTOP:
            return android_studio::EmulatorAvdInfo::DESKTOP_AVD;
        case AVD_TV:
            return android_studio::EmulatorAvdInfo::TV_AVD;
        case AVD_WEAR:
            return android_studio::EmulatorAvdInfo::WEAR_AVD;
        case AVD_ANDROID_AUTO:
            return android_studio::EmulatorAvdInfo::ANDROIDAUTO_AVD;
        case AVD_OTHER:
            return android_studio::EmulatorAvdInfo::UNKNOWN_EMULATOR_AVD_FLAG;
    }
    return android_studio::EmulatorAvdInfo::UNKNOWN_EMULATOR_AVD_FLAG;
}

static void fillAvdMetrics(android_studio::AndroidStudioEvent* event) {
    VERBOSE_PRINT(metrics, "Filling AVD metrics");

    auto eventAvdInfo = event->mutable_emulator_details()->mutable_avd_info();
    // AVD name is a user-generated data, so won't report it.
    eventAvdInfo->set_api_level(avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()));
    eventAvdInfo->set_image_kind(
            avdInfo_isUserBuild(getConsoleAgents()->settings->avdInfo()) &&
                            isEnabled(android::featurecontrol::PlayStoreImage)
                    ? android_studio::EmulatorAvdInfo::PLAY_STORE_KIND
            : avdInfo_isGoogleApis(getConsoleAgents()->settings->avdInfo())
                    ? avdInfo_isAtd(getConsoleAgents()->settings->avdInfo())
                              ? android_studio::EmulatorAvdInfo::GOOGLE_ATD
                              : android_studio::EmulatorAvdInfo::GOOGLE
            : avdInfo_isAtd(getConsoleAgents()->settings->avdInfo())
                    ? android_studio::EmulatorAvdInfo::AOSP_ATD
                    : android_studio::EmulatorAvdInfo::AOSP);

    eventAvdInfo->set_arch(toClearcutLogGuestArch(getConsoleAgents()->settings->hw()->hw_cpu_arch));
    if (avdInfo_inAndroidBuild(getConsoleAgents()->settings->avdInfo())) {
        // no real AVD, so no creation times or file infos.
        return;
    }

    if (auto creationTime = getAvdCreationTimeSec(getConsoleAgents()->settings->avdInfo())) {
        eventAvdInfo->set_creation_timestamp(*creationTime);
        VERBOSE_PRINT(metrics, "AVD creation timestamp %ld", *creationTime);
    }

    const auto buildProps = avdInfo_getBuildProperties(getConsoleAgents()->settings->avdInfo());
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    if (const int64_t buildTimestamp = ini.getInt64("ro.build.date.utc", 0)) {
        eventAvdInfo->set_build_timestamp(buildTimestamp);
        VERBOSE_PRINT(metrics, "AVD build timestamp %ld", buildTimestamp);
    }
    auto buildId = ini.getString("ro.build.fingerprint", "");
    if (buildId.empty()) {
        buildId = ini.getString("ro.build.display.id", "");
    }
    eventAvdInfo->set_build_id(buildId);

    fillAvdFileInfo(
            eventAvdInfo, android_studio::EmulatorAvdFile::KERNEL,
            getConsoleAgents()->settings->hw()->kernel_path,
            getConsoleAgents()->settings->android_cmdLineOptions()->kernel !=
                    nullptr);
    fillAvdFileInfo(
            eventAvdInfo, android_studio::EmulatorAvdFile::SYSTEM,
            (getConsoleAgents()->settings->hw()->disk_systemPartition_path &&
             getConsoleAgents()->settings->hw()->disk_systemPartition_path[0])
                    ? getConsoleAgents()->settings->hw()->disk_systemPartition_path
                    : getConsoleAgents()->settings->hw()->disk_systemPartition_initPath,
            (getConsoleAgents()->settings->android_cmdLineOptions()->system ||
             getConsoleAgents()->settings->android_cmdLineOptions()->sysdir));
    fillAvdFileInfo(
            eventAvdInfo, android_studio::EmulatorAvdFile::RAMDISK,
            getConsoleAgents()->settings->hw()->disk_ramdisk_path,
            getConsoleAgents()->settings->android_cmdLineOptions()->ramdisk !=
                    nullptr);

    eventAvdInfo->add_properties(
            toClearcutLogAvdProperty(avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo())));
}

// Update this (and the proto) whenever feature flags change.
static android_studio::EmulatorFeatureFlagState::EmulatorFeatureFlag
toClearcutFeatureFlag(android::featurecontrol::Feature feature) {
    switch (feature) {
        case android::featurecontrol::GLPipeChecksum:
            return android_studio::EmulatorFeatureFlagState::GL_PIPE_CHECKSUM;
        case android::featurecontrol::GrallocSync:
            return android_studio::EmulatorFeatureFlagState::GRALLOC_SYNC;
        case android::featurecontrol::EncryptUserData:
            return android_studio::EmulatorFeatureFlagState::ENCRYPT_USER_DATA;
        case android::featurecontrol::IntelPerformanceMonitoringUnit:
            return android_studio::EmulatorFeatureFlagState::
                    INTEL_PERFORMANCE_MONITORING_UNIT;
        case android::featurecontrol::GLAsyncSwap:
            return android_studio::EmulatorFeatureFlagState::GL_ASYNC_SWAP;
        case android::featurecontrol::GLDMA:
            return android_studio::EmulatorFeatureFlagState::GLDMA;
        case android::featurecontrol::GLDMA2:
            return android_studio::EmulatorFeatureFlagState::GLDMA2;
        case android::featurecontrol::GLDirectMem:
            return android_studio::EmulatorFeatureFlagState::GL_DIRECT_MEM;
        case android::featurecontrol::GLESDynamicVersion:
            return android_studio::EmulatorFeatureFlagState::
                    GLES_DYNAMIC_VERSION;
        case android::featurecontrol::ForceANGLE:
            return android_studio::EmulatorFeatureFlagState::FORCE_ANGLE;
        case android::featurecontrol::ForceSwiftshader:
            return android_studio::EmulatorFeatureFlagState::FORCE_SWIFTSHADER;
        case android::featurecontrol::Wifi:
            return android_studio::EmulatorFeatureFlagState::WIFI;
        case android::featurecontrol::PlayStoreImage:
            return android_studio::EmulatorFeatureFlagState::PLAY_STORE_IMAGE;
        case android::featurecontrol::LogcatPipe:
            return android_studio::EmulatorFeatureFlagState::LOGCAT_PIPE;
        case android::featurecontrol::SystemAsRoot:
            return android_studio::EmulatorFeatureFlagState::SYSTEM_AS_ROOT;
        case android::featurecontrol::HYPERV:
            return android_studio::EmulatorFeatureFlagState::HYPERV;
        case android::featurecontrol::HVF:
            return android_studio::EmulatorFeatureFlagState::HVF;
        case android::featurecontrol::KVM:
            return android_studio::EmulatorFeatureFlagState::KVM;
        case android::featurecontrol::HAXM:
            return android_studio::EmulatorFeatureFlagState::HAXM;
        case android::featurecontrol::FastSnapshotV1:
            return android_studio::EmulatorFeatureFlagState::FAST_SNAPSHOT_V1;
        case android::featurecontrol::ScreenRecording:
            return android_studio::EmulatorFeatureFlagState::SCREEN_RECORDING;
        case android::featurecontrol::VirtualScene:
            return android_studio::EmulatorFeatureFlagState::VIRTUAL_SCENE;
        case android::featurecontrol::VideoPlayback:
            return android_studio::EmulatorFeatureFlagState::VIDEO_PLAYBACK;
        case android::featurecontrol::GenericSnapshotsUI:
            return android_studio::EmulatorFeatureFlagState::
                    GENERIC_SNAPSHOTS_UI;
        case android::featurecontrol::AllowSnapshotMigration:
            return android_studio::EmulatorFeatureFlagState::
                    ALLOW_SNAPSHOT_MIGRATION;
        case android::featurecontrol::WindowsOnDemandSnapshotLoad:
            return android_studio::EmulatorFeatureFlagState::
                    WINDOWS_ON_DEMAND_SNAPSHOT_LOAD;
        case android::featurecontrol::WindowsHypervisorPlatform:
            return android_studio::EmulatorFeatureFlagState::
                    WINDOWS_HYPERVISOR_PLATFORM;
        case android::featurecontrol::KernelDeviceTreeBlobSupport:
            return android_studio::EmulatorFeatureFlagState::
                    KERNEL_DEVICE_TREE_BLOB_SUPPORT;
        case android::featurecontrol::DynamicPartition:
            return android_studio::EmulatorFeatureFlagState::DYNAMIC_PARTITION;
        case android::featurecontrol::LocationUiV2:
            return android_studio::EmulatorFeatureFlagState::LOCATION_UI_V2;
        case android::featurecontrol::SnapshotAdb:
            return android_studio::EmulatorFeatureFlagState::SNAPSHOT_ADB;
        case android::featurecontrol::HostComposition:
            return android_studio::EmulatorFeatureFlagState::
                    HOST_COMPOSITION_V1;
        case android::featurecontrol::QuickbootFileBacked:
            return android_studio::EmulatorFeatureFlagState::
                    QUICKBOOT_FILE_BACKED;
        case android::featurecontrol::Offworld:
            return android_studio::EmulatorFeatureFlagState::OFFWORLD;
        case android::featurecontrol::OffworldDisableSecurity:
            return android_studio::EmulatorFeatureFlagState::
                    OFFWORLD_DISABLE_SECURITY;
        case android::featurecontrol::RefCountPipe:
            return android_studio::EmulatorFeatureFlagState::REFCOUNT_PIPE;
        case android::featurecontrol::OnDemandSnapshotLoad:
            return android_studio::EmulatorFeatureFlagState::
                    ON_DEMAND_SNAPSHOT_LOAD;
        case android::featurecontrol::Feature_n_items:
            return android_studio::EmulatorFeatureFlagState::
                    EMULATOR_FEATURE_FLAG_UNSPECIFIED;
        case android::featurecontrol::WifiConfigurable:
            return android_studio::EmulatorFeatureFlagState::WIFI_CONFIGURABLE;
        case android::featurecontrol::Vulkan:
            return android_studio::EmulatorFeatureFlagState::VULKAN;
        case android::featurecontrol::MacroUi:
            return android_studio::EmulatorFeatureFlagState::MACRO_UI;
        case android::featurecontrol::CarVHalTable:
            return android_studio::EmulatorFeatureFlagState::CAR_VHAL_TABLE;
        case android::featurecontrol::IpDisconnectOnLoad:
            return android_studio::EmulatorFeatureFlagState::
                    IP_DISCONNECT_ON_LOAD;
        case android::featurecontrol::VulkanSnapshots:
            return android_studio::EmulatorFeatureFlagState::VULKAN_SNAPSHOTS;
        case android::featurecontrol::VirtioInput:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_INPUT;
        case android::featurecontrol::MultiDisplay:
            return android_studio::EmulatorFeatureFlagState::MULTI_DISPLAY;
        case android::featurecontrol::VulkanNullOptionalStrings:
            return android_studio::EmulatorFeatureFlagState::
                    VULKAN_NULL_OPTIONAL_STRINGS;
        case android::featurecontrol::VulkanIgnoredHandles:
            return android_studio::EmulatorFeatureFlagState::
                    VULKAN_IGNORED_HANDLES;
        case android::featurecontrol::DynamicMediaProfile:
            return android_studio::EmulatorFeatureFlagState::
                    DYNAMIC_MEDIA_PROFILE;
        case android::featurecontrol::YUV420888toNV21:
            return android_studio::EmulatorFeatureFlagState::YUV420_888_to_NV21;
        case android::featurecontrol::YUVCache:
            return android_studio::EmulatorFeatureFlagState::YUV_Cache;
        case android::featurecontrol::KeycodeForwarding:
            return android_studio::EmulatorFeatureFlagState::KEYCODE_FORWARDING;
        case android::featurecontrol::VirtioGpuNext:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_GPU_NEXT;
        case android::featurecontrol::Mac80211hwsimUserspaceManaged:
            return android_studio::EmulatorFeatureFlagState::
                    MAC80211HWSIM_USERSPACE_MANAGED;
        case android::featurecontrol::HasSharedSlotsHostMemoryAllocator:
            return android_studio::EmulatorFeatureFlagState::
                    HAS_SHARED_SLOTS_HOST_MEMORY_ALLOCATOR;
        case android::featurecontrol::CarVhalReplay:
            return android_studio::EmulatorFeatureFlagState::CAR_VHAL_REPLAY;
        case android::featurecontrol::HardwareDecoder:
            return android_studio::EmulatorFeatureFlagState::HARDWARE_DECODER;
        case android::featurecontrol::NoDelayCloseColorBuffer:
            return android_studio::EmulatorFeatureFlagState::
                    NO_DELAY_CLOSE_COLOR_BUFFER;
        case android::featurecontrol::NoDeviceFrame:
            return android_studio::EmulatorFeatureFlagState::NO_DEVICE_FRAME;
        case android::featurecontrol::VirtioGpuNativeSync:
            return android_studio::EmulatorFeatureFlagState::
                    VIRTIO_GPU_NATIVE_SYNC;
        case android::featurecontrol::VirtioWifi:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_WIFI;
        case android::featurecontrol::VulkanShaderFloat16Int8:
            return android_studio::EmulatorFeatureFlagState::
                    VULKAN_SHADER_FLOAT16_INT8;
        case android::featurecontrol::CarRotary:
            return android_studio::EmulatorFeatureFlagState::CAR_ROTARY;
        case android::featurecontrol::ModemSimulator:
            return android_studio::EmulatorFeatureFlagState::MODEM_SIMULATOR;
        case android::featurecontrol::TvRemote:
            return android_studio::EmulatorFeatureFlagState::TV_REMOTE;
        case android::featurecontrol::NativeTextureDecompression:
            return android_studio::EmulatorFeatureFlagState::
                    NATIVE_TEXTURE_DECOMPRESSION;
        case android::featurecontrol::GuestUsesAngle:
            return android_studio::EmulatorFeatureFlagState::GUEST_USES_ANGLE;
        case android::featurecontrol::VirtioVsockPipe:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_VSOCK_PIPE;
        case android::featurecontrol::VirtioMouse:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_MOUSE;
        case android::featurecontrol::VirtconsoleLogcat:
            return android_studio::EmulatorFeatureFlagState::VIRTCONSOLE_LOGCAT;
        case android::featurecontrol::VulkanQueueSubmitWithCommands:
            return android_studio::EmulatorFeatureFlagState::
                    VULKAN_QUEUE_SUBMIT_WITH_COMMANDS;
        case android::featurecontrol::VulkanBatchedDescriptorSetUpdate:
            return android_studio::EmulatorFeatureFlagState::
                    VULKAN_BATCHED_DESCRIPTOR_SET_UPDATE;
        case android::featurecontrol::Minigbm:
            return android_studio::EmulatorFeatureFlagState::MINIGBM;
        case android::featurecontrol::NoDraw:
            return android_studio::EmulatorFeatureFlagState::NO_DRAW;
        case android::featurecontrol::GnssGrpcV1:
            return android_studio::EmulatorFeatureFlagState::GNSS_GRPC_V1;
        case android::featurecontrol::MigratableSnapshotSave:
            return android_studio::EmulatorFeatureFlagState::
                    MIGRATABLE_SNAPSHOT_SAVE;
        case android::featurecontrol::AndroidbootProps:
            return android_studio::EmulatorFeatureFlagState::ANDROIDBOOT_PROPS;
        case android::featurecontrol::AndroidbootProps2:
            return android_studio::EmulatorFeatureFlagState::ANDROIDBOOT_PROPS2;
        case android::featurecontrol::DeviceSkinOverlay:
            return android_studio::EmulatorFeatureFlagState::DEVICESKINOVERLAY;
        case android::featurecontrol::BluetoothEmulation:
            return android_studio::EmulatorFeatureFlagState::
                    BLUETOOTH_EMULATION;
        case android::featurecontrol::DeviceStateOnBoot:
            return android_studio::EmulatorFeatureFlagState::
                    DEVICESTATE_ON_BOOT;
        case android::featurecontrol::HWCMultiConfigs:
            return android_studio::EmulatorFeatureFlagState::HWC_MULTI_CONFIGS;
        case android::featurecontrol::AsyncComposeSupport:
            return android_studio::EmulatorFeatureFlagState::
                    ASYNC_COMPOSE_SUPPORT;
        case android::featurecontrol::VirtioSndCard:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_SND_CARD;
        case android::featurecontrol::DownloadableSnapshot:
            return android_studio::EmulatorFeatureFlagState::
                    DOWNLOADABLE_SNAPSHOT;
        case android::featurecontrol::VirtioTablet:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_TABLET;
        case android::featurecontrol::VulkanNativeSwapchain:
            return android_studio::EmulatorFeatureFlagState::VULKAN_NATIVE_SWAPCHAIN;
        case android::featurecontrol::VirtioGpuFenceContexts:
            return android_studio::EmulatorFeatureFlagState::VIRTIO_GPU_FENCE_CONTEXTS;
        case android::featurecontrol::VsockSnapshotLoadFixed_b231345789:
            return android_studio::EmulatorFeatureFlagState::
                    VSOCK_SNAPSHOT_LOAD_FIXED_B231345789;
        case android::featurecontrol::VulkanAstcLdrEmulation:
            return android_studio::EmulatorFeatureFlagState::VULKAN_ASTC_LDR_EMULATION;
        case android::featurecontrol::VulkanYcbcrEmulation:
            return android_studio::EmulatorFeatureFlagState::VULKAN_YCBCR_EMULATION;
        case android::featurecontrol::VulkanEtc2Emulation:
            return android_studio::EmulatorFeatureFlagState::VULKAN_ETC2_EMULATION;
        case android::featurecontrol::ExternalBlob:
            return android_studio::EmulatorFeatureFlagState::EXTERNAL_BLOB;
        case android::featurecontrol::SystemBlob:
            return android_studio::EmulatorFeatureFlagState::SYSTEM_BLOB;
    }
    return android_studio::EmulatorFeatureFlagState::
            EMULATOR_FEATURE_FLAG_UNSPECIFIED;
}

static void fillFeatureFlagState(android_studio::AndroidStudioEvent* event) {
    android_studio::EmulatorFeatureFlagState* featureFlagState =
            event->mutable_emulator_details()->mutable_feature_flag_state();

    for (const auto elt : android::featurecontrol::getEnabledNonOverride()) {
        featureFlagState->add_attempted_enabled_feature_flags(
                toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getEnabledOverride()) {
        featureFlagState->add_user_overridden_enabled_features(
                toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getDisabledOverride()) {
        featureFlagState->add_user_overridden_disabled_features(
                toClearcutFeatureFlag(elt));
    }

    for (const auto elt : android::featurecontrol::getEnabled()) {
        featureFlagState->add_resulting_enabled_features(
                toClearcutFeatureFlag(elt));
    }
}

void android_metrics_fill_common_info(bool openglAlive, void* opaque) {
    android_studio::AndroidStudioEvent* event =
            static_cast<android_studio::AndroidStudioEvent*>(opaque);

    event->mutable_product_details()->set_channel(
            toClearcutLogUpdateChannel(android::studio::updateChannel()));
    event->mutable_product_details()->set_os_architecture(
            toClearcutLogCpuArchitecture(System::get()->getHostBitness()));

    event->mutable_emulator_details()->set_session_phase(
            android_studio::EmulatorDetails::RUNNING_GENERAL);
    event->mutable_emulator_details()->set_is_opengl_alive(openglAlive);
    event->mutable_emulator_details()->set_guest_arch(
            toClearcutLogGuestArch(getConsoleAgents()->settings->hw()->hw_cpu_arch));
    event->mutable_emulator_details()->set_guest_api_level(
            avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()));

    // TODO: Check that CpuAccelerator enum +1 is the same
    // as the proto enum.
    event->mutable_emulator_details()->set_hypervisor(
            (android_studio::EmulatorDetails::EmulatorHypervisor)(
                    android::GetCurrentCpuAccelerator() + 1));

    fillFeatureFlagState(event);

    event->mutable_emulator_details()->set_renderer(
            toClearcutLogEmulatorRenderer(
                    emuglConfig_get_renderer(getConsoleAgents()->settings->hw()->hw_gpu_mode)));

    event->mutable_emulator_details()->set_guest_gpu_enabled(
            getConsoleAgents()->settings->hw()->hw_gpu_enabled);

    if (getConsoleAgents()->settings->hw()->hw_gpu_enabled) {
        fillGuestGlMetrics(event);
        if (openglAlive) {
            const gfxstream::RendererPtr& renderer = android_getOpenglesRenderer();
            if (renderer) {
                renderer->fillGLESUsages(event->mutable_emulator_details()
                                                 ->mutable_gles_usages());
            }
        }
    }

    for (const GpuInfo& gpu : globalGpuInfoList().infos) {
        auto hostGpu = event->mutable_emulator_details()->add_host_gpu();
        hostGpu->set_device_id(gpu.device_id);
        hostGpu->set_make(gpu.make);
        hostGpu->set_model(gpu.model);
        hostGpu->set_renderer(gpu.renderer);
        hostGpu->set_revision_id(gpu.revision_id);
        hostGpu->set_version(gpu.version);
    }

    fillAvdMetrics(event);
    event->set_kind(android_studio::AndroidStudioEvent::EMULATOR_HOST);
    event->mutable_emulator_host()->set_os_bit_count(
            System::get()->getHostBitness());
    const AndroidCpuInfoFlags cpuFlags = android::GetCpuInfo().first;
    event->mutable_emulator_host()->set_virt_support(
            cpuFlags & ANDROID_CPU_INFO_VIRT_SUPPORTED);
    event->mutable_emulator_host()->set_running_in_vm(cpuFlags &
                                                      ANDROID_CPU_INFO_VM);
    event->mutable_emulator_host()->set_cpu_manufacturer(
            (cpuFlags & ANDROID_CPU_INFO_INTEL) ? "INTEL"
            : (cpuFlags & ANDROID_CPU_INFO_AMD) ? "AMD"
                                                : "OTHER");

    uint32_t cpuid_mfs;
    android_get_x86_cpuid(1, 0, &cpuid_mfs, nullptr, nullptr, nullptr);
    uint32_t cpuid_stepping = cpuid_mfs & 0x0000000f;
    uint32_t cpuid_model = (cpuid_mfs & 0x000000f0) >> 4;
    uint32_t cpuid_family = (cpuid_mfs & 0x00000f00) >> 8;
    uint32_t cpuid_type = (cpuid_mfs & 0x00003000) >> 12;
    uint32_t cpuid_extmodel = (cpuid_mfs & 0x000f0000) >> 16;
    uint32_t cpuid_extfamily = (cpuid_mfs & 0x0ff00000) >> 20;
    event->mutable_emulator_host()->set_cpuid_stepping(cpuid_stepping);
    event->mutable_emulator_host()->set_cpuid_model(cpuid_model);
    event->mutable_emulator_host()->set_cpuid_family(cpuid_family);
    event->mutable_emulator_host()->set_cpuid_type(cpuid_type);
    event->mutable_emulator_host()->set_cpuid_extmodel(cpuid_extmodel);
    event->mutable_emulator_host()->set_cpuid_extfamily(cpuid_extfamily);

    {
        android_studio::EmulatorHost forCommonInfoHost = event->emulator_host();
        android_studio::EmulatorDetails forCommonInfoDetails =
                event->emulator_details();
        android::CommonReportedInfo::setHostInfo(&forCommonInfoHost);
        android::CommonReportedInfo::setDetails(&forCommonInfoDetails);
    }

    // Whether we are running with standalone metrics
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->metrics_collection) {
        event->mutable_emulator_details()
                ->mutable_used_features()
                ->set_launch_type(android_studio::EmulatorFeatures::CONTAINER);
    }

    if (getConsoleAgents()->settings->android_cmdLineOptions()->fuchsia) {
        event->mutable_emulator_details()
                ->mutable_used_features()
                ->set_launch_type(android_studio::EmulatorFeatures::FUCHSIA);
    }

    // Set the emulator.exe pid as well as the qemu-*.pid.
    // They are usually the same on Linux, but different on Windows.
    event->mutable_emulator_details()->set_emu_pid(atoi(
            System::get()->envGet("ANDROID_EMULATOR_WRAPPER_PID").c_str()));
    event->mutable_emulator_details()->set_qemu_pid(
            System::get()->getCurrentProcessId());
}

void android_metrics_report_common_info(bool openglAlive) {
    MetricsReporter::get().report(
            [openglAlive](android_studio::AndroidStudioEvent* event) {
                android_metrics_fill_common_info(openglAlive, event);
            });
}

bool android_metrics_start(const char* emulatorVersion,
                           const char* emulatorFullVersion,
                           const char* qemuVersion,
                           int controlConsolePort) {
    MetricsReporter::start(android::base::Uuid::generate().toString(),
                           emulatorVersion, emulatorFullVersion, qemuVersion);
    PeriodicReporter::start(&MetricsReporter::get(),
                            android::base::ThreadLooper::get());

    MetricsEngine::get()->setEmulatorName(
            android::base::StringFormat("emulator-%d", controlConsolePort));

    // Add a task that reports emulator's uptime (just in case that we don't
    // have enough of other messages reported).
    PeriodicReporter::get().addTask(
            5 * 60 * 1000,  // reporting period
            [](android_studio::AndroidStudioEvent* event) {
                // uptime fields are always filled for all events, so there's
                // nothing to do here.
                return true;
            });
    // Collect PerfStats metrics every 5 seconds.
    auto perfStatReporter = android::metrics::PerfStatReporter::create(
            android::base::ThreadLooper::get(), 5 * 1000);
    perfStatReporter->start();
    MetricsEngine::get()->registerReporter(perfStatReporter);

    return true;
}

void android_metrics_stop(MetricsStopReason reason) {
    MetricsEngine::get()->stop();
    PeriodicReporter::stop();
    MetricsReporter::stop(reason);
}
