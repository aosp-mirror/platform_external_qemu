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
#include "android/base/perflogger/Metric.h"

#include "android/base/files/PathUtils.h"
#include "android/base/JsonWriter.h"
#include "android/base/perflogger/Benchmark.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"

using namespace android::base;
using namespace android::perflogger;

// static
std::string Metric::getPreferredOutputDirectory() {
    auto distDirEnv = System::getEnvironmentVariable("DIST_DIR");
    if (distDirEnv != "") return distDirEnv;
    return pj(System::getProgramDirectoryFromPlatform(), "perfgate");
}

Metric::Metric(const std::string& metricName)
    : mName(metricName),
      mOutputDirectory(Metric::getPreferredOutputDirectory()) {}

Metric::Metric(const std::string& metricName, const std::string& outputDir)
    : mName(metricName), mOutputDirectory(outputDir) {}

std::string Metric::getMetricName() const {
    return mName;
}

std::string Metric::getOutputDirectory() const {
    return mOutputDirectory;
}

void Metric::addSamples(Benchmark* benchmark,
                        const std::vector<MetricSample>& data) {
    auto& currSamples = mSamples[benchmark];
    currSamples.insert(currSamples.end(), data.begin(), data.end());
}

void Metric::setAnalyzers(Benchmark* benchmark,
                          const std::vector<Analyzer> &analyzers) {
    mAnalyzers[benchmark] = analyzers;
}

void Metric::commit() {
    if (mSamples.empty())
        return;

    fprintf(stderr, "%s: out dir: %s\n", __func__, mOutputDirectory.c_str());

    std::string outputPath = pj(mOutputDirectory, mName + ".json");
    path_mkdir_if_needed(mOutputDirectory.c_str(), 0755);
    JsonWriter writer(outputPath);

    writer.setIndent("  ");
    writer.beginObject()
            .name("metric")
            .value(mName)
            .name("benchmarks")
            .beginArray();
    {
        for (const auto it : mSamples) {
            Benchmark* benchmark = it.first;
            const auto& samples = it.second;
            if (samples.empty()) {
                continue;
            }
            writer.beginObject()
                   .name("benchmark")
                   .value(benchmark->getName())
                   .name("project")
                   .value(benchmark->getProjectName())
                   .name("data");
            {
                writer.beginObject();
                for (MetricSample sample : samples) {
                    writer.nameAsStr(sample.timestampMs)
                          .value(sample.data);
                }
                writer.endObject();
            }

            const auto& analyzers = mAnalyzers[benchmark];
            if (!analyzers.empty()) {
                writer.name("analyzers").beginArray();
                for (auto a : analyzers) {
                    a.outputJson(&writer);
                }
                writer.endArray();
            }
            writer.endObject();
        }
        writer.endArray();
    }
    writer.endObject();
    writer.flush();
}