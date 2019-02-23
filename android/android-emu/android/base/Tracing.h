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
        endTraceImpl();
    }
private:
    void beginTraceImpl(const char* name);
    void endTraceImpl();
};

bool shouldEnableTracing();
void beginTrace(const char* name);
void endTrace();

} // namespace base
} // namespace android

#define __AEMU_GENSYM2(x,y) x##y
#define __AEMU_GENSYM1(x,y) __AEMU_GENSYM2(x,y)
#define AEMU_GENSYM(x) __AEMU_GENSYM1(x,__COUNTER__)

#define AEMU_SCOPED_TRACE(tag) __attribute__ ((unused)) android::base::ScopedTrace AEMU_GENSYM(aemuScopedTrace_)(tag)