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
#pragma once

#include <inttypes.h>

// Library to perform tracing. Talks to platform-specific
// tracing libraries.
namespace android {
namespace base {

class ScopedTrace {
public:
    ScopedTrace(const char* name) {
        beginTraceImpl(name);
    }

    ~ScopedTrace() {
        endTraceImpl(name_);
    }
private:
    void beginTraceImpl(const char* name);
    void endTraceImpl(const char* name);
    const char* const name_ = nullptr;
};

bool shouldEnableTracing();
void beginTrace(const char* name);
void endTrace();

class ScopedThresholdTrace {
public:
    ScopedThresholdTrace(const char* name, uint64_t thresholdUs = 1000) {
        beginTraceImpl(name, thresholdUs);
    }

    ~ScopedThresholdTrace() {
        endTraceImpl(name_);
    }
private:
    void beginTraceImpl(const char* name, uint64_t thresholdUs);
    void endTraceImpl(const char* name);
    const char* const name_ = nullptr;
};

void beginThresholdTrace(const char* name, uint64_t thresholdUs = 1000);
void endThresholdTrace();

} // namespace base
} // namespace android

#define __AEMU_GENSYM2(x,y) x##y
#define __AEMU_GENSYM1(x,y) __AEMU_GENSYM2(x,y)
#define AEMU_GENSYM(x) __AEMU_GENSYM1(x,__COUNTER__)

#define AEMU_SCOPED_TRACE(tag) __attribute__ ((unused)) android::base::ScopedTrace AEMU_GENSYM(aemuScopedTrace_)(tag)
#define AEMU_SCOPED_THRESHOLD_TRACE(tag) __attribute__ ((unused)) android::base::ScopedThresholdTrace AEMU_GENSYM(aemuScopedTrace_)(tag)
#define AEMU_SCOPED_THRESHOLD_TRACE_TIMED(tag, thresholdUs) __attribute__ ((unused)) android::base::ScopedThresholdTrace AEMU_GENSYM(aemuScopedTrace_)(tag, thresholdUs)

#define AEMU_SCOPED_TRACE_CALL() AEMU_SCOPED_TRACE(__func__)
#define AEMU_SCOPED_THRESHOLD_TRACE_CALL() AEMU_SCOPED_THRESHOLD_TRACE(__func__)
