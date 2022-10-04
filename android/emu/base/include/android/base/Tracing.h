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

#if defined(AEMU_TRACING_SHARED) && defined(_MSC_VER)
#if defined(TRACING_EXPORTS)
#define TRACING_API __declspec(dllexport)
#define TRACING_API_TEMPLATE_DECLARE
#define TRACING_API_TEMPLATE_DEFINE __declspec(dllexport)
#else
#define TRACING_API __declspec(dllimport)
#define TRACING_API_TEMPLATE_DECLARE
#define TRACING_API_TEMPLATE_DEFINE __declspec(dllimport)
#endif  // defined(TRACING_EXPORTS)
#elif defined(AEMU_UI_SHARED)
#define TRACING_API __attribute__((visibility("default")))
#define TRACING_API_TEMPLATE_DECLARE __attribute__((visibility("default")))
#define TRACING_API_TEMPLATE_DEFINE
#else
#define TRACING_API
#define TRACING_API_TEMPLATE_DECLARE
#define TRACING_API_TEMPLATE_DEFINE
#endif
// Library to perform tracing. Talks to platform-specific
// tracing libraries.
namespace android {
namespace base {

// New tracing API that talks to an underlying tracing library, possibly
// perfetto.
//
// Sets up global state useful for tracing.
TRACING_API void initializeTracing();

// Enable/disable tracing
TRACING_API void enableTracing();
TRACING_API void disableTracing();

// Set the time of traces on the host to be at this guest time.
// Not needed if we assume timestamps can be transferrable (e.g.,
// when RDTSC with raw passthrough is used)
TRACING_API void setGuestTime(uint64_t guestTime);

// Record a counter of some kind.
TRACING_API void traceCounter(const char* tag, int64_t value);

TRACING_API void beginTrace(const char* name);
TRACING_API void endTrace();
TRACING_API bool shouldEnableTracing();

class TRACING_API ScopedTrace {
public:
    ScopedTrace(const char* name);
    ~ScopedTrace();
};

class ScopedTraceDerived : public ScopedTrace {
public:
    void* member = nullptr;
};
}  // namespace base
}  // namespace android

#define __AEMU_GENSYM2(x, y) x##y
#define __AEMU_GENSYM1(x, y) __AEMU_GENSYM2(x, y)
#define AEMU_GENSYM(x) __AEMU_GENSYM1(x, __COUNTER__)

#define AEMU_SCOPED_TRACE(tag) \
    __attribute__((unused))    \
    android::base::ScopedTrace AEMU_GENSYM(aemuScopedTrace_)(tag)
#define AEMU_SCOPED_TRACE_CALL() AEMU_SCOPED_TRACE(__func__)
#define AEMU_SCOPED_THRESHOLD_TRACE_CALL()
#define AEMU_SCOPED_THRESHOLD_TRACE(...)
