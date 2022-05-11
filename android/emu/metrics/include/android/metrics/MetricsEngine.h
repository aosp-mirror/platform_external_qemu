// Copyright (C) 2015 The Android Open Source Project
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

#include <memory>
#include <string>
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/export.h"

namespace android {
namespace metrics {

class AEMU_METRICS_API MetricsEngine {
public:
    virtual void registerReporter(std::shared_ptr<Reporter> reporter) = 0;
    virtual void stop() = 0;

    virtual void setEmulatorName(std::string name) = 0;
    virtual std::string emulatorName() = 0;
    static MetricsEngine* get();
};

}  // namespace metrics
}  // namespace android