// Copyright 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"
#include "android/base/CpuTime.h"

#include <functional>
#include <memory>
#include <string>

#include <inttypes.h>

namespace android {
namespace base {

class Looper;

class CpuUsage {
    DISALLOW_COPY_ASSIGN_AND_MOVE(CpuUsage);
public:
    enum UsageArea {
        MainLoop = 0,
        Vcpu = 16,
        RenderThreads = 128,
        Max = 512,
    };

    using IntervalUs = int64_t;
    using CpuTimeReader = std::function<void(const CpuTime& cputime)>;

    CpuUsage();

    static CpuUsage* get();

    void addLooper(int usageArea, Looper* looper);
    void setEnabled(bool enable);
    void setMeasurementInterval(IntervalUs interval);

    // Every 10 seconds
    static constexpr CpuUsage::IntervalUs kDefaultMeasurementIntervalUs = 10ULL * 1000000ULL;

    void forEachUsage(UsageArea area, CpuTimeReader readerFunc);
    float getSingleAreaUsage(int area);
    float getTotalMainLoopAndVcpuUsage();
    std::string printUsage();

    void stop();
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace base
} // namespace android
