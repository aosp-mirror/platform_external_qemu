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

#include <fcntl.h>     // for open, O_CLOEXEC
#include <stdio.h>     // for printf, fprintf
#include <string.h>    // for strcmp, strlen
#include <sys/mman.h>  // for mmap, munmap
#include <cstdlib>
#include <string>

#include <algorithm>      // for find_if, remove_if
#include <fstream>        // for operator<<, ost...
#include <functional>     // for __base
#include <iterator>       // for end
#include <memory>         // for unique_ptr
#include <string_view>
#include <type_traits>    // for decay<>::type
#include <unordered_set>  // for unordered_set<>...
#include <utility>        // for hash

#include "host-common/hw-config.h"        // for androidHwConfig...
#include "host-common/hw-config-helper.h"        // for androidHwConfig...
#include "android/avd/info.h"             // for avdInfo_getCach...
#include "aemu/base/ArraySize.h"       // for ARRAY_SIZE
#include "aemu/base/Log.h"             // for LogStreamVoidify
#include "aemu/base/Optional.h"        // for Optional, makeO...
#include "aemu/base/ProcessControl.h"  // for loadLaunchParam...
#include "aemu/base/StringFormat.h"    // for StringFormat
#include "aemu/base/files/GzipStreambuf.h"
#include "aemu/base/files/PathUtils.h"  // for PathUtils, pj
#include "aemu/base/files/ScopedFd.h"   // for ScopedFd
#include "aemu/base/files/TarStream.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/memory/ScopedPtr.h"  // for makeCustomScope...
#include "aemu/base/misc/StringUtils.h"  // for endsWith
#include "android/base/files/IniFile.h" // for IniFile
#include "android/base/system/System.h"     // for System, System:...
#include "android/console.h"  // for getConsoleAgents()->settings->avdInfo()
#include "android/emulation/ConfigDirs.h"             // for ConfigDirs
#include "android/emulation/resizable_display_config.h"
#include "host-common/vm_operations.h"  // for VmConfiguration
#include "host-common/window_agent.h"   // for QAndroidEmulato...
#include "host-common/FeatureControl.h"    // for getEnabled, isE...
#include "host-common/Features.h"          // for Feature, AllowS...
#include "host-common/opengl/emugl_config.h"              // for emuglConfig_get...
#include "android/opengl/gpuinfo.h"                   // for GpuInfo, global...
#include "android/protobuf/LoadSave.h"                // for ProtobufSaveResult
#include "android/skin/rect.h"                        // for SkinRotation
#include "android/snapshot/Loader.h"                  // for Loader
#include "android/snapshot/PathUtils.h"               // for getAvdDir, getS...
#include "android/snapshot/Snapshotter.h"             // for Snapshotter
#include "android/utils/fd.h"                         // for O_CLOEXEC
#include "android/utils/file_data.h"                  // for (anonymous)
#include "android/utils/file_io.h"                    // for android_stat
#include "android/utils/ini.h"                        // for CIniFile
#include "android/utils/path.h"                       // for path_delete_file
#include "android/utils/system.h"                     // for STRINGIFY
#include "google/protobuf/stubs/port.h"               // for int32

#define ALLOW_CHANGE_RENDERER

#ifdef AEMU_GFXSTREAM_BACKEND
static constexpr bool HAS_GFXSTREAM = true;
#else
static constexpr bool HAS_GFXSTREAM = false;
#endif

using android::base::c_str;
using android::base::endsWith;
using android::base::IniFile;
using android::base::makeCustomScopedPtr;
using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::ScopedFd;
using android::base::StringFormat;
using android::base::System;


using android::featurecontrol::Feature;

using android::protobuf::loadProtobuf;
using android::protobuf::ProtobufLoadResult;
using android::protobuf::ProtobufSaveResult;
using android::protobuf::saveProtobuf;

namespace fc = android::featurecontrol;
namespace proto = emulator_snapshot;

namespace android {
namespace snapshot {

static void fillImageInfo(proto::Image::Type type,
                          const char* path,
                          proto::Image* image) {
    image->set_type(type);
    image->set_path(path ? path : "");
    if (!path || std::string(path).empty() || !path_is_regular(c_str(path))) {
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

static bool verifyImageInfo(proto::Image::Type type,
                            const char* path,
                            const proto::Image& in) {
    std::string pathStr(path ? path : "");
    if (in.type() != type) {
        return false;
    }
    const bool savedPresent = in.has_present() && in.present();
    const bool realPresent = !pathStr.empty() && path_is_regular(pathStr.data());
    if (savedPresent != realPresent) {
        return false;
    }
    struct stat st;
    static_assert(sizeof(st.st_size >= sizeof(uint64_t)),
                  "Bad size member in struct stat, fix build options");
    if (android_stat(pathStr.data(), &st) != 0) {
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

const char* const sHwConfigSnapshotInsensitiveKeys[] = {
        "hw.gpu.mode",
        "avd.id",
};

static bool iniFileKeyMismatches(const IniFile& expected,
                                 const IniFile& actual,
                                 const std::string& key) {
    return !actual.hasKey(key) ||
           (actual.getString(key, "$$$") != expected.getString(key, "$$$"));
}

static bool isKeySnapshotSensitive(const std::string& key) {
    for (auto insensitiveKey : sHwConfigSnapshotInsensitiveKeys) {
        if (!strcmp(key.c_str(), insensitiveKey))
            return false;
    }
    return true;
}

bool areHwConfigsEqual(const IniFile& expected, const IniFile& actual) {
    // We need to explicitly check the list because when booted with -wipe-data
    // the emulator will add a line (disk.dataPartition.initPath) which should
    // not affect snapshot.
    std::unordered_set<std::string> ignore{"android.sdk.root",
                                           "android.avd.home"};
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
        if (ignore.count(key) > 0) {
            continue;  // The set of properties we feel that we can safely
                       // ignore.
        }

        if (iniFileKeyMismatches(expected, actual, key) &&
            isKeySnapshotSensitive(key))
            return false;
    }

    for (auto&& key : actual) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
            if (key != "disk.dataPartition.initPath") {
                numPathActual++;
            }
            continue;  // these contain absolute paths and will be checked
                       // separately.
        }

        if (ignore.count(key) > 0) {
            continue;  // The set of properties we feel that we can safely
                       // ignore.
        }

        if (iniFileKeyMismatches(actual, expected, key) &&
            isKeySnapshotSensitive(key))
            return false;
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

bool Snapshot::verifyHost(const proto::Host& host, bool writeFailure) {
    if (Snapshotter::get().vmOperations().getVmConfiguration != nullptr) {
        VmConfiguration vmConfig;
        Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
        if (host.has_hypervisor() &&
            host.hypervisor() != vmConfig.hypervisorType) {
            if (writeFailure) {
                saveFailure(FailureReason::ConfigMismatchHostHypervisor);
            }
            return false;
        }
    }
    // Do not worry about backend if in swiftshader_indirect
    if (emuglConfig_get_current_renderer() ==
        SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
        return true;
    }
    if (auto gpuString = currentGpuDriverString()) {
        if (!host.has_gpu_driver() || host.gpu_driver() != *gpuString) {
            if (writeFailure) {
                saveFailure(FailureReason::ConfigMismatchHostGpu);
            }
            return false;
        }
    } else if (host.has_gpu_driver()) {
        if (writeFailure) {
            saveFailure(FailureReason::ConfigMismatchHostGpu);
        }
        return false;
    }
    return true;
}

static const std::unordered_set<Feature, std::hash<size_t>>
        sInsensitiveFeatures({
#undef FEATURE_CONTROL_ITEM
#define FEATURE_CONTROL_ITEM(feature) fc::feature,

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

static bool featureSetsEqual(
        const std::unordered_set<Feature>& snapshotFeatures,
        const std::unordered_set<Feature>& emulatorFeatures) {
    // We uset set1 subset set2 && set2 subset set1 algorithm
    // Mainly so we can inform the user (or debuggers) which feature is
    // required, but now missing.
    // Note that sets are fixed size and very small, so we are not concerned about
    // performance.

    // snapshot \subset emulatorFeatures
    for (const auto& element : snapshotFeatures) {
        if (emulatorFeatures.find(element) == emulatorFeatures.end()) {
            derror("The snapshot requires the feature: %d, which the emulator "
                   "does not support",
                   element);
            return false;
        }
    }

    // emulatorFeatures \subset snapshot
    for (const auto& element : emulatorFeatures) {
        if (snapshotFeatures.find(element) == snapshotFeatures.end()) {
            derror("The emulator has the feature: %d, which is missing in the "
                   "snapshot", element);
            return false;
        }
    }

    return true;
}

bool Snapshot::verifyFeatureFlags(const proto::Config& config) {
    auto enabled = fc::getEnabled();
    enabled.erase(
            std::remove_if(enabled.begin(), enabled.end(),
                           isFeatureSnapshotInsensitive),
            enabled.end());

    auto emulatorFeatures = std::unordered_set<Feature>(enabled.begin(), enabled.end());

    std::unordered_set<Feature> snapshotFeatures;
    for (auto it = config.enabled_features().begin();
         it != config.enabled_features().end(); ++it) {
        Feature f = static_cast<Feature>(*it);
        if (!isFeatureSnapshotInsensitive(f))
            snapshotFeatures.insert(f);
    }

    return featureSetsEqual(snapshotFeatures, emulatorFeatures);
}

bool Snapshot::verifyConfig(const proto::Config& config, bool writeFailure) {
    if (Snapshotter::get().vmOperations().getVmConfiguration != nullptr) {
        VmConfiguration vmConfig;
        Snapshotter::get().vmOperations().getVmConfiguration(&vmConfig);
        if (config.has_cpu_core_count() &&
            config.cpu_core_count() != vmConfig.numberOfCpuCores) {
            if (writeFailure) {
                saveFailure(FailureReason::ConfigMismatchAvd);
            }
            return false;
        }
        if (config.has_ram_size_bytes() &&
            config.ram_size_bytes() != vmConfig.ramSizeBytes) {
            if (writeFailure) {
                saveFailure(FailureReason::ConfigMismatchAvd);
            }
            return false;
        }
    }
    if (config.has_selected_renderer() &&
        config.selected_renderer() != int(emuglConfig_get_current_renderer())) {
#ifdef ALLOW_CHANGE_RENDERER
        dwarning("change of renderer detected.");
#else   // ALLOW_CHANGE_RENDERER
        if (writeFailure) {
            saveFailure(FailureReason::ConfigMismatchRenderer);
        }
        return false;
#endif  // ALLOW_CHANGE_RENDERER
    }

    if (!verifyFeatureFlags(config)) {
        if (writeFailure) {
            saveFailure(FailureReason::ConfigMismatchFeatures);
        }
        return false;
    }

    return true;
}

bool Snapshot::isLoaded() {
    auto loader = android::snapshot::Snapshotter::get().hasLoader();
    return loader &&
           android::snapshot::Snapshotter::get().loader().snapshot() == *this;
}

bool Snapshot::writeSnapshotToDisk() {
    if (path_mkdir_if_needed_no_cow(mDataDir.c_str(), 0744)) {
        return false;
    }

    auto res = saveProtobuf(PathUtils::join(mDataDir, kSnapshotProtobufName),
                            mSnapshotPb, &mSize);
    return res == ProtobufSaveResult::Success;
}

struct {
    proto::Image::Type type;
    char* (*pathGetter)(const AvdInfo* info);
} static constexpr kImages[] = {
        {proto::Image::IMAGE_TYPE_KERNEL, avdInfo_getKernelPath},
        {proto::Image::IMAGE_TYPE_KERNEL_RANCHU, avdInfo_getRanchuKernelPath},
        {proto::Image::IMAGE_TYPE_SYSTEM, avdInfo_getSystemInitImagePath},
        {proto::Image::IMAGE_TYPE_SYSTEM_COPY, avdInfo_getSystemImagePath},
        {proto::Image::IMAGE_TYPE_DATA, avdInfo_getDataInitImagePath},
        {proto::Image::IMAGE_TYPE_DATA_COPY, avdInfo_getDataImagePath},
        {proto::Image::IMAGE_TYPE_RAMDISK, avdInfo_getRamdiskPath},
        {proto::Image::IMAGE_TYPE_SDCARD, avdInfo_getSdCardPath},
        {proto::Image::IMAGE_TYPE_CACHE, avdInfo_getCachePath},
        {proto::Image::IMAGE_TYPE_VENDOR, avdInfo_getVendorImagePath},
        {proto::Image::IMAGE_TYPE_ENCRYPTION_KEY,
         avdInfo_getEncryptionKeyImagePath},
};

#if 1
// Calculate snapshot version based on a base version plus featurecontrol-derived integer.
static constexpr int kVersionBase = 82;
static_assert(kVersionBase < (1 << 20), "Base version number is too high.");

#define FEATURE_CONTROL_ITEM(item, idx) + 1

// 0 + 1 + 1 + 1 (+ 1) for each item in feature control
static constexpr int kFeatureOffset = 0
    #include "host-common/FeatureControlDefHost.h"
    #include "host-common/FeatureControlDefGuest.h"
;

#undef FEATURE_CONTROL_ITEM

static_assert(kFeatureOffset < 1024, "Too many features to include in the feature "
                                     "offset component of the snapshot version");
static constexpr int kVersion = (kVersionBase << 10) + kFeatureOffset;

#else
// We currently hardcode the kVersion as we are in the process of migrating
// To a different versioning scheme see: b/287119326
#error Consider updating the line below
static constexpr int kVersion = 83037;
#endif

static constexpr int kMaxSaveStatsHistory = 10;

std::string Snapshot::dataDir(const char* name) {
    return getSnapshotDir(name);
}

base::Optional<std::string> Snapshot::parent() {
    auto info = getGeneralInfo();
    if (!info || !info->has_parent())
        return base::kNullopt;
    auto parentName = info->parent();
    if (parentName == "")
        return base::kNullopt;
    return parentName;
}

Snapshot::Snapshot(const char* name)
    : Snapshot(name, getSnapshotDir(name).c_str(), false) {}

Snapshot::Snapshot(const char* name, const char* dataDir, bool verifyQcow)
    : mName(name),
      mDataDir(dataDir),
      mSaveStats(kMaxSaveStatsHistory),
     mVerifyQcow(verifyQcow) {
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
        if (isFeatureSnapshotInsensitive(f))
            continue;
        mSnapshotPb.mutable_config()->add_enabled_features(int(f));
    }
}

base::Optional<Snapshot> Snapshot::getSnapshotById(std::string id) {
    std::vector<Snapshot> snapshots = getExistingSnapshots();
    auto snapshot =
            std::find_if(snapshots.begin(), snapshots.end(),
                         [&id](const Snapshot& s) { return s.name() == id; });
    if (snapshot != std::end(snapshots)) {
        return base::makeOptional<Snapshot>(*snapshot);
    }
    return {};
}

std::string Snapshot::screenshot() {
    return PathUtils::join(mDataDir, "screenshot.png");
}

bool Snapshot::save() {
    // In saving, we assume the state is different,
    // so we reset the invalid/successful counters.

    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    if (path_copy_file(targetHwIni.c_str(),
                       avdInfo_getCoreHwIniPath(getConsoleAgents()->settings->avdInfo())) != 0) {
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
        ScopedCPtr<char> path(image.pathGetter(getConsoleAgents()->settings->avdInfo()));
        fillImageInfo(image.type, path.get(), mSnapshotPb.add_images());
    }

    mSnapshotPb.set_guest_data_partition_mounted(
        getConsoleAgents()->settings->guest_data_partition_mounted());
    mSnapshotPb.set_rotation(
            int(Snapshotter::get().windowAgent().getRotation()));

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
    if (mName != kDefaultBootSnapshot && parentSnapshot != "" &&
        parentSnapshot != kDefaultBootSnapshot) {
        if (mName != parentSnapshot) {
            mSnapshotPb.set_parent(parentSnapshot);
        }
    }

    const char* contentPath = avdInfo_getContentPath(getConsoleAgents()->settings->avdInfo());
    auto path = PathUtils::join(contentPath ? contentPath : "",
                                base::kLaunchParamsFileName);
    if (System::get()->pathExists(path)) {
        // TODO(yahan@): -read-only emulators might not have this file.
        base::ProcessLaunchParameters launchParameters =
                base::loadLaunchParameters(path);
        for (const std::string& argv : launchParameters.argv) {
            mSnapshotPb.add_launch_parameters(argv);
        }
    }

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#ifdef ANDROID_SDK_TOOLS_BUILD_NUMBER
    mSnapshotPb.set_emulator_build_id(
            STRINGIFY(ANDROID_SDK_TOOLS_BUILD_NUMBER));
#endif
    const auto buildProps = avdInfo_getBuildProperties(getConsoleAgents()->settings->avdInfo());
    android::base::IniFile ini((const char*)buildProps->data, buildProps->size);
    auto buildId = ini.getString("ro.build.fingerprint", "");
    if (buildId.empty()) {
        buildId = ini.getString("ro.build.display.id", "");
    }
    mSnapshotPb.set_system_image_build_id(buildId);

    dinfo("Saving with gfxstream=%d", HAS_GFXSTREAM);
    mSnapshotPb.set_gfxstream(HAS_GFXSTREAM);

    if (resizableEnabled()) {
        int resizeDisplay = getResizableActiveConfigId();
        mSnapshotPb.set_resizable_display(resizeDisplay);
    }
    return writeSnapshotToDisk();
}

bool Snapshot::saveFailure(FailureReason reason) {
    switch (reason) {
    case FailureReason::Empty:
    case FailureReason::NoSnapshotPb:
    case FailureReason::NoSnapshotInImage:
        mLatestFailureReason = reason;
        return true;  // Don't write this to disk

    default:
        if (reason == mLatestFailureReason) {
            return true;  // Nothing to do
        }
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
    const auto file = ScopedFd(
            path_open(PathUtils::join(mDataDir, kSnapshotProtobufName).c_str(),
                   O_RDONLY | O_BINARY | O_CLOEXEC, 0755));
    System::FileSize size;
    if (!System::get()->fileSize(file.get(), &size)) {
        saveFailure(FailureReason::NoSnapshotPb);
        return;
    }
    mSize = size;
    mFolderSize = System::get()->recursiveSize(mDataDir);

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
    if (!isVersionCompatible()) {
        saveFailure(FailureReason::IncompatibleVersion);
        return;
    }
    // Success
}

bool Snapshot::isVersionCompatible() const {
    if (mSnapshotPb.has_version()) {
        if (fc::isEnabled(fc::DownloadableSnapshot)) {
            return (mSnapshotPb.version() >> 10) == (kVersion >> 10);
        } else {
            return (mSnapshotPb.version() == kVersion);
        }
    } else {
        return false;
    }
}

// preload(): Obtain the protobuf metadata. Logic for deciding
// whether or not to load based on it, plus initialization
// of properties like skin rotation, is done afterward in load().
// Checks whether or not the version of the protobuf is compatible.
bool Snapshot::preload() {
    loadProtobufOnce();

    return isVersionCompatible();
}

const emulator_snapshot::Snapshot* Snapshot::getGeneralInfo() {
    loadProtobufOnce();

    if (isUnrecoverableReason(
                FailureReason(mSnapshotPb.failed_to_load_reason_code()))) {
        saveFailure(FailureReason(mSnapshotPb.failed_to_load_reason_code()));
    }

    return &mSnapshotPb;
}

const bool Snapshot::checkOfflineValid(bool writeFailure) {
    if (!preload()) {
        return false;
    }

    if (mSnapshotPb.images_size() > int(ARRAY_SIZE(kImages))) {
        if (writeFailure)
            saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    if (mVerifyQcow) {
        int matchedImages = 0;
        for (const auto& image : kImages) {
            ScopedCPtr<char> path(image.pathGetter(getConsoleAgents()->settings->avdInfo()));
            const auto type = image.type;
            const auto it = std::find_if(
                    mSnapshotPb.images().begin(), mSnapshotPb.images().end(),
                    [type](const proto::Image& im) {
                        return im.has_type() && im.type() == type;
                    });
            if (it != mSnapshotPb.images().end()) {
                if (!verifyImageInfo(image.type, path.get(), *it)) {
                    // If in build env, nuke the qcow2 as well.
                    if (avdInfo_inAndroidBuild(getConsoleAgents()->settings->avdInfo())) {
                        std::string qcow2Path(path.get());
                        qcow2Path += ".qcow2";
                        path_delete_file(qcow2Path.c_str());
                    }

                    if (writeFailure)
                        saveFailure(FailureReason::SystemImageChanged);
                    return false;
                }
                ++matchedImages;
            } else {
                // Should match an empty image info
                if (!verifyImageInfo(image.type, path.get(), proto::Image())) {
                    // If in build env, nuke the qcow2 as well.
                    if (avdInfo_inAndroidBuild(getConsoleAgents()->settings->avdInfo())) {
                        std::string qcow2Path(path.get());
                        qcow2Path += ".qcow2";
                        path_delete_file(qcow2Path.c_str());
                    }

                    if (writeFailure)
                        saveFailure(FailureReason::NoSnapshotInImage);
                    return false;
                }
            }
        }
        if (matchedImages != mSnapshotPb.images_size()) {
            if (writeFailure)
                saveFailure(FailureReason::ConfigMismatchAvd);
            return false;
        }
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read(false)) {
        if (writeFailure)
            saveFailure(FailureReason::CorruptedData);
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(getConsoleAgents()->settings->avdInfo()));
    if (!actualConfig.read(false)) {
        if (writeFailure)
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
        if (writeFailure)
            saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    return true;
}

const bool Snapshot::checkValid(bool writeFailure) {
    if (!preload()) {
        return false;
    }

    bool is_gfxstream_snapshot = mSnapshotPb.has_gfxstream() ? mSnapshotPb.gfxstream() : false;
    if (is_gfxstream_snapshot != HAS_GFXSTREAM) {
        dwarning("snapshot was created with gfxstream=%d, but this emulator has gfxstream=%d\n",
                is_gfxstream_snapshot, HAS_GFXSTREAM);
        if (writeFailure) {
            saveFailure(FailureReason::ConfigMismatchRenderer);
        }
        return false;
    }

    if (fc::isEnabled(fc::AllowSnapshotMigration)) {
        return true;
    }

    if (fc::isEnabled(fc::DownloadableSnapshot)) {
        return true;
    }

    if (mSnapshotPb.has_host() &&
        !verifyHost(mSnapshotPb.host(), writeFailure)) {
        return false;
    }

    if (mSnapshotPb.has_config() &&
        !verifyConfig(mSnapshotPb.config(), writeFailure)) {
        return false;
    }

    if (mSnapshotPb.images_size() > int(ARRAY_SIZE(kImages))) {
        if (writeFailure)
            saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    int matchedImages = 0;
    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(getConsoleAgents()->settings->avdInfo()));
        const auto type = image.type;
        const auto it = std::find_if(
                mSnapshotPb.images().begin(), mSnapshotPb.images().end(),
                [type](const proto::Image& im) {
                    return im.has_type() && im.type() == type;
                });
        if (it != mSnapshotPb.images().end()) {
            if (!verifyImageInfo(image.type, path.get(), *it)) {
                // If in build env, nuke the qcow2 as well.
                if (avdInfo_inAndroidBuild(getConsoleAgents()->settings->avdInfo())) {
                    std::string qcow2Path(path.get());
                    qcow2Path += ".qcow2";
                    path_delete_file(qcow2Path.c_str());
                }

                if (writeFailure)
                    saveFailure(FailureReason::SystemImageChanged);
                return false;
            }
            ++matchedImages;
        } else {
            // Should match an empty image info
            if (!verifyImageInfo(image.type, path.get(), proto::Image())) {
                // If in build env, nuke the qcow2 as well.
                if (avdInfo_inAndroidBuild(getConsoleAgents()->settings->avdInfo())) {
                    std::string qcow2Path(path.get());
                    qcow2Path += ".qcow2";
                    path_delete_file(qcow2Path.c_str());
                }

                if (writeFailure)
                    saveFailure(FailureReason::NoSnapshotInImage);
                return false;
            }
        }
    }
    if (matchedImages != mSnapshotPb.images_size()) {
        if (writeFailure)
            saveFailure(FailureReason::ConfigMismatchAvd);
        return false;
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read(false)) {
        if (writeFailure)
            saveFailure(FailureReason::CorruptedData);
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(getConsoleAgents()->settings->avdInfo()));
    if (!actualConfig.read(false)) {
        if (writeFailure)
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
        derror("The emulator hardware cannot load snapshot: %s", mName);
        if (writeFailure)
            saveFailure(FailureReason::ConfigMismatchAvd);
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
            saveFailure(
                    FailureReason(mSnapshotPb.failed_to_load_reason_code()));
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
        getConsoleAgents()->settings->set_guest_data_partition_mounted(
                mSnapshotPb.guest_data_partition_mounted());
    }

    if (mSnapshotPb.has_rotation() &&
        Snapshotter::get().windowAgent().getRotation() !=
                SkinRotation(mSnapshotPb.rotation())) {
        Snapshotter::get().windowAgent().rotate(
                SkinRotation(mSnapshotPb.rotation()));
    }
    if (resizableEnabled() && mSnapshotPb.has_resizable_display()) {
        Snapshotter::get().windowAgent().changeResizableDisplay(mSnapshotPb.resizable_display());
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

bool Snapshot::isImported() {
    return !getQcow2Files(dataDir().data()).empty();
}
static bool replace(std::string& src,
                    const std::string& what,
                    const std::string& replacement) {
    auto what_pos = src.find(what);
    if (what_pos != std::string::npos) {
        src.replace(what_pos, what.size(), replacement);
        return true;
    }
    return false;
}

static bool createEmptyQcow() {
    auto qemu_img = System::get()->findBundledExecutable("qemu-img");
    auto dst_img =
            android::base::pj(android::snapshot::getAvdDir(), "empty.qcow2");
    if (!System::get()->pathExists(dst_img)) {
        // TODO(jansene): replace with internal functions, vs calling qemu-img.
        // See qemu-img.c: static int img_create(int argc, char **argv)
        auto res = android::base::System::get()->runCommandWithResult(
                {qemu_img, "create", "-f", "qcow2", dst_img, "0"});
        if (!res) {
            LOG(ERROR) << "Could not create empty qcow2: " << dst_img;
            return false;
        }
    }
    return true;
}

bool Snapshot::fixImport() {
    // We do 2 things, we fix up hw.ini, and update the protobuf
    const std::string sdk = "android.sdk.root";
    const std::string home = "android.avd.home";
    const std::string curr_avd_dir = getAvdDir();
    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    IniFile hwIni(targetHwIni);
    hwIni.read();  // I wasted a lot of time discovering this...

    if (!hwIni.hasKey(sdk) || !hwIni.hasKey(home)) {
        LOG(ERROR) << "Snapshot import is missing android.sdk.root, "
                      "android.avd.home";
        return false;
    }
    auto old_sdk = hwIni.getString(sdk, "");
    auto old_home = hwIni.getString(home, "");
    std::string old_avd_dir =
            PathUtils::join(old_home, hwIni.getString("avd.name", "")) + ".avd";
    std::string curr_sdk = ConfigDirs::getSdkRootDirectory();
    std::string curr_home = ConfigDirs::getAvdRootDirectory();

    // Fixup hardware ini..
    for (auto& entry : hwIni) {
        auto val = hwIni.getString(entry, "");
        if (replace(val, old_sdk, curr_sdk)) {
            hwIni.setString(entry, val);
        }
        if (replace(val, old_home, curr_home)) {
            hwIni.setString(entry, val);
        }
        if (replace(val, old_avd_dir, curr_avd_dir)) {
            hwIni.setString(entry, val);
        }
    }
    // Fixup names & ids..
    hwIni.setString("avd.name", avdInfo_getName(getConsoleAgents()->settings->avdInfo()));
    hwIni.setString("avd.id", avdInfo_getId(getConsoleAgents()->settings->avdInfo()));

    // Fix up the protobuf paths..
    for (int i = 0; i < mSnapshotPb.images_size(); i++) {
        auto image = mSnapshotPb.mutable_images(i);
        std::string path = image->path();
        replace(path, old_sdk, curr_sdk);
        replace(path, old_home, curr_home);
        image->set_path(path);
    }

    // Reset stats
    mSnapshotPb.set_failed_to_load_reason_code(0);
    mSnapshotPb.set_invalid_loads(0);
    mSnapshotPb.set_successful_loads(0);

    if (!(writeSnapshotToDisk() && hwIni.writeIfChanged())) {
        // Bad news, unable to write out hardware ini , or protobuf.
        LOG(ERROR) << "Failed to write snapshot/hardware.ini, snapshot will "
                      "not work.";
        return false;
    }

    return createEmptyQcow();
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
    if (mSaveStats.empty())
        return false;

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
    return !mSuccessfulLoads || mInvalidLoads > 1;
}

base::Optional<FailureReason> Snapshot::failureReason() const {
    return mSnapshotPb.has_failed_to_load_reason_code()
                   ? base::makeOptional(FailureReason(
                             mSnapshotPb.failed_to_load_reason_code()))
                   : base::kNullopt;
}

}  // namespace snapshot
}  // namespace android

static int checkLoadable(android::snapshot::Snapshot& snapshot) {
    if (snapshot.checkValid(true)) {
        printf("Loadable\n");
        return 1;
    } else {
        printf("Not loadable\n");
        if (snapshot.failureReason().ptr() == nullptr) {
            printf("Reason: Unknown.\n");
        } else {
            printf("Reason: %s\n",
                   failureReasonToString(*snapshot.failureReason(),
                                         SNAPSHOT_LOAD));
        }
        return 0;
    }
}

extern "C" int android_check_snapshot_loadable(const char* snapshot_name) {
    using namespace android::snapshot;
    const char* const kExportedSnapshotExt[] = {".tar", ".tar.gz"};
    const char* format = nullptr;
    bool exportedSnapshot = false;
    size_t snapshotNameLength = strlen(snapshot_name);
    for (const char* ext : kExportedSnapshotExt) {
        size_t extLen = strlen(ext);
        if (snapshotNameLength > extLen &&
            0 == strcmp(snapshot_name + snapshotNameLength - extLen, ext)) {
            exportedSnapshot = true;
            format = ext;
            break;
        }
    }
    if (exportedSnapshot) {
        std::unique_ptr<std::istream> stream;
        if (0 == strcmp(format, ".tar.gz")) {
            stream.reset(new android::base::GzipInputStream(snapshot_name));
        } else {
            stream.reset(new std::ifstream(PathUtils::asUnicodePath(snapshot_name).c_str()));
            if (!static_cast<std::ifstream*>(stream.get())->is_open()) {
                printf("Not loadable\n");
                printf("Reason: File not exist: %s\n", snapshot_name);
                return 0;
            }
        }
        std::string temp_parent = System::get()->getTempDir();
        std::string temp_dir;
        // Create a random temp dir
        do {
            temp_dir = android::base::pj(temp_parent,
                                         ("snapshot" + std::to_string(rand())));
        } while (path_exists(temp_dir.c_str()) || path_is_dir(temp_dir.c_str()));
        path_android_mkdir(temp_dir.c_str(), 0777);
        android::base::TarReader tr(temp_dir, *stream, true);
        for (auto entry = tr.first(); tr.good(); entry = tr.next(entry)) {
            if (entry.name == "snapshot.pb" || entry.name == "hardware.ini") {
                tr.extract(entry);
            }
        }
        Snapshot snapshot("", temp_dir.c_str(), false);
        int ret = checkLoadable(snapshot);
        path_delete_dir(temp_dir.c_str());
        return ret;
    } else {
        printf("snapshot name %s\n", snapshot_name);
        android::base::Optional<Snapshot> snapshot =
                Snapshot::getSnapshotById(snapshot_name);
        if (snapshot.ptr() == nullptr) {
            printf("Not loadable\n");
            printf("Reason: Snapshot not found with name: %s\n", snapshot_name);
            return 0;
        } else {
            return checkLoadable(*snapshot);
        }
    }
}
