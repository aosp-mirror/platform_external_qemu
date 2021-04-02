// Copyright (C) 2021 The Android Open Source Project
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

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <cstdint>

extern "C" {
struct media_vtb_utils_copy_context {
    media_vtb_utils_copy_context(void* surface, unsigned int w, unsigned int h)
        : iosurface(surface), dest_width(w), dest_height(h){};
    void* iosurface;
    unsigned int dest_width;
    unsigned int dest_height;
};

void media_vtb_utils_nv12_updater(void* privData,
                                  uint32_t type,
                                  uint32_t* textures,
                                  void* callerData);

}  // end of extern C
