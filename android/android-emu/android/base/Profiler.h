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
#include "android/base/system/System.h"

#include <functional>
#include <string>

namespace android {
namespace base {

class Profiler {
public:
    Profiler() = default;
    void start() {
        mStartUs = System::get()->getHighResTimeUs();
    }
    uint64_t elapsedUs() {
        return System::get()->getHighResTimeUs() - mStartUs;
    }
private:
    uint64_t mStartUs = 0;
    DISALLOW_COPY_ASSIGN_AND_MOVE(Profiler);
};

class ScopedProfiler {
public:
    using Callback = std::function<void(const char*, uint64_t)>;
    ScopedProfiler(const std::string& tag, Callback c) :
        mProfiler(), mTag(tag), mCallback(c) {
        mProfiler.start();
    }
    ~ScopedProfiler() {
        mCallback(mTag.c_str(), mProfiler.elapsedUs());
    }
private:
    Profiler mProfiler;
    std::string mTag;
    Callback mCallback;
    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedProfiler);
};

#define PROFILE_CALL_STDERR(tag) \
    android::base::ScopedProfiler __scoped_profiler(tag, [](const char* tag2, uint64_t elapsed) \
            { fprintf(stderr, "%s: %" PRIu64 " us\n", tag2, elapsed); }); \

class MemoryProfiler {
public:
    MemoryProfiler() = default;

    void start() {
        auto memUsage = System::get()->getMemUsage();
        mStartResident = memUsage.resident;
    }

    int64_t queryCurrentResident() {
        auto memUsage = System::get()->getMemUsage();
        return memUsage.resident;
    }

    int64_t queryStartResident() {
        return mStartResident;
    }

private:
    int64_t mStartResident = 0;
    DISALLOW_COPY_ASSIGN_AND_MOVE(MemoryProfiler);
};

class ScopedMemoryProfiler {
public:
    using Callback = std::function<void(const char*, int64_t, int64_t)>;

    ScopedMemoryProfiler(const std::string& tag) :
        mProfiler(), mTag(tag) {
        mProfiler.start();
        check("(start)");
    }

    ScopedMemoryProfiler(const std::string& tag, Callback c) :
        mProfiler(), mTag(tag), mCallback(c) {
        mProfiler.start();
        check("(end)");
    }

    void check(const std::string& tag2 = "") {
        int64_t currRes = mProfiler.queryCurrentResident();
        mCallback((mTag + tag2).c_str(),
                  currRes, currRes - mProfiler.queryStartResident());
    }

    ~ScopedMemoryProfiler() {
        check();
    }

private:
    MemoryProfiler mProfiler;
    std::string mTag;

    Callback mCallback = { [](const char* tag, int64_t currentResident, int64_t change) {
        fprintf(stderr, "%s: %f mb current. change: %f mb\n",
                tag, (float)currentResident / 1048675.0f, (float)change / 1048576.0f);
    }};

    DISALLOW_COPY_ASSIGN_AND_MOVE(ScopedMemoryProfiler);
};

#define PROFILE_CALL_MEMORY_STDERR(tag) \
    android::base::ScopedMemoryProfiler __scoped_mem_profiler(tag, [](const char* tag2, int64_t currentResident, int64_t change) \
            { fprintf(stderr, "%s: %f mb current, change %f mb since start\n", tag2, (float)currentResident / 1048675.0f, (float)change / 1048576.0f); } \


}  // namespace base
}  // namespace android
