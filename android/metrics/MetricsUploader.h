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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/files/IniFile.h"

#include <string>
#include <vector>


namespace android {
namespace metrics {

enum class MetricsType {
    kQtToolWindow
};

}  // namespace metrics
}  // namespace android

namespace std {

template<>
struct hash<android::metrics::MetricsType> {
public:
    std::size_t operator() (const android::metrics::MetricsType& type) const {
        return static_cast<std::size_t>(type);
    }
};

}  // namespace std

namespace android {
namespace metrics {

// An object that processes a single completed metrics IniFile and prepares the
// URLs to be pinged to our remote server.
class IMetricsFileProcessor {
public:
    // This function "consumes" a single IniFile and returns a vector of URLs
    // that we should POST to.
    virtual std::vector<std::string> operator() (const android::base::IniFile& iniFile) const = 0;
};

class IMetricsUploader {
public:
    IMetricsUploader(MetricsType type,
                     const std::string& avdHome) : mType(type), mAvdHome(avdHome) {}
    // Generates a unique path for the given metrics type.
    // Use this to create a new metrics file.
    // - When called the first time, this object guarantees that the path does
    // not exist.
    // - When called repeatedly, the same path is returned for this process.
    //
    // Returns empty string iff an error occurs.
    virtual std::string generateUniquePath() = 0;

    // Takes an object that can marshal individual metrics files and uses it to
    // report all completed metrics.
    // - This will only report completed metrics (any concurrently running
    //   emulator instances are not affected).
    // - Only one of concurrently running instances will ever report from any
    //   metrics file.
    virtual bool uploadCompletedMetrics(const IMetricsFileProcessor& processor) = 0;

    virtual ~IMetricsUploader() = default;

protected:
    MetricsType type() const { return mType; }
    const std::string& avdHome() const { return mAvdHome; }

private:
    MetricsType mType;
    std::string mAvdHome;
};

class MetricsUploader : public IMetricsUploader {
public:
    MetricsUploader(MetricsType type, std::string avdHome);

    std::string generateUniquePath() override;
    bool uploadCompletedMetrics(const IMetricsFileProcessor& processor) override;

protected:
    bool makeGetRequest(const std::string& url);

private:
    std::string mMetricsDir;
    std::string mMetricsFilePath;

    DISALLOW_COPY_AND_ASSIGN(MetricsUploader);
};

class DefaultMetricsFileProcessor : public IMetricsFileProcessor {
    std::vector<std::string> operator() (const android::base::IniFile& iniFile) const override;
};

}  // namespace metrics
}  // namespace android

