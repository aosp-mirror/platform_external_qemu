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
#include "android/featurecontrol/Features.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "android/protobuf/LoadSave.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Quickboot.h"
#include "android/snapshot/Snapshotter.h"
#include "android/utils/fd.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

#include <fcntl.h>
#include <sys/mman.h>

#include <algorithm>
#include <functional>
#include <unordered_set>

#define ALLOW_CHANGE_RENDERER

using android::base::c_str;
using android::base::endsWith;
using android::base::IniFile;
using android::base::makeCustomScopedPtr;
using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;

using android::featurecontrol::Feature;

using android::protobuf::loadProtobuf;
using android::protobuf::ProtobufLoadResult;
using android::protobuf::saveProtobuf;
using android::protobuf::ProtobufSaveResult;

namespace fc = android::featurecontrol;
namespace pb = emulator_snapshot;

namespace android {
namespace snapshot {

static void fillImageInfo(pb::Image::Type type,
                          StringView path,
                          pb::Image* image) {
    image->set_type(type);
    *image->mutable_path() = path;
    if (path.empty() || !path_is_regular(c_str(path))) {
        image->set_present(false);
        return;
    }

    image->set_present(true);
    struct stat st;
    if (!android_stat(c_str(path), &st)) {
        image->set_size(st.st_size);
        image->set_modification_time(st.st_mtime);
    }
}

static bool verifyImageInfo(pb::Image::Type type,
                            StringView path,
                            const pb::Image& in) {
    auto pathStr = path.str();

    if (in.type() != type) {
        return false;
    }
    const bool savedPresent = in.has_present() && in.present();
    const bool realPresent = !path.empty() && path_is_regular(c_str(path));
    if (savedPresent != realPresent) {
        return false;
    }
    struct stat st;
    static_assert(sizeof(st.st_size >= sizeof(uint64_t)),
                  "Bad size member in struct stat, fix build options");
    if (android_stat(c_str(path), &st) != 0) {
        if (in.has_size() || in.has_modification_time()) {
            return false;
        }
    } else {
        if (!in.has_size() || in.size() != st.st_size) {
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
#ifdef ALLOW_CHANGE_RENDERER
        if (!actual.hasKey(key) || actual.getString(key, "$$$")
                != expected.getString(key, "$$$")) {
            if (key != "hw.gpu.mode") {
                return false;
            }
        }
#else // ALLOW_CHANGE_RENDERER
        if (!actual.hasKey(key)) {
            return false;
        }
        if (actual.getString(key, "$$$") != expected.getString(key, "$$$")) {
            return false;
        }
#endif // ALLOW_CHANGE_RENDERER
    }

    for (auto&& key : actual) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
            if (key != "disk.dataPartition.initPath") {
                numPathActual++;
            }
            continue;  // these contain absolute paths and will be checked
                       // separately.
        }
#ifdef ALLOW_CHANGE_RENDERER
        if (!expected.hasKey(key) || actual.getString(key, "$$$")
                != expected.getString(key, "$$$")) {
            if (key != "hw.gpu.mode") {
                return false;
            }
        }
#else // ALLOW_CHANGE_RENDERER
        if (!expected.hasKey(key)) {
            return false;
        }
        if (actual.getString(key, "$$$") != expected.getString(key, "$$$")) {
            return false;
        }
#endif // ALLOW_CHANGE_RENDERER
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

bool Snapshot::verifyHost(const pb::Host& host, bool writeFailure) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (host.has_hypervisor() && host.hypervisor() != vmConfig.hypervisorType) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchHostHypervisor);
        return false;
    }
    // Do not worry about backend if in swiftshader_indirect
    if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
        return true;
    }
    if (auto gpuString = currentGpuDriverString()) {
        if (!host.has_gpu_driver() || host.gpu_driver() != *gpuString) {
            if (writeFailure) saveFailure(FailureReason::ConfigMismatchHostGpu);
            return false;
        }
    } else if (host.has_gpu_driver()) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchHostGpu);
        return false;
    }
    return true;
}

static const std::unordered_set<Feature, std::hash<size_t>> sInsensitiveFeatures({
#undef FEATURE_CONTROL_ITEM
#define FEATURE_CONTROL_ITEM(feature) \
    fc::feature,

#include "android/featurecontrol/FeatureControlDefSnapshotInsensitive.h"

#undef FEATURE_CONTROL_ITEM
});

const std::vector<Feature> Snapshot::getSnapshotInsensitiveFeatures() {
    return std::vector<Feature>(sInsensitiveFeatures.begin(),
        sInsensitiveFeatures.end());
}

// static
bool Snapshot::isFeatureSnapshotInsensitive(Feature input) {
    return sInsensitiveFeatures.count(input);
}

bool Snapshot::isCompatibleWithCurrentFeatures() {
    return verifyFeatureFlags(mSnapshotPb.config());
}

bool Snapshot::verifyFeatureFlags(const pb::Config& config) {

    auto enabledFeatures = fc::getEnabled();

    enabledFeatures.erase(
        std::remove_if(
            enabledFeatures.begin(),
            enabledFeatures.end(),
            isFeatureSnapshotInsensitive),
        enabledFeatures.end());

    // need a conversion from int to Feature here
    std::vector<Feature> configFeatures;

    for (auto it = config.enabled_features().begin();
         it != config.enabled_features().end();
         ++it) {
        configFeatures.push_back((Feature)(*it));
    }

    configFeatures.erase(
        std::remove_if(
            configFeatures.begin(),
            configFeatures.end(),
            isFeatureSnapshotInsensitive),
        configFeatures.end());

    if (int(enabledFeatures.size()) != configFeatures.size() ||
        !std::equal(configFeatures.begin(),
                    configFeatures.end(), enabledFeatures.begin(),
                    [](int l, Feature r) {
                        return int(l) == r;
                    })) {
        return false;
    }

    return true;
}

bool Snapshot::verifyConfig(const pb::Config& config, bool writeFailure) {
    VmConfiguration vmConfig;
    Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
    if (config.has_cpu_core_count() &&
        config.cpu_core_count() != vmConfig.numberOfCpuCores) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (config.has_ram_size_bytes() &&
        config.ram_size_bytes() != vmConfig.ramSizeBytes) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }
    if (config.has_selected_renderer() &&
        config.selected_renderer() != int(emuglConfig_get_current_renderer())) {
#ifdef ALLOW_CHANGE_RENDERER
        fprintf(stderr, "WARNING: change of renderer detected.\n");
#else // ALLOW_CHANGE_RENDERER
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchRenderer);
        return false;
#endif // ALLOW_CHANGE_RENDERER
    }

    if (!verifyFeatureFlags(config)) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchFeatures);
        return false;
    }

    return true;
}

bool Snapshot::writeSnapshotToDisk() {
    auto res =
        saveProtobuf(
            PathUtils::join(mDataDir, "snapshot.pb"),
            mSnapshotPb,
            &mSize);
    return res == ProtobufSaveResult::Success;
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

static constexpr int kVersion = 38;
static constexpr int kMaxSaveStatsHistory = 10;

base::StringView Snapshot::dataDir(const char* name) {
    return getSnapshotDir(name);
}

base::Optional<std::string> Snapshot::parent() {
    auto info = getGeneralInfo();
    if (!info || !info->has_parent()) return base::kNullopt;
    auto parentName = info->parent();
    if (parentName == "") return base::kNullopt;
    return parentName;
}


Snapshot::Snapshot(const char* name)
    : Snapshot(name, getSnapshotDir(name).c_str()) { }

Snapshot::Snapshot(const char* name, const char* dataDir)
    : mName(name), mDataDir(dataDir), mSaveStats(kMaxSaveStatsHistory) {

    // All persisted snapshot protobuf fields across
    // saves / loads go here.
    if (preload()) {
        for (int i = 0; i < mSnapshotPb.save_stats_size(); i++) {
            mSaveStats.push_back(mSnapshotPb.save_stats(i));
        }
    }
}

// static
std::vector<Snapshot> Snapshot::getExistingSnapshots() {
    std::vector<Snapshot> res = {};
    auto snapshotFiles = getSnapshotDirEntries();
    for (const auto& filename : snapshotFiles) {
        Snapshot s(filename.c_str());
        if (s.preload()) {
            res.push_back(s);
        }
    }
    return res;
}

void Snapshot::initProto() {
    mSnapshotPb.Clear();
    mSnapshotPb.set_version(kVersion);
    mSnapshotPb.set_creation_time(System::get()->getUnixTime());
}

void Snapshot::saveEnabledFeatures() {
    for (auto f : fc::getEnabled()) {
        if (isFeatureSnapshotInsensitive(f)) continue;
        mSnapshotPb.mutable_config()->add_enabled_features(int(f));
    }
}

bool Snapshot::save() {
    // In saving, we assume the state is different,
    // so we reset the invalid/successful counters.

    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    if (path_copy_file(targetHwIni.c_str(),
                       avdInfo_getCoreHwIniPath(android_avdInfo)) != 0) {
        return false;
    }

    initProto();

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

    saveEnabledFeatures();

    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        fillImageInfo(image.type, path.get(), mSnapshotPb.add_images());
    }

    mSnapshotPb.set_guest_data_partition_mounted(guest_data_partition_mounted);
    mSnapshotPb.set_rotation(
            int(Snapshotter::get().windowAgent().getRotation()));
    mSnapshotPb.set_folded(
            int(Snapshotter::get().windowAgent().isFolded()));

    mSnapshotPb.set_invalid_loads(mInvalidLoads);
    mSnapshotPb.set_successful_loads(mSuccessfulLoads);

    for (size_t i = 0; i < mSaveStats.size(); i++) {
        auto toSave = mSnapshotPb.add_save_stats();
        *toSave = mSaveStats[i];
    }

    auto parentSnapshot = Snapshotter::get().loadedSnapshotFile();
    // We want to maintain the default_boot snapshot as outside
    // the hierarchy. For that reason, don't set parent if default
    // boot is involved either as the current snapshot or the parent snapshot.
    if (mName != kDefaultBootSnapshot &&
        parentSnapshot != "" &&
        parentSnapshot != kDefaultBootSnapshot) {
        if (mName != parentSnapshot) {
            mSnapshotPb.set_parent(parentSnapshot);
        }
    }

    return writeSnapshotToDisk();
}

bool Snapshot::saveFailure(FailureReason reason) {
    if (reason == FailureReason::Empty) {
        mLatestFailureReason = reason;
        // Don't write this to disk
        return true;
    }
    if (reason == mLatestFailureReason) {
        // Nothing to do
        return true;
    }
    mSnapshotPb.set_failed_to_load_reason_code(int64_t(reason));
    if (!mSnapshotPb.has_version()) {
        mSnapshotPb.set_version(kVersion);
    }
    mSnapshotPb.set_invalid_loads(mInvalidLoads);
    mSnapshotPb.set_successful_loads(mSuccessfulLoads);
    if (writeSnapshotToDisk()) {
        // Success
        mLatestFailureReason = reason;
        return true;
    }
    return false;
}

static bool isUnrecoverableReason(FailureReason reason) {
    return reason != FailureReason::Empty &&
           reason < FailureReason::UnrecoverableErrorLimit;
}

void Snapshot::loadProtobufOnce() {
    if (mSnapshotPb.has_version()) {
        return;
    }
    const auto file =
            ScopedFd(::open(PathUtils::join(mDataDir, "snapshot.pb").c_str(),
                            O_RDONLY | O_BINARY | O_CLOEXEC, 0755));
    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        saveFailure(FailureReason::NoSnapshotPb);
        return;
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
        return;
    }
    if (!mSnapshotPb.ParseFromArray(fileMap.get(), size)) {
        saveFailure(FailureReason::BadSnapshotPb);
        return;
    }
    if (mSnapshotPb.has_failed_to_load_reason_code() &&
        isUnrecoverableReason(
                FailureReason(mSnapshotPb.failed_to_load_reason_code()))) {
        return;
    }
    if (!mSnapshotPb.has_version() || mSnapshotPb.version() != kVersion) {
        saveFailure(FailureReason::IncompatibleVersion);
        return;
    }
    // Success
}

// preload(): Obtain the protobuf metadata. Logic for deciding
// whether or not to load based on it, plus initialization
// of properties like skin rotation, is done afterward in load().
// Checks whether or not the version of the protobuf is compatible.
bool Snapshot::preload() {
    loadProtobufOnce();

    return (mSnapshotPb.has_version() && mSnapshotPb.version() == kVersion);
}

const emulator_snapshot::Snapshot* Snapshot::getGeneralInfo() {
    loadProtobufOnce();

    if (isUnrecoverableReason(
            FailureReason(mSnapshotPb.failed_to_load_reason_code()))) {
        saveFailure(FailureReason(mSnapshotPb.failed_to_load_reason_code()));
    }

    return &mSnapshotPb;
}

const bool Snapshot::checkValid(bool writeFailure) {
    if (!preload()) {
        return false;
    }

    if (fc::isEnabled(fc::AllowSnapshotMigration)) {
        return true;
    }

    if (mSnapshotPb.has_host() && !verifyHost(mSnapshotPb.host(), writeFailure)) {
        return false;
    }
    if (mSnapshotPb.has_config() && !verifyConfig(mSnapshotPb.config(), writeFailure)) {
        return false;
    }
    if (mSnapshotPb.images_size() > int(ARRAY_SIZE(kImages))) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchAvd);
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

                // If in build env, nuke the qcow2 as well.
                if (avdInfo_inAndroidBuild(android_avdInfo)) {
                    std::string qcow2Path(path.get());
                    qcow2Path += ".qcow2";
                    path_delete_file(qcow2Path.c_str());
                }

                if (writeFailure) saveFailure(FailureReason::SystemImageChanged);
                return false;
            }
            ++matchedImages;
        } else {
            // Should match an empty image info
            if (!verifyImageInfo(image.type, path.get(), pb::Image())) {

                // If in build env, nuke the qcow2 as well.
                if (avdInfo_inAndroidBuild(android_avdInfo)) {
                    std::string qcow2Path(path.get());
                    qcow2Path += ".qcow2";
                    path_delete_file(qcow2Path.c_str());
                }

                if (writeFailure) saveFailure(FailureReason::NoSnapshotInImage);
                return false;
            }
        }
    }
    if (matchedImages != mSnapshotPb.images_size()) {
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read(false)) {
        if (writeFailure) saveFailure(FailureReason::CorruptedData);
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(android_avdInfo));
    if (!actualConfig.read(false)) {
        if (writeFailure) saveFailure(FailureReason::InternalError);
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
        if (writeFailure) saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    if (mSnapshotPb.has_invalid_loads()) {
        mInvalidLoads = mSnapshotPb.invalid_loads();
    } else {
        mInvalidLoads = 0;
    }

    if (mSnapshotPb.has_successful_loads()) {
        mSuccessfulLoads = mSnapshotPb.successful_loads();
    } else {
        mSuccessfulLoads = 0;
    }

    if (isUnrecoverableReason(
            FailureReason(mSnapshotPb.failed_to_load_reason_code()))) {
        if (writeFailure) {
            saveFailure(FailureReason(mSnapshotPb.failed_to_load_reason_code()));
        }
        return false;
    }

    return true;
}

// load(): Loads all snapshot metadata and checks it against other files
// for integrity.
bool Snapshot::load() {

    if (!checkValid(true /* save the failure reason */)) {
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

    if (mSnapshotPb.has_folded() &&
        Snapshotter::get().windowAgent().isFolded() !=
                mSnapshotPb.folded()) {
        Snapshotter::get().windowAgent().fold(mSnapshotPb.folded());
    }

    return true;
}

void Snapshot::incrementInvalidLoads() {
    ++mInvalidLoads;
    mSnapshotPb.set_invalid_loads(mInvalidLoads);
    if (!mSnapshotPb.has_version()) {
        mSnapshotPb.set_version(kVersion);
    }
    writeSnapshotToDisk();
}

void Snapshot::incrementSuccessfulLoads() {
    ++mSuccessfulLoads;
    mSnapshotPb.set_successful_loads(mSuccessfulLoads);
    if (!mSnapshotPb.has_version()) {
        mSnapshotPb.set_version(kVersion);
    }
    writeSnapshotToDisk();
}

void Snapshot::addSaveStats(bool incremental,
                            const base::System::Duration duration,
                            uint64_t ramChangedBytes) {

    emulator_snapshot::SaveStats stats;

    stats.set_incremental(incremental ? 1 : 0);
    stats.set_duration((uint64_t)duration);
    stats.set_ram_changed_bytes((uint64_t)ramChangedBytes);

    mSaveStats.push_back(stats);
}

bool Snapshot::areSavesSlow() const {

    if (mSaveStats.empty()) return false;

    static constexpr float kSlowSaveThresholdMs = 20000.0;
    float maxSaveTimeMs = 0.0f;

    for (size_t i = 0; i < mSaveStats.size(); ++i) {
        float durationMs = mSaveStats[i].duration() / 1000.0f;
        maxSaveTimeMs = std::max(durationMs, maxSaveTimeMs);
    }

   return maxSaveTimeMs >= kSlowSaveThresholdMs;
}

bool Snapshot::shouldInvalidate() const {
    // Either there wasn't any successful loads,
    // or there was more than one invalid load
    // of this snapshot.
    return !mSuccessfulLoads ||
           mInvalidLoads > 1;
}

base::Optional<FailureReason> Snapshot::failureReason() const {
    return mSnapshotPb.has_failed_to_load_reason_code()
                   ? base::makeOptional(FailureReason(
                             mSnapshotPb.failed_to_load_reason_code()))
                   : base::kNullopt;
}

}  // namespace snapshot
}  // namespace android
