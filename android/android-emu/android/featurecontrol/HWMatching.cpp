#include "HWMatching.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/containers/Lookup.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/base/threads/ParallelTask.h"
#include "android/base/threads/Thread.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/utils/x86_cpuid.h"

#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <fstream>

#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG 1

#ifdef DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while(0) \

#else

#define D(...)

#endif

using android::base::LazyInstance;
using android::base::Looper;
using android::base::OsType;
using android::base::ParallelTask;
using android::base::System;

namespace android {
namespace featurecontrol {

static void queryFeaturePatternFn(bool* res);

static void queryFeaturePatternDoneFn(const bool& res) { }

static const int kQueryFeaturePatternCheckIntervalMs = 60;
static const int kQueryFeaturePatternTimeoutMs = 5000;

class FeaturePatternQueryThread : public android::base::Thread {
public:
    FeaturePatternQueryThread() { this->start(); }
    virtual intptr_t main() override {
        Looper* looper = android::base::ThreadLooper::get();
        android::base::runParallelTask<bool>
            (looper, &queryFeaturePatternFn, &queryFeaturePatternDoneFn,
             kQueryFeaturePatternCheckIntervalMs);
        looper->runWithTimeoutMs(kQueryFeaturePatternTimeoutMs);
        looper->forceQuit();
        return 0;
    }
};

static LazyInstance<FeaturePatternQueryThread> sFeaturePatternQueryThread =
    LAZY_INSTANCE_INIT;

static const char kCachedPatternsFilename[] = "emu-last-feature-patterns.json";

static void readAndApplyFromCachedPatterns();

void downloadAndApplyFeaturePatterns() {
    // Don't wait for this to finish, apply the patterns next run.
    sFeaturePatternQueryThread.ptr();
}

// Internal implementation//////////////////////////////////////////////////////

HostHwInfo queryHostHwInfo() {
    AndroidCpuInfoFlags cpuFlags =
        android::GetCpuInfo().first;

    std::string cpu_manufacturer_str = "Unknown";
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
    res.gpuinfolist = get_gpu_info();

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

    for (int i = 0; i < res.gpuinfolist->infos.size(); i++) {
        const GpuInfo& info = res.gpuinfolist->infos[i];
        D("gpu %d:\n"
          "    make %s\n"
          "    model %s\n"
          "    device_id %s\n"
          "    revision_id %s\n"
          "    version %s\n"
          "    renderer %s\n",
          i, info.make.c_str(), info.model.c_str(),
          info.device_id.c_str(), info.revision_id.c_str(),
          info.version.c_str(), info.renderer.c_str());
    }

    return res;
}

bool matchFeaturePattern(
        const HostHwInfo& hwinfo,
        const emulator_features::EmulatorFeaturePattern* pattern) {

    bool res = true;

    for (const auto& hwconfig : pattern->hwconfig()) {

        if (hwconfig.has_hostinfo()) {

            const emulator_features::EmulatorHost& hostinfo =
                hwconfig.hostinfo();

#define MATCH_FIELD(n) \
        if (hostinfo.has_##n()) res &= hwinfo.n == hostinfo.n(); \

            MATCH_FIELD(cpu_manufacturer)
            MATCH_FIELD(virt_support)
            MATCH_FIELD(running_in_vm)
            MATCH_FIELD(os_bit_count)
            MATCH_FIELD(cpu_model_name)
            MATCH_FIELD(os_platform)

        }

        // If the pattern contains GPU info, the system must
        // have GPU info that matches, or the pattern doesn't match.
        for (const auto& patterngpuinfo : hwconfig.hostgpuinfo()) {

            if (!hwinfo.gpuinfolist) {
                // If no gpu info list, fail match.
                res = false;
                break;
            }

            bool matchesPatternGpu = false;

            for (const auto& hostgpuinfo : hwinfo.gpuinfolist->infos) {

                bool thisHostGpuMatches = true;

#define MATCH_GPU_FIELD(n) \
    if (patterngpuinfo.has_##n()) \
        thisHostGpuMatches &= hostgpuinfo.n == patterngpuinfo.n(); \

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

    D("Num patterns: %d\n", input->pattern_size());

    for (const auto& pattern : input->pattern()) {

        bool thisHostMatches =
            matchFeaturePattern(hwinfo, &pattern);

        D("host match pattern? %d\n", thisHostMatches);

        if (!thisHostMatches) continue;

        for (const auto action : pattern.featureaction()) {
            FeatureAction nextAction;
            nextAction.name = action.featurename();
            nextAction.shouldDisable = action.shoulddisable();
            nextAction.shouldEnable = action.shouldenable();
            res.push_back(nextAction);
        }
    }

    return res;
}

static void doFeatureAction(const FeatureAction& action) {

}

static const char kFeaturePatternsUrl[] =
    "https://www.dropbox.com/s/x7uysz3zlyf8uy9/test-featurepatterns.txt?dl=1";

static size_t curlDownloadFeaturePatternsCallback(
        char* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    // corrupted contents, don't proceed.
    if (contents[total] != '\0')
        return total;

    auto& res = *static_cast<std::string*>(userp);
    res += contents;

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

void outputCachedFeaturePatterns(const std::string& text) {
    const std::string outputFileName =
        android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename);

    std::ofstream outFile(
            outputFileName,
            std::ios_base::out | std::ios_base::trunc);

    if (!outFile)
        fprintf(stderr, "%s: not valid file: %s\n", __func__, outputFileName.c_str());

    outFile << text;
}

static void readAndApplyFromCachedPatterns() {
    const std::string inputFileName =
        android::base::PathUtils::join(
                android::ConfigDirs::getUserDirectory(),
                kCachedPatternsFilename);

    std::ifstream in(inputFileName);
    google::protobuf::io::IstreamInputStream istream(&in);

    emulator_features::EmulatorFeaturePatterns patterns;
    if (!google::protobuf::TextFormat::Parse(&istream, &patterns)) {
        fprintf(stderr, "%s: failed to parse protobuf file\n", __func__);
    }
    
    std::vector<FeatureAction> todo =
        matchFeaturePatterns(queryHostHwInfo(), &patterns);

    for (const auto& elt : todo) {
        doFeatureAction(elt);
    }

    fprintf(stderr, "%s: call\n", __func__);
}

static void queryFeaturePatternFn(bool* res) {
    readAndApplyFromCachedPatterns();
    std::string featurePattern =
        downloadFeaturePatterns();
    outputCachedFeaturePatterns(featurePattern);
}


} // namespace hwinfo
} // namespace android
