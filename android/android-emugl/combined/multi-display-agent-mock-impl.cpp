// Copyright 2015-2017 The Android Open Source Project
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

#include <stdio.h>
#include "android/emulation/control/multi_display_agent.h"

static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) -> int{
            printf("setMultiDisplay (mock)\n");
            return 0;
        },
        .getMultiDisplay = [](uint32_t id,
                             int32_t* x,
                             int32_t* y,
                             uint32_t* w,
                             uint32_t* h,
                             uint32_t* dpi,
                             uint32_t* flag,
                             bool* enable) -> bool{
            printf("getMultiDisplay (mock)\n");
            return false;
        },
        .getNextMultiDisplay = [](int32_t start_id,
                                uint32_t* id,
                                int32_t* x,
                                int32_t* y,
                                uint32_t* w,
                                uint32_t* h,
                                uint32_t* dpi,
                                uint32_t* flag,
                                uint32_t* cb) -> bool {
            printf("getNextMultiDisplay (mock)\n");
            return false;
        },
        .isMultiDisplayEnabled = [](void) -> bool {
            printf("isMultiDisplayEnabled (mock)\n");
            return false;
        },
        .getCombinedDisplaySize = [](uint32_t* width,
                                     uint32_t* height) {
            printf("getCombinedDisplaySize (mock)\n");
        },
        .multiDisplayParamValidate = [](uint32_t id,
                                        uint32_t w,
                                        uint32_t h,
                                        uint32_t dpi,
                                        uint32_t flag) -> bool {
            printf("multiDisplayParamValidate (mock)\n");
            return false;
        },
        .translateCoordination = [](uint32_t* x,
                                    uint32_t*y,
                                    uint32_t* displayId) -> bool {
            printf("translateCoordination (mock)\n");
            return true;
        },
        .createDisplay = [](uint32_t* displayId) -> int {
            printf("createDisplay (mock)\n");
            return 0;
        },
        .destroyDisplay = [](uint32_t displayId) -> int {
            printf("destroyDisplay (mock)\n");
            return 0;
        },
        .setDisplayPose = [](uint32_t displayId,
                            int32_t x,
                            int32_t y,
                            uint32_t w,
                            uint32_t h,
                            uint32_t dpi) -> int {
            printf("setDisplayPose (mock)\n");
            return 0;
        },
        .getDisplayPose = [](uint32_t displayId,
                            int32_t* x,
                            int32_t* y,
                            uint32_t* w,
                            uint32_t* h) -> int {
            printf("getDisplayPose (mock)\n");
            return 0;
        },
        .getDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t* colorBuffer) -> int {
            printf("getDisplayColorBuffer (mock)\n");
            return 0;
         },
         .getColorBufferDisplay = [](uint32_t colorBuffer,
                                     uint32_t* displayId) -> int {
            printf("getColorBufferDisplay (mock)\n");
            return 0;
        },
        .setDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t colorBuffer) -> int {
            printf("setDisplayColorBuffer (mock)\n");
            return 0;
        }
};

const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent = &sMultiDisplayAgent;
