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

#include "android/metrics/MetricsUploader.h"

#include "android/base/Log.h"
#include "android/curl-support.h"
#include "android/utils/dirscanner.h"
#include "android/utils/filelock.h"
#include "android/utils/path.h"
#include "android/utils/string.h"
#include "android/utils/uri.h"

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace android {
namespace metrics {

using android::base::IniFile;
using std::string;
using std::vector;

static const char kMetricsRelativeDir[] = "metrics";

// File prefixes for different MetricsType's.
static const char kQtToolWindowFileSuffix[] = "qttool.mets";

static string getFileSuffixForType(MetricsType type) {
    switch (type) {
        case MetricsType::kQtToolWindow:
            return kQtToolWindowFileSuffix;
        default:
            return "";
    }
}

MetricsUploader::MetricsUploader(MetricsType type, string avdHome)
    : IMetricsUploader(type, avdHome) {
    // I want to use PathUtils, but don't want to deal with String n' friends.
    if (!path_is_dir(avdHome.c_str())) {
        LOG(WARNING) << "Could not obtain valid AVD dir.";
        return;
    }

    mMetricsDir = avdHome + PATH_SEP + kMetricsRelativeDir;
    if (path_mkdir_if_needed(mMetricsDir.c_str(), 0744) != 0) {
        LOG(WARNING) << "Could not create metrics directory.";
        mMetricsDir.clear();
    }

    if (getFileSuffixForType(type).empty()) {
        //LOG(WARNING) << "Unrecognized MetricsType" << type;
        mMetricsDir.clear();
    }
}

string MetricsUploader::generateUniquePath() {
    if (mMetricsDir.empty()) {
        return "";
    }

    if (!mMetricsFilePath.empty()) {
        return mMetricsFilePath;
    }

    auto fileSuffix = getFileSuffixForType(type());
    if (fileSuffix.empty()) {
        // LOG(WARNING) << "Unkown metrics type: " << type;
        return "";
    }

    auto myPid = getpid();
    char pidBuf[24];
    snprintf(pidBuf, sizeof(pidBuf), "%d", myPid);
    auto fileName = string("metrics") + "." + pidBuf + "." + fileSuffix;
    auto path = mMetricsDir + PATH_SEP + fileName;
    path = path_get_absolute(path.c_str());
    // We ignore the returned FileLock. It will be released when the process
    // exits.
    if (filelock_create(path.c_str()) == 0) {
        LOG(WARNING) << "Failed to lock file at " << path << ". "
                    << "This indicates metrics file collission.";
        return "";
    }

    mMetricsFilePath = path;
    return path;
}

bool MetricsUploader::uploadCompletedMetrics(const IMetricsFileProcessor& processor) {
    if (mMetricsDir.empty()) {
        return false;
    }

    bool success = true;
    int numProcessed = 0, numQueued = 0;
    auto suffix = getFileSuffixForType(type()).c_str();
    const char* filePath;
    auto scanner = dirScanner_new(mMetricsDir.c_str());
    if (!scanner) {
        return false;
    }

    while ((filePath = dirScanner_nextFull(scanner)) != NULL) {
        if (!str_ends_with(filePath, suffix) ||
            0 == strcmp(mMetricsFilePath.c_str(), filePath)) {
            continue;
        }

        /* Quickly strdup the returned string to workaround a bug in
         * dirScanner. As the contents of the scanned directory change, the
         * pointer returned by dirScanner gets changed behind our back.
         * Even taking a filelock changes the directory contents...
         */
        char* dupedFilePath = strdup(filePath);

        FileLock* fileLock = filelock_create(dupedFilePath);
        /* We may not get this |fileLock| if the emulator process that created
         * the metrics file is still running. This is by design -- we don't want
         * to process partial metrics files.
         */
        if (fileLock != NULL) {
            ++numProcessed;
            IniFile iniFile(dupedFilePath);
            if (iniFile.read()) {
                auto urls = processor(iniFile);
                // TODO(pprabhu) // make this async
                for (const auto& url : urls) {
                    LOG(VERBOSE) << "Metrics url ping: " << url;
                    if(makeGetRequest(url)) {
                        ++numQueued;
                    } else {
                        success = false;
                    }
                }
            } else {
                success = false;
            }

            /* Current strategy is to delete the metrics dump on failed upload,
             * noting that we missed the metric. This protects us from leaving
             * behind too many files if we consistently fail to upload.
             */
            success &= (0 == path_delete_file(dupedFilePath));
            filelock_release(fileLock);
        }
        free(dupedFilePath);
    }
    dirScanner_free(scanner);

    LOG(INFO) << "metrics(" << suffix << "): Processed "
               << numProcessed << " reports.";
    LOG(INFO) << "metrics(" << suffix << "): Queued "
               << numQueued << " reports for upload.";
    return success;
}

// Dummy write function to pass to curl to avoid dumping the returned output to
// stdout.
static size_t curlWriteFunction(CURL* handle,
                                size_t size,
                                size_t nmemb,
                                void* userdata) {
    // Report that we "took care of" all the data provided.
    return size * nmemb;
}

bool MetricsUploader::makeGetRequest(const string& url) {
    bool success = true;
    CURL* const curl = curl_easy_default_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
    CURLcode curlRes = curl_easy_perform(curl);
    if (curlRes != CURLE_OK) {
        success = false;
        LOG(WARNING) << "curl_easy_perform() failed with code " << curlRes
                     << " (" << curl_easy_strerror(curlRes) << ")";
    }

    // toolbar returns a 404 by design.
    long http_response = 0;
    curlRes = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response);
    if (curlRes == CURLE_OK) {
        if (http_response != 200 && http_response != 404) {
            LOG(WARNING) << "Got HTTP response code " << http_response;
            success = false;
        }
    } else if (curlRes == CURLE_UNKNOWN_OPTION) {
        LOG(WARNING) << "Can not get a valid response code: not supported";
    } else {
        LOG(WARNING) << "Unexpected error while checking http response: "
                     << curl_easy_strerror(curlRes);
    }
    curl_easy_cleanup(curl);
    return success;
}

std::vector<std::string> DefaultMetricsFileProcessor::operator() (
        const IniFile& iniFile) const {
    static const char kToolbarUrl[] = "https://tools.google.com/service/update";
    string args;
    bool first = true;
    for (const auto& key : iniFile) {
        if (first) {
            first = false;
        } else {
            args += "&";
        }
        args += key + "=" + iniFile.getString(key, "");
    }
    if (args.empty()) {
        return {};
    }

    auto url = string(kToolbarUrl) + "?" + uri_encode(args.c_str());
    return {url};
}

}  // namespace metrics
}  // namespace android
