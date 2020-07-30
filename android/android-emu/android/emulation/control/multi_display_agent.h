// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidMultiDisplayAgent {
    int (*setMultiDisplay)(uint32_t id,
                           int32_t x,
                           int32_t y,
                           uint32_t w,
                           uint32_t h,
                           uint32_t dpi,
                           uint32_t flag,
                           bool add);
    bool (*getMultiDisplay)(uint32_t id,
                            int32_t* x,
                            int32_t* y,
                            uint32_t* w,
                            uint32_t* h,
                            uint32_t* dpi,
                            uint32_t* flag,
                            bool* enable);
    bool (*getNextMultiDisplay)(int32_t start_id,
                                uint32_t* id,
                                int32_t* x,
                                int32_t* y,
                                uint32_t* w,
                                uint32_t* h,
                                uint32_t* dpi,
                                uint32_t* flag,
                                uint32_t* cb);
    bool (*isMultiDisplayEnabled)(void);
    void (*getCombinedDisplaySize)(uint32_t* width, uint32_t* height);
    bool (*multiDisplayParamValidate)(uint32_t id, uint32_t w, uint32_t h,
                                      uint32_t dpi, uint32_t flag);
    bool (*translateCoordination)(uint32_t* x, uint32_t*y, uint32_t* displayId);
    void (*setGpuMode)(bool isGuestMode, uint32_t w, uint32_t h);
    int (*createDisplay)(uint32_t* displayId);
    int (*destroyDisplay)(uint32_t displayId);
    int (*setDisplayPose)(uint32_t displayId,
                          int32_t x,
                          int32_t y,
                          uint32_t w,
                          uint32_t h,
                          uint32_t dpi);
    int (*getDisplayPose)(uint32_t displayId,
                         int32_t* x,
                         int32_t* y,
                         uint32_t* w,
                         uint32_t* h);
    int (*getDisplayColorBuffer)(uint32_t displayId, uint32_t* colorBuffer);
    int (*getColorBufferDisplay)(uint32_t colorBuffer, uint32_t* displayId);
    int (*setDisplayColorBuffer)(uint32_t displayId, uint32_t colorBuffer);
} QAndroidMultiDisplayAgent;


ANDROID_END_HEADER
