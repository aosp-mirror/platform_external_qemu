// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "metrics.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/Optional.h"
#include "android/base/StringFormat.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"
#include "android/cmdline-option.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/opengles.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/StudioConfig.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"

#include "android/metrics/proto/studio_stats.pb.h"

#include <utility>

using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::metrics::MetricsReporter;
using android::metrics::PeriodicReporter;

namespace {

// A struct to contain all metrics reporters we need to be running for the
// whole emulator livecycle.
// For now it's a single member, but soon to be more.
struct InstanceData {
    android::metrics::AdbLivenessChecker::Ptr livenessChecker;

    void reset() {
        if (livenessChecker) {
            livenessChecker->stop();
            livenessChecker.reset();
        }
    }
};

static android::base::LazyInstance<InstanceData> sGlobalData = {};

}  // namespace

bool android_metrics_start(const char* emulatorVersion,
                           const char* emulatorFullVersion,
                           const char* qemuVersion,
                           int controlConsolePort) {
    MetricsReporter::start(android::base::Uuid::generate().toString(),
                           emulatorVersion, emulatorFullVersion, qemuVersion);
    PeriodicReporter::start(&MetricsReporter::get(),
                            android::base::ThreadLooper::get());

    // Start the ADB liveness monitor.
    const auto emulatorName =
            android::base::StringFormat("emulator-%d", controlConsolePort);
    sGlobalData->livenessChecker = android::metrics::AdbLivenessChecker::create(
            android::base::ThreadLooper::get(), &MetricsReporter::get(),
            emulatorName, 20 * 1000);
    sGlobalData->livenessChecker->start();

    // Add a task that reports emulator's uptime (just in case that we don't
    // have enough of other messages reported).
    PeriodicReporter::get().addTask(
            5 * 60 * 1000,  // reporting period
            [](android_studio::AndroidStudioEvent* event) {
                // uptime fields are always filled for all events, so there's
                // nothing to do here.
                return true;
            });

    return true;
}

void android_metrics_stop() {
    sGlobalData->reset();
    PeriodicReporter::stop();
    MetricsReporter::stop();
}

static android_studio::EmulatorDetails::GuestCpuArchitecture
toClearcutLogGuestArch(const char* hw_cpu_arch) {
    using android_studio::EmulatorDetails;

    static constexpr std::pair<StringView,
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
    switch (bitness) {
        case 32:
            return ProductDetails::X86;
        case 64:
            return ProductDetails::X86_64;
        default:
            return ProductDetails::UNKNOWN_CPU_ARCHITECTURE;
    }
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
    if (const auto createTime =
                System::get()->pathCreationTime(avdInfo_getContentPath(avd))) {
        return *createTime / 1000000;
    }
    // We were either unable to get the creation time, or this is Linux/really
    // old Mac and the system libs don't support creation times at all. Let's
    // use modification time for some rarely updated file in the avd,
    // e.g. <content_path>/data directory, or the root .ini.
    struct stat st;
    if (!android_stat(
                PathUtils::join(avdInfo_getContentPath(avd), "data").c_str(),
                &st)) {
        return st.st_mtime;
    }
    if (!android_stat(avdInfo_getRootIniPath(avd), &st)) {
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

static void fillAvdMetrics(android_studio::AndroidStudioEvent* event) {
    VERBOSE_PRINT(metrics, "Filling AVD metrics");

    auto eventAvdInfo = event->mutable_emulator_details()->mutable_avd_info();
    eventAvdInfo->set_name(StringView(avdInfo_getName(android_avdInfo)));
    eventAvdInfo->set_api_level(avdInfo_getApiLevel(android_avdInfo));
    eventAvdInfo->set_image_kind(
            avdInfo_isGoogleApis(android_avdInfo)
                    ? android_studio::EmulatorAvdInfo::GOOGLE
                    : android_studio::EmulatorAvdInfo::AOSP);
    eventAvdInfo->set_arch(toClearcutLogGuestArch(android_hw->hw_cpu_arch));
    if (avdInfo_inAndroidBuild(android_avdInfo)) {
        // no real AVD, so no creation times or file infos.
        return;
    }

    if (auto creationTime = getAvdCreationTimeSec(android_avdInfo)) {
        eventAvdInfo->set_creation_timestamp(*creationTime);
        VERBOSE_PRINT(metrics, "AVD creation timestamp %ld", *creationTime);
    }

    const auto buildProps = avdInfo_getBuildProperties(android_avdInfo);
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    if (const int64_t buildTimestamp = ini.getInt64("ro.build.date.utc", 0)) {
        eventAvdInfo->set_build_timestamp(buildTimestamp);
        VERBOSE_PRINT(metrics, "AVD build timestamp %ld", buildTimestamp);
    }
    eventAvdInfo->set_build_id(ini.getString("ro.build.display.id", ""));

    fillAvdFileInfo(eventAvdInfo, android_studio::EmulatorAvdFile::KERNEL,
                android_hw->kernel_path,
                android_cmdLineOptions->kernel != nullptr);
    fillAvdFileInfo(
            eventAvdInfo, android_studio::EmulatorAvdFile::SYSTEM,
            (android_hw->disk_systemPartition_path && android_hw->disk_systemPartition_path[0])
                    ? android_hw->disk_systemPartition_path
                    : android_hw->disk_systemPartition_initPath,
            (android_cmdLineOptions->system || android_cmdLineOptions->sysdir));
    fillAvdFileInfo(eventAvdInfo, android_studio::EmulatorAvdFile::RAMDISK,
                android_hw->disk_ramdisk_path,
                android_cmdLineOptions->ramdisk != nullptr);
}

void android_metrics_report_common_info(bool openglAlive) {
    MetricsReporter::get().report([openglAlive](
            android_studio::AndroidStudioEvent* event) {
        event->mutable_product_details()->set_channel(
                toClearcutLogUpdateChannel(android::studio::updateChannel()));
        event->mutable_product_details()->set_os_architecture(
                toClearcutLogCpuArchitecture(System::get()->getHostBitness()));

        event->mutable_emulator_details()->set_session_phase(
                android_studio::EmulatorDetails::RUNNING_GENERAL);
        event->mutable_emulator_details()->set_is_opengl_alive(openglAlive);
        event->mutable_emulator_details()->set_guest_arch(
                toClearcutLogGuestArch(android_hw->hw_cpu_arch));
        event->mutable_emulator_details()->set_guest_api_level(
                avdInfo_getApiLevel(android_avdInfo));

        event->mutable_emulator_details()->set_renderer(
                toClearcutLogEmulatorRenderer(
                        emuglConfig_get_renderer(android_hw->hw_gpu_mode)));

        event->mutable_emulator_details()->set_guest_gpu_enabled(
                android_hw->hw_gpu_enabled);
        if (android_hw->hw_gpu_enabled) {
            fillGuestGlMetrics(event);
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

        event->mutable_emulator_host()->set_os_bit_count(
                    System::get()->getHostBitness());
        const AndroidCpuInfoFlags cpuFlags = android::GetCpuInfo().first;
        event->mutable_emulator_host()->set_virt_support(
                    cpuFlags & ANDROID_CPU_INFO_VIRT_SUPPORTED);
        event->mutable_emulator_host()->set_running_in_vm(
                    cpuFlags & ANDROID_CPU_INFO_VM);
        event->mutable_emulator_host()->set_cpu_manufacturer(
                    (cpuFlags & ANDROID_CPU_INFO_INTEL) ? "INTEL" :
                    (cpuFlags & ANDROID_CPU_INFO_AMD) ? "AMD" : "OTHER");
    });
}
