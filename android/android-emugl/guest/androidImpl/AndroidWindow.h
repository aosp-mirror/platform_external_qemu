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
#pragma once

#include "AndroidBufferQueue.h"

#include <system/window.h>

namespace aemu {

class AndroidWindow : public ANativeWindow {
public:
    AndroidWindow(int width, int height);

    ~AndroidWindow() = default;

    static AndroidWindow* getSelf(ANativeWindow* window) {
        return (AndroidWindow*)(window);
    }

    static const AndroidWindow* getSelfConst(const ANativeWindow* window) {
        return (const AndroidWindow*)(window);
    }

    void setProducer(AndroidBufferQueue* fromProducer,
                     AndroidBufferQueue* toProducer);
    int dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd);
    int queueBuffer(ANativeWindowBuffer* buffer, int fenceFd);
    int cancelBuffer(ANativeWindowBuffer* buffer, int fenceFd);

    int query(int what, int* value) const;
    int perform(int operation, va_list args);

    AndroidBufferQueue* fromProducer = nullptr;
    AndroidBufferQueue* toProducer = nullptr;

    int swapInterval = 1;
    int width = 256;
    int height = 256;
};

} // namespace aemu

extern "C" {

ANativeWindow* create_host_anativewindow(int width, int height);
void destroy_host_anativewindow(ANativeWindow* window);

} // extern "C"