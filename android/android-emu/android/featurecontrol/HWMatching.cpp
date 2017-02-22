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
#include "android/opengl/gpuinfo.h"
#include "android/utils/x86_cpuid.h"

#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <fstream>

#include <sys/stat.h>
#include <fcntl.h>

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
    readAndApplyFromCachedPatterns();
    // Don't wait for this to finish, apply the patterns next run.
    sFeaturePatternQueryThread.ptr();
}

// Internal implementation//////////////////////////////////////////////////////

// CPU related//////////////////////////////////////////////////////////////////

static AndroidCpuInfoFlags sCpuInfoFlags = (AndroidCpuInfoFlags)0;
static void getCpuInfo() {
    if (!sCpuInfoFlags)
        android::GetCpuInfo().first;
}

static std::string getCpuManufacturer() {
    getCpuInfo();
    if (sCpuInfoFlags | ANDROID_CPU_INFO_AMD)
        return "AMD";
    if (sCpuInfoFlags | ANDROID_CPU_INFO_INTEL)
        return "Intel";
    return "Unknown";
}

static bool getCpuVirtSupported() {
    getCpuInfo();
    return sCpuInfoFlags | ANDROID_CPU_INFO_VIRT_SUPPORTED;
}

static bool getCpuInsideVM() {
    getCpuInfo();
    return sCpuInfoFlags | ANDROID_CPU_INFO_VM;;
}

static int getOsBitness() {
    return System::get()->getHostBitness();
}

static int getCpuModelFamilyStepping() {
    uint32_t eax;
    android_get_x86_cpuid(1, 0, &eax, nullptr, nullptr, nullptr);
    fprintf(stderr, "%s: mfs 0x%x\n", __func__, eax);
    return eax;
}

static std::string getOsPlatform() {
    return android::base::toString(System::get()->getOsType());
}

// GPU related//////////////////////////////////////////////////////////////////

// Matching and querying////////////////////////////////////////////////////////

static std::vector<FeatureAction> matchHostHWAgainstFeaturePatterns(
        const emulator_features::EmulatorFeaturePatterns& input) {
    std::vector<FeatureAction> res;

    fprintf(stderr, "%s: patterns: %d\n", __func__, input.pattern_size());

    for (const auto& pattern : input.pattern()) {
        fprintf(stderr, "%s: found pattern\n", __func__);
        bool thisHostMatches = true;
        for (const auto& hwconfig : pattern.hwconfig()) {

            const emulator_features::EmulatorHost& hostinfo =
                hwconfig.hostinfo();

            if (hostinfo.has_cpu_manufacturer())
                thisHostMatches &=
                    getCpuManufacturer() == hostinfo.cpu_manufacturer();

            if (hostinfo.has_virt_support())
                thisHostMatches &=
                    getCpuVirtSupported() == hostinfo.virt_support();

            if (hostinfo.has_running_in_vm())
                thisHostMatches &=
                    getCpuInsideVM() == hostinfo.running_in_vm();

            if (hostinfo.has_os_bit_count())
                thisHostMatches &=
                    getOsBitness() == hostinfo.os_bit_count();

            if (hostinfo.has_cpu_model_name())
                thisHostMatches &=
                    getCpuModelFamilyStepping() == hostinfo.cpu_model_name();

            if (hostinfo.has_os_platform())
                thisHostMatches &=
                    getOsPlatform() == hostinfo.os_platform();

            for (const auto& hostgpuinfo : get_gpu_info()->infos) {
                bool matchesPatternGpu = false;
                for (const auto& patterngpuinfo : hwconfig.hostgpuinfo()) {
                    if (patterngpuinfo.has_make())
                        matchesPatternGpu |=
                            hostgpuinfo.make == patterngpuinfo.make();
                    if (patterngpuinfo.has_model())
                        matchesPatternGpu |=
                            hostgpuinfo.model == patterngpuinfo.model();
                    if (patterngpuinfo.has_device_id())
                        matchesPatternGpu |=
                            hostgpuinfo.device_id == patterngpuinfo.device_id();
                    if (patterngpuinfo.has_revision_id())
                        matchesPatternGpu |=
                            hostgpuinfo.revision_id == patterngpuinfo.revision_id();
                    if (patterngpuinfo.has_version())
                        matchesPatternGpu |=
                            hostgpuinfo.version == patterngpuinfo.version();
                    if (patterngpuinfo.has_renderer())
                        matchesPatternGpu |=
                            hostgpuinfo.renderer == patterngpuinfo.renderer();
                }
                thisHostMatches &= matchesPatternGpu;
            }
        }

        fprintf(stderr, "%s: host match this pattern? %d\n", __func__, thisHostMatches);

        if (!thisHostMatches) continue;

        for (const auto action : pattern.featureaction()) {
            fprintf(stderr, "%s: found featureaction\n", __func__);
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
    fprintf(stderr, "%s: call. url %s\n", __func__, kFeaturePatternsUrl);
    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kFeaturePatternsUrl, nullptr,
            &curlDownloadFeaturePatternsCallback,
            &res, &curlError)) {
        return "Error: failed to download feature patterns from server.";
    }

    fprintf(stderr, "%s: got: %s\n", __func__, res.c_str());
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

static void queryFeaturePatternFn(bool* res) {
    std::string featurePattern =
        downloadFeaturePatterns();
    outputCachedFeaturePatterns(featurePattern);
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
        matchHostHWAgainstFeaturePatterns(patterns);

    for (const auto& elt : todo) {
        doFeatureAction(elt);
    }

    fprintf(stderr, "%s: call\n", __func__);
}


} // namespace hwinfo
} // namespace android
