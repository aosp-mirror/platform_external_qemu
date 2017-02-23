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

#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/utils/x86_cpuid.h"

#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
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

static const char kCachedPatternsFilename[] = "emu-last-feature-patterns.protobuf";

void asyncUpdateServerFeaturePatterns() {
    sFeaturePatternQueryThread.ptr();
}

// Internal implementation//////////////////////////////////////////////////////

HostHwInfo queryHostHwInfo() {
    AndroidCpuInfoFlags cpuFlags =
        android::GetCpuInfo().first;

    StringView cpu_manufacturer_str = "Unknown";
    if (cpuFlags | ANDROID_CPU_INFO_AMD)
        cpu_manufacturer_str = "AMD";
    if (cpuFlags | ANDROID_CPU_INFO_INTEL)
        cpu_manufacturer_str = "Intel";

    uint32_t model_family_stepping;
    android_get_x86_cpuid(1, 0, &model_family_stepping,
                          nullptr, nullptr, nullptr);

    HostHwInfo res;
    res.cpu_manufacturer = cpu_manufacturer_str;
    res.virt_support = cpuFlags & ANDROID_CPU_INFO_VIRT_SUPPORTED;
    res.running_in_vm = cpuFlags & ANDROID_CPU_INFO_VM;
    res.os_bit_count = System::get()->getHostBitness();
    res.cpu_model_name = model_family_stepping;
    res.os_platform = android::base::toString(System::get()->getOsType());
    res.gpuinfolist = &globalGpuInfoList();

    D("cpu info:\n"
      "    manufacturer %s\n"
      "    virt support %d invm %d\n"
      "    osbitcount %d\n"
      "    cpumodel 0x%x\n"
      "    osplatform %s",
      res.cpu_manufacturer.c_str(),
      res.virt_support,
      res.running_in_vm,
      res.os_bit_count,
      res.cpu_model_name,
      res.os_platform.c_str());

    for (size_t i = 0; i < res.gpuinfolist->infos.size(); i++) {
        const GpuInfo& info = res.gpuinfolist->infos[i];
        D("gpu %zu:\n"
          "    make %s\n"
          "    model %s\n"
          "    device_id %s\n"
          "    revision_id %s\n"
          "    version %s\n"
          "    renderer %s\n",
          i, info.make.c_str(), info.model.c_str(),
          info.device_id.c_str(), info.revision_id.c_str(),
          info.version.c_str(), info.renderer.c_str());
        (void)info;
    }

    return res;
}

bool matchFeaturePattern(
        const HostHwInfo& hwinfo,
        const emulator_features::EmulatorFeaturePattern* pattern) {

    bool res = true;

    for (const auto& patternHwConfig : pattern->hwconfig()) {

        if (patternHwConfig.has_hostinfo()) {

#define MATCH_FIELD(n) \
        if (patternHwConfig.hostinfo().has_##n()) \
            res &= hwinfo.n == patternHwConfig.hostinfo().n(); \

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
                res = false;
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

            res &= matchesPatternGpu;
        }
    }

    return res;
}

std::vector<FeatureAction> matchFeaturePatterns(
        const HostHwInfo& hwinfo,
        const emulator_features::EmulatorFeaturePatterns* input) {
    std::vector<FeatureAction> res;

    D("Num patterns: %d", input->pattern_size());

    for (const auto& pattern : input->pattern()) {

        bool thisHostMatches =
            matchFeaturePattern(hwinfo, &pattern);

        D("host match pattern? %d", thisHostMatches);

        if (!thisHostMatches) continue;

        for (const auto action : pattern.featureaction()) {

            if (!action.has_feature() ||
                !action.has_enable()) continue;

            FeatureAction nextAction;
            nextAction.name = emulator_features::FeatureFlagAction_Feature_Name(action.feature());
            nextAction.enable = action.enable();
            res.push_back(nextAction);
        }
    }

    return res;
}

static void doFeatureAction(const FeatureAction& action) {

    Feature feature = stringToFeature(action.name);

    if (stringToFeature(action.name) == Feature::Feature_n_items) return;

    setIfNotOverriden(feature, action.enable);

    if (action.enable) {
        D("server has enabled %s", action.name.c_str());
    } else {
        D("server has disabled %s", action.name.c_str());
    }

}

static const char kFeaturePatternsUrl[] =
    "https://www.dropbox.com/s/x7uysz3zlyf8uy9/test-featurepatterns.txt?dl=1";

static size_t curlDownloadFeaturePatternsCallback(
        char* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    auto& res = *static_cast<std::string*>(userp);
    res.append(contents, contents + total);

    return total;
}

std::string downloadFeaturePatterns() {
    D("load: %s\n", kFeaturePatternsUrl);

    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kFeaturePatternsUrl, nullptr,
            &curlDownloadFeaturePatternsCallback,
            &res, &curlError)) {
        return "Error: failed to download feature patterns from server.";
    }

    D("got: %s", res.c_str());
    return res;
}

static void outputCachedFeaturePatterns(
        emulator_features::EmulatorFeaturePatterns& patterns) {
    const std::string outputFileName =
        android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename);

    std::ofstream outFile(
            outputFileName,
            std::ios_base::out | std::ios_base::trunc);

    if (!outFile)
        D("not valid file: %s\n", outputFileName.c_str());

    patterns.set_last_download_time(System::get()->getUnixTime());
    google::protobuf::io::OstreamOutputStream ostream(&outFile);
    google::protobuf::TextFormat::Print(patterns, &ostream);

    outFile << patterns.DebugString();
}

static LazyInstance<emulator_features::EmulatorFeaturePatterns> sCachedFeaturePatterns =
    LAZY_INSTANCE_INIT;

void applyCachedServerFeaturePatterns() {
    const std::string inputFileName =
        android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename);

    std::ifstream in(inputFileName);
    google::protobuf::io::IstreamInputStream istream(&in);

    if (!google::protobuf::TextFormat::Parse(
                &istream, sCachedFeaturePatterns.ptr())) {
        D("failed to parse protobuf file");
        return;
    }

    std::vector<FeatureAction> todo =
        matchFeaturePatterns(queryHostHwInfo(), sCachedFeaturePatterns.ptr());

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

    std::string featurePattern =
        downloadFeaturePatterns();
    emulator_features::EmulatorFeaturePatterns patterns;
    if (!google::protobuf::TextFormat::ParseFromString(
                featurePattern, &patterns)) {
        D("Downloaded protobuf file corrupt.");
        return;
    }
    outputCachedFeaturePatterns(patterns);
}


} // namespace hwinfo
} // namespace android
