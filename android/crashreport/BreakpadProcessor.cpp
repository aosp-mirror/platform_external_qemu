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

#include "android/crashreport/crash-handler.h"
#include <stdio.h>
#include <stdlib.h>
#include "android/crashreport/BreakpadUtils.h"
#include "android/curl-support.h"
#include "android/base/system/System.h"

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

#ifdef ANDROID_SDK_TOOLS_REVISION
#define VERSION_STRING STRINGIFY(ANDROID_SDK_TOOLS_REVISION) ".0"
#else
#define VERSION_STRING "standalone"
#endif

#ifdef ANDROID_BUILD_NUMBER
#define BUILD_STRING STRINGIFY(ANDROID_BUILD_NUMBER)
#else
#define BUILD_STRING "0"
#endif

#define PRODUCT_VERSION VERSION_STRING "-" BUILD_STRING


#define PROD_ACCESS PROD
#define STAGING_ACCESS STAGING
using namespace android::base;
namespace BreakpadProcessor {
namespace {

// Callback to get the response data from server.
size_t WriteCallback(void *ptr, size_t size,
                            size_t nmemb, void *userp) {
  if (!userp)
    return 0;

  std::string *response = reinterpret_cast<std::string *>(userp);
  size_t real_size = size * nmemb;
  response->append(reinterpret_cast<char *>(ptr), real_size);
  return real_size;
}

#ifdef CRASHUPLOAD
bool upload_crash (const std::string& dumppath)
{
    printf("\nSending Crash %s\n", dumppath.c_str());

    struct curl_httppost *formpost_ = NULL;
    struct curl_httppost *lastptr_ = NULL;
    std::string http_response_data;
    CURLcode curlRes;

#if CRASHUPLOAD == PROD_ACCESS
    static const char crash_url[] = "https://clients2.google.com/cr/report";
#elif CRASHUPLOAD == STAGING_ACCESS
    static const char crash_url[] = "https://clients2.google.com/cr/staging_report";
#endif

    static const char name_key[] = "prod";
    static const char version_key[] = "ver";
    static const char name[] = "AndroidEmulator";
    static const char version[] = PRODUCT_VERSION;

    CURL* const curl = curl_easy_default_init();
    if (!curl) {
        fprintf(stderr, "Curl instantiation failed\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,crash_url);
    curl_formadd(&formpost_, &lastptr_,
                 CURLFORM_COPYNAME, name_key,
                 CURLFORM_COPYCONTENTS, name,
                 CURLFORM_END);

    curl_formadd(&formpost_, &lastptr_,
                 CURLFORM_COPYNAME, version_key,
                 CURLFORM_COPYCONTENTS, version,
                 CURLFORM_END);

    curl_formadd(&formpost_, &lastptr_,
                 CURLFORM_COPYNAME,"upload_file_minidump",
                 CURLFORM_FILE, dumppath.c_str(),
                 CURLFORM_END);


    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost_);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
          reinterpret_cast<void *>(&http_response_data));

    curlRes = curl_easy_perform(curl);

    bool success = true;

    if (curlRes != CURLE_OK) {
        success = false;
        fprintf(stderr, "curl_easy_perform() failed with code %d (%s)", curlRes,
        curl_easy_strerror(curlRes));
    }

    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200) {
            fprintf(stderr, "Got HTTP response code %ld", http_response);
            success = false;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        fprintf(stderr, "Can not get a valid response code: not supported");
        success = false;
    } else {
        fprintf(stderr, "Unexpected error while checking http response: %s",
                 curl_easy_strerror(curlRes));
        success = false;
    }

    if (success) {
        printf ("Crash Send complete\n");
        printf ("Crash Report ID: %s\n",http_response_data.c_str());
    }

    curl_formfree(formpost_);
    curl_easy_cleanup(curl);

    return success;
}
#endif //CRASHUPLOAD

bool process_crash(const char * crashpath) {
    const std::string crashDirectory = BreakpadUtils::get()->getCrashDirectory();
    if (crashpath != NULL && BreakpadUtils::isDump(crashpath)) {
#ifndef CRASHUPLOAD
        fprintf(stderr, "ERROR:Not configured to upload crash reports\n");
        return false;
#else
        curl_init(BreakpadUtils::get()->getCaBundlePath().c_str());
        if (upload_crash(crashpath)) {
            remove(crashpath);
        }
        curl_cleanup();
        return true;
#endif
    } else {
        return false;
    }
}

void list_crashes() {
    const std::string crashDirectory = BreakpadUtils::get()->getCrashDirectory();
    StringVector crashDumps = System::get()->scanDirEntries(crashDirectory.c_str(), true);
    for (auto i: crashDumps) {
        if (!BreakpadUtils::isDump(i.c_str())) {
            continue;
        }
        printf ("%s\n", i.c_str());

    }
}

} //namespace
}; //namespace BreakpadProcessor



extern "C" bool process_crash (const char * crashpath) {
    return BreakpadProcessor::process_crash(crashpath);
}

extern "C" void list_crashes () {
    BreakpadProcessor::list_crashes();
}
