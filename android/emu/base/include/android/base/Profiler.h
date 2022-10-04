// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#include <functional>
#include <string>
#include <string_view>

namespace android {
namespace base {

class MemoryProfiler {
public:
    using MemoryUsageBytes = int64_t;

    MemoryProfiler() = default;

    void start() {
        mStartResident = queryCurrentResident();
    }

    MemoryUsageBytes queryCurrentResident() {
        auto memUsage = System::get()->getMemUsage();
        return (MemoryUsageBytes)memUsage.resident;
    }

    MemoryUsageBytes queryStartResident() {
        return mStartResident;
    }

private:
    MemoryUsageBytes mStartResident = 0;
    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryProfiler);
};

class ScopedMemoryProfiler {
public:
    using Callback = std::function<void(std::string_view,
                                        std::string_view,
                                        MemoryProfiler::MemoryUsageBytes,
                                        MemoryProfiler::MemoryUsageBytes)>;

    ScopedMemoryProfiler(std::string_view tag) : mProfiler(), mTag(tag) {
        mProfiler.start();
        check("(start)");
    }

    ScopedMemoryProfiler(std::string_view tag, Callback c)
        : mProfiler(), mTag(tag), mCallback(c) {
        mProfiler.start();
        check("(start)");
    }

    void check(std::string_view stage = "") {
        int64_t currRes = mProfiler.queryCurrentResident();
        mCallback(mTag.c_str(), stage,
                  currRes, currRes - mProfiler.queryStartResident());
    }

    ~ScopedMemoryProfiler() {
        check("(end)");
    }

private:
    MemoryProfiler mProfiler;
    std::string mTag;

    Callback mCallback = {[](std::string_view tag,
                             std::string_view stage,
                             MemoryProfiler::MemoryUsageBytes currentResident,
                             MemoryProfiler::MemoryUsageBytes change) {
        double megabyte = 1024.0 * 1024.0;
        derror("%s %s: %f mb current. change: %f mb",
                c_str(tag).get(), c_str(stage).get(),
                (double)currentResident / megabyte, (double)change / megabyte);
    }};

    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedMemoryProfiler);
};

}  // namespace base
}  // namespace android
