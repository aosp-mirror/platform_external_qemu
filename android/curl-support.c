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

#include "android/curl-support.h"

#include "android/utils/debug.h"

#include <curl/curl.h>

static int initCount = 0;

bool curl_init() {
    if (initCount == 0) {
        // first time - try to initialize the library
        const CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK) {
            dwarning("CURL: global init failed with code %d (%s)", res,
                     curl_easy_strerror(res));
            return false;
        }
    }

    ++initCount;
    return true;
}

void curl_cleanup() {
    if (initCount == 0) {
        return;
    } else if (--initCount == 0) {
        curl_global_cleanup();
    }
}
