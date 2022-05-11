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

#include "perfetto-tracing-only.h"

#include <string>
#include <vector>

#include <fcntl.h>

namespace android {
namespace base {

const bool* tracingDisabledPtr = nullptr;

void initializeTracing() {
    virtualdeviceperfetto::initialize(&tracingDisabledPtr);
}

void enableTracing() {
    if (virtualdeviceperfetto::queryTraceConfig().tracingDisabled) {
        virtualdeviceperfetto::enableTracing();
    }
}

void disableTracing() {
    if (!virtualdeviceperfetto::queryTraceConfig().tracingDisabled) {
        virtualdeviceperfetto::disableTracing();
    }
}

bool shouldEnableTracing() {
    return !(virtualdeviceperfetto::queryTraceConfig().tracingDisabled);
}

#ifdef __cplusplus
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

__attribute__((always_inline)) void beginTrace(const char* name) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    virtualdeviceperfetto::beginTrace(name);
}

__attribute__((always_inline)) void endTrace() {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    virtualdeviceperfetto::endTrace();
}

__attribute__((always_inline)) void traceCounter(const char* name, int64_t value) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    virtualdeviceperfetto::traceCounter(name, value);
}

ScopedTrace::ScopedTrace(const char* name) {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    virtualdeviceperfetto::beginTrace(name);
}

ScopedTrace::~ScopedTrace() {
    if (CC_LIKELY(*tracingDisabledPtr)) return;
    virtualdeviceperfetto::endTrace();
}

void setGuestTime(uint64_t t) {
    virtualdeviceperfetto::setGuestTime(t);
}

} // namespace base
} // namespace android
