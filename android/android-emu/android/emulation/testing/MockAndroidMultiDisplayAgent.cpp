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

#include <stdint.h>                                         // for uint32_t
#include <map>                                              // for map, __ma...
#include <utility>                                          // for pair

#include "android/emulation/MultiDisplay.h"                 // for MultiDisp...
#include "android/emulation/control/multi_display_agent.h"  // for QAndroidM...

std::map<uint32_t, android::MultiDisplayInfo> mMultiDisplay;
static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) -> int{
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
            return true;
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
            uint32_t key;
            std::map<uint32_t, android::MultiDisplayInfo>::iterator i;
            if (start_id < 0) {
                key = 0;
            } else {
                key = start_id + 1;
            }
            i = mMultiDisplay.lower_bound(key);
            if (i == mMultiDisplay.end()) {
                return false;
            } else {
                if (id) {
                    *id = i->first;
                }
                if (x) {
                    *x = i->second.pos_x;
                }
                if (y) {
                    *y = i->second.pos_y;
                }
                if (w) {
                    *w = i->second.width;
                }
                if (h) {
                    *h = i->second.height;
                }
                if (dpi) {
                    *dpi = i->second.dpi;
                }
                if (flag) {
                    *flag = i->second.flag;
                }
                if (cb) {
                    *cb = i->second.cb;
                }
                return true;
            }
        },
        .isMultiDisplayEnabled = [](void) -> bool {
            return mMultiDisplay.size() > 1;
        },
        .getCombinedDisplaySize = [](uint32_t* width, uint32_t* height) {
        },
        .multiDisplayParamValidate = [](uint32_t id,
                                        uint32_t w,
                                        uint32_t h,
                                        uint32_t dpi,
                                        uint32_t flag) -> bool {
            return true;
        },
        .translateCoordination = [](uint32_t* x,
                                    uint32_t*y,
                                    uint32_t* displayId) -> bool {
            return true;
        },
        .setGpuMode = [](bool isGuestMode, uint32_t w, uint32_t h) { },
        .createDisplay = [](uint32_t* displayId) -> int {
            mMultiDisplay.emplace(*displayId, android::MultiDisplayInfo());
            return 0;
        },
        .destroyDisplay = [](uint32_t displayId) -> int {
            mMultiDisplay.erase(displayId);
            return 0;
        },
        .setDisplayPose = [](uint32_t displayId,
                             int32_t x,
                             int32_t y,
                             uint32_t w,
                             uint32_t h,
                             uint32_t dpi) -> int {
            mMultiDisplay[displayId].pos_x = x;
            mMultiDisplay[displayId].pos_y = y;
            mMultiDisplay[displayId].width = w;
            mMultiDisplay[displayId].height = h;
            mMultiDisplay[displayId].dpi = dpi;
            return 0;
        },
        .getDisplayPose = [](uint32_t displayId,
                            int32_t* x,
                            int32_t* y,
                            uint32_t* w,
                            uint32_t* h) -> int {
            *x = mMultiDisplay[displayId].pos_x;
            *y = mMultiDisplay[displayId].pos_y;
            *w = mMultiDisplay[displayId].width;
            *h = mMultiDisplay[displayId].height;
            return 0;
        },
        .getDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t* colorBuffer) -> int {
            *colorBuffer = mMultiDisplay[displayId].cb;
            return 0;
         },
         .getColorBufferDisplay = [](uint32_t colorBuffer,
                                     uint32_t* displayId) -> int {
            for (const auto& iter : mMultiDisplay) {
                if (iter.second.cb == colorBuffer) {
                    *displayId = iter.first;
                    return 0;
                }
            }
            return -1;
        },
        .setDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t colorBuffer) -> int {
            mMultiDisplay[displayId].cb = colorBuffer;
            return 0;
        }
};
extern "C" const QAndroidMultiDisplayAgent* const
        gMockQAndroidMultiDisplayAgent = &sMultiDisplayAgent;
