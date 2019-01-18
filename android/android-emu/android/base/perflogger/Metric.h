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

#include "android/base/perflogger/Analyzer.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace android {
namespace perflogger {

class Benchmark;

class Metric {
public:
    struct MetricSample {
        long timestampMs;
        long data;
    };

    static std::string getPreferredOutputDirectory();

    Metric(const std::string& metricName);
    Metric(const std::string& metricName, const std::string& outputDir);

    std::string getMetricName() const;
    std::string getOutputDirectory() const;

    void addSamples(Benchmark* benchmark,
                    const std::vector<MetricSample>& data);

    void setAnalyzers(Benchmark* benchmark,
                      const std::vector<Analyzer>& analyzers);

    void commit();

private:
    std::string mName;
    std::string mOutputDirectory;
    std::unordered_map<Benchmark*, std::vector<MetricSample>> mSamples;
    std::unordered_map<Benchmark*, std::vector<Analyzer>> mAnalyzers;
};

} // namespace perflogger
} // namespace android