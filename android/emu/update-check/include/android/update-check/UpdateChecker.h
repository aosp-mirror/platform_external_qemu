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

#pragma once

#include "aemu/base/Version.h"
#include "aemu/base/threads/Thread.h"
#include "android/update-check/IVersionExtractor.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace android {
namespace update_check {

// Set of interfaces for the operations UpdateChecker performs:
//  IDataLoader - download the xml manifest from the Web site
//  ITimeStorage - lock the file to store last update checking time,
//                 save/load time from the file
//  INewerVersionReporter - report to the user about available newer version

class IDataLoader {
public:
    virtual ~IDataLoader() = default;

    virtual std::string load() = 0;
    virtual std::string getUniqueDataKey() = 0;
};

class ITimeStorage {
public:
    virtual ~ITimeStorage() = default;

    virtual bool lock() = 0;
    virtual std::chrono::seconds getTime(const std::string& key) = 0;
    virtual void setTime(const std::string& key, std::chrono::seconds time) = 0;
};

class INewerVersionReporter {
public:
    virtual ~INewerVersionReporter() = default;

    virtual void reportNewerVersion(const android::base::Version& existing,
                                    const android::base::Version& newer) = 0;
};

// UpdateChecker - Class that incapsulates the logic for update checking
//  once a day. It runs the check asynchronously and only if there was no
//  check performed today.
//  The class can also return the version that it discovered is available.
//
// Usage:
//   To check the version daily:
//     Create instance of the class dynamically, and if init() and
//     needsCheck() return true, call runAsyncCheck(). It will do the
//     checking and delete itself if runAsyncCheck() returned true.
//     If any of the calls returned false, one has to delete the object itself.
//
//   To get the version discoved by the last 'runAsyncCheck()':
//     Create an instance of the class and call getLatestVersion().

class UpdateChecker {
public:
    using VersionInfo =
            std::pair<android::base::Version, android::studio::UpdateChannel>;

    // |coreVersion| is the application's core version (e.g. qemu2 2.2.0)
    //      'nullptr' means 'don't send any emulator-specific information in the
    //      request, just check the version
    explicit UpdateChecker(const char* coreVersion = nullptr);
    virtual ~UpdateChecker() = default;

    bool init();

    bool needsCheck() const;

    bool runAsyncCheck();

    std::optional<VersionInfo> getLatestVersion();

    virtual std::chrono::seconds currentTimeSinceEpoch() const = 0;

protected:
    // constructor for tests
    UpdateChecker(IVersionExtractor*,
                  IDataLoader*,
                  ITimeStorage*,
                  INewerVersionReporter*);

    void asyncWorker();

private:
    std::unique_ptr<IVersionExtractor> mVersionExtractor;
    std::unique_ptr<IDataLoader> mDataLoader;
    std::unique_ptr<ITimeStorage> mTimeStorage;
    std::unique_ptr<INewerVersionReporter> mReporter;
};

class SystemClockUpdateChecker : public UpdateChecker {
public:
    explicit SystemClockUpdateChecker(const char* coreVersion = nullptr)
        : UpdateChecker(coreVersion){};
    std::chrono::seconds currentTimeSinceEpoch() const override {
        return std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch());
    }
};

}  // namespace update_check
}  // namespace android
