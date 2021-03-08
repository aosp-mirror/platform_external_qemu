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

// New tracing API that talks to an underlying tracing library, possibly perfetto.
//
// Sets up global state useful for tracing.
void initializeTracing();

// Enable/disable tracing
void enableTracing();
void disableTracing();

// Set the time of traces on the host to be at this guest time.
// Not needed if we assume timestamps can be transferrable (e.g.,
// when RDTSC with raw passthrough is used)
void setGuestTime(uint64_t guestTime);

// Record a counter of some kind.
void traceCounter(const char* tag, int64_t value);

void beginTrace(const char* name);
void endTrace();

class ScopedTrace {
public:
    ScopedTrace(const char* name);
    ~ScopedTrace();
};

class ScopedTraceDerived : public ScopedTrace {
public:
    void* member = nullptr;
};

void setGuestTime(uint64_t t);

void enableTracing();
void disableTracing();

bool shouldEnableTracing();

void traceCounter(const char* name, int64_t value);

} // namespace base
} // namespace android

#define __AEMU_GENSYM2(x,y) x##y
#define __AEMU_GENSYM1(x,y) __AEMU_GENSYM2(x,y)
#define AEMU_GENSYM(x) __AEMU_GENSYM1(x,__COUNTER__)

#define AEMU_SCOPED_TRACE(tag) __attribute__ ((unused)) android::base::ScopedTrace AEMU_GENSYM(aemuScopedTrace_)(tag)
#define AEMU_SCOPED_TRACE_CALL() AEMU_SCOPED_TRACE(__func__)
#define AEMU_SCOPED_THRESHOLD_TRACE_CALL()
#define AEMU_SCOPED_THRESHOLD_TRACE(...)
