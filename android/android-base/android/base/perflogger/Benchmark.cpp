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
#include "android/base/perflogger/Benchmark.h"

#include "android/base/perflogger/Metric.h"

#include "android/base/system/System.h"

#include <memory>
#include <vector>

using android::base::System;

using namespace android::perflogger;

Benchmark::Benchmark(const std::string& benchmarkName,
                     const std::string& projectName,
                     const std::string& description,
                     const Benchmark::Metadata& metadata)
    : mName(benchmarkName),
      mProjectName(projectName),
      mDescription(description),
      mMetadata(metadata) {}

Benchmark::Benchmark(const std::string& customOutputDir,
                     const std::string& benchmarkName,
                     const std::string& projectName,
                     const std::string& description,
                     const Benchmark::Metadata& metadata)
    : mName(benchmarkName),
      mProjectName(projectName),
      mDescription(description),
      mMetadata(metadata),
      mCustomOutputDir(customOutputDir) {}

Benchmark::~Benchmark() = default;

android::base::Optional<std::string>
Benchmark::getCustomOutputDir() const {
    return mCustomOutputDir;
}

std::string Benchmark::getName() const { return mName; }
std::string Benchmark::getProjectName() const { return mProjectName; }
std::string Benchmark::getDescription() const { return mDescription; }
const Benchmark::Metadata& Benchmark::getMetadata() const { return mMetadata; }

void Benchmark::log(const std::string& metricName, long data) {
    log(metricName, data, nullptr);
}

void Benchmark::log(const std::string& metricName,
                    long data,
                    Analyzer* analyzer) {
    long utcMs = System::get()->getUnixTimeUs() / 1000;

    std::unique_ptr<Metric> metric;

    if (mCustomOutputDir) {
        metric.reset(new Metric(metricName, *mCustomOutputDir));
    } else {
        metric.reset(new Metric(metricName));
    }

    Metric::MetricSample sample;
    sample.timestampMs = utcMs;
    sample.data = data;

    std::vector<Metric::MetricSample> samples;
    samples.push_back(sample);

    metric->addSamples(this, samples);

    if (analyzer) {
        std::vector<Analyzer> analyzers;
        analyzers.push_back(*analyzer);
        metric->setAnalyzers(this, analyzers);
    }

    metric->commit();
}