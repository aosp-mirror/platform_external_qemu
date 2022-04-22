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

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include "android/openssl-support.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <curl/curl.h>

#include <stdlib.h>
#include <string.h>

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
        if (!android_openssl_init()) {
            return false;
        }
    }

    free(cached_ca_info);
    cached_ca_info = NULL;
    if (ca_info != NULL) {
        cached_ca_info = strdup(ca_info);
    }
    ++initCount;
    return true;
}

void curl_cleanup() {
    if (initCount == 0) {
        return;
    } else if (--initCount == 0) {
        free(cached_ca_info);
        cached_ca_info = NULL;
        // We know we're leaking memory by not calling curl_global_cleanup.
        // We can not guarantee that no threads exist when the program exits
        // (e.g. android::base::async has unknown lifetime).
        //
        // Ditto for android_openssl_finish.
    }
}

void* curl_easy_default_init(char** error) {
    // |curl_easy_init| will try to initialize libcurl globally if it isn't
    // already initialized. This behaviour is dangerous in multi-threaded
    // environment.
    if (!initCount) {
        *error = strdup("libcurl is not initialized. Bailing.");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    CURLcode curlRes;

    if (!curl) {
        *error = strdup("Failed to initialize libcurl");
        return NULL;
    }

    curlRes = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    if (curlRes != CURLE_OK) {
        int err = asprintf(error, "Could not disable signals: %s",
                           curl_easy_strerror(curlRes));
        (void)err;
        curl_easy_cleanup(curl);
        return NULL;
    }

    if (cached_ca_info != NULL) {
        curlRes = curl_easy_setopt(curl, CURLOPT_CAINFO, cached_ca_info);
        if (curlRes != CURLE_OK) {
            int err = asprintf(error, "Could not set CURLOPT_CAINFO: %s",
                               curl_easy_strerror(curlRes));
            (void)err;
            curl_easy_cleanup(curl);
            return NULL;
        }
    }
    return curl;
}

// A CurlWriteCallback that drops any downloaded data.
// Using it avoids dumping the content to stdout, the default CURL behaviour.
static size_t null_write_callback(char* ptr,
                                  size_t size,
                                  size_t nmemb,
                                  void* userdata) {
    return size * nmemb;
}

// This function can block forever. We do not set any timeout for
// curl_easy_perform. Since we disable signals, the DNS lookup timeout is ignore
// by libcurl.
// TODO: build using c-ares, and set timeout for curl_easy_perform.
static bool curl_post_internal(const char* url,
                               const char* data,
                               const size_t cData,
                               const char** httpHeaders,
                               const size_t cHttpHeaders,
                               CurlWriteCallback callback_func,
                               void* callback_userdata,
                               bool allow_404,
                               char** error) {
    CURL* curl = curl_easy_default_init(error);
    if (!curl) {
        return false;
    }

    bool result = false;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (callback_func) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, callback_userdata);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, null_write_callback);
    }

    if (data && cData) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)cData);
    }

    struct curl_slist* headers = NULL;
    int i;
    for (i = 0; i < cHttpHeaders; i++) {
        headers = curl_slist_append(headers, httpHeaders[i]);
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_VERBOSE, VERBOSE_CHECK(curl));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);

    const CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) {
        int err = asprintf(error, "%s", curl_easy_strerror(res));
        (void)err;
    } else {
        // toolbar returns a 404 by design.
        long http_response = 0;
        int curlRes =
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
        if (curlRes == CURLE_OK) {
            if (http_response == 200) {
                result = true;
            } else if (http_response == 404 && allow_404) {
                result = true;
            } else {
                int err = asprintf(error, "%s", curl_easy_strerror(curlRes));
                (void)err;
            }
        } else {
            int err = asprintf(
                    error, "Unexpected error while checking http response: %s",
                    curl_easy_strerror(curlRes));
            (void)err;
        }
    }

    curl_easy_cleanup(curl);

    return result;
}

static bool curl_download_internal(const char* url,
                                   const char* post_fields,
                                   CurlWriteCallback callback_func,
                                   void* callback_userdata,
                                   bool allow_404,
                                   char** error) {
    return curl_post_internal(
            url, post_fields, post_fields ? strlen(post_fields) : 0, NULL, 0,
            callback_func, callback_userdata, allow_404, error);
}

bool curl_download(const char* url,
                   const char* post_fields,
                   CurlWriteCallback callback_func,
                   void* callback_userdata,
                   char** error) {
    return curl_download_internal(url, post_fields, callback_func,
                                  callback_userdata, false, error);
}

bool curl_post(const char* url,
               const char* data,
               const size_t cData,
               const char** headers,
               const size_t cHeaders,
               CurlWriteCallback callback_func,
               void* callback_userdata,
               char** error) {
    return curl_post_internal(url, data, cData, headers, cHeaders,
                              callback_func, callback_userdata, false, error);
}

extern bool curl_download_null(const char* url,
                               const char* post_fields,
                               bool allow_404,
                               char** error) {
    return curl_download_internal(url, post_fields, NULL, NULL, allow_404,
                                  error);
}
