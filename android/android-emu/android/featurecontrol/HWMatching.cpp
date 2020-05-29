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

#include "HWMatching.h"

#include "android/version.h"
#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/Version.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/utils/filelock.h"

#include "emulator_feature_patterns.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <fstream>

#define DEBUG 0

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while(0) \

#else

#define D(...)

#endif

using android::base::LazyInstance;
using android::base::OsType;
using android::base::StringView;
using android::base::System;
using android::base::Version;

namespace android {
namespace featurecontrol {

static void queryFeaturePatternFn();

static const int kQueryFeaturePatternTimeoutMs = 5000;

class FeaturePatternQueryThread {
public:
    FeaturePatternQueryThread() :
        mThread([]() { queryFeaturePatternFn(); }) {
        mThread.start();
    }
private:
    android::base::FunctorThread mThread;
    DISALLOW_COPY_AND_ASSIGN(FeaturePatternQueryThread);
};

static LazyInstance<FeaturePatternQueryThread> sFeaturePatternQueryThread =
    LAZY_INSTANCE_INIT;

static const char kCachedPatternsFilename[] =
    "emu-last-feature-flags.protobuf";

static const char kBackupPatternsFilename[] =
    "emu-original-feature-flags.protobuf";

static bool tryParseFeaturePatternsProtobuf(
        const std::string& filename,
        emulator_features::EmulatorFeaturePatterns* out_patterns) {
    std::ifstream in(filename, std::ios::binary);

    if (out_patterns->ParseFromIstream(&in)) {
        D("successfully parsed as binary.");
        return true;
    }

    D("failed to parse protobuf file in binary mode, retrying in text mode");
    in.close();

    auto asText = android::readFileIntoString(filename);

    if (!asText) {
        D("could not read file into string, failed");
        return false;
    }

    if (!google::protobuf::TextFormat::ParseFromString(*asText, out_patterns)) {
        D("could not parse in text mode, failed");
        return false;
    }

    D("successfully parsed as text.");

    return true;
}

// Scoped accessor to help read the server-side feature patterns
// in an organized way, especially when other emulator instances
// are involved.
// Concurrent access may occur on |kCachedPatternsFilename| on
// multiple emulator instances.
// Current policy is to pretend the file is not there for the 2nd and later
// accessors of the file, if concurrent access happens.
class PatternsFileAccessor {
public:
    PatternsFileAccessor() :
        mFilename(
            android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename)),
        mOriginalFileName(
            android::base::PathUtils::join(
               System::get()->getLauncherDirectory(), "lib",
               kBackupPatternsFilename)) { }

    ~PatternsFileAccessor() {
        if (mFileLock)
            filelock_release(mFileLock);
    }

    bool read(emulator_features::EmulatorFeaturePatterns* patterns) {
        if (!acquire()) return false;

        if (tryParseFeaturePatternsProtobuf(mFilename, patterns)) {
            D("Found downloaded feature flags\n");
            return true;
        }

        D("Could not find downloaded feature flags, trying origin %s\n", mOriginalFileName.c_str());
        return tryParseFeaturePatternsProtobuf(mOriginalFileName, patterns);
    }

    bool write(emulator_features::EmulatorFeaturePatterns& patterns) {
        if (!acquire()) return false;

        {
            std::ofstream outFile(
                    mFilename,
                    std::ios_base::binary | std::ios_base::trunc);

            if (!outFile) {
                D("not valid file: %s\n", mFilename.c_str());
                return false;
            }

            patterns.set_last_download_time(System::get()->getUnixTime());
            patterns.SerializeToOstream(&outFile);
        }

        // Check if we wrote a parseable protobuf, if not, delete immediately.
        emulator_features::EmulatorFeaturePatterns test_pattern;
        if (!tryParseFeaturePatternsProtobuf(mFilename, &test_pattern)) {
            D("we have invalid protobuf, delete it");
            System::get()->deleteFile(mFilename);
            return false;
        }

        return true;
    }
private:

    bool acquire() {
        if (mFileLock) {
            D("acquire() called twice by same process.");
            return false;
        }

        mFileLock = filelock_create(mFilename.c_str());

        if (!mFileLock) {
            D("another emulator process has lock or path is RO");
            return false;
        }

        D("successful acquire");
        return true;
    }

    std::string mFilename = {};
    std::string mOriginalFileName = {};
    FileLock* mFileLock = nullptr;

    DISALLOW_COPY_AND_ASSIGN(PatternsFileAccessor);
};

void asyncUpdateServerFeaturePatterns() {
    sFeaturePatternQueryThread.ptr();
}

bool matchFeaturePattern(
        const HostHwInfo::Info& hwinfo,
        const emulator_features::EmulatorFeaturePattern* pattern) {

    bool res = false;

    for (const auto& patternHwConfig : pattern->hwconfig()) {

        bool thisConfigMatches = true;

        if (patternHwConfig.has_hostinfo()) {

#define MATCH_FIELD(n) \
        if (patternHwConfig.hostinfo().has_##n()) \
            thisConfigMatches &= hwinfo.n == patternHwConfig.hostinfo().n(); \

            MATCH_FIELD(cpu_manufacturer)
            MATCH_FIELD(virt_support)
            MATCH_FIELD(running_in_vm)
            MATCH_FIELD(os_bit_count)
            MATCH_FIELD(cpu_model_name)
            MATCH_FIELD(os_platform)

        }

        // If the pattern contains GPU info, the system must
        // have GPU info that matches, or the pattern doesn't match.
        for (const auto& patternGpuInfo : patternHwConfig.hostgpuinfo()) {

            if (!hwinfo.gpuinfolist) {
                // If no gpu info list, fail match.
                thisConfigMatches = false;
                break;
            }

            bool matchesPatternGpu = false;

            for (const auto& hostgpuinfo : hwinfo.gpuinfolist->infos) {

                bool thisHostGpuMatches = true;

#define MATCH_GPU_FIELD(n) \
    if (patternGpuInfo.has_##n()) \
        thisHostGpuMatches &= hostgpuinfo.n == patternGpuInfo.n(); \

                MATCH_GPU_FIELD(make)
                MATCH_GPU_FIELD(model)
                MATCH_GPU_FIELD(device_id)
                MATCH_GPU_FIELD(revision_id)
                MATCH_GPU_FIELD(version)
                MATCH_GPU_FIELD(renderer)

                matchesPatternGpu |= thisHostGpuMatches;
            }

            thisConfigMatches &= matchesPatternGpu;
        }

        res |= thisConfigMatches;
    }

    return res;
}

template <unsigned int defaultVal>
static Version versionFromProto(const emulator_features::EmulatorVersion& ver) {
    return Version(
            ver.has_major() ? ver.major() : defaultVal,
            (ver.has_major() && ver.has_minor()) ? ver.minor() : defaultVal,
            (ver.has_major() && ver.has_minor() && ver.has_patch())
                    ? ver.patch()
                    : defaultVal);
}

static bool featureActionAppliesToEmulatorVersion(
        const emulator_features::FeatureFlagAction& action) {
    Version currentVersion(EMULATOR_VERSION_STRING_SHORT);
    // if invalid version, then all min-version constrained features
    // apply and all max-version constrained features don't apply.
    if (!currentVersion.isValid()) {
        return !action.has_max_version();
    }

    const Version minVersion =
            action.has_min_version() ? versionFromProto<0>(action.min_version())
                                     : Version(0, 0, 0);
    if (currentVersion < minVersion) {
        return false;
    }
    const Version maxVersion =
            action.has_max_version()
                    ? versionFromProto<Version::kNone>(action.max_version())
                    : Version::invalid();
    if (currentVersion > maxVersion) {
        return false;
    }
    return true;
}

std::vector<FeatureAction> matchFeaturePatterns(
        const HostHwInfo::Info& hwinfo,
        const emulator_features::EmulatorFeaturePatterns* input) {
    std::vector<FeatureAction> res;

    D("Num patterns: %d", input->pattern_size());

    for (const auto& pattern : input->pattern()) {

        bool thisHostMatches =
            matchFeaturePattern(hwinfo, &pattern);

        D("host match pattern? %d", thisHostMatches);

        if (!thisHostMatches) continue;

        for (const auto action : pattern.featureaction()) {

            if (!action.has_feature() || !action.has_enable()) {
                D("Not enough info about this feature. Skip");
                continue;
            }
            if (!featureActionAppliesToEmulatorVersion(action)) {
                D("Feature %s doesn't apply to this emulator version.",
                  emulator_features::Feature_Name(action.feature()).c_str());
                continue;
            }

            FeatureAction nextAction;
            nextAction.name = emulator_features::Feature_Name(action.feature());
            nextAction.enable = action.enable();
            res.push_back(nextAction);
        }
    }

    return res;
}

static void doFeatureAction(const FeatureAction& action) {

    Feature feature = stringToFeature(action.name);

    if (stringToFeature(action.name) == Feature::Feature_n_items) {
        D("Unknown feature action.");
        return;
    }

    // Conservatively:
    // If the server wants to enable: enable if guest didn't disable
    // If the server wants to disable: disable if user did not override
    if (action.enable) {
        setIfNotOverridenOrGuestDisabled(feature, action.enable);
        D("server has tried to enable %s", action.name.c_str());
    } else {
        setIfNotOverriden(feature, action.enable);
        D("server has tried to disable %s", action.name.c_str());
    }
}

static const char kFeaturePatternsUrl[] =
    "https://dl.google.com/dl/android/studio/metadata/emulator-feature-flags.protobuf.bin";
static const char kFeaturePatternsUrlText[] =
    "https://dl.google.com/dl/android/studio/metadata/emulator-feature-flags.protobuf";

static size_t curlDownloadFeaturePatternsCallback(
        char* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    auto& res = *static_cast<std::string*>(userp);
    res.append(contents, contents + total);

    return total;
}

bool downloadFeaturePatternsText(emulator_features::EmulatorFeaturePatterns* patternsOut) {
    D("load: %s\n", kFeaturePatternsUrlText);

    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kFeaturePatternsUrlText, nullptr,
            &curlDownloadFeaturePatternsCallback,
            &res, &curlError)) {
        D("failed to download feature flags from server: %s.\n", curlError);
        free(curlError);
        return false;
    }

    D("got: %s", res.c_str());

    if (!google::protobuf::TextFormat::ParseFromString(res, patternsOut)) {
        D("failed to parse text feature flags\n");
        return false;
    }
    return true;
}

bool downloadFeaturePatternsBinary(emulator_features::EmulatorFeaturePatterns* patternsOut) {
    D("load: %s\n", kFeaturePatternsUrl);

    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kFeaturePatternsUrl, nullptr,
            &curlDownloadFeaturePatternsCallback,
            &res, &curlError)) {
        D("failed to download feature flags from server: %s.\n", curlError);
        free(curlError);
        return false;
    }

    D("got: %s", res.c_str());

    if (!patternsOut->ParseFromString(res)) {
        D("failed to parse binary feature flags\n");
    }

    return true;
}

static void outputCachedFeaturePatterns(
        emulator_features::EmulatorFeaturePatterns& patterns) {
    PatternsFileAccessor access;

    access.write(patterns);
}

static LazyInstance<emulator_features::EmulatorFeaturePatterns> sCachedFeaturePatterns =
    LAZY_INSTANCE_INIT;

void applyCachedServerFeaturePatterns() {
    PatternsFileAccessor access;

    if (!access.read(sCachedFeaturePatterns.ptr()))
        return;

    std::vector<FeatureAction> todo =
        matchFeaturePatterns(HostHwInfo::query(), sCachedFeaturePatterns.ptr());

    for (const auto& elt : todo) {
        doFeatureAction(elt);
    }
}

static const uint32_t kFeaturePatternDownloadIntervalSeconds = 24 * 60 * 60;

static void queryFeaturePatternFn() {

    uint32_t currTime = System::get()->getUnixTime();

    if (currTime - sCachedFeaturePatterns->last_download_time() <
        kFeaturePatternDownloadIntervalSeconds) {
        D("Not downloading; last download time %u current time %u",
          sCachedFeaturePatterns->last_download_time(),
          currTime);
        return;
    } else {
        D("Downloading new feature patterns.");
    }

    emulator_features::EmulatorFeaturePatterns patterns;
    if (!downloadFeaturePatternsBinary(&patterns)) {
        D("Could not find binary protobuf, trying text.");
        if (!downloadFeaturePatternsText(&patterns)) {
            D("Downloaded protobuf file corrupt.");
            return;
        }
    }

    outputCachedFeaturePatterns(patterns);
}


} // namespace hwinfo
} // namespace android
