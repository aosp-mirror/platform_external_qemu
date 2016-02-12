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

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/StringFormat.h"
#include "android/base/Uri.h"
#include "android/base/system/System.h"
#include "android/base/threads/Async.h"

#include "android/curl-support.h"
#include "android/utils/filelock.h"
#include "android/utils/path.h"

#include <memory>

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace android {
namespace metrics {

using base::IniFile;
using base::PathUtils;
using base::ScopedCPtr;
using base::ScopedPtr;
using base::String;
using base::StringFormat;
using base::StringVector;
using base::StringView;
using base::System;
using base::Uri;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

static const char kMetricsRelativeDir[] = "metrics";

MetricsUploader::MetricsUploader(base::Looper* looper,
                                 base::StringView avdHome,
                                 base::StringView fileSuffix)
    : mAvdHome(avdHome), mFileSuffix(fileSuffix), mMetricsFileFlusher(looper) {}

// static
shared_ptr<MetricsUploader> MetricsUploader::create(
        base::Looper* looper,
        base::StringView avdHome,
        base::StringView fileSuffix) {
    shared_ptr<MetricsUploader> obj(
            new MetricsUploader(looper, avdHome, fileSuffix));
    if (obj->init()) {
        return obj;
    }
    return nullptr;
}

bool MetricsUploader::init() {
    if (!System::get()->pathIsDir(mAvdHome)) {
        LOG(WARNING) << "Could not obtain valid AVD dir.";
        return false;
    }

    mMetricsDir = PathUtils::join(mAvdHome, kMetricsRelativeDir);
    if (path_mkdir_if_needed(mMetricsDir.c_str(), 0744) != 0) {
        LOG(WARNING) << "Could not create metrics directory.";
        return false;
    }

    auto fileName =
            StringFormat("metrics.%d%s", getpid(), mFileSuffix.c_str());
    auto mMetricsFilePath = PathUtils::join(mMetricsDir, fileName);
    mMetricsFilePath = path_get_absolute(mMetricsFilePath.c_str());
    // We ignore the returned FileLock. It will be released when the process
    // exits.
    if (filelock_create(mMetricsFilePath.c_str()) == 0) {
        LOG(WARNING) << "Failed to lock file at " << mMetricsFilePath << ". "
                     << "This indicates metrics file collission.";
        return false;
    }

    std::unique_ptr<IniFile> iniFile(new IniFile(mMetricsFilePath.c_str()));
    if (!mMetricsFileFlusher.start(std::move(iniFile))) {
        LOG(WARNING) << "Failed to start metrics flush-to-disk service for "
                     << mMetricsFilePath;
        return false;
    }

    mInited = true;
    return true;
}

IniFile* MetricsUploader::metricsFile() {
    if (!mInited) {
        return nullptr;
    }

    return mMetricsFileFlusher.iniFile();
}

namespace internal {

struct FileDeleter {
    void operator()(const char* ptr) const {
        path_delete_file(ptr);
        // We don't actually own |ptr|, so don't touch it.
    }
};

struct FileLockReleaser {
    void operator()(FileLock* ptr) const {
        if (ptr) {
            filelock_release(ptr);
            ::free(ptr);
        }
    }
};

}

void MetricsUploader::uploadCompletedMetrics(IMetricsFileProcessor* processor,
                                             IMetricsReporter* reporter) {
    auto handle = shared_from_this();
    android::base::async([handle, processor, reporter]() {
        return MetricsUploader::uploadCompletedMetricsImpl(handle, processor,
                                                           reporter);
    });
}

void MetricsUploader::uploadCompletedMetrics() {
    uploadCompletedMetrics(new DefaultMetricsFileProcessor(),
                           new DefaultMetricsReporter());
}

// static
// This function runs on a worker thread.
// - Hold on to a shared_ptr for the backing object, to guarantee object
//   lifetime.
// - Take ownership of arguments to guarante object lifetime through the
//   function call.
bool MetricsUploader::uploadCompletedMetricsImpl(
        shared_ptr<MetricsUploader> handle,
        IMetricsFileProcessor* processor,
        IMetricsReporter* reporter) {
    unique_ptr<IMetricsFileProcessor> processorPtr(processor);
    unique_ptr<IMetricsReporter> reporterPtr(reporter);

    auto thisPtr = handle.get();
    if (!thisPtr->mInited) {
        return false;
    }

    bool success = true;
    int numProcessed = 0, numReported = 0;

    for (const auto& filePath :
         System::get()->scanDirEntries(thisPtr->mMetricsDir, true)) {
        // TODO(pprabhu) I don't trust our absolute path logic enough to always
        // return the same result. Instead, implement a function to check if two
        // file paths point to the same actual file.
        if (thisPtr->mFileSuffix != PathUtils::extension(filePath) ||
            0 == strcmp(thisPtr->mMetricsFilePath.c_str(), filePath.c_str())) {
            continue;
        }

        // This variable order is significant. We want the destruction to be in
        // the reverse order of allocation here.
        unique_ptr<const char, internal::FileDeleter> scopedFilePath(nullptr);
        unique_ptr<FileLock, internal::FileLockReleaser> fileLock(nullptr);
        unique_ptr<IniFile> iniFile(nullptr);

        // We may not get this |fileLock| if the emulator process that created
        // the metrics file is still running. This is by design -- we don't want
        // to process partial metrics files.
        fileLock.reset(filelock_create(filePath.c_str()));
        if (!fileLock) {
            continue;
        }
        ++numProcessed;

        // Delete the metrics dump on failed upload, noting that we missed the
        // metric. This protects us from leaving behind too many files if we
        // consistently fail to upload.
        scopedFilePath.reset(filePath.c_str());
        iniFile.reset(new IniFile(filePath.c_str()));

        if (!iniFile->read()) {
            success = false;
            continue;
        }

        auto urls = (*processor)(*iniFile);
        for (const auto& url : urls) {
            LOG(VERBOSE) << "Metrics url ping: " << url;
            String error;
            if ((*reporter)(url, &error)) {
                ++numReported;
            } else {
                LOG(WARNING) << "Failed to upload metrics: " << error;
                success = false;
            }
        }
    }

    LOG(INFO) << "metrics(" << thisPtr->mFileSuffix << "): Processed "
              << numProcessed << " reports.";
    LOG(INFO) << "metrics(" << thisPtr->mFileSuffix << "): Reported "
              << numReported << " successfully.";
    return success;
}

#if 0
        /* Quickly strdup the returned string to workaround a bug in
         * dirScanner. As the contents of the scanned directory change, the
         * pointer returned by dirScanner gets changed behind our back.
         * Even taking a filelock changes the directory contents...
         */
#endif

std::vector<base::String> DefaultMetricsFileProcessor::operator()(
        const IniFile& iniFile) const {
    bool first = true;
    String args;
    for (const auto& key : iniFile) {
        static const char kFirstFmt[] = "?%s=%s";
        static const char kRestFmt[] = "&%s=%s";
        args += StringFormat(first ? kFirstFmt : kRestFmt, Uri::Encode(key),
                             Uri::Encode(iniFile.getString(key, "")));
        first = false;
    }
    if (args.empty()) {
        return {};
    }

    static const char kToolbarUrl[] = "https://tools.google.com/service/update";
    auto url = String(kToolbarUrl);
    url += args;
    return {url};
}

bool DefaultMetricsReporter::operator()(StringView url,
                                        String* outError) const {
    char* cerror = nullptr;
    ScopedCPtr<char> cerrorScoped(cerror);
    auto ret = curl_download_null(url.c_str(), nullptr, true, &cerror);
    // TODO(pprabhu): Any way to avoid this copy?
    if (cerror) {
        *outError = cerror;
    }
    return ret;
}

}  // namespace metrics
}  // namespace android
