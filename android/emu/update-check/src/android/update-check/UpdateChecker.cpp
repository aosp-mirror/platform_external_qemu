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

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/ScopedPtr.h"

#include "aemu/base/Uri.h"
#include "aemu/base/threads/Async.h"
#include "android/base/files/IniFile.h"
#include "android/base/system/System.h"
#include "android/console.h"
#include "android/curl-support.h"
#include "android/emulation/ConfigDirs.h"
#include "android/metrics/StudioConfig.h"
#include "android/update-check/VersionExtractor.h"
#include "android/update-check/update_check.h"
#include "android/utils/debug.h"
#include "android/utils/filelock.h"
#include "android/utils/misc.h"
#include "android/version.h"

#include <fstream>
#include <iterator>
#include <new>
#include <string>
#include <string_view>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

using android::base::async;
using android::base::Optional;
using android::base::PathUtils;
using android::base::ScopedCPtr;
using android::base::System;
using android::base::Uri;
using android::base::Version;
using android::update_check::UpdateChecker;

static const char kDataFileName[] = "emu-update-last-check.ini";
// TODO: kVersionUrl should not be fixed; XY in repository-XY.xml
//       might change with studio updates irrelevant to the emulator
static constexpr std::string_view kVersionUrl =
        "https://dl.google.com/android/repository/repository2-3.xml";

static const char kNewerVersionMessage[] =
        R"(Your emulator is out of date, please update by launching Android Studio:
 - Start Android Studio
 - Select menu "Tools > Android > SDK Manager"
 - Click "SDK Tools" tab
 - Check "Android Emulator" checkbox
 - Click "OK"
)";

void android_checkForUpdates(const char* coreVersion) {
    auto checker =
            std::make_unique<android::update_check::SystemClockUpdateChecker>(
                    coreVersion);

    if (checker->runAsyncCheck()) {
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

static std::string_view toOsArchString(int bitness) {
    switch (bitness) {
        case 32:
            return "x86";
        case 64:
            return "x86_64";
        default:
            assert(false);
            return "unknown";
    }
}

class DataLoader final : public IDataLoader {
public:
    DataLoader(const char* coreVersion) {
        if (coreVersion)
            mCoreVersion = coreVersion;
    }

    virtual std::string load() override {
        std::string xml;
        std::string url(kVersionUrl);
        if (!mCoreVersion.empty()) {
            const auto& id = android::studio::getInstallationId();
            url += Uri::FormatEncodeArguments(
                    "?tool=emulator&uid=%s&os=%s&osa=%s&", id,
                    toString(System::get()->getOsType()),
                    toOsArchString(System::get()->getHostBitness()));
            // append the fields which may change from run to run: version and
            // core version
            url += getVersionUriFields();
        }
        char* error = nullptr;
        if (!curl_download(url.c_str(), nullptr, &curlWriteCallback, &xml,
                           &error)) {
            dwarning("UpdateCheck: Failure: %s", error);
            ::free(error);
        }
        return xml;
    }

    virtual std::string getUniqueDataKey() override {
        return getVersionUriFields();
    }

private:
    std::string getVersionUriFields() const {
        return Uri::FormatEncodeArguments("version=" EMULATOR_VERSION_STRING
                                          "&coreVersion=%s",
                                          mCoreVersion);
    }

private:
    std::string mCoreVersion;
};

class TimeStorage final : public ITimeStorage {
    using IniFile = android::base::IniFile;

public:
    TimeStorage() {
        const std::string configPath = android::ConfigDirs::getUserDirectory();
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
        return mFileLock != nullptr;
    }

    std::chrono::seconds getTime(const std::string& key) override {
        IniFile file(mDataFileName);
        if (!file.read()) {
            // no file at all, return the lowest possible timestamp
            return std::chrono::seconds(0);
        }

        return std::chrono::seconds(
                file.getInt64(IniFile::makeValidKey(key), 0));
    }

    void setTime(const std::string& key, std::chrono::seconds time) override {
        IniFile file(mDataFileName);
        file.read();  // who cares if it didn't exist - we'll create it anyway
        file.setInt64(IniFile::makeValidKey(key), time.count());
        if (!file.write()) {
            dwarning("UpdateCheck: couldn't save the data file");
        }
    }

private:
    std::string mDataFileName;
    FileLock* mFileLock = nullptr;
};

class NewerVersionReporter final : public INewerVersionReporter {
public:
    virtual void reportNewerVersion(
            const android::base::Version& /*existing*/,
            const android::base::Version& /*newer*/) override {
        dinfo("%s", kNewerVersionMessage);
    }
};

UpdateChecker::UpdateChecker(const char* coreVersion)
    : mVersionExtractor(new VersionExtractor()),
      mDataLoader(new DataLoader(coreVersion)),
      mTimeStorage(new TimeStorage()),
      mReporter(new NewerVersionReporter()) {}

UpdateChecker::UpdateChecker(IVersionExtractor* extractor,
                             IDataLoader* loader,
                             ITimeStorage* storage,
                             INewerVersionReporter* reporter)
    : mVersionExtractor(extractor),
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
    std::chrono::seconds now = currentTimeSinceEpoch();
    // Check only if the previous check was 4+ hours ago
    return now - mTimeStorage->getTime(mDataLoader->getUniqueDataKey()) >=
           std::chrono::hours(4);
}

bool UpdateChecker::runAsyncCheck() {
    return async([this] {
        if (init() && needsCheck()) {
            asyncWorker();
        }
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

    // Update the last version check time
    mTimeStorage->setTime(mDataLoader->getUniqueDataKey(),
                          currentTimeSinceEpoch());
}

std::optional<UpdateChecker::VersionInfo> UpdateChecker::getLatestVersion() {
    const auto repositoryXml = mDataLoader->load();
    const auto versions = mVersionExtractor->extractVersions(repositoryXml);
    if (versions.empty()) {
        return std::nullopt;
    }

    const auto updateChannel = android::studio::updateChannel();

    // now find the first channel which is equal to or lower than the selected
    // update channel
    const auto greaterIt = versions.upper_bound(updateChannel);
    if (greaterIt == versions.begin()) {
        // even the first update channel in the list is greater than the
        // one from Android Studio settings
        return std::nullopt;
    }

    return std::make_pair(std::prev(greaterIt)->second,
                          std::prev(greaterIt)->first);
}

}  // namespace update_check
}  // namespace android
