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

#include "android/base/files/IniFile.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/snapshot/PathUtils.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"

using android::base::endsWith;
using android::base::IniFile;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;

static std::string key(StringView type, StringView param) {
    return std::string(type) + "." + std::string(param);
}

static void putImageInfo(StringView type, StringView path, IniFile* out) {
    out->setString(key(type, "path"), path);
    if (path.empty() || !path_is_regular(path.c_str())) {
        out->setBool(key(type, "present"), false);
        return;
    }

    out->setBool(key(type, "present"), true);
    System::FileSize size;
    if (System::get()->pathFileSize(path, &size)) {
        out->setInt64(key(type, "size"), int64_t(size));
    }
    struct stat st;
    if (!android_stat(path.c_str(), &st)) {
        out->setInt64(key(type, "modified"), st.st_mtime);
    }
}

static bool verifyImageInfo(StringView type,
                            StringView path,
                            const IniFile& in) {
    const bool expectPresent = in.getBool(key(type, "present"), false);
    const bool realPresent = !path.empty() && path_is_regular(path.c_str());
    if (expectPresent != realPresent) {
        return false;
    }

    const System::FileSize expectedSize =
            in.getDiskSize(key(type, "size"), IniFile::DiskSize(-1));
    System::FileSize realSize;
    if (System::get()->pathFileSize(path, &realSize)) {
        if (expectedSize != realSize) {
            return false;
        }
    } else {
        if (expectedSize != System::FileSize(-1)) {
            return false;
        }
    }

    const int64_t expectedModified = in.getInt64(key(type, "modified"), -1);
    struct stat st;
    if (!android_stat(path.c_str(), &st)) {
        if (st.st_mtime != expectedModified) {
            return false;
        }
    } else {
        if (expectedModified != -1) {
            return false;
        }
    }

    return true;
}

bool areConfigsEqual(const IniFile& expected, const IniFile& actual) {
    if (expected.size() != actual.size()) {
        return false;
    }
    for (auto&& key : expected) {
        if (endsWith(key, ".path") || endsWith(key, ".initPath")) {
            continue;   // these contain absolute paths and will be checked
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

struct {
    const char* type;
    char* (*pathGetter)(const AvdInfo* info);
} static constexpr kImages[] = {
        {"kernel", avdInfo_getKernelPath},
        {"kernel_ranchu", avdInfo_getRanchuKernelPath},
        {"system", avdInfo_getSystemInitImagePath},
        {"system_copy", avdInfo_getSystemImagePath},
        {"data", avdInfo_getDataInitImagePath},
        {"data_copy", avdInfo_getDataImagePath},
        {"ramdisk", avdInfo_getRamdiskPath},
        {"sdcard", avdInfo_getSdCardPath},
        {"cache", avdInfo_getCachePath},
        {"vendor", avdInfo_getVendorImagePath},
        {"encryption_key", avdInfo_getEncryptionKeyImagePath},
};

static constexpr int kVersion = 1;

namespace android {
namespace snapshot {

Snapshot::Snapshot(const char* name)
    : mName(name), mDataDir(getSnapshotDir(name)) {}

bool Snapshot::save() {
    auto targetHwIni = PathUtils::join(mDataDir, "hardware.ini");
    if (path_copy_file(targetHwIni.c_str(),
                       avdInfo_getCoreHwIniPath(android_avdInfo)) != 0) {
        return false;
    }

    IniFile rootIni(PathUtils::join(mDataDir, "root.ini"));
    rootIni.setInt("version", 1);
    rootIni.setInt64("creation_time", System::get()->getUnixTime());

    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        putImageInfo(image.type, path.get(), &rootIni);
    }

    if (!rootIni.write()) {
        return false;
    }
    return true;
}

bool Snapshot::load() {
    IniFile rootIni(PathUtils::join(mDataDir, "root.ini"));
    if (!rootIni.read()) {
        return false;
    }
    if (rootIni.getInt("version", -1) != kVersion) {
        return false;
    }
    for (const auto& image : kImages) {
        ScopedCPtr<char> path(image.pathGetter(android_avdInfo));
        if (!verifyImageInfo(image.type, path.get(), rootIni)) {
            return false;
        }
    }

    IniFile expectedConfig(PathUtils::join(mDataDir, "hardware.ini"));
    if (!expectedConfig.read()) {
        return false;
    }
    IniFile actualConfig(avdInfo_getCoreHwIniPath(android_avdInfo));
    if (!actualConfig.read()) {
        return false;
    }
    if (!areConfigsEqual(expectedConfig, actualConfig)) {
        return false;
    }

    return true;
}

}  // namespace snapshot
}  // namespace android
