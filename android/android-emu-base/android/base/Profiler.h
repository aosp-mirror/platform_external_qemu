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
#include "android/base/StringView.h"
#include "android/base/system/System.h"

#include <functional>
#include <string>

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
    using Callback = std::function<void(StringView, StringView,
                                        MemoryProfiler::MemoryUsageBytes,
                                        MemoryProfiler::MemoryUsageBytes)>;

    ScopedMemoryProfiler(StringView tag) : mProfiler(), mTag(tag) {
        mProfiler.start();
        check("(start)");
    }

    ScopedMemoryProfiler(StringView tag, Callback c)
        : mProfiler(), mTag(tag), mCallback(c) {
        mProfiler.start();
        check("(start)");
    }

    void check(StringView stage = "") {
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

    Callback mCallback = { [](StringView tag, StringView stage,
                              MemoryProfiler::MemoryUsageBytes currentResident,
                              MemoryProfiler::MemoryUsageBytes change) {
        double megabyte = 1024.0 * 1024.0;
        fprintf(stderr, "%s %s: %f mb current. change: %f mb\n",
                c_str(tag).get(), c_str(stage).get(),
                (double)currentResident / megabyte, (double)change / megabyte);
    }};

    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedMemoryProfiler);
};

}  // namespace base
}  // namespace android
