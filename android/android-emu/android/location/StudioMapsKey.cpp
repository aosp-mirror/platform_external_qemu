// Copyright 2019 The Android Open Source Project
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

#include "StudioMapsKey.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"

#include <fstream>

#define DEBUG 0

#if DEBUG
#define D(fmt,...) do { \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \
} while(0)
#else
#define D(...)
#endif

using android::base::StringView;
using android::base::System;

namespace android {
namespace location {

namespace {
class StudioMapsKeyImpl : public StudioMapsKey {
public:
    StudioMapsKeyImpl(UpdateCallback cb, void* opaque);

private:
    void updateWorkerFn();
    static size_t curlDownloadCallback(
        char* contents, size_t size, size_t nmemb, void* userp);
    bool downloadMapsFile();

    android::base::FunctorThread mThread;
    std::string mMapsFile;
    std::string mKeyText;
    void* mOpaque;
    UpdateCallback mCallback;
    static constexpr char kMapsFileName[] = "maps.key";
    static constexpr char kMapsFileUrl[] =
        "https://dl.google.com/dl/android/studio/metadata/maps.key";
    static const uint64_t kMapsDownloadIntervalSec = 24 * 60 * 60;
    DISALLOW_COPY_AND_ASSIGN(StudioMapsKeyImpl);
};


// We can remove this once we upgrade to C++17.
// (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0386r2.pdf)
constexpr char StudioMapsKeyImpl::kMapsFileName[];
constexpr char StudioMapsKeyImpl::kMapsFileUrl[];

StudioMapsKeyImpl::StudioMapsKeyImpl(UpdateCallback cb, void* opaque) :
        mCallback(cb),
        mOpaque(opaque),
        mThread([this](){ updateWorkerFn(); }) {
    mMapsFile = android::base::PathUtils::join(
            android::ConfigDirs::getUserDirectory(),
            StudioMapsKeyImpl::kMapsFileName);
    mThread.start();
}

// static
size_t StudioMapsKeyImpl::curlDownloadCallback(
        char* contents, size_t size, size_t nmemb, void* userp) {

    size_t total = size * nmemb;

    auto& res = *static_cast<std::string*>(userp);
    res.append(contents, contents + total);

    return total;
}

bool StudioMapsKeyImpl::downloadMapsFile() {
    D("load: %s\n", kMapsFileUrl);

    char* curlError = nullptr;
    std::string res;
    if (!curl_download(
            kMapsFileUrl, nullptr,
            &curlDownloadCallback,
            &res, &curlError)) {
        D("failed to download maps file from server: %s.\n", curlError);
        free(curlError);
        return false;
    }

    D("got: %s", res.c_str());

    std::ofstream outFile(mMapsFile, std::ios_base::trunc);
    if (!outFile) {
        D("not valid file: %s\n", mMapsFile.c_str());
        return false;
    }
    outFile << res;
    return true;
}

void StudioMapsKeyImpl::updateWorkerFn() {
    bool hasFile = false;
    if (System::get()->pathExists(mMapsFile)) {
        hasFile = true;
        // Check the file creation time against the current time.
        if (const auto modTimeUs = System::get()->pathModificationTime(mMapsFile)) {
            auto modTimeSec = *modTimeUs / 1000000;
            uint64_t currTimeSec = System::get()->getUnixTimeUs() / 1000000;
            if (currTimeSec - modTimeSec < kMapsDownloadIntervalSec) {
                D("Not downloading maps file; last download time %lu current time %lu",
                  modTimeSec, currTimeSec);
                mCallback(mMapsFile, mOpaque);
                return;
            } else {
                D("Updating maps file. last_download=[%lu], curr_time=[%lu]", modTimeSec, currTimeSec);
            }
        } else {
            D("Updating maps file.");
        }
    }

    if (!downloadMapsFile() && !hasFile) {
        D("Could not download maps file");
        mMapsFile = "";
    }
    mCallback(mMapsFile, mOpaque);
}
}  // namespace

// static
StudioMapsKey* StudioMapsKey::create(UpdateCallback cb, void* opaque) {
    return new StudioMapsKeyImpl(cb, opaque);
}


} // namespace location
} // namespace android
