// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/base/async/Looper.h"
#include "android/base/Compiler.h"
#include "android/base/String.h"
#include "android/base/StringView.h"
#include "android/base/files/IniFile.h"
#include "android/metrics/IniFileAutoFlusher.h"

#include <memory>
#include <vector>

namespace android {
namespace metrics {

// An object that processes a single completed metrics IniFile and prepares the
// URLs to be pinged to our remote server.
class IMetricsFileProcessor {
public:
    // This function "consumes" a single IniFile and returns a vector of URLs
    // that we should POST to.
    virtual std::vector<base::String> operator()(
            const android::base::IniFile& iniFile) const = 0;
};

// An object that accepts a completely processed URL to be used for reporting
// metrics and takes the last mile effort of actually posting the metrics.
// Mostly useful for easy unittesting.
// |*outError| must by non-null and is used to return errors.
class IMetricsReporter {
public:
    virtual bool operator()(base::StringView url,
                            base::String* outError) const = 0;
};

class MetricsUploader : public std::enable_shared_from_this<MetricsUploader> {
public:
    // Create a new MetricsUploader.
    //
    // - Generates a unique path on disk where metrics will be dropped for this
    // run.
    // - Creates an |android::base::IniFile| backed by a file at the generated
    // path.
    //
    // Returns: A handle to a brand new, initialized object on success, nullptr
    //         on failure.
    static std::shared_ptr<MetricsUploader> create(base::Looper* looper,
                                                   base::StringView avdHome,
                                                   base::StringView fileSuffix);

    // Returns a handle to the |android::base::IniFile| to drop the metrics
    // into. This object is owned by the |MetricsUploader|.
    // Returns |nullptr| if a valid file does not exist.
    base::IniFile* metricsFile();

    // Takes an object that can marshal individual metrics files and uses it to
    // report all completed metrics.
    // - This will only report completed metrics (any concurrently running
    //   emulator instances are not affected).
    // - Only one of concurrently running emulator instances will ever report
    //   from any metrics file.
    //
    // NOTE: Takes ownership of arguments to guarantee lifetime in the thread
    // launched for this task.
    void uploadCompletedMetrics(IMetricsFileProcessor* processor,
                                IMetricsReporter* reporter);
    // Same, with default args.
    void uploadCompletedMetrics();

protected:
    MetricsUploader(base::Looper* looper,
                    base::StringView avdHome,
                    base::StringView fileSuffix);
    bool init();
    static bool uploadCompletedMetricsImpl(
            std::shared_ptr<MetricsUploader> handle,
            IMetricsFileProcessor* processor,
            IMetricsReporter* reporter);

private:
    bool mInited = false;
    const base::String mAvdHome;
    const base::String mFileSuffix;
    base::String mMetricsDir;
    base::String mMetricsFilePath;
    IniFileAutoFlusher mMetricsFileFlusher;

    DISALLOW_COPY_AND_ASSIGN(MetricsUploader);
};

class DefaultMetricsFileProcessor final : public IMetricsFileProcessor {
    std::vector<base::String> operator()(
            const android::base::IniFile& iniFile) const override;
};

// The default implemenation that uses curl to do an HTTP post query.
class DefaultMetricsReporter : public IMetricsReporter {
public:
    bool operator()(base::StringView url,
                    base::String* outError) const override;
};

}  // namespace metrics
}  // namespace android
