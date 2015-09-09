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

#include "android/utils/curl.h"

#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <curl/curl.h>

#define mwarning(fmt, ...) \
    dwarning("%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

static int initCount = 0;
static char* cached_ca_info = NULL;

bool curl_init(const char* ca_info) {
    if (initCount == 0) {
        // first time - try to initialize the library
        const CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK) {
            dwarning("CURL: global init failed with code %d (%s)", res,
                     curl_easy_strerror(res));
            return false;
        }
    }

    AFREE(cached_ca_info);
    cached_ca_info = ASTRDUP(ca_info);
    ++initCount;
    return true;
}

void curl_cleanup() {
    if (initCount == 0) {
        return;
    } else if (--initCount == 0) {
        AFREE(cached_ca_info);
        cached_ca_info = NULL;
        curl_global_cleanup();
    }
}

CURL* curl_default_init() {
    CURL* const curl = curl_easy_init();
    CURLcode curlRes;

    if (!curl) {
        mwarning("Failed to initialize libcurl");
        return NULL;
    }
    if (cached_ca_info != NULL) {
        curlRes = curl_easy_setopt(curl, CURLOPT_CAINFO, cached_ca_info);
        if (curlRes != CURLE_OK) {
            mwarning("Could not set CURLOPT_CAINFO: %s",
                     curl_easy_strerror(curlRes));
        }
    }
    return curl;
}
