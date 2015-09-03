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

#include "android/update-check/update_check.h"
#include "android/utils/debug.h"
#include <android/utils/misc.h>
#include "qemu/thread.h"

#include <curl/curl.h>

#include <errno.h>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <string.h>


#define  STRINGIFY(x)   _STRINGIFY(x)
#define  _STRINGIFY(x)  #x

#ifdef ANDROID_SDK_TOOLS_REVISION
#  define  SDK_VERSION_STRING  STRINGIFY(ANDROID_SDK_TOOLS_REVISION)
#else
#  define  SDK_VERSION_STRING  "1.0" // "standalone"
#endif

static const char dataFileName[] = ".emu-update-last-check";
static const char versionUrl[] =
        "https://dl.google.com/android/repository/repository-10.xml";

using android::base::String;
using android::base::Version;
using android::update_check::UpdateChecker;

void checkForUpdates(const char* homePath) {

    std::auto_ptr<UpdateChecker> checker(
            new (std::nothrow) UpdateChecker(homePath));

    if (checker->init() && checker->needsCheck() && checker->runAsyncCheck()) {
        checker.release();  // it will delete itself after the check
    }
}

namespace android {
namespace update_check {

static int parseXmlValue(const char* name, const std::string& xml,
                          size_t offset) {
    const size_t pos = xml.find(name, offset);
    if (pos == std::string::npos) {
        dwarning("UpdateCheck: can't find version attribute '%s'", name);
        return -1;
    }
    char* end;
    const int value = strtoi(xml.c_str() + pos + strlen(name), &end, 10);
    if (errno || *end != '<') {
        dwarning("UpdateCheck: invalid value of a version attribute '%s'", name);
        return -1;
    }
    return value;
}

class VersionExtractor : public IVersionExtractor {
public:
    virtual android::base::Version extractVersion(const std::string& data) {
        // quick and dirty
        // TODO: use libxml2 to properly parse XML and extract node value

        size_t toolPos = data.find("<sdk:tool>");
        if (toolPos == std::string::npos) {
            return Version::Invalid;
        }

        const int major = parseXmlValue("<sdk:major>", data, toolPos);
        const int minor = parseXmlValue("<sdk:minor>", data, toolPos);
        const int micro = parseXmlValue("<sdk:micro>", data, toolPos);
        if (major < 0 || minor < 0 || micro < 0) {
            return Version::Invalid;
        }

        return Version(major, minor, micro);
    }
};

static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb,
                                void* userp) {
    std::string* const xml = static_cast<std::string*>(userp);
    const size_t total = size * nmemb;

    xml->insert(xml->end(),
        static_cast<char*>(contents), static_cast<char*>(contents) + total);

    return total;
}

class DataLoader : public IDataLoader {
public:
    virtual std::string load() {
        std::string xml;

        curl_global_init(CURL_GLOBAL_ALL);

        if (CURL* const curl = curl_easy_init()) {
            curl_easy_setopt(curl, CURLOPT_URL, versionUrl);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xml);
            const CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                dwarning("UpdateCheck: failed to get an URL: %d (%s)\n",
                         res, curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return xml;
    }
};

class TimeStorage : public ITimeStorage {
public:
    TimeStorage(const char* homePath)
        : mFileLock(NULL) {
        mDataFileName = String(homePath).append("/").append(dataFileName);
    }

    ~TimeStorage() {
        if (mFileLock) {
            filelock_release(mFileLock);
        }
    }

    bool lock() {
        if (mFileLock) {
            dwarning("UpdateCheck: lock() called twice by the same process");
            return true;
        }

        mFileLock = filelock_create(mDataFileName.c_str());
        // if someone's already checking it - don't do it twice
        return mFileLock != NULL;
    }

    time_t getTime() {
        std::ifstream dataFile(mDataFileName.c_str());
        if (!dataFile) {
            return time_t(); // no data file
        }
        int64_t intTime = 0;
        dataFile >> intTime;
        return static_cast<time_t>(intTime);
    }

    void setTime(time_t time) {
        std::ofstream dataFile(mDataFileName.c_str());
        if (!dataFile) {
            dwarning("UpdateCheck: couldn't open data file for writing");
        } else {
            dataFile << static_cast<int64_t>(time) << '\n';
        }
    }

private:
    android::base::String mDataFileName;
    FileLock* mFileLock;
};

UpdateChecker::UpdateChecker(const char* homePath)
    : mVersionExtractor(new VersionExtractor())
    , mDataLoader(new DataLoader())
    , mTimeStorage(new TimeStorage(homePath)) {
}

UpdateChecker::UpdateChecker(IVersionExtractor *extractor, IDataLoader *loader,
                             ITimeStorage *storage)
    : mVersionExtractor(extractor)
    , mDataLoader(loader)
    , mTimeStorage(storage) {
}


bool UpdateChecker::init() {
    if (!mTimeStorage->lock()) {
        return false;
    }

    return true;
}

bool UpdateChecker::needsCheck() const {
    const time_t now = time(NULL);
    // check only if the date of previous check is before today date
    return clearHMS(mTimeStorage->getTime()) < clearHMS(now);
}

bool UpdateChecker::runAsyncCheck() {
    QemuThread thread;
    qemu_thread_create(&thread, &UpdateChecker::asyncWorkerHelper, this,
        QEMU_THREAD_DETACHED);

    return true;    // qemu threads don't return failure - they just kill
                    // the whole process
}

void* UpdateChecker::asyncWorkerHelper(void* checker) {
    reinterpret_cast<UpdateChecker*>(checker)->asyncWorker();
    return NULL;
}

time_t UpdateChecker::clearHMS(time_t t) {
    static const time_t secondsPerDay = 24 * 60 * 60;
    return (t / secondsPerDay) * secondsPerDay;
}

void UpdateChecker::asyncWorker() {
    Version current = getCurrentVersion();
    Version last = loadLatestVersion();
    if (current < last) {
        printNewerVersionWarning();
    }

    // update the last version check time
    mTimeStorage->setTime(time(NULL));

    delete this;    // We've been abandoned on a separate thread. So just die...
}

void UpdateChecker::printNewerVersionWarning() {
    printf("%s", "Your emulator is out of date, "
            "please update by launching Android Studio\n");
}

Version UpdateChecker::getCurrentVersion() {
    return Version(SDK_VERSION_STRING);
}

Version UpdateChecker::loadLatestVersion() {
    const std::string xml = mDataLoader->load();
    const Version ver = mVersionExtractor->extractVersion(xml);
    return ver;
}

}  // namespace android
}  // namespace update_check
