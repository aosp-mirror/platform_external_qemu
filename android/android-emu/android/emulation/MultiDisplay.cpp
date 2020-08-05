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

#include "MultiDisplay.h"
#include "android/avd/info.h"
#include "android/base/LayoutResolver.h"
#include "android/base/files/StreamSerializing.h"
#include "android/cmdline-option.h"
#include "android/emulation/MultiDisplayPipe.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/emulator-window.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/hw-sensors.h"
#include "android/skin/winsys.h"

#include <set>

using android::base::AutoLock;

namespace android {

static MultiDisplay* sMultiDisplay = nullptr;

MultiDisplay::MultiDisplay(const QAndroidEmulatorWindowAgent* const windowAgent,
                           const QAndroidRecordScreenAgent* const recordAgent,
                           bool isGuestMode)
    : mWindowAgent(windowAgent),
      mRecordAgent(recordAgent),
      mGuestMode(isGuestMode) { }

//static
MultiDisplay* MultiDisplay::getInstance() {
    return sMultiDisplay;
}

int MultiDisplay::setMultiDisplay(uint32_t id,
                                  int32_t x,
                                  int32_t y,
                                  uint32_t w,
                                  uint32_t h,
                                  uint32_t dpi,
                                  uint32_t flag,
                                  bool add) {
    int ret = 0;
    SkinRotation rotation = SKIN_ROTATION_0;
    LOG(VERBOSE) << "setMultiDisplay id " << id << " "
                 << x << " " << y << " " << w << " " << h << " "
                << dpi << " " << flag << " " << (add? "add":"del");
    if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
        return -1;
    }
    if (android_foldable_any_folded_area_configured()) {
        return -1;
    }
    if (mGuestMode) {
        return -1;
    }
    if (add && !multiDisplayParamValidate(id, w, h, dpi, flag)) {
        return -1;
    }

    // fetch rotation from EmulatorWindow
    // TODO: link to libui source code???
    EmulatorWindow* window = emulator_window_get();
    if (window) {
        SkinLayout* layout = emulator_window_get_layout(window);
        if (layout) {
            rotation = layout->orientation;
        }
    }
    if (rotation != SKIN_ROTATION_0) {
        mWindowAgent->showMessage("Please apply multiple displays without rotation",
                                  WINDOW_MESSAGE_ERROR, 1000);
        return -1;
    }

    if (add) {
        ret = createDisplay(&id);
        if (ret != 0) {
            return ret;
        }
        ret = setDisplayPose(id, x, y, w, h, dpi);
        if (ret != 0) {
            return ret;
        }
    } else {
        ret = destroyDisplay(id);
        if (ret != 0) {
            return ret;
        }
    }

    // Service in guest has already started through QemuMiscPipe when
    // bootCompleted. But this service may be killed, e.g., Android low
    // memory. Send broadcast again to guarantee servce running.
    // P.S. guest Service has check to avoid duplication.
    auto adbInterface = emulation::AdbInterface::getGlobal();
    if (!adbInterface) {
        LOG(ERROR) << "Adb interface unavailable";
        return -1;
    }
    adbInterface->enqueueCommand(
        {"shell", "am", "broadcast", "-a",
         "com.android.emulator.multidisplay.START", "-n",
         "com.android.emulator.multidisplay/"
         ".MultiDisplayServiceReceiver"});

    MultiDisplayPipe* pipe = MultiDisplayPipe::getInstance();
    if (pipe) {
        std::vector<uint8_t> data;
        pipe->fillData(data, id, w, h, dpi, flag, add);
        LOG(VERBOSE) << "MultiDisplayPipe send " << (add ? "add":"del") << " id " << id
                     << " width " << w << " height " << h << " dpi " << dpi
                     << " flag " << flag;
        pipe->send(std::move(data));
    }
    return 0;
}

bool MultiDisplay::getMultiDisplay(uint32_t id,
                                  int32_t* x,
                                  int32_t* y,
                                  uint32_t* w,
                                  uint32_t* h,
                                  uint32_t* dpi,
                                  uint32_t* flag,
                                  uint32_t* cb,
                                  bool* enabled) {
    AutoLock lock(mLock);
    if (mMultiDisplay.find(id) == mMultiDisplay.end()) {
        if (enabled) {
            *enabled = false;
        }
        return false;
    }
    if (x) {
        *x = mMultiDisplay[id].pos_x;
    }
    if (y) {
        *y = mMultiDisplay[id].pos_y;
    }
    if (w) {
        *w = mMultiDisplay[id].width;
    }
    if (h) {
        *h = mMultiDisplay[id].height;
    }
    if (dpi) {
        *dpi = mMultiDisplay[id].dpi;
    }
    if (flag) {
        *flag = mMultiDisplay[id].flag;
    }
    if (enabled) {
        *enabled = mMultiDisplay[id].enabled;
    }
    LOG(VERBOSE) << "getMultiDisplay " << id <<  "x " << mMultiDisplay[id].pos_x
                 << " y " << mMultiDisplay[id].pos_y
                 << " w " << mMultiDisplay[id].width
                 << " h " << mMultiDisplay[id].height
                 << " dpi " << mMultiDisplay[id].dpi
                 << " flag " << mMultiDisplay[id].flag
                 << " enable " << mMultiDisplay[id].enabled;
    return mMultiDisplay[id].enabled;
}

bool MultiDisplay::getNextMultiDisplay(int32_t start_id,
                                       uint32_t* id,
                                       int32_t* x,
                                       int32_t* y,
                                       uint32_t* w,
                                       uint32_t* h,
                                       uint32_t* dpi,
                                       uint32_t* flag,
                                       uint32_t* cb) {
    uint32_t key;
    std::map<uint32_t, MultiDisplayInfo>::iterator i;

    AutoLock lock(mLock);
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
}

bool MultiDisplay::translateCoordination(uint32_t* x, uint32_t* y, uint32_t* displayId) {
    if (mGuestMode) {
        *displayId = 0;
        return true;
    }
    AutoLock lock(mLock);
    uint32_t totalH, pos_x, pos_y, w, h;
    getCombinedDisplaySizeLocked(nullptr, &totalH);
    for (const auto iter : mMultiDisplay) {
        if (iter.first != 0 && iter.second.cb == 0) {
            continue;
        }
        // QT window uses the top left corner as the origin.
        // So we need to transform the (x, y) coordinates from
        // bottom left corner to top left corner.
        pos_x = iter.second.pos_x;
        pos_y = totalH - iter.second.height - iter.second.pos_y;
        w = iter.second.width;
        h = iter.second.height;
        if ((*x - pos_x) < w && (*y - pos_y) < h) {
            *x = *x - pos_x;
            *y = *y - pos_y;
            *displayId = iter.first;
            return true;
        }
    }
    return false;
}

void MultiDisplay::setGpuMode(bool isGuestMode, uint32_t w, uint32_t h) {
    mGuestMode = isGuestMode;
    if (isGuestMode) {
        // Guest mode will not start renderer, which in turn will not set the
        // default display from FrameBuffer. So we set display 0 here.
        AutoLock lock(mLock);
        mMultiDisplay.emplace(0, MultiDisplayInfo(0, 0, w, h, 0, 0, true, 0));
    }
}

int MultiDisplay::createDisplay(uint32_t* displayId) {
    if (mGuestMode) {
        return -1;
    }

    if (displayId == nullptr) {
        LOG(ERROR) << "null displayId pointer";
        return -1;
    }

    AutoLock lock(mLock);

    if (mMultiDisplay.size() > s_maxNumMultiDisplay) {
        LOG(ERROR) << "cannot create more displays, exceeding limits "
            << s_maxNumMultiDisplay;
        return -1;
    }
    if (mMultiDisplay.find(*displayId) != mMultiDisplay.end()) {
        return 0;
    }

    // displays created by internal rcCommands
    if (*displayId == s_invalidIdMultiDisplay) {
        for (int i = s_displayIdInternalBegin; i < s_maxNumMultiDisplay; i++) {
            if (mMultiDisplay.find(i) == mMultiDisplay.end()) {
                *displayId = i;
                break;
            }
        }
    }
    if (*displayId == s_invalidIdMultiDisplay) {
        LOG(ERROR) << "cannot create more internaldisplays, exceeding limits " <<
            s_maxNumMultiDisplay - s_displayIdInternalBegin;
        return -1;
    }

    mMultiDisplay.emplace(*displayId, MultiDisplayInfo());
    LOG(VERBOSE) << "create display " << *displayId;
    return 0;
}

int MultiDisplay::destroyDisplay(uint32_t displayId) {
    uint32_t width, height;
    bool needUIUpdate = false;
    bool restoreSkin = false;

    if (mGuestMode) {
        return -1;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            return 0;
        }
        needUIUpdate = ((mMultiDisplay[displayId].cb != 0) ? true : false);
        mMultiDisplay.erase(displayId);
        if (needUIUpdate) {
            recomputeLayoutLocked();
            getCombinedDisplaySizeLocked(&width, &height);
            if (getNumberActiveMultiDisplaysLocked() == 1) {
                // only display 0 remains, restore skin
                restoreSkin = true;
            }
        }
    }

    if (needUIUpdate) {
        // stop recording of this display if it is happening.
        RecorderStates states = mRecordAgent->getRecorderState();
        if (states.displayId == displayId && states.state == RECORDER_RECORDING) {
            mRecordAgent->stopRecording();
        }
        mWindowAgent->setUIDisplayRegion(0, 0, width, height);
        if (restoreSkin) {
            mWindowAgent->restoreSkin();
        }
    }
    LOG(VERBOSE) << "delete display " << displayId;
    return 0;
}

int MultiDisplay::setDisplayPose(uint32_t displayId,
                                int32_t x,
                                int32_t y,
                                uint32_t w,
                                uint32_t h,
                                uint32_t dpi) {
    bool UIUpdate = false;
    bool checkRecording = false;
    uint32_t width, height;
    if (mGuestMode) {
        return -1;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            LOG(ERROR) << "cannot find display " << displayId;
            return -1;
        }
        if (mMultiDisplay[displayId].cb != 0 &&
            (mMultiDisplay[displayId].width != w || mMultiDisplay[displayId].height != h)) {
                checkRecording = true;
        }
        mMultiDisplay[displayId].width = w;
        mMultiDisplay[displayId].height = h;
        mMultiDisplay[displayId].dpi = dpi;
        mMultiDisplay[displayId].pos_x = x;
        mMultiDisplay[displayId].pos_y = y;
        if (mMultiDisplay[displayId].cb != 0) {
            if (x == -1 && y == -1) {
                recomputeLayoutLocked();
            }
            getCombinedDisplaySizeLocked(&width, &height);
            UIUpdate = true;
        }
    }
    if (checkRecording) {
        // stop recording of this display if it is happening.
        RecorderStates states = mRecordAgent->getRecorderState();
        if (states.displayId == displayId && states.state == RECORDER_RECORDING) {
            mRecordAgent->stopRecording();
        }
    }
    if (UIUpdate) {
        mWindowAgent->setUIDisplayRegion(0, 0, width, height);
    }
    LOG(VERBOSE) << "setDisplayPose " << displayId << " x " << x
                 << " y " << y << " w " << w << " h " << h
                 << " dpi " << dpi;
    return 0;
}

int MultiDisplay::getDisplayPose(uint32_t displayId,
                                 int32_t* x,
                                 int32_t* y,
                                 uint32_t* w,
                                 uint32_t* h) {
    if (mGuestMode) {
        return -1;
    }
    AutoLock lock(mLock);
    if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
        LOG(ERROR) << "cannot find display " << displayId;
        return -1;
    }
    *x = mMultiDisplay[displayId].pos_x;
    *y = mMultiDisplay[displayId].pos_y;
    *w = mMultiDisplay[displayId].width;
    *h = mMultiDisplay[displayId].height;
    return 0;
}

int MultiDisplay::setDisplayColorBuffer(uint32_t displayId, uint32_t colorBuffer) {
    uint32_t width, height;
    bool noSkin = false;
    bool needUpdate = false;
    if (mGuestMode) {
        return -1;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            LOG(ERROR) << "cannot find display" << displayId;
            return -1;
        }
        if (mMultiDisplay[displayId].cb == colorBuffer) {
            return 0;
        }
        if (mMultiDisplay[displayId].cb == 0) {
            mMultiDisplay[displayId].cb = colorBuffer;
            // first time cb assigned, update the UI
            needUpdate = true;
            recomputeLayoutLocked();
            getCombinedDisplaySizeLocked(&width, &height);
            if (getNumberActiveMultiDisplaysLocked() == 2) {
                //disable skin when first display set, index 0 is the default one.
                noSkin = true;
            }
        }
        mMultiDisplay[displayId].cb = colorBuffer;
    }
    if (noSkin) {
        mWindowAgent->setNoSkin();
    }
    if (needUpdate) {
        // Explicitly adjust host window size
        mWindowAgent->setUIDisplayRegion(0, 0, width, height);
    }
    LOG(VERBOSE) << "setDisplayColorBuffer " << displayId << " cb " << colorBuffer;
    return 0;
}

int MultiDisplay::getDisplayColorBuffer(uint32_t displayId, uint32_t* colorBuffer) {
    if (mGuestMode) {
        return -1;
    }
    AutoLock lock(mLock);
    if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
        return -1;
    }
    *colorBuffer = mMultiDisplay[displayId].cb;
    return 0;
}

int MultiDisplay::getColorBufferDisplay(uint32_t colorBuffer, uint32_t* displayId) {
    if (mGuestMode) {
        return -1;
    }
    AutoLock lock(mLock);
    for (const auto& iter : mMultiDisplay) {
        if (iter.second.cb == colorBuffer) {
            *displayId = iter.first;
            return 0;
        }
    }
    return -1;
}

void MultiDisplay::getCombinedDisplaySize(uint32_t* w, uint32_t* h) {
    AutoLock lock(mLock);
    getCombinedDisplaySizeLocked(w, h);
}

void MultiDisplay::getCombinedDisplaySizeLocked(uint32_t* w, uint32_t* h) {
    uint32_t total_h = 0;
    uint32_t total_w = 0;
    for (const auto& iter : mMultiDisplay) {
        if (iter.first == 0 || iter.second.cb != 0) {
            total_h = std::max(total_h, iter.second.height + iter.second.pos_y);
            total_w = std::max(total_w, iter.second.width + iter.second.pos_x);
        }
    }
    if (h)
        *h = total_h;
    if (w)
        *w = total_w;
}

int MultiDisplay::getNumberActiveMultiDisplaysLocked() {
    int count = 0;
    for (const auto& iter : mMultiDisplay) {
        if (iter.first == 0 || iter.second.cb != 0) {
            count++;
        }
    }
    return count;
}

/*
 * Given that there are at most 11 displays, we can iterate through all possible
 * ways of showing each display in either the first row or the second row. It is
 * also possible to have an empty row. The best combination is to satisfy the
 * following two criteria: 1, The combined rectangle which contains all the
 * displays should have an aspect ratio that is close to the monitor's aspect
 * ratio. 2, The width of the first row should be close to the width of the
 * second row.
 *
 * Important detail of implementations: the x and y offsets saved in
 * mMultiDisplay use the bottom-left corner as origin. This coordinates will
 * be used by glviewport() in Postworker.cpp. However, the x and y offsets saved
 * by invoking setUIMultiDisplay() will be using top-left corner as origin. Thus,
 * input coordinates willl be calculated correctly when mouse events are
 * captured by QT window.
 *
 * TODO: We assume all displays pos_x/pos_y is adjustable here. This may
 * overwrite the specified pos_x/pos_y in setDisplayPos();
 */
void MultiDisplay::recomputeLayoutLocked() {
    uint32_t monitorWidth, monitorHeight;
    double monitorAspectRatio = 1.0;
    if (!mWindowAgent->getMonitorRect(&monitorWidth, &monitorHeight)) {
        LOG(WARNING) << "Fail to get monitor width and height, use default ratio 1.0";
    } else {
        monitorAspectRatio = (double) monitorHeight / (double) monitorWidth;
    }
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> rectangles;
    for (const auto& iter : mMultiDisplay) {
        if (iter.first == 0 || iter.second.cb != 0) {
            rectangles[iter.first] =
                std::make_pair(iter.second.width, iter.second.height);
        }
    }
    for (const auto& iter :
        android::base::resolveLayout(rectangles, monitorAspectRatio)) {
        mMultiDisplay[iter.first].pos_x = iter.second.first;
        mMultiDisplay[iter.first].pos_y = iter.second.second;
    }
}

bool MultiDisplay::multiDisplayParamValidate(uint32_t id, uint32_t w, uint32_t h,
                                             uint32_t dpi, uint32_t flag) {
    // According the Android 9 CDD,
    // * 120 <= dpi <= 640
    // * 320 * (dpi / 160) <= width
    // * 320 * (dpi / 160) <= height
    // * Screen aspect ratio cannot be longer (or wider) than 21:9 (or 9:21).
    //
    // Also we don't want a screen too big to limit the performance impact.
    // * 4K might be a good upper limit

    if (dpi < 120 || dpi > 640) {
        mWindowAgent->showMessage("dpi should be between 120 and 640",
                                  WINDOW_MESSAGE_ERROR, 1000);
        fprintf(stderr, "dpi should be between 120 and 640\n");
        return false;
    }
    if (w < 320 * dpi / 160 || h < 320 * dpi / 160) {
        mWindowAgent->showMessage("width and height should be >= 320dp",
                                  WINDOW_MESSAGE_ERROR, 1000);
        fprintf(stderr, "width and height should be >= 320dp\n");
        return false;
    }
    if (!((w <= 4096 && h <= 2160) || (w <= 2160 && h <= 4096))) {
        mWindowAgent->showMessage("resolution should not exceed 4k (4096*2160)",
                                  WINDOW_MESSAGE_ERROR, 1000);
        fprintf(stderr, "resolution should not exceed 4k (4096*2160)\n");
        return false;
    }
    if (w * 21 < h * 9 || w * 9 > h * 21) {
        mWindowAgent->showMessage("Aspect ratio cannot be longer (or wider) than 21:9 (or 9:21)",
                                  WINDOW_MESSAGE_ERROR, 1000);
        fprintf(stderr, "Aspect ratio cannot be longer (or wider) than 21:9 (or 9:21)\n");
        return false;
    }
    if (id > s_maxNumMultiDisplay) {
        mWindowAgent->showMessage("Display index cannot be more than 3",
                                  WINDOW_MESSAGE_ERROR, 1000);
        fprintf(stderr, "Display index cannot be more than 3\n");
        return false;
    }
    return true;
}

std::map<uint32_t, MultiDisplayInfo> MultiDisplay::parseConfig() {
    std::map<uint32_t, MultiDisplayInfo> ret;
    if (!android_cmdLineOptions || !android_cmdLineOptions->multidisplay) {
        return ret;
    }
    std::string s = android_cmdLineOptions->multidisplay;
    std::vector<uint32_t> params;
    size_t last = 0, next = 0;
    while ((next = s.find(",", last)) != std::string::npos) {
        params.push_back(std::stoi(s.substr(last, next - last)));
        last = next + 1;
    }
    params.push_back(std::stoi(s.substr(last)));
    if (params.size() < 5 || params.size() % 5 != 0) {
        LOG(ERROR) << "Not enough parameters for multidisplay command";
        return ret;
    }
    int i = 0;
    for (i = 0; i < params.size(); i+=5) {
        if (params[i] == 0 || params[i] > 3) {
            LOG(ERROR) << "multidisplay index should only be 1, 2, or 3";
            ret.clear();
            return ret;
        }
        if (multiDisplayParamValidate(params[i],
                                      params[i + 1],
                                      params[i + 2],
                                      params[i + 3],
                                      params[i + 4])) {
            LOG(ERROR) << "Invalid index/width/height/dpi settings for multidisplay command";
            ret.clear();
            return ret;
        }
        ret.emplace(params[i], MultiDisplayInfo(-1, -1, params[i + 1], params[i + 2],
                                                params[i + 3], params[i + 4], true));
    }
    return ret;
}

void MultiDisplay::loadConfig() {
    // Get the multidisplay configs from startup parameters, if yes,
    // override the configs in config.ini
    // This stage happens before the MultiDisplayPipe created (bootCompleted)
    // or restored (snapshot). MultiDisplay configs will not send to guest
    // immediately.
    // For cold boot, MultiDisplayPipe queries configs when it is created.
    // For snapshot, MultiDisplayPipe query will not happen, instead,
    // onLoad() function later may overwrite the multidisplay states to
    // in sync with guest states.
    if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
        return;
    }
    if (android_foldable_any_folded_area_configured()) {
        return;
    }
    if (mGuestMode) {
        return;
    }

    std::map<uint32_t, MultiDisplayInfo> info = parseConfig();
    if (info.size()) {
        LOG(VERBOSE) << "config multidisplay with command-line";
        for (const auto& i : info) {
            setMultiDisplay(i.first,
                            -1,
                            -1,
                            i.second.width,
                            i.second.height,
                            i.second.dpi,
                            i.second.flag,
                            true);
            mWindowAgent->updateUIMultiDisplayPage(i.first);
        }
    } else {
        LOG(VERBOSE) << "config multidisplay with config.ini "
                        << android_hw->hw_display1_width
                        << "x" << android_hw->hw_display1_height << " " <<
                        android_hw->hw_display2_width << "x" <<
                        android_hw->hw_display2_height << " " <<
                        android_hw->hw_display3_width << "x" <<
                        android_hw->hw_display3_height;
        if (android_hw->hw_display1_width != 0 &&
            android_hw->hw_display1_height != 0) {
            LOG(VERBOSE) << " add display 1";
            setMultiDisplay(1,
                            android_hw->hw_display1_xOffset,
                            android_hw->hw_display1_yOffset,
                            android_hw->hw_display1_width,
                            android_hw->hw_display1_height,
                            android_hw->hw_display1_density,
                            android_hw->hw_display1_flag,
                            true);
            mWindowAgent->updateUIMultiDisplayPage(1);
        }
        if (android_hw->hw_display2_width != 0 &&
            android_hw->hw_display2_height != 0) {
            LOG(VERBOSE) << " add display 2";
            setMultiDisplay(2,
                            android_hw->hw_display2_xOffset,
                            android_hw->hw_display2_yOffset,
                            android_hw->hw_display2_width,
                            android_hw->hw_display2_height,
                            android_hw->hw_display2_density,
                            android_hw->hw_display2_flag,
                            true);
            mWindowAgent->updateUIMultiDisplayPage(2);
        }
        if (android_hw->hw_display3_width != 0 &&
            android_hw->hw_display3_height != 0) {
            LOG(VERBOSE) << " add display 3";
            setMultiDisplay(3,
                            android_hw->hw_display3_xOffset,
                            android_hw->hw_display3_yOffset,
                            android_hw->hw_display3_width,
                            android_hw->hw_display3_height,
                            android_hw->hw_display3_density,
                            android_hw->hw_display3_flag,
                            true);
            mWindowAgent->updateUIMultiDisplayPage(3);
        }
    }
}

void MultiDisplay::onSave(base::Stream* stream) {
    AutoLock lock(mLock);
    base::saveCollection(
        stream, mMultiDisplay,
        [](base::Stream* s,
           const std::map<uint32_t, MultiDisplayInfo>::value_type& pair) {
        s->putBe32(pair.first);
        s->putBe32(pair.second.pos_x);
        s->putBe32(pair.second.pos_y);
        s->putBe32(pair.second.width);
        s->putBe32(pair.second.height);
        s->putBe32(pair.second.dpi);
        s->putBe32(pair.second.flag);
        s->putBe32(pair.second.cb);
        s->putByte(pair.second.enabled);
    });
}

void MultiDisplay::onLoad(base::Stream* stream) {
    std::map<uint32_t, MultiDisplayInfo> displaysOnLoad;
    base::loadCollection(stream, &displaysOnLoad,
                         [this](base::Stream* stream) -> std::map<uint32_t, MultiDisplayInfo>::value_type {
        const uint32_t idx = stream->getBe32();
        const int32_t pos_x = stream->getBe32();
        const int32_t pos_y = stream->getBe32();
        const uint32_t width = stream->getBe32();
        const uint32_t height = stream->getBe32();
        const uint32_t dpi = stream->getBe32();
        const uint32_t flag = stream->getBe32();
        const uint32_t cb = stream->getBe32();
        const bool enabled = stream->getByte();
        return {idx, {pos_x, pos_y, width, height, dpi, flag, enabled, cb}};
    });
    // Restore the multidisplays of the snapshot.
    std::set<uint32_t> ids;
    uint32_t combinedDisplayWidth = 0;
    uint32_t combinedDisplayHeight = 0;
    bool activeBeforeLoad, activeAfterLoad;
    {
        AutoLock lock(mLock);
        for (const auto& iter : mMultiDisplay) {
            ids.insert(iter.first);
        }
        for (const auto& iter: displaysOnLoad) {
            ids.insert(iter.first);
        }
        activeBeforeLoad = getNumberActiveMultiDisplaysLocked() > 1;
        mMultiDisplay.clear();
        mMultiDisplay = displaysOnLoad;
        activeAfterLoad = getNumberActiveMultiDisplaysLocked() > 1;
        getCombinedDisplaySizeLocked(&combinedDisplayWidth, &combinedDisplayHeight);
    }
    if (activeAfterLoad) {
        if (!activeBeforeLoad) {
            mWindowAgent->setNoSkin();
        }
        mWindowAgent->setUIDisplayRegion(0, 0, combinedDisplayWidth, combinedDisplayHeight);
    } else {
        if (activeBeforeLoad) {
            mWindowAgent->setUIDisplayRegion(0, 0, combinedDisplayWidth, combinedDisplayHeight);
            mWindowAgent->restoreSkin();
        }
    }
    for (const auto& iter : ids) {
        mWindowAgent->updateUIMultiDisplayPage(iter);
    }
}

}   // namespace android

void android_init_multi_display(const QAndroidEmulatorWindowAgent* const windowAgent,
                                const QAndroidRecordScreenAgent* const recordAgent,
                                bool isGuestMode) {
    android::sMultiDisplay = new android::MultiDisplay(windowAgent, recordAgent, isGuestMode);
}

extern "C" {
void android_load_multi_display_config() {
    if (!android::sMultiDisplay) {
        LOG(ERROR) << "Multidisplay not initiated yet, cannot config";
        return;
    }
    android::sMultiDisplay->loadConfig();
}
}
