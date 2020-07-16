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

#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/MultiDisplay.h"
#include "android/opengles.h"

using android::MultiDisplay;

static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) -> int{
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->setMultiDisplay(id, x, y, w, h, dpi, flag, add);
            }
            return -1;
        },
        .getMultiDisplay = [](uint32_t id,
                             int32_t* x,
                             int32_t* y,
                             uint32_t* w,
                             uint32_t* h,
                             uint32_t* dpi,
                             uint32_t* flag,
                             bool* enable) -> bool{
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->getMultiDisplay(id, x, y, w, h, dpi, flag, enable);
            }
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
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->getNextMultiDisplay(start_id, id, x, y, w, h, dpi, flag, cb);
            }
            return false;
        },
        .isMultiDisplayEnabled = [](void) -> bool {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->isMultiDisplayEnabled();
            }
            return false;
        },
        .getCombinedDisplaySize = [](uint32_t* width, uint32_t* height) {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                instance->getCombinedDisplaySize(width, height);
            } else {
                *width = -1;
                *height = -1;
            }
        },
        .multiDisplayParamValidate = [](uint32_t id, uint32_t w, uint32_t h,
                                        uint32_t dpi, uint32_t flag) -> bool {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->multiDisplayParamValidate(id, w, h, dpi, flag);
            } else {
                return false;
            }
        },
        .translateCoordination = [](uint32_t* x, uint32_t*y, uint32_t* displayId) -> bool {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->translateCoordination(x, y, displayId);
            } else {
                return false;
            }
        },
        .setGpuMode = [](bool isGuestMode, uint32_t w, uint32_t h) {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                instance->setGpuMode(isGuestMode, w, h);
            }
        },
        .createDisplay = [](uint32_t* displayId) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->createDisplay(displayId);
            } else {
                return -1;
            }
        },
        .destroyDisplay = [](uint32_t displayId) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->destroyDisplay(displayId);
            } else {
                return -1;
            }
        },
        .setDisplayPose = [](uint32_t displayId,
                            int32_t x,
                            int32_t y,
                            uint32_t w,
                            uint32_t h,
                            uint32_t dpi) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->setDisplayPose(displayId, x, y, w, h, dpi);
            } else {
                return -1;
            }
        },
        .getDisplayPose = [](uint32_t displayId,
                            int32_t* x,
                            int32_t* y,
                            uint32_t* w,
                            uint32_t* h) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->getDisplayPose(displayId, x, y, w, h);
            } else {
                return -1;
            }
        },
        .getDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t* colorBuffer) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->getDisplayColorBuffer(displayId, colorBuffer);
            } else {
                return -1;
            }
         },
         .getColorBufferDisplay = [](uint32_t colorBuffer,
                                     uint32_t* displayId) -> int {
            auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->getColorBufferDisplay(colorBuffer, displayId);
            } else {
                return -1;
            }
        },
        .setDisplayColorBuffer = [](uint32_t displayId,
                                    uint32_t colorBuffer) -> int {
                        auto instance = MultiDisplay::getInstance();
            if (instance) {
                return instance->setDisplayColorBuffer(displayId, colorBuffer);
            } else {
                return -1;
            }
        },
};

extern "C" const QAndroidMultiDisplayAgent* const gQAndroidMultiDisplayAgent = &sMultiDisplayAgent;
