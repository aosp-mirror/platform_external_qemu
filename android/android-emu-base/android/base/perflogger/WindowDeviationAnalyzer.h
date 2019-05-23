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

#include <vector>

namespace android {
namespace base {

class JsonWriter;

} // namespace base
} // namespace android

namespace android {
namespace perflogger {

class WindowDeviationAnalyzer : public Analyzer {
public:
    struct MedianToleranceParams {
        double constTerm;
        double medianCoeff;
        double madCoeff;
    };

    struct MeanToleranceParams {
        double constTerm;
        double meanCoeff;
        double stddevCoeff;
    };

    WindowDeviationAnalyzer(
        MetricAggregate aggregate,
        int runInfoQueryLimit,
        int recentWindowSize,
        const std::vector<MeanToleranceParams>& meanTolerances,
        const std::vector<MedianToleranceParams>& medianTolerances);

    WindowDeviationAnalyzer(
        const WindowDeviationAnalyzer& other);

    void outputJson(base::JsonWriter*) override;

private:
    MetricAggregate mAggregate;
    int mRunInfoQueryLimit;
    int mRecentWindowSize;
    std::vector<MeanToleranceParams> mMeanTolerances;
    std::vector<MedianToleranceParams> mMedianTolerances;
};

} // namespace perflogger
} // namespace android