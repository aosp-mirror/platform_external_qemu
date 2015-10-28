/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/crashreport/CrashService.h"
#include "android/base/String.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#endif
#include "android/curl-support.h"
#if CONFIG_QT
#include "android/crashreport/ui/ConfirmDialog.h"
#endif
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"
#if defined(__linux__)
#include "client/linux/crash_generation/crash_generation_server.h"
#elif defined(__APPLE__)
#include "client/mac/crash_generation/crash_generation_server.h"
#elif defined(_WIN32)
#include "client/windows/crash_generation/crash_generation_server.h"
#else
#error Breakpad not supported on this platform
#endif
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "processor/stackwalk_common.h"

#include <stdio.h>
#include <stdlib.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define WAIT_INTERVAL_MS 100

extern "C" const unsigned char* android_emulator_icon_find(const char* name,
                                                           size_t* psize);

const char kNameKey[] = "prod";
const char kVersionKey[] = "ver";
const char kName[] = "AndroidEmulator";

// Callback to get the response data from server.
static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    if (!userp)
        return 0;

    std::string* response = static_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response->append(static_cast<char*>(ptr), real_size);
    return real_size;
}

namespace android {
namespace crashreport {

CrashService::CrashService(const std::string& version, const std::string& build)
    : mDumpRequestContext(),
      mServerState(),
      mVersion(version),
      mBuild(build),
      mVersionId(),
      mDumpFile(),
      mReportId() {
    mVersionId = version;
    mVersionId += "-";
    mVersionId += build;
};

CrashService::~CrashService() {
    if (validDumpFile()) {
        remove(mDumpFile.c_str());
    }
}

void CrashService::setDumpFile(const std::string& dumpfile) {
    mDumpFile = dumpfile;
}

std::string CrashService::getDumpFile() {
    return mDumpFile;
}

bool CrashService::validDumpFile() {
    return ::android::crashreport::CrashSystem::get()->isDump(mDumpFile);
}

std::string CrashService::getReportId() {
    return mReportId;
}

bool CrashService::uploadCrash(const std::string& url) {
    curl_init(::android::crashreport::CrashSystem::get()
                      ->getCaBundlePath()
                      .c_str());
    curl_httppost* formpost = NULL;
    curl_httppost* lastptr = NULL;
    std::string http_response_data;
    CURLcode curlRes;

    CURL* const curl = curl_easy_default_init();
    if (!curl) {
        E("Curl instantiation failed\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, kNameKey,
                 CURLFORM_COPYCONTENTS, kName, CURLFORM_END);

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, kVersionKey,
                 CURLFORM_COPYCONTENTS, mVersionId.c_str(), CURLFORM_END);

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME,
                 "upload_file_minidump", CURLFORM_FILE, mDumpFile.c_str(),
                 CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     static_cast<void*>(&http_response_data));

    curlRes = curl_easy_perform(curl);

    bool success = true;

    if (curlRes != CURLE_OK) {
        success = false;
        E("curl_easy_perform() failed with code %d (%s)", curlRes,
          curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            E("Got HTTP response code %ld", http_response);
            success = false;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        E("Can not get a valid response code: not supported");
        success = false;
    } else {
        E("Unexpected error while checking http response: %s",
          curl_easy_strerror(curlRes));
        success = false;
    }

    if (success) {
        I("Crash Send complete\n");
        I("Crash Report ID: %s\n", http_response_data.c_str());
        mReportId = http_response_data;
    }

    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    curl_cleanup();
    return success;
}

const std::string CrashService::getCrashDetails() {
    std::string details;
    google_breakpad::BasicSourceLineResolver resolver;
    google_breakpad::MinidumpProcessor minidump_processor(nullptr, &resolver);

    // Process the minidump.
    google_breakpad::Minidump dump(mDumpFile);
    if (!dump.Read()) {
        E("Minidump %s could not be read", dump.path().c_str());
        return details;
    }
    google_breakpad::ProcessState process_state;
    if (minidump_processor.Process(&dump, &process_state) !=
        google_breakpad::PROCESS_OK) {
        E("MinidumpProcessor::Process failed");
        return details;
    }

    // Capture printf's from PrintProcessState to file
    FILE* fp = tmpfile();
    if (!fp) {
        E("Couldn't open tmpfile for writing crash details\n");
        return details;
    }
    google_breakpad::SetPrintStream(fp);
    google_breakpad::PrintProcessState(process_state, true, &resolver);

    fseek(fp, 0, SEEK_END);
    details.resize(std::ftell(fp));
    rewind(fp);
    fread(&details[0], 1, details.size(), fp);

    fclose(fp);

    return details;
}

void CrashService::initCrashServer(void) {
    mServerState.waiting = true;
    mServerState.connected = 0;
}

int64_t CrashService::waitForDumpFile(int clientpid, int timeout) {
    int waitduration_ms = 0;
    while (true) {
        if (!isClientAlive(clientpid)) {
            if ((!mServerState.waiting) && (mServerState.connected == 0)) {
                break;
            } else {
                E("CrashService parent died unexpectedly\n");
                return -1;
            }
        }
        if (timeout > 0 && waitduration_ms >= timeout) {
            return -1;
        }
        sleep_ms(WAIT_INTERVAL_MS);
        waitduration_ms += WAIT_INTERVAL_MS;
    }
    if (mDumpRequestContext.file_path.size()) {
        mDumpFile = mDumpRequestContext.file_path;
    }
    return waitduration_ms;
}

}  // namespace crashreport
}  // namespace android
