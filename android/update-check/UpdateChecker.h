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

#ifndef ANDROID_UPDATECHECK_UPDATECHECKER_H
#define ANDROID_UPDATECHECK_UPDATECHECKER_H

#include "android/base/String.h"
#include "android/base/Version.h"
#include "android/utils/filelock.h"

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

class IVersionExtractor {
public:
    virtual ~IVersionExtractor() {}
    virtual android::base::Version extractVersion(const std::string& data) = 0;
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

// UpdateChecker - class that incapsulates the logic for update checking
//  once a day. It runs the check asynchronously and only if there was no
//  check performed today.
// Usage: create instance of the class dynamically, and if init() and
//  needsCheck() return true, call runAsyncCheck(). it will do the checking
//  and delete itself if runAsyncCheck() returned true.
// TODO: create a separate interface for an action performed if there's
//  a newer version available

class UpdateChecker {
public:
    explicit UpdateChecker(const char* homePath);

    // contructor for tests
    UpdateChecker(IVersionExtractor*, IDataLoader*, ITimeStorage*);

    bool init();

    bool needsCheck() const;

    bool runAsyncCheck();

private:
    static void* asyncWorkerHelper(void* checker);

    static time_t clearHMS(time_t t);

    static android::base::Version getCurrentVersion();

    static void printNewerVersionWarning();

    void asyncWorker();
    android::base::Version loadLatestVersion();

private:
    std::auto_ptr<IVersionExtractor> mVersionExtractor;
    std::auto_ptr<IDataLoader> mDataLoader;
    std::auto_ptr<ITimeStorage> mTimeStorage;
};

}  // namespace android
}  // namespace update_check

#endif  // ANDROID_UPDATECHECK_UPDATECHECKER_H
