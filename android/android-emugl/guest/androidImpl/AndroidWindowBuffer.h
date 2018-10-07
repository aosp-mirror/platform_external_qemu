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

#include "AndroidWindow.h"

#include <hardware/gralloc.h>
#include <system/window.h>

namespace aemu {

// Mostly a container class that takes the information
// from an existing Android window and native buffer.
class AndroidWindowBuffer : public ANativeWindowBuffer {
public:
    // This does not refer directly to |window|
    // after construction, merely using it
    // to get information.
    // Lifetime of this class versus |*buffer| is
    // externally managed.
    AndroidWindowBuffer(AndroidWindow* window,
                        buffer_handle_t buffer,
                        int usage = GRALLOC_USAGE_HW_RENDER,
                        int format = HAL_PIXEL_FORMAT_RGBA_8888,
                        int stride = 0);

    // Creating a buffer without needing a window.
    AndroidWindowBuffer(int width, int height,
                        buffer_handle_t buffer,
                        int usage = GRALLOC_USAGE_HW_RENDER,
                        int format = HAL_PIXEL_FORMAT_RGBA_8888,
                        int stride = 0);

    ~AndroidWindowBuffer() = default;
};

} // namespace aemu