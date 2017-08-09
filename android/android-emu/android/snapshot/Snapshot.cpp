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
#include "android/snapshot/proto/snapshot.pb.h"
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
    if (expected.size() != actual.size()) {
        return false;
    }
    for (auto&& key : expected) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
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

    return true;
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

static bool verifyHost(const pb::Host& host) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (host.has_hypervisor() && host.hypervisor() != vmConfig.hypervisorType) {
        return false;
    }
    if (auto gpuString = currentGpuDriverString()) {
        if (!host.has_gpu_driver() || host.gpu_driver() != *gpuString) {
            return false;
        }
    } else if (host.has_gpu_driver()) {
        return false;
    }
    return true;
}

static bool verifyConfig(const pb::Config& config) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (config.has_cpu_core_count() &&
        config.cpu_core_count() != vmConfig.numberOfCpuCores) {
        return false;
    }
    if (config.has_ram_size_bytes() &&
        config.ram_size_bytes() != vmConfig.ramSizeBytes) {
        return false;
    }
    if (config.has_cpu_core_count() &&
        config.cpu_core_count() != vmConfig.numberOfCpuCores) {
        return false;
    }
    if (config.has_selected_renderer() &&
        config.selected_renderer() != int(emuglConfig_get_current_renderer())) {
        return false;
    }

    const auto enabledFeatures = android::featurecontrol::getEnabled();
    if (int(enabledFeatures.size()) != config.enabled_features().size() ||
        !std::equal(config.enabled_features().begin(),
                    config.enabled_features().end(), enabledFeatures.begin(),
                    [](int l, android::featurecontrol::Feature r) {
                        return int(l) == r;
                    })) {
        return false;
    }

    return true;
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

static constexpr int kVersion = 1;

Snapshot::Snapshot(const char* name)
    : mName(name), mDataDir(getSnapshotDir(name)) {}

bool Snapshot::save() {
    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    if (path_copy_file(targetHwIni.c_str(),
                       avdInfo_getCoreHwIniPath(android_avdInfo)) != 0) {
        return false;
    }

    pb::Snapshot snapshot;
    snapshot.set_version(kVersion);
    snapshot.set_creation_time(System::get()->getUnixTime());

    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    snapshot.mutable_config()->set_cpu_core_count(vmConfig.numberOfCpuCores);
    snapshot.mutable_config()->set_ram_size_bytes(vmConfig.ramSizeBytes);
    snapshot.mutable_config()->set_selected_renderer(
            int(emuglConfig_get_current_renderer()));
    snapshot.mutable_host()->set_hypervisor(vmConfig.hypervisorType);
    if (auto gpuString = currentGpuDriverString()) {
        snapshot.mutable_host()->set_gpu_driver(*gpuString);
    }
    for (auto f : android::featurecontrol::getEnabled()) {
        snapshot.mutable_config()->add_enabled_features(int(f));
    }

    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        fillImageInfo(image.type, path.get(), snapshot.add_images());
    }

    google::protobuf::io::FileOutputStream stream(
            ::open(PathUtils::join(mDataDir, "snapshot.pb").c_str(),
                   O_WRONLY | O_BINARY | O_CREAT | O_TRUNC | O_CLOEXEC, 0755));
    stream.SetCloseOnDelete(true);
    if (!snapshot.SerializeToZeroCopyStream(&stream)) {
        return false;
    }
    return true;
}

bool Snapshot::load() {
    const auto file =
            ScopedFd(::open(PathUtils::join(mDataDir, "snapshot.pb").c_str(),
                            O_RDONLY | O_BINARY | O_CLOEXEC, 0755));
    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        return false;
    }
    const auto fileMap = makeCustomScopedPtr(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file.get(), 0),
            [size](void* ptr) {
                if (ptr != MAP_FAILED)
                    munmap(ptr, size);
            });
    if (!fileMap || fileMap.get() == MAP_FAILED) {
        return false;
    }
    pb::Snapshot snapshot;
    if (!snapshot.ParseFromArray(fileMap.get(), size)) {
        return false;
    }
    if (!snapshot.has_version() || snapshot.version() != kVersion) {
        return false;
    }
    if (snapshot.has_host() && !verifyHost(snapshot.host())) {
        return false;
    }
    if (snapshot.has_config() && !verifyConfig(snapshot.config())) {
        return false;
    }
    if (snapshot.images_size() > int(ARRAY_SIZE(kImages))) {
        return false;
    }
    int matchedImages = 0;
    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        const auto it =
                std::find_if(snapshot.images().begin(), snapshot.images().end(),
                             [type = image.type](const pb::Image& im) {
                                 return im.has_type() && im.type() == type;
                             });
        if (it != snapshot.images().end()) {
            if (!verifyImageInfo(image.type, path.get(), *it)) {
                return false;
            }
            ++matchedImages;
        } else {
            // Should match an empty image info
            if (!verifyImageInfo(image.type, path.get(), pb::Image())) {
                return false;
            }
        }
    }
    if (matchedImages != snapshot.images_size()) {
        return false;
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read()) {
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(android_avdInfo));
    if (!actualConfig.read()) {
        return false;
    }
    if (!areHwConfigsEqual(expectedConfig, actualConfig)) {
        return false;
    }

    return true;
}

}  // namespace snapshot
}  // namespace android
