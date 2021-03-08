// Copyright (C) 2020 The Android Open Source Project
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

#include <cstdint>

extern "C" {
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#include <stdio.h>
#include <string.h>

#include <stddef.h>

extern "C" {

struct media_cuda_utils_copy_context {
    CUdeviceptr src_frame;
    unsigned int src_pitch;

    // this usually >= dest_height due to padding, e.g.
    // src_surface_height: 1088, dest_height: 1080
    // so, when copying UV data, the src has to start at
    // offset = src_pitch * src_surface_height
    unsigned int src_surface_height;

    unsigned int dest_width;
    unsigned int dest_height;
};

void media_cuda_utils_nv12_updater(void* privData,
                                   uint32_t type,
                                   uint32_t* textures);
}
