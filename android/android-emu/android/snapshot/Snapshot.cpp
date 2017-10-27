// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Snapshot.h"

#include "android/base/ArraySize.h"
#include "android/base/Optional.h"
#include "android/base/StringFormat.h"
#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/Features.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Snapshotter.h"
#include "android/utils/fd.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

#include "google/protobuf/io/zero_copy_stream_impl.h"

#include <fcntl.h>
#include <sys/mman.h>

#include <algorithm>

using android::base::IniFile;
using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::base::endsWith;
using android::base::makeCustomScopedPtr;

namespace pb = emulator_snapshot;

namespace android {
namespace snapshot {

static void fillImageInfo(pb::Image::Type type,
                          StringView path,
                          pb::Image* image) {
    image->set_type(type);
    *image->mutable_path() = path;
    if (path.empty() || !path_is_regular(path.c_str())) {
        image->set_present(false);
        return;
    }

    image->set_present(true);
    struct stat st;
    if (!android_stat(path.c_str(), &st)) {
        image->set_size(st.st_size);
        image->set_modification_time(st.st_mtime);
    }
}

static bool verifyImageInfo(pb::Image::Type type,
                            StringView path,
                            const pb::Image& in) {
    if (in.type() != type) {
        return false;
    }
    const bool savedPresent = in.has_present() && in.present();
    const bool realPresent = !path.empty() && path_is_regular(path.c_str());
    if (savedPresent != realPresent) {
        return false;
    }
    struct stat st;
    static_assert(sizeof(st.st_size >= sizeof(uint64_t)),
                  "Bad size member in struct stat, fix build options");
    if (android_stat(path.c_str(), &st) != 0) {
        if (in.has_size() || in.has_modification_time()) {
            return false;
        }
    } else {
        if (!in.has_size() || in.size() != st.st_size) {
            return false;
        }
        if (!in.has_modification_time() ||
            in.modification_time() != st.st_mtime) {
            return false;
        }
    }

    return true;
}

bool areHwConfigsEqual(const IniFile& expected, const IniFile& actual) {
    // We need to explicitly check the list because when booted with -wipe-data
    // the emulator will add a line (disk.dataPartition.initPath) which should
    // not affect snapshot.
    int numPathExpected = 0;
    int numPathActual = 0;
    for (auto&& key : expected) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
            if (key != "disk.dataPartition.initPath") {
                numPathExpected++;
            }
            continue;  // these contain absolute paths and will be checked
                       // separately.
        }
        if (!actual.hasKey(key)) {
            return false;
        }
        if (actual.getString(key, "$$$") != expected.getString(key, "$$$")) {
            return false;
        }
    }

    for (auto&& key : actual) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
            if (key != "disk.dataPartition.initPath") {
                numPathActual++;
            }
            continue;  // these contain absolute paths and will be checked
                       // separately.
        }
        if (!expected.hasKey(key)) {
            return false;
        }
        if (actual.getString(key, "$$$") != expected.getString(key, "$$$")) {
            return false;
        }
    }

    return numPathActual == numPathExpected;
}

static std::string gpuDriverString(const GpuInfo& info) {
    return StringFormat("%s-%s-%s", info.make, info.model, info.version);
}

static Optional<std::string> currentGpuDriverString() {
    const auto& gpuList = globalGpuInfoList().infos;
    auto currentIt =
            std::find_if(gpuList.begin(), gpuList.end(),
                         [](const GpuInfo& i) { return i.current_gpu; });
    if (currentIt == gpuList.end() && !gpuList.empty()) {
        currentIt = gpuList.begin();
    }
    if (currentIt == gpuList.end()) {
        return {};
    }
    return gpuDriverString(*currentIt);
}

bool Snapshot::verifyHost(const pb::Host& host) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (host.has_hypervisor() && host.hypervisor() != vmConfig.hypervisorType) {
        saveFailure(FailureReason::ConfigMismatchHostHypervisor);
        return false;
    }
    if (auto gpuString = currentGpuDriverString()) {
        if (!host.has_gpu_driver() || host.gpu_driver() != *gpuString) {
            saveFailure(FailureReason::ConfigMismatchHostGpu);
            return false;
        }
    } else if (host.has_gpu_driver()) {
        saveFailure(FailureReason::ConfigMismatchHostGpu);
        return false;
    }
    return true;
}

bool Snapshot::verifyConfig(const pb::Config& config) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (config.has_cpu_core_count() &&
        config.cpu_core_count() != vmConfig.numberOfCpuCores) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (config.has_ram_size_bytes() &&
        config.ram_size_bytes() != vmConfig.ramSizeBytes) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (config.has_cpu_core_count() &&
        config.cpu_core_count() != vmConfig.numberOfCpuCores) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (config.has_selected_renderer() &&
        config.selected_renderer() != int(emuglConfig_get_current_renderer())) {
        saveFailure(FailureReason::ConfigMismatchRenderer);
        return false;
    }

    const auto enabledFeatures = android::featurecontrol::getEnabled();
    if (int(enabledFeatures.size()) != config.enabled_features().size() ||
        !std::equal(config.enabled_features().begin(),
                    config.enabled_features().end(), enabledFeatures.begin(),
                    [](int l, android::featurecontrol::Feature r) {
                        return int(l) == r;
                    })) {
        saveFailure(FailureReason::ConfigMismatchFeatures);
        return false;
    }

    return true;
}

bool Snapshot::writeSnapshotToDisk() {
    google::protobuf::io::FileOutputStream stream(
            ::open(PathUtils::join(mDataDir, "snapshot.pb").c_str(),
                   O_WRONLY | O_BINARY | O_CREAT | O_TRUNC | O_CLOEXEC, 0755));
    stream.SetCloseOnDelete(true);
    if (mSnapshotPb.SerializeToZeroCopyStream(&stream)) {
        mSize = uint64_t(stream.ByteCount());
        return true;
    } else {
        mSize = 0;
        return false;
    }
}

struct {
    pb::Image::Type type;
    char* (*pathGetter)(const AvdInfo* info);
} static constexpr kImages[] = {
        {pb::Image::IMAGE_TYPE_KERNEL, avdInfo_getKernelPath},
        {pb::Image::IMAGE_TYPE_KERNEL_RANCHU, avdInfo_getRanchuKernelPath},
        {pb::Image::IMAGE_TYPE_SYSTEM, avdInfo_getSystemInitImagePath},
        {pb::Image::IMAGE_TYPE_SYSTEM_COPY, avdInfo_getSystemImagePath},
        {pb::Image::IMAGE_TYPE_DATA, avdInfo_getDataInitImagePath},
        {pb::Image::IMAGE_TYPE_DATA_COPY, avdInfo_getDataImagePath},
        {pb::Image::IMAGE_TYPE_RAMDISK, avdInfo_getRamdiskPath},
        {pb::Image::IMAGE_TYPE_SDCARD, avdInfo_getSdCardPath},
        {pb::Image::IMAGE_TYPE_CACHE, avdInfo_getCachePath},
        {pb::Image::IMAGE_TYPE_VENDOR, avdInfo_getVendorImagePath},
        {pb::Image::IMAGE_TYPE_ENCRYPTION_KEY,
         avdInfo_getEncryptionKeyImagePath},
};

static constexpr int kVersion = 7;

base::StringView Snapshot::dataDir(const char* name) {
    return getSnapshotDir(name);
}

Snapshot::Snapshot(const char* name)
    : mName(name), mDataDir(getSnapshotDir(name)) {}

bool Snapshot::save() {
    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    if (path_copy_file(targetHwIni.c_str(),
                       avdInfo_getCoreHwIniPath(android_avdInfo)) != 0) {
        return false;
    }

    mSnapshotPb.Clear();
    mSnapshotPb.set_version(kVersion);
    mSnapshotPb.set_creation_time(System::get()->getUnixTime());

    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    mSnapshotPb.mutable_config()->set_cpu_core_count(vmConfig.numberOfCpuCores);
    mSnapshotPb.mutable_config()->set_ram_size_bytes(vmConfig.ramSizeBytes);
    mSnapshotPb.mutable_config()->set_selected_renderer(
            int(emuglConfig_get_current_renderer()));
    mSnapshotPb.mutable_host()->set_hypervisor(vmConfig.hypervisorType);
    if (auto gpuString = currentGpuDriverString()) {
        mSnapshotPb.mutable_host()->set_gpu_driver(*gpuString);
    }
    for (auto f : android::featurecontrol::getEnabled()) {
        mSnapshotPb.mutable_config()->add_enabled_features(int(f));
    }

    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        fillImageInfo(image.type, path.get(), mSnapshotPb.add_images());
    }

    mSnapshotPb.set_guest_data_partition_mounted(guest_data_partition_mounted);
    mSnapshotPb.set_rotation(
            int(Snapshotter::get().windowAgent().getRotation()));

    return writeSnapshotToDisk();
}

bool Snapshot::saveFailure(FailureReason reason) {
    mSnapshotPb.set_failed_to_load_reason_code(int64_t(reason));
    if (!mSnapshotPb.has_version()) {
        mSnapshotPb.set_version(kVersion);
    }
    return writeSnapshotToDisk();
}

static bool isUnrecoverableReason(FailureReason reason) {
    return reason != FailureReason::Empty &&
           reason < FailureReason::UnrecoverableErrorLimit;
}

bool Snapshot::preload() {
    if (mSnapshotPb.has_version()) {
        return true;
    }
    const auto file =
            ScopedFd(::open(PathUtils::join(mDataDir, "snapshot.pb").c_str(),
                            O_RDONLY | O_BINARY | O_CLOEXEC, 0755));
    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        saveFailure(FailureReason::NoSnapshotPb);
        return false;
    }
    mSize = size;

    const auto fileMap = makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file.get(), 0),
            [size](void* ptr) {
                if (ptr != MAP_FAILED) {
                    munmap(ptr, size);
                }
            });
    if (!fileMap || fileMap.get() == MAP_FAILED) {
        saveFailure(FailureReason::BadSnapshotPb);
        return false;
    }
    if (!mSnapshotPb.ParseFromArray(fileMap.get(), size)) {
        saveFailure(FailureReason::BadSnapshotPb);
        return false;
    }
    if (mSnapshotPb.has_failed_to_load_reason_code() &&
        isUnrecoverableReason(
                FailureReason(mSnapshotPb.failed_to_load_reason_code()))) {
        return false;
    }
    if (!mSnapshotPb.has_version() || mSnapshotPb.version() != kVersion) {
        saveFailure(FailureReason::IncompatibleVersion);
        return false;
    }

    return true;
}

bool Snapshot::load() {
    if (!preload()) {
        return false;
    }

    if (mSnapshotPb.has_host() && !verifyHost(mSnapshotPb.host())) {
        return false;
    }
    if (mSnapshotPb.has_config() && !verifyConfig(mSnapshotPb.config())) {
        return false;
    }
    if (mSnapshotPb.images_size() > int(ARRAY_SIZE(kImages))) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    int matchedImages = 0;
    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        const auto type = image.type;
        const auto it = std::find_if(
                mSnapshotPb.images().begin(), mSnapshotPb.images().end(),
                [type](const pb::Image& im) {
                    return im.has_type() && im.type() == type;
                });
        if (it != mSnapshotPb.images().end()) {
            if (!verifyImageInfo(image.type, path.get(), *it)) {
                saveFailure(FailureReason::SystemImageChanged);
                return false;
            }
            ++matchedImages;
        } else {
            // Should match an empty image info
            if (!verifyImageInfo(image.type, path.get(), pb::Image())) {
                saveFailure(FailureReason::SystemImageChanged);
                return false;
            }
        }
    }
    if (matchedImages != mSnapshotPb.images_size()) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read(false)) {
        saveFailure(FailureReason::CorruptedData);
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(android_avdInfo));
    if (!actualConfig.read(false)) {
        saveFailure(FailureReason::InternalError);
        return false;
    }
    IniFile expectedStripped;
    androidHwConfig_stripDefaults(
            reinterpret_cast<CIniFile*>(&expectedConfig),
            reinterpret_cast<CIniFile*>(&expectedStripped));
    IniFile actualStripped;
    androidHwConfig_stripDefaults(reinterpret_cast<CIniFile*>(&actualConfig),
                                  reinterpret_cast<CIniFile*>(&actualStripped));
    if (!areHwConfigsEqual(expectedStripped, actualStripped)) {
        saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (mSnapshotPb.has_guest_data_partition_mounted()) {
        guest_data_partition_mounted =
                mSnapshotPb.guest_data_partition_mounted();
    }
    if (mSnapshotPb.has_rotation() &&
        Snapshotter::get().windowAgent().getRotation() !=
                SkinRotation(mSnapshotPb.rotation())) {
        Snapshotter::get().windowAgent().rotate(
                SkinRotation(mSnapshotPb.rotation()));
    }

    return true;
}

base::Optional<FailureReason> Snapshot::failureReason() const {
    return mSnapshotPb.has_failed_to_load_reason_code()
                   ? base::makeOptional(FailureReason(
                             mSnapshotPb.failed_to_load_reason_code()))
                   : base::kNullopt;
}

}  // namespace snapshot
}  // namespace android
