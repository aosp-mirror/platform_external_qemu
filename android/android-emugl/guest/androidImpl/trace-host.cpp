/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cutils/trace.h>

#include "aemu/base/Tracing.h"

atomic_bool             atrace_is_ready      = ATOMIC_VAR_INIT(false);
int                     atrace_marker_fd     = -1;
uint64_t                atrace_enabled_tags  = 0;

void atrace_set_debuggable(bool /*debuggable*/) {}
void atrace_set_tracing_enabled(bool /*enabled*/) {}
void atrace_update_tags() { }

void atrace_setup() {
    if (android::base::shouldEnableTracing()) {
        atrace_enabled_tags =
            ATRACE_TAG_ALWAYS |
            ATRACE_TAG_GRAPHICS |
            ATRACE_TAG_INPUT |
            ATRACE_TAG_AUDIO |
            ATRACE_TAG_VIDEO |
            ATRACE_TAG_HAL;
    }
    atomic_store_explicit(&atrace_is_ready, true, memory_order_release);
}

void atrace_begin_body(const char* name) {
    android::base::beginTrace(name);
}

void atrace_end_body() {
    android::base::endTrace();
}

void atrace_async_begin_body(const char* /*name*/, int32_t /*cookie*/) {}
void atrace_async_end_body(const char* /*name*/, int32_t /*cookie*/) {}
void atrace_int_body(const char* /*name*/, int32_t /*value*/) {}
void atrace_int64_body(const char* /*name*/, int64_t /*value*/) {}
