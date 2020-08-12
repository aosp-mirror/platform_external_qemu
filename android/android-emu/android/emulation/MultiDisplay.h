/*
* Copyright (C) 2020 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "android/base/synchronization/Lock.h"
#include "android/base/files/Stream.h"
#include "android/emulation/control/record_screen_agent.h"
#include "android/emulation/control/window_agent.h"

#include <map>

namespace android {

struct MultiDisplayInfo {
    int32_t pos_x;
    int32_t pos_y;
    uint32_t width;
    uint32_t height;
    uint32_t dpi;
    uint32_t flag;
    uint32_t cb;
    bool     enabled;
    MultiDisplayInfo() :
      pos_x(0), pos_y(0), width(0), height(0), dpi(0), flag(0), cb(0), enabled(true) {}
    MultiDisplayInfo(int32_t x, int32_t y, uint32_t w, uint32_t h,
                     uint32_t d, uint32_t f, bool e, uint32_t c = 0) :
      pos_x(x), pos_y(y), width(w), height(h), dpi(d), flag(f), enabled(e), cb(c) {}

};

class MultiDisplay {
public:
    MultiDisplay(const QAndroidEmulatorWindowAgent* const windowAgent,
                 const QAndroidRecordScreenAgent* const recordAgent,
                 bool isGuestMode);
    static MultiDisplay* getInstance();
    bool isMultiDisplayEnabled() { base::AutoLock lock(mLock); return mMultiDisplay.size() > 1; }
    int setMultiDisplay(uint32_t id,
                         int32_t x,
                         int32_t y,
                         uint32_t w,
                         uint32_t h,
                         uint32_t dpi,
                         uint32_t flag,
                         bool add);
    bool getMultiDisplay(uint32_t id,
                         int32_t* x,
                         int32_t* y,
                         uint32_t* width,
                         uint32_t* height,
                         uint32_t* dpi,
                         uint32_t* flag,
                         bool* enabled) {
        return getMultiDisplay(id, x, y, width, height, dpi, flag, nullptr, enabled);
    }
    bool getNextMultiDisplay(int32_t start_id,
                             uint32_t* id,
                             int32_t* x,
                             int32_t* y,
                             uint32_t* w,
                             uint32_t* h,
                             uint32_t* dpi,
                             uint32_t* flag,
                             uint32_t* cb);
    bool multiDisplayParamValidate(uint32_t id, uint32_t w, uint32_t h,
                                   uint32_t dpi, uint32_t flag);
    bool translateCoordination(uint32_t* x, uint32_t* y, uint32_t* displayId);
    void setGpuMode(bool isGuestMode, uint32_t w, uint32_t h);
    int createDisplay(uint32_t* displayId);
    int setDisplayPose(uint32_t displayId,
                       int32_t x,
                       int32_t y,
                       uint32_t w,
                       uint32_t h,
                       uint32_t dpi);
    int destroyDisplay(uint32_t displayId);
    int getDisplayPose(uint32_t displayId,
                       int32_t* x,
                       int32_t* y,
                       uint32_t* w,
                       uint32_t* h);
    int getDisplayColorBuffer(uint32_t displayId, uint32_t* colorBuffer);
    int getColorBufferDisplay(uint32_t colorBuffer, uint32_t* displayId);
    int setDisplayColorBuffer(uint32_t displayId, uint32_t colorBuffer);
    void getCombinedDisplaySize(uint32_t* w, uint32_t* h);
    void loadConfig();
    void onSave(base::Stream* stream);
    void onLoad(base::Stream* stream);

    // 0 for default Android display
    // 1-5 for Emulator UI
    // 6-10 for developer from rcControl
    static const uint32_t s_displayIdInternalBegin = 6;
    static const uint32_t s_maxNumMultiDisplay = 11;
    static const uint32_t s_invalidIdMultiDisplay = 0xFFFFFFAB;

private:
    const QAndroidEmulatorWindowAgent* mWindowAgent;
    const QAndroidRecordScreenAgent* mRecordAgent;
    bool mGuestMode;
    std::map<uint32_t, MultiDisplayInfo> mMultiDisplay;
    android::base::Lock mLock;

    void recomputeLayoutLocked();
    void getCombinedDisplaySizeLocked(uint32_t* w, uint32_t* h);
    bool getMultiDisplay(uint32_t id,
                         int32_t* x,
                         int32_t* y,
                         uint32_t* width,
                         uint32_t* height,
                         uint32_t* dpi,
                         uint32_t* flag,
                         uint32_t* cb,
                         bool* enabled);
    int getNumberActiveMultiDisplaysLocked();

    std::map<uint32_t, MultiDisplayInfo> parseConfig();
};
} // namespace android

void android_init_multi_display(const QAndroidEmulatorWindowAgent* const windowAgent,
                                const QAndroidRecordScreenAgent* const recordAgent,
                                bool isGUestMode = false);
