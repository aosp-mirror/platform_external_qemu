// Copyright 2017 The Android Open Source Project
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

#include "android/crashreport/structured_info.h"
#include "android/crashreport/StructuredInfo.h"

#include "android/globals.h"
#include "android/base/files/IniFile.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringView.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/proto/crash_info.pb.h"
#include "android/opengles.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <fstream>

#define DEBUG 1

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while(0) \

#else

#define D(...)

#endif

namespace android {
namespace crashreport {

using namespace android_emulator;
using base::LazyInstance;
using base::Lock;
using base::AutoLock;
using base::StringView;

static LazyInstance<StructuredInfo> sStructuredInfo =
    LAZY_INSTANCE_INIT;

static LazyInstance<EmulatorCrashInfo> sCrashProto =
    LAZY_INSTANCE_INIT;

// static
StructuredInfo* StructuredInfo::get() {
    return sStructuredInfo.ptr();
}

void StructuredInfo::initEmulatorCommonInfo() {
    D("start");
    setHostHwInfo(HostHwInfo::query());
    setHypervisor(GetCurrentCpuAccelerator());

    setFeatureFlagUsage(
            featurecontrol::getEnabledNonOverride(),
            featurecontrol::getEnabledOverride(),
            featurecontrol::getDisabledOverride(),
            featurecontrol::getEnabled());

    setRenderer(emuglConfig_get_current_renderer());

    // Guest GL info
    {
        char* vendor = nullptr; char* renderer = nullptr; char* version = nullptr;
        android_getOpenglesHardwareStrings(&vendor, &renderer, &version);
        if (vendor) { setGuestGlVendor(vendor); free(vendor); }
        if (renderer) { setGuestGlRenderer(renderer); free(renderer); }
        if (version) { setGuestGlVersion(version); free(version); }
    }
}

void StructuredInfo::clear() {
    AutoLock lock(mLock);
    clearLocked();
}

void StructuredInfo::clearLocked() {
    sCrashProto.get().Clear();
}

void StructuredInfo::toBinaryString(std::string* res) const {
    AutoLock lock(mLock);
    D("[%s]", sCrashProto.get().DebugString().c_str());
    sCrashProto.get().SerializeToString(res);
}

void StructuredInfo::toOstream(std::ofstream* res) const {
    AutoLock lock(mLock);
    D("[%s]", sCrashProto.get().DebugString().c_str());
    sCrashProto.get().SerializeToOstream(res);
}

void StructuredInfo::reloadFromBinaryString(const std::string& buf) {
    AutoLock lock(mLock);
    clearLocked();
    sCrashProto.get().ParseFromString(buf);
    D("[%s]", sCrashProto.get().DebugString().c_str());
}

void StructuredInfo::reloadFromFile(const std::string& filename) {
    AutoLock lock(mLock);
    clearLocked();
    std::ifstream in(filename);
    sCrashProto.get().ParseFromIstream(&in);
    D("[%s]", sCrashProto.get().DebugString().c_str());
}

void StructuredInfo::setHostHwInfo(const HostHwInfo::Info& info) {
    D("start");
    AutoLock lock(mLock);

    EmulatorHost* hostinfo = sCrashProto.get().mutable_emulator_host_info();

    hostinfo->set_virt_support(info.virt_support);
    hostinfo->set_running_in_vm(info.running_in_vm);

    for (const auto& hostgpu : info.gpuinfolist->infos) {
        EmulatorGpuInfo* outGpu = hostinfo->add_host_gpu();
        outGpu->set_make(hostgpu.make);
        outGpu->set_model(hostgpu.model);
        outGpu->set_device_id(hostgpu.device_id);
        outGpu->set_revision_id(hostgpu.revision_id);
        outGpu->set_version(hostgpu.version);
        outGpu->set_renderer(hostgpu.renderer);
    }

    D("%s", hostinfo->DebugString().c_str());
}

// Based on a similar function for clearcut log (metrics.cpp)
static EmulatorAvdInfo::GuestCpuArchitecture
toProtoGuestArch(const char* hw_cpu_arch) {
    static constexpr std::pair<StringView,
                               EmulatorAvdInfo::GuestCpuArchitecture>
            map[] = {
                    {"x86", EmulatorAvdInfo::X86},
                    {"x86_64", EmulatorAvdInfo::X86_64},
                    {"arm", EmulatorAvdInfo::ARM},
                    {"arm64", EmulatorAvdInfo::ARM_64},
                    {"mips", EmulatorAvdInfo::MIPS},
                    {"mips64", EmulatorAvdInfo::MIPS_64},
            };

    for (const auto& pair : map) {
        if (pair.first == hw_cpu_arch) {
            return pair.second;
        }
    }
    return EmulatorAvdInfo::UNKNOWN_GUEST_CPU_ARCHITECTURE;
}

void StructuredInfo::setAvdInfo(AvdInfo* info) {
    D("start");

    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    EmulatorAvdInfo* avd = details->mutable_avd_info();

    avd->set_api_level(avdInfo_getApiLevel(info));
    avd->set_image_kind(
            avdInfo_isGoogleApis(info)
                    ? EmulatorAvdInfo::GOOGLE
                    : EmulatorAvdInfo::AOSP);
    avd->set_arch(toProtoGuestArch(android_hw->hw_cpu_arch));
    const auto buildProps = avdInfo_getBuildProperties(info);
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    avd->set_build_id(ini.getString("ro.build.display.id", "unknown-image-build"));
}

void StructuredInfo::setSessionPhase(AndroidSessionPhase phase) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();

    // TODO: Session phases same for now between the proto and the emulator.
    details->set_session_phase(
        (EmulatorDetails::EmulatorSessionPhase)phase);
}

void StructuredInfo::setHypervisor(CpuAccelerator accel) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    EmulatorDetails::Hypervisor protoAccel =
        EmulatorDetails::UNKNOWN_HYPERVISOR;
    switch (accel) {
        case CPU_ACCELERATOR_NONE:
            protoAccel = EmulatorDetails::NO_HYPERVISOR;
            break;
        case CPU_ACCELERATOR_KVM:
            protoAccel = EmulatorDetails::KVM;
            break;
        case CPU_ACCELERATOR_HAX:
            protoAccel = EmulatorDetails::HAXM;
            break;
        case CPU_ACCELERATOR_HVF:
            protoAccel = EmulatorDetails::HVF;
            break;
        case CPU_ACCELERATOR_MAX:
            protoAccel = EmulatorDetails::UNKNOWN_HYPERVISOR;
            break;
    }
    details->set_current_hypervisor(protoAccel);
}

// Update this (and the proto) whenever feature flags change.
static EmulatorFeatureFlagUsage::Flag toProtoFeature(
        featurecontrol::Feature feature) {
    switch (feature) {
        case featurecontrol::GLPipeChecksum:
            return EmulatorFeatureFlagUsage::GL_PIPE_CHECKSUM;
        case featurecontrol::GrallocSync:
            return EmulatorFeatureFlagUsage::GRALLOC_SYNC;
        case featurecontrol::EncryptUserData:
            return EmulatorFeatureFlagUsage::ENCRYPT_USER_DATA;
        case featurecontrol::IntelPerformanceMonitoringUnit:
            return EmulatorFeatureFlagUsage::INTEL_PERFORMANCE_MONITORING_UNIT;
        case featurecontrol::GLAsyncSwap:
            return EmulatorFeatureFlagUsage::GL_ASYNC_SWAP;
        case featurecontrol::GLDMA:
            return EmulatorFeatureFlagUsage::GLDMA;
        case featurecontrol::GLESDynamicVersion:
            return EmulatorFeatureFlagUsage::GLES_DYNAMIC_VERSION;
        case featurecontrol::ForceANGLE:
            return EmulatorFeatureFlagUsage::FORCE_ANGLE;
        case featurecontrol::ForceSwiftshader:
            return EmulatorFeatureFlagUsage::FORCE_SWIFTSHADER;
        case featurecontrol::Wifi:
            return EmulatorFeatureFlagUsage::WIFI;
        case featurecontrol::PlayStoreImage:
            return EmulatorFeatureFlagUsage::PLAY_STORE_IMAGE;
        case featurecontrol::LogcatPipe:
            return EmulatorFeatureFlagUsage::LOGCAT_PIPE;
        case featurecontrol::HYPERV:
            return EmulatorFeatureFlagUsage::HYPERV;
        case featurecontrol::HVF:
            return EmulatorFeatureFlagUsage::HVF;
        case featurecontrol::KVM:
            return EmulatorFeatureFlagUsage::KVM;
        case featurecontrol::HAXM:
            return EmulatorFeatureFlagUsage::HAXM;
        case featurecontrol::Feature_n_items:
            return EmulatorFeatureFlagUsage::EMULATOR_FEATURE_FLAG_UNSPECIFIED;
    }
}

void StructuredInfo::setFeatureFlagUsage(
         const std::vector<featurecontrol::Feature>& enabledNonOverride,
         const std::vector<featurecontrol::Feature>& enabledOverride,
         const std::vector<featurecontrol::Feature>& disabledOverride,
         const std::vector<featurecontrol::Feature>& enabled) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    EmulatorFeatureFlagUsage* usage = details->mutable_used_feature_flags();
    for (const auto feature : enabledNonOverride) {
        usage->add_attempted_enabled_feature_flags(toProtoFeature(feature));
    }
    for (const auto feature : enabledOverride) {
        usage->add_user_overridden_enabled_features(toProtoFeature(feature));
    }
    for (const auto feature : disabledOverride) {
        usage->add_user_overridden_disabled_features(toProtoFeature(feature));
    }
    for (const auto feature : enabled) {
        usage->add_resulting_enabled_features(toProtoFeature(feature));
    }
}

void StructuredInfo::setRenderer(SelectedRenderer renderer) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_renderer((EmulatorDetails::EmulatorRenderer)renderer);
}

void StructuredInfo::setUptime(uint64_t uptimeMs) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_wall_time((int64_t)uptimeMs);
}

void StructuredInfo::setGuestGlVendor(const char* vendor) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_guest_gl_vendor(vendor);
}

void StructuredInfo::setGuestGlRenderer(const char* renderer) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_guest_gl_renderer(renderer);
}

void StructuredInfo::setGuestGlVersion(const char* version) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_guest_gl_version(version);
}

void StructuredInfo::setMemUsage(uint64_t resident,
                                 uint64_t resident_max,
                                 uint64_t virt,
                                 uint64_t virt_max,
                                 uint64_t total_phys,
                                 uint64_t total_page) {
    D("start");
    AutoLock lock(mLock);
    EmulatorDetails* details = sCrashProto.get().mutable_emulator_details();
    details->set_resident_memory(resident);
    details->set_resident_memory_max(resident_max);
    details->set_virtual_memory(virt);
    details->set_virtual_memory_max(virt_max);
    details->set_total_phys_memory(total_phys);
    details->set_total_page_file(total_page);
}

}  // crashreport
}  // android

extern "C" {

void android_structured_info_init() {
    D("start");
    android::crashreport::StructuredInfo::get()->initEmulatorCommonInfo();
}

void android_structured_info_set_avd_info(AvdInfo* info) {
    D("start");
    android::crashreport::StructuredInfo::get()->setAvdInfo(info);
}

extern void android_structured_info_set_session_phase(AndroidSessionPhase phase) {
    D("start");
    android::crashreport::StructuredInfo::get()->setSessionPhase(phase);
}

}
