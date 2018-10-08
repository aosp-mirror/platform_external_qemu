// Copyright (C) 2018 The Android Open Source Project
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
#include "AndroidWindowBuffer.h"

#include <stdio.h>

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace aemu {

static void hook_incRef(struct android_native_base_t* common) {
    (void)common;
    E("Not implemented");
}

static void hook_decRef(struct android_native_base_t* common) {
    (void)common;
    E("Not implemented");
}

AndroidWindowBuffer::AndroidWindowBuffer(AndroidWindow* window,
                                         buffer_handle_t buffer,
                                         int usage,
                                         int format,
                                         int stride)
    : AndroidWindowBuffer(window->width,
                          window->height,
                          buffer,
                          usage,
                          format,
                          stride) {}

AndroidWindowBuffer::AndroidWindowBuffer(int _width,
                                         int _height,
                                         buffer_handle_t buffer,
                                         int usage,
                                         int format,
                                         int stride)
    : ANativeWindowBuffer() {
    this->width = _width;
    this->height = _height;
    this->stride = stride;
    this->format = format;
    this->usage_deprecated = usage;
    this->usage = usage;

    this->handle = buffer;

    this->common.incRef = hook_incRef;
    this->common.decRef = hook_decRef;
}

} // namespace aemu