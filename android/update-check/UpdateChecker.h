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

#include "android/base/threads/Thread.h"
#include "android/base/Version.h"

#include <memory>
#include <string>
#include <time.h>

namespace android {
namespace update_check {

// Set of interfaces for the operations UpdateChecker performs:
//  IVersionExtractor - extract emulator version from the xml manifest
//  IDataLoader - download the xml manifest from the Web
//  ITimeStorage - lock the file to store last update checking time,
//                 save/load time from the file
//  INewerVersionReporter - report to the user about available newer version

class IVersionExtractor {
public:
    virtual ~IVersionExtractor() {}
    virtual android::base::Version extractVersion(const std::string& data) const = 0;
    virtual android::base::Version getCurrentVersion() const = 0;
};

class IDataLoader {
public:
    virtual ~IDataLoader() {}
    virtual std::string load() = 0;
};

class ITimeStorage {
public:
    virtual ~ITimeStorage() {}

    virtual bool lock() = 0;
    virtual time_t getTime() = 0;
    virtual void setTime(time_t time) = 0;
};

class INewerVersionReporter {
public:
    virtual ~INewerVersionReporter() {}

    virtual void reportNewerVersion(const android::base::Version& existing,
                                    const android::base::Version& newer) = 0;
};

// UpdateChecker - class that incapsulates the logic for update checking
//  once a day. It runs the check asynchronously and only if there was no
//  check performed today.
// Usage: create instance of the class dynamically, and if init() and
//  needsCheck() return true, call runAsyncCheck(). it will do the checking
//  and delete itself if runAsyncCheck() returned true.
// If any of the calls returned false, one has to delete the object itself

class UpdateChecker {
public:
    // |configPath| is the path to the emulator configuration directory
    // where the checker can store its records about last check time
    explicit UpdateChecker(const char* configPath);

    bool init();

    bool needsCheck() const;

    bool runAsyncCheck();

protected:
    // constructor for tests
    UpdateChecker(IVersionExtractor*,
                  IDataLoader*,
                  ITimeStorage*,
                  INewerVersionReporter*);

    static time_t clearHMS(time_t t);

    void asyncWorker();

    android::base::Version loadLatestVersion();

    // Helper class to run the worker in a new thread
    class CheckerThread : public android::base::Thread {
    public:
        CheckerThread(UpdateChecker* checker) : mChecker(checker) {}

        virtual intptr_t main() {
            mChecker->asyncWorker();
            return 0;
        }

        virtual void onExit() {
            // The checker has been abandoned on this thread. So just kill it...
            delete mChecker;
        }

    private:
        UpdateChecker* mChecker;
    };

private:
    CheckerThread mThread;
    std::auto_ptr<IVersionExtractor> mVersionExtractor;
    std::auto_ptr<IDataLoader> mDataLoader;
    std::auto_ptr<ITimeStorage> mTimeStorage;
    std::auto_ptr<INewerVersionReporter> mReporter;
};

}  // namespace update_check
}  // namespace android
