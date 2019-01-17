// Copyright 2019 The Android Open Source Project
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
#pragma once

#include "android/base/Optional.h"

#include <string>
#include <map>

namespace android {
namespace perflogger {

class Analyzer;

class Benchmark {
public:
    using Metadata = std::map<std::string, std::string>;

    Benchmark(
        const std::string& benchmarkName,
        const std::string& projectName,
        const std::string& description,
        const Metadata& metadata);

    Benchmark(
        const std::string& customOutputDir,
        const std::string& benchmarkName,
        const std::string& projectName,
        const std::string& description,
        const Metadata& metadata);

    ~Benchmark();

    base::Optional<std::string> getCustomOutputDir() const;
    std::string getName() const;
    std::string getProjectName() const;
    std::string getDescription() const;
    const Metadata& getMetadata() const;

    void log(const std::string& metricName, long data);
    void log(const std::string& metricName, long data, Analyzer* analyzer);

    bool operator==(const Benchmark& other) const;

private:
    std::string mName;
    std::string mProjectName;
    std::string mDescription;
    Metadata mMetadata;

    base::Optional<std::string> mCustomOutputDir = {};
};

} // namespace perflogger
} // namespace android