// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/update-check/UpdateChecker.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"
#include "android/base/Uri.h"
#include "android/curl-support.h"
#include "android/metrics/studio-config.h"
#include "android/metrics/StudioConfig.h"
#include "android/update-check/update_check.h"
#include "android/update-check/VersionExtractor.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/misc.h"
#include "android/version.h"

#include <fstream>
#include <iterator>
#include <new>
#include <string>

#include <errno.h>
#include <string.h>
#include <time.h>

using android::base::async;
using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::StringView;
using android::base::System;
using android::base::Uri;
using android::base::Version;
using android::update_check::UpdateChecker;

static const char kDataFileName[] = ".emu-update-last-check";
// TODO: kVersionUrl should not be fixed; XY in repository-XY.xml
//       might change with studio updates irrelevant to the emulator
static constexpr StringView kVersionUrl =
        "https://dl.google.com/android/repository/repository2-1.xml";

static const char kNewerVersionMessage[] =
        R"(Your emulator is out of date, please update by launching Android Studio:
 - Start Android Studio
 - Select menu "Tools > Android > SDK Manager"
 - Click "SDK Tools" tab
 - Check "Android SDK Tools" checkbox
 - Click "OK"
)";

void android_checkForUpdates(const char* homePath, const char* coreVersion) {
    std::unique_ptr<UpdateChecker> checker(
            new UpdateChecker(homePath, coreVersion));

    if (checker->init() && checker->needsCheck() && checker->runAsyncCheck()) {
        // checker will delete itself after the check in the worker thread
        checker.release();
    } else {
        VERBOSE_PRINT(updater, "UpdateChecker: skipped version check");
    }
}

namespace android {
namespace update_check {

static size_t curlWriteCallback(char* contents,
                                size_t size,
                                size_t nmemb,
                                void* userp) {
    auto& xml = *static_cast<std::string*>(userp);
    const size_t total = size * nmemb;

    xml.insert(xml.end(), contents, contents + total);

    return total;
}

class DataLoader final : public IDataLoader {
public:
    virtual std::string load(const char* version) override {
        std::string xml;
        std::string url = kVersionUrl;
        if (version) {
            const ScopedCPtr<char> id(android_studio_get_installation_id());
            url += Uri::FormatEncodeArguments(
                    "?tool=emulator&uid=%s&os=%s"
                    "&version=" EMULATOR_VERSION_STRING "&coreVersion=%s",
                    id.get(), toString(System::get()->getOsType()), version);
        }
        char* error = nullptr;
        if (!curl_download(url.c_str(), nullptr, &curlWriteCallback, &xml,
                           &error)) {
            dwarning("UpdateCheck: Failure: %s", error);
            ::free(error);
        }
        return xml;
    }
};

class TimeStorage final : public ITimeStorage {
public:
    TimeStorage(const char* configPath) : mFileLock() {
        mDataFileName = PathUtils::join(configPath, kDataFileName);
    }

    ~TimeStorage() {
        if (mFileLock) {
            filelock_release(mFileLock);
        }
    }

    bool lock() override {
        if (mFileLock) {
            dwarning("UpdateCheck: lock() called twice by the same process");
            return true;
        }

        mFileLock = filelock_create(mDataFileName.c_str());
        // if someone's already checking it - don't do it twice
        return mFileLock != NULL;
    }

    time_t getTime() override {
        std::ifstream dataFile(mDataFileName.c_str());
        if (!dataFile) {
            return time_t();  // no data file
        }
        int64_t intTime = 0;
        dataFile >> intTime;
        return static_cast<time_t>(intTime);
    }

    void setTime(time_t time) override {
        std::ofstream dataFile(mDataFileName.c_str());
        if (!dataFile) {
            dwarning("UpdateCheck: couldn't open data file for writing");
        } else {
            dataFile << static_cast<int64_t>(time) << '\n';
        }
    }

private:
    std::string mDataFileName;
    FileLock* mFileLock;
};

class NewerVersionReporter final : public INewerVersionReporter {
public:
    virtual void reportNewerVersion(
            const android::base::Version& /*existing*/,
            const android::base::Version& /*newer*/) override {
        printf("%s\n", kNewerVersionMessage);
    }
};

UpdateChecker::UpdateChecker(const char* configPath, const char* coreVersion)
    : mCoreVersion(coreVersion),
      mVersionExtractor(new VersionExtractor()),
      mDataLoader(new DataLoader()),
      mTimeStorage(new TimeStorage(configPath)),
      mReporter(new NewerVersionReporter()) {}

UpdateChecker::UpdateChecker(IVersionExtractor* extractor,
                             IDataLoader* loader,
                             ITimeStorage* storage,
                             INewerVersionReporter* reporter)
    : mCoreVersion(nullptr),
      mVersionExtractor(extractor),
      mDataLoader(loader),
      mTimeStorage(storage),
      mReporter(reporter) {}

bool UpdateChecker::init() {
    if (!mTimeStorage->lock()) {
        return false;
    }

    return true;
}

bool UpdateChecker::needsCheck() const {
    const time_t now = System::get()->getUnixTime();
    // Check only if the previous check was 4+ hours ago
    return now - mTimeStorage->getTime() >= 4 * 60 * 60;
}

bool UpdateChecker::runAsyncCheck() {
    return async([this] {
        asyncWorker();
        delete this;
    });
}

void UpdateChecker::asyncWorker() {
    Version current = mVersionExtractor->getCurrentVersion();
    const auto last = getLatestVersion();

    if (!last) {
        // don't record the last check time if we were not able to retrieve
        // the last version - next time we may be more lucky
        dwarning(
                "UpdateCheck: failed to get the latest version, skipping "
                "check (current version '%s')",
                current.toString().c_str());
        return;
    }

    VERBOSE_PRINT(updater,
                  "UpdateCheck: current version '%s', last version '%s'",
                  current.toString().c_str(), last->first.toString().c_str());

    if (current < last->first) {
        mReporter->reportNewerVersion(current, last->first);
    }

    // Update the last version check time
    mTimeStorage->setTime(System::get()->getUnixTime());
}

Optional<UpdateChecker::VersionInfo> UpdateChecker::getLatestVersion() {
    const auto repositoryXml = mDataLoader->load(mCoreVersion);
    const auto versions = mVersionExtractor->extractVersions(repositoryXml);
    if (versions.empty()) {
        return {};
    }

    const auto updateChannel = android::studio::updateChannel();

    // now find the first channel which is equal to or lower than the selected
    // update channel
    const auto greaterIt = versions.upper_bound(updateChannel);
    if (greaterIt == versions.begin()) {
        // even the first update channel in the list is greater than the
        // one from Android Studio settings
        return {};
    }

    return std::make_pair(std::prev(greaterIt)->second,
                          std::prev(greaterIt)->first);
}

}  // namespace update_check
}  // namespace android
