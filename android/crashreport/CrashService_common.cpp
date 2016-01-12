// Copyright 2015 The Android Open Source Project
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

#include "android/crashreport/CrashService.h"

#include "android/base/String.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashSystem.h"
#include "android/curl-support.h"
#include "android/utils/debug.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/minidump.h"
#include "google_breakpad/processor/minidump_processor.h"
#include "google_breakpad/processor/process_state.h"
#include "processor/stackwalk_common.h"

#include <curl/curl.h>

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

    auto& response = *static_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response.append(static_cast<char*>(ptr), real_size);
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

std::string CrashService::getDumpFile() const {
    return mDumpFile;
}

bool CrashService::validDumpFile() const {
    return ::android::crashreport::CrashSystem::get()->isDump(mDumpFile);
}

std::string CrashService::getReportId() const {
    return mReportId;
}

bool CrashService::uploadCrash(const std::string& url) {
    curl_init(::android::crashreport::CrashSystem::get()
                      ->getCaBundlePath().c_str());
    char* error = nullptr;
    CURL* const curl = curl_easy_default_init(&error);
    if (!curl) {
        E("Curl instantiation failed: %s\n", error);
        ::free(error);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    curl_httppost* formpost = nullptr;
    curl_httppost* lastptr = nullptr;

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, kNameKey,
                 CURLFORM_COPYCONTENTS, kName, CURLFORM_END);

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, kVersionKey,
                 CURLFORM_COPYCONTENTS, mVersionId.c_str(), CURLFORM_END);

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "upload_file_minidump",
                 CURLFORM_FILE, mDumpFile.c_str(), CURLFORM_END);

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "hw_info",
                 CURLFORM_FILE, mHWTmpFilePath.c_str(), CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    // The 4th pointer to be passed to the WRITEFUNCTION above
    std::string http_response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     static_cast<void*>(&http_response_data));

    bool success = true;
    CURLcode curlRes = curl_easy_perform(curl);

    if (curlRes != CURLE_OK) {
        success = false;
        E("curl_easy_perform() failed with code %d (%s)", curlRes,
          curl_easy_strerror(curlRes));
    } else {
        long http_response = 0;
        curlRes =
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
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
            I("Crash Report ID: %s\n", http_response_data.c_str());
            mReportId = http_response_data;
        }
    }

    curl_formfree(formpost);
    curl_easy_cleanup(curl);
    cleanupHWInfo();
    return success;
}

std::string CrashService::getCrashDetails() {
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
    std::string detailsFile(mDumpFile);
    detailsFile += "details";
    FILE* fp = fopen (detailsFile.c_str(), "w+");
    if (!fp) {
        E("Couldn't open %s for writing crash details\n", detailsFile.c_str());
        return details;
    }
    google_breakpad::SetPrintStream(fp);
    google_breakpad::PrintProcessState(process_state, true, &resolver);

    fseek(fp, 0, SEEK_END);
    details.resize(std::ftell(fp));
    rewind(fp);
    fread(&details[0], 1, details.size(), fp);

    fclose(fp);
    remove(detailsFile.c_str());

    return details;
}

std::string CrashService::collectSysInfo() {
    mHWInfo = getHWInfo();
    return mHWInfo;
}


void CrashService::initCrashServer() {
    mServerState.waiting = true;
    mServerState.connected = 0;
}

int64_t CrashService::waitForDumpFile(int clientpid, int timeout) {
    int64_t waitduration_ms = 0;
    if (!setClient(clientpid)) {
        return -1;
    }
    while (true) {
        if (!isClientAlive()) {
            if ((!mServerState.waiting) && (mServerState.connected == 0)) {
                break;
            } else {
                E("CrashService client died unexpectedly\n");
                return -1;
            }
        }
        if (timeout >= 0 && waitduration_ms >= timeout) {
            return -1;
        }
        ::android::base::System::sleepMs(WAIT_INTERVAL_MS);
        waitduration_ms += WAIT_INTERVAL_MS;
    }
    if (!mDumpRequestContext.file_path.empty()) {
        mDumpFile = mDumpRequestContext.file_path;
    }
    return waitduration_ms;
}

bool CrashService::setClient(int clientpid) {
    // return false for invalid pid
    if (clientpid <= 0) {
        return false;
    }
    mClientPID = clientpid;
    return true;
}

}  // namespace crashreport
}  // namespace android
