// Copyright (C) 2019 The Android Open Source Project
// Copyright (C) 2019 Google Inc.
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
#include "android/base/Tracing.h"

#include "android/base/system/System.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"
#include "android/base/threads/ThreadStore.h"

#include <string>
#include <vector>

namespace android {
namespace base {

static constexpr size_t kDefaultIndentLevels = 8;
static constexpr char kIndentString[] = "  ";

class IndentPrinter {
public:
    IndentPrinter() {
        ensureIndentLevel(kDefaultIndentLevels);
    }

    void begin(const char* name) {
        auto us = System::getSystemTimeUs();

        mTimes[mIndentLevel] = us;
        float ms = ((float)us) / 1000.0f;
        mNames[mIndentLevel] = name;

        printf("tid:0x%016llx:%s%s @ %f ms {\n",
               (unsigned long long)getCurrentThreadId(),
               mIndents[mIndentLevel].c_str(),
               name,
               ms);

        ++mIndentLevel;

        ensureIndentLevel(mIndentLevel);

    }

    void end() {
        --mIndentLevel;

        auto us = System::getSystemTimeUs();

        mTimes[mIndentLevel] = us - mTimes[mIndentLevel];
        float ms = (float)mTimes[mIndentLevel] / 1000.0f;

        printf("tid:0x%016llx:%s} %s for %f ms\n",
               (unsigned long long)getCurrentThreadId(),
               mIndents[mIndentLevel].c_str(),
               mNames[mIndentLevel].c_str(),
               ms);
    }

private:
    void ensureIndentLevel(size_t level) {
        if (mIndents.size() < level) {
            size_t prev_size = mIndents.size();
            mIndents.resize(level * 2);
            for (size_t i = prev_size;
                 i < mIndents.size(); ++i) {
                std::string s;
                for (size_t j = 0; j < i; ++j) {
                    s += kIndentString;
                }
                mIndents[i] = s;
            }
        }

        if (mTimes.size() < level) {
            mTimes.resize(level * 2, 0.0f);
        }

        if (mNames.size() < level) {
            mNames.resize(level * 2);
        }
    }

    size_t mIndentLevel = 0;
    std::vector<std::string> mIndents;
    std::vector<System::WallDuration> mTimes;
    std::vector<std::string> mNames;
};

class ThresholdPrinter {
public:
    ThresholdPrinter() {
        ensureIndentLevel(kDefaultIndentLevels);
    }

    void begin(const char* name, uint64_t thresholdUs) {
        auto us = System::getSystemTimeUs();

        mTimes[mIndentLevel] = us;
        mThresholds[mIndentLevel] = thresholdUs;
        float ms = ((float)us) / 1000.0f;
        mNames[mIndentLevel] = name;

        ++mIndentLevel;

        ensureIndentLevel(mIndentLevel);
    }

    void end() {
        --mIndentLevel;

        auto us = System::getSystemTimeUs();

        mTimes[mIndentLevel] = us - mTimes[mIndentLevel];

        if (mTimes[mIndentLevel] > mThresholds[mIndentLevel]) {
            float thresholdMs = mThresholds[mIndentLevel] / 1000.0f;
            float ms = mTimes[mIndentLevel] / 1000.0f;
            printf("tid:0x%016llx:%s [%s] over threshold %f ms: %f ms\n",
                   (unsigned long long)getCurrentThreadId(),
                   mIndents[mIndentLevel].c_str(),
                   mNames[mIndentLevel].c_str(),
                   thresholdMs, ms);
        }
    }

private:
    void ensureIndentLevel(size_t level) {
        if (mIndents.size() < level) {
            size_t prev_size = mIndents.size();
            mIndents.resize(level * 2);
            for (size_t i = prev_size;
                 i < mIndents.size(); ++i) {
                std::string s;
                for (size_t j = 0; j < i; ++j) {
                    s += kIndentString;
                }
                mIndents[i] = s;
            }
        }

        if (mTimes.size() < level) {
            mTimes.resize(level * 2, 0);
        }

        if (mThresholds.size() < level) {
            mThresholds.resize(level * 2, 0);
        }

        if (mNames.size() < level) {
            mNames.resize(level * 2);
        }
    }

    size_t mIndentLevel = 0;
    std::vector<std::string> mIndents;
    std::vector<System::WallDuration> mTimes;
    std::vector<System::WallDuration> mThresholds;
    std::vector<std::string> mNames;
};

typedef ThreadStore<IndentPrinter> IndentPrinterStore;
typedef ThreadStore<ThresholdPrinter> ThresholdPrinterStore;

static LazyInstance<IndentPrinterStore> sIndentPrinterStore =
    LAZY_INSTANCE_INIT;
static LazyInstance<ThresholdPrinterStore> sThresholdPrinterStore =
    LAZY_INSTANCE_INIT;

static IndentPrinter* indentPrinter_getForThread() {
    auto state = sIndentPrinterStore.ptr();

    auto printer = state->get();
    if (!printer) {
        printer = new IndentPrinter;
        state->set(printer);
    }

    return state->get();
}

static void indentTrace_begin(const char* name) {
    indentPrinter_getForThread()->begin(name);
}

static void indentTrace_end() {
    indentPrinter_getForThread()->end();
}

static ThresholdPrinter* thresholdPrinter_getForThread() {
    auto state = sThresholdPrinterStore.ptr();

    auto printer = state->get();
    if (!printer) {
        printer = new ThresholdPrinter;
        state->set(printer);
    }

    return state->get();
}

static void thresholdTrace_begin(const char* name, uint64_t thresholdUs) {
    thresholdPrinter_getForThread()->begin(name, thresholdUs);
}

static void thresholdTrace_end() {
    thresholdPrinter_getForThread()->end();
}

class TraceConfig {
public:
    TraceConfig() {
        mEnabled =
            System::getEnvironmentVariable(
                "ANDROID_EMU_TRACING") == "1";
    }

    bool enabled() const { return mEnabled; }

private:
    bool mEnabled = false;
};

static LazyInstance<TraceConfig> sTraceConfig =
    LAZY_INSTANCE_INIT;

bool shouldEnableTracing() {
    return sTraceConfig->enabled();
}

#ifdef __cplusplus
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

void ScopedTrace::beginTraceImpl(const char* name) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        indentTrace_begin(name);
    }
}

void ScopedTrace::endTraceImpl(const char*) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        indentTrace_end();
    }
}

void beginTrace(const char* name) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        indentTrace_begin(name);
    }
}

void endTrace() {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        indentTrace_end();
    }
}

void ScopedThresholdTrace::beginTraceImpl(const char* name, uint64_t thresholdUs) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        thresholdTrace_begin(name, thresholdUs);
    }
}

void ScopedThresholdTrace::endTraceImpl(const char*) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        thresholdTrace_end();
    }
}

void beginThresholdTrace(const char* name, uint64_t thresholdUs) {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        thresholdTrace_begin(name, thresholdUs);
    }
}

void endThresholdTrace() {
    if (CC_UNLIKELY(shouldEnableTracing())) {
        thresholdTrace_end();
    }
}

} // namespace base
} // namespace android
