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
#include "android/base/perflogger/WindowDeviationAnalyzer.h"

#include "android/base/JsonWriter.h"

#include <sstream>

using namespace android::base;
using namespace android::perflogger;

WindowDeviationAnalyzer::WindowDeviationAnalyzer(
        MetricAggregate aggregate,
        int runInfoQueryLimit,
        int recentWindowSize,
        const std::vector<MeanToleranceParams>& meanTolerances,
        const std::vector<MedianToleranceParams>& medianTolerances)
    : mAggregate(aggregate),
      mRunInfoQueryLimit(runInfoQueryLimit),
      mRecentWindowSize(recentWindowSize),
      mMeanTolerances(meanTolerances),
      mMedianTolerances(medianTolerances) {}

WindowDeviationAnalyzer::WindowDeviationAnalyzer(
        const WindowDeviationAnalyzer& other)
    : mAggregate(other.mAggregate),
      mRunInfoQueryLimit(other.mRunInfoQueryLimit),
      mRecentWindowSize(other.mRecentWindowSize),
      mMeanTolerances(other.mMeanTolerances),
      mMedianTolerances(other.mMedianTolerances) {}

void WindowDeviationAnalyzer::outputJson(JsonWriter* writerPtr) {
    auto& writer = *writerPtr;

    writer.beginObject();
    writer.name("type").value("WindowDeviationAnalyzer");
    switch (mAggregate) {
        case MetricAggregate::Mean:
            writer.name("metricAggregate").value("Mean");
            break;
        case MetricAggregate::Median:
            writer.name("metricAggregate").value("Median");
            break;
        case MetricAggregate::Min:
            writer.name("metricAggregate").value("Min");
            break;
        case MetricAggregate::Max:
            writer.name("metricAggregate").value("Max");
            break;
        default:
            writer.name("metricAggregate").value("Unknown");
    }

    writer.name("runInfoQueryLimit").valueAsStr(mRunInfoQueryLimit);
    writer.name("recentWindowSize").valueAsStr(mRecentWindowSize);
    writer.name("toleranceParams").beginArray();
    {
        for (const auto& param : mMeanTolerances) {
            writer.beginObject();
            writer.name("type").value("Mean");
            writer.name("constTerm").valueAsStr(param.constTerm);
            writer.name("meanCoeff").valueAsStr(param.meanCoeff);
            writer.name("stddevCoeff").valueAsStr(param.stddevCoeff);
            writer.endObject();
        }
        for (const auto& param : mMedianTolerances) {
            writer.beginObject();
            writer.name("type").value("Median");
            writer.name("constTerm").valueAsStr(param.constTerm);
            writer.name("medianCoeff").valueAsStr(param.medianCoeff);
            writer.name("madCoeff").valueAsStr(param.madCoeff);
            writer.endObject();
        }
    }
    writer.endArray();
    writer.endObject();
}