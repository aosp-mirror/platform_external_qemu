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

#include "host-common/MultiDisplay.h"

#include <stddef.h>       // for size_t
#include <algorithm>      // for max
#include <cstdint>        // for uint32_t
#include <ostream>        // for operator<<
#include <set>            // for set
#include <string>         // for string, stoi
#include <unordered_map>  // for unordered_map
#include <utility>        // for pair, make_pair
#include <vector>         // for vector

#include "aemu/base/LayoutResolver.h"                 // for resolveLayout
#include "aemu/base/Log.h"                            // for LogStreamVoi...
#include "aemu/base/files/Stream.h"                   // for Stream
#include "aemu/base/files/StreamSerializing.h"        // for loadCollection
#include "android/cmdline-option.h"                      // for android_cmdL...
#include "host-common/MultiDisplayPipe.h"          // for MultiDisplay...
#include "android/emulation/AutoDisplays.h"              // For AutoDisplays...
#include "android/emulation/control/adb/AdbInterface.h"  // for AdbInterface
#include "android/emulation/resizable_display_config.h"
#include "android/emulator-window.h"                // for emulator_win...
#include "host-common/FeatureControl.h"  // for isEnabled
#include "host-common/Features.h"        // for MultiDisplay
#include "android/avd/info.h"
#include "host-common/hw-config.h"
#include "android/console.h"                        // for android_hw
#include "android/hw-sensors.h"                     // for android_fold...
#include "host-common/screen-recorder.h"      // for RecorderStates
#include "android/skin/file.h"                      // for SkinLayout
#include "android/skin/rect.h"                      // for SKIN_ROTATION_0

#define MULTI_DISPLAY_DEBUG 0

using android::base::AutoLock;

namespace android {

static MultiDisplay* sMultiDisplay = nullptr;

MultiDisplay::MultiDisplay(const QAndroidEmulatorWindowAgent* const windowAgent,
                           const QAndroidRecordScreenAgent* const recordAgent,
                           const QAndroidVmOperations* const vmAgent,
                           bool isGuestMode)
    : mWindowAgent(windowAgent),
      mRecordAgent(recordAgent),
      mVmAgent(vmAgent),
      mGuestMode(isGuestMode) {}

// static
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

    printf("%s:%d\n", __func__, __LINE__);
    LOG(DEBUG) << "setMultiDisplay id " << id << " " << x << " " << y << " "
                 << w << " " << h << " " << dpi << " " << flag << " "
                 << (add ? "add" : "del");

    printf("%s:%d\n", __func__, __LINE__);
    if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (android_foldable_any_folded_area_configured()) {
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (resizableEnabled()) {
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_TV ||
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_WEAR) {
        LOG(ERROR) << "Multidisplay does not support TV or WEAR";
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (mGuestMode) {
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (add && !multiDisplayParamValidate(id, w, h, dpi, flag)) {
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (flag == 0 &&
        avdInfo_getAvdFlavor(
            getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO) {
        flag = automotive::getDefaultFlagsForDisplay(id);
        LOG(DEBUG) << "Setting flags " << flag << " for display id " << id;
    }


    SkinLayout* layout = (SkinLayout*) getConsoleAgents()->emu->getLayout();
    if (layout) {
        rotation = layout->orientation;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if (rotation != SKIN_ROTATION_0) {
        mWindowAgent->showMessage(
                "Please apply multiple displays without rotation",
                WINDOW_MESSAGE_ERROR, 1000);
        return -1;
    }

    printf("%s:%d\n", __func__, __LINE__);
    if(hotPlugDisplayEnabled()) {
        if (add) {
            mVmAgent->setDisplay(id, w, h, dpi);
        } else {
            mVmAgent->setDisplay(id, 0, 0, 0);
        }
    } else {
        if (add) {
            ret = createDisplay(&id);
            if (ret != 0) {
                return ret;
            }
            ret = setDisplayPose(id, x, y, w, h, dpi, flag);
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
            derror("Adb unavailable, not starting multidisplay "
                   "service in android. Please install adb and restart the "
                   "emulator. Multi display might not work as expected.");
            return -1;
        }
        adbInterface->enqueueCommand({"shell", "am", "broadcast", "-a",
                                      "com.android.emulator.multidisplay.START",
                                      "-n",
                                      "com.android.emulator.multidisplay/"
                                      ".MultiDisplayServiceReceiver"});

        MultiDisplayPipe* pipe = MultiDisplayPipe::getInstance();
        if (pipe) {
            std::vector<uint8_t> data;
            pipe->fillData(data, id, w, h, dpi, flag, add);
            LOG(DEBUG) << "MultiDisplayPipe send " << (add ? "add" : "del")
                         << " id " << id << " width " << w << " height " << h
                         << " dpi " << dpi << " flag " << flag;
            pipe->send(std::move(data));
        }
    }

    printf("%s:%d\n", __func__, __LINE__);
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
#if MULTI_DISPLAY_DEBUG
    LOG(DEBUG) << "getMultiDisplay " << id << " x " << mMultiDisplay[id].pos_x
                 << " y " << mMultiDisplay[id].pos_y << " w "
                 << mMultiDisplay[id].width << " h " << mMultiDisplay[id].height
                 << " dpi " << mMultiDisplay[id].dpi << " flag "
                 << mMultiDisplay[id].flag << " enable "
                 << mMultiDisplay[id].enabled;
#endif
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

bool MultiDisplay::translateCoordination(uint32_t* x,
                                         uint32_t* y,
                                         uint32_t* displayId) {
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
        derror("Cannot create a display, the displayId is a nullptr");
        return -1;
    }

    AutoLock lock(mLock);

    if (mMultiDisplay.size() > s_maxNumMultiDisplay) {
        derror("Failed to create display. The limit of %d displays has been "
               "reached.", s_maxNumMultiDisplay);
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
        derror("Failed to create display. The limit of %d displays has been "
               "reached.", s_maxNumMultiDisplay);
        return -1;
    }

    mMultiDisplay.emplace(*displayId, MultiDisplayInfo());
    LOG(DEBUG) << "create display " << *displayId;
    fireEvent(DisplayChangeEvent{DisplayChange::DisplayAdded, *displayId});
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
        if (needUIUpdate && !isMultiDisplayWindow()) {
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
        if (states.displayId == displayId &&
            states.state == RECORDER_RECORDING) {
            mRecordAgent->stopRecording();
        }
        if (isMultiDisplayWindow()) {
            mWindowAgent->addMultiDisplayWindow(displayId, false, 0, 0);
        } else {
            mWindowAgent->setUIDisplayRegion(0, 0, width, height, true);
        }
        if (restoreSkin) {
            mWindowAgent->restoreSkin();
        }
    }
    LOG(DEBUG) << "delete display " << displayId;
    fireEvent(DisplayChangeEvent{DisplayChange::DisplayRemoved, displayId});
    return 0;
}

int MultiDisplay::setDisplayPose(uint32_t displayId,
                                 int32_t x,
                                 int32_t y,
                                 uint32_t w,
                                 uint32_t h,
                                 uint32_t dpi,
                                 uint32_t flag) {
    bool UIUpdate = false;
    bool checkRecording = false;
    uint32_t width, height;
    if (mGuestMode) {
        return -1;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            derror("Display with id: %d does not exist", displayId);
            return -1;
        }
        if (mMultiDisplay[displayId].cb != 0 &&
            (mMultiDisplay[displayId].width != w ||
             mMultiDisplay[displayId].height != h)) {
            checkRecording = true;
        }
        mMultiDisplay[displayId].width = w;
        mMultiDisplay[displayId].height = h;
        mMultiDisplay[displayId].dpi = dpi;
        mMultiDisplay[displayId].flag = flag;
        mMultiDisplay[displayId].pos_x = x;
        mMultiDisplay[displayId].pos_y = y;
        if (mMultiDisplay[displayId].cb != 0) {
            if (!isMultiDisplayWindow()) {
                if (x == -1 && y == -1) {
                    recomputeLayoutLocked();
                }
                getCombinedDisplaySizeLocked(&width, &height);
            }
            UIUpdate = true;
        }
    }
    if (checkRecording) {
        // stop recording of this display if it is happening.
        RecorderStates states = mRecordAgent->getRecorderState();
        if (states.displayId == displayId &&
            states.state == RECORDER_RECORDING) {
            mRecordAgent->stopRecording();
        }
    }
    if (UIUpdate) {
        if (isMultiDisplayWindow()) {
            mWindowAgent->addMultiDisplayWindow(displayId, true, w, h);
        } else {
            mWindowAgent->setUIDisplayRegion(0, 0, width, height, true);
        }
    }
    LOG(DEBUG) << "setDisplayPose " << displayId << " x " << x << " y " << y
                 << " w " << w << " h " << h << " dpi " << dpi;

    fireEvent(DisplayChangeEvent{DisplayChange::DisplayChanged, displayId});
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
        derror("Display with id: %d does not exist", displayId);
        return -1;
    }
    *x = mMultiDisplay[displayId].pos_x;
    *y = mMultiDisplay[displayId].pos_y;
    *w = mMultiDisplay[displayId].width;
    *h = mMultiDisplay[displayId].height;
    return 0;
}

int MultiDisplay::setDisplayColorBuffer(uint32_t displayId,
                                        uint32_t colorBuffer) {
    uint32_t width, height, dpi;
    bool noSkin = false;
    bool needUpdate = false;
    if (mGuestMode) {
        return -1;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            derror("Display with id: %d does not exist", displayId);
            return -1;
        }
        if (mMultiDisplay[displayId].cb == colorBuffer) {
            return 0;
        }
        if (mMultiDisplay[displayId].cb == 0) {
            mMultiDisplay[displayId].cb = colorBuffer;
            // first time cb assigned, update the UI
            needUpdate = true;
            dpi = mMultiDisplay[displayId].dpi;
            if (isMultiDisplayWindow()) {
                width = mMultiDisplay[displayId].width;
                height = mMultiDisplay[displayId].height;
            } else {
                recomputeLayoutLocked();
                getCombinedDisplaySizeLocked(&width, &height);
                if (getNumberActiveMultiDisplaysLocked() == 2) {
                    // disable skin when first display set, index 0 is the
                    // default
                    // one.
                    noSkin = true;
                }
            }
        }
        mMultiDisplay[displayId].cb = colorBuffer;
    }
    if (noSkin) {
        LOG(DEBUG) << "disable skin";
        mWindowAgent->setNoSkin();
    }
    if (needUpdate) {
        if (isMultiDisplayWindow()) {
            mWindowAgent->addMultiDisplayWindow(displayId, true, width, height);
        } else {
            // Explicitly adjust host window size
            LOG(DEBUG) << "change window size to " << width << "x" << height;
            mWindowAgent->setUIDisplayRegion(0, 0, width, height, true);
        }
    }
    LOG(DEBUG) << "setDisplayColorBuffer " << displayId << " cb "
                 << colorBuffer;
    return 0;
}

int MultiDisplay::getDisplayColorBuffer(uint32_t displayId,
                                        uint32_t* colorBuffer) {
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

int MultiDisplay::getColorBufferDisplay(uint32_t colorBuffer,
                                        uint32_t* displayId) {
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

bool MultiDisplay::notifyDisplayChanges() {
    fireEvent(
            DisplayChangeEvent{DisplayChange::DisplayTransactionCompleted, 0});
    return true;
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

bool MultiDisplay::isMultiDisplayWindow() {
    return getConsoleAgents()->settings->hw()->hw_multi_display_window;
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
 * by invoking setUIMultiDisplay() will be using top-left corner as origin.
 * Thus, input coordinates willl be calculated correctly when mouse events are
 * captured by QT window.
 *
 * TODO: We assume all displays pos_x/pos_y is adjustable here. This may
 * overwrite the specified pos_x/pos_y in setDisplayPos();
 */
void MultiDisplay::recomputeLayoutLocked() {
    uint32_t monitorWidth, monitorHeight;
    double monitorAspectRatio = 1.0;
    if (!mWindowAgent->getMonitorRect(&monitorWidth, &monitorHeight)) {
        dwarning(
                "Unable to get monitor width and height, using default "
                "ratio of 1.0");
    } else {
        monitorAspectRatio = (double)monitorHeight / (double)monitorWidth;
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

bool MultiDisplay::multiDisplayParamValidate(uint32_t id,
                                             uint32_t w,
                                             uint32_t h,
                                             uint32_t dpi,
                                             uint32_t flag) {
    // According the Android 9 CDD,
    // * 120 <= dpi <= 640
    // * 320 * (dpi / 160) <= width
    // * 320 * (dpi / 160) <= height
    //
    // Also we don't want a screen too big to limit the performance impact.
    // * 4K might be a good upper limit

    if (dpi < 120 || dpi > 640) {
        mWindowAgent->showMessage("dpi should be between 120 and 640",
                                  WINDOW_MESSAGE_ERROR, 1000);
        derror("Display dpi should be between 120 and 640, not %d", dpi);
        return false;
    }
    if (w < 320 * dpi / 160 || h < 320 * dpi / 160) {
        mWindowAgent->showMessage("width and height should be >= 320dp",
                                  WINDOW_MESSAGE_ERROR, 1000);
        derror("Display width and height should be >= 320dp, not %d", dpi);
        return false;
    }
    if (!((w <= 7680 && h <= 4320) || (w <= 4320 && h <= 7680))) {
        mWindowAgent->showMessage("resolution should not exceed 8k (7680*4320)",
                                  WINDOW_MESSAGE_ERROR, 1000);
        derror("Display resolution should not exceed 8k (7680x4320) vs (%dx%d)",
               w, h);
        return false;
    }
    if (id > s_maxNumMultiDisplay) {
        std::string msg = "Display index cannot be more than " +
                          std::to_string(s_maxNumMultiDisplay);
        mWindowAgent->showMessage(msg.c_str(), WINDOW_MESSAGE_ERROR, 1000);
        derror("%s", msg.c_str());
        return false;
    }
    return true;
}

std::map<uint32_t, MultiDisplayInfo> MultiDisplay::parseConfig() {
    std::map<uint32_t, MultiDisplayInfo> ret;
    if (!getConsoleAgents()->settings->android_cmdLineOptions()->multidisplay) {
        return ret;
    }
    std::string s = getConsoleAgents()
                            ->settings->android_cmdLineOptions()
                            ->multidisplay;
    std::vector<uint32_t> params;
    size_t last = 0, next = 0;
    while ((next = s.find(",", last)) != std::string::npos) {
        params.push_back(std::stoi(s.substr(last, next - last)));
        last = next + 1;
    }
    params.push_back(std::stoi(s.substr(last)));
    if (params.size() < 5 || params.size() % 5 != 0) {
        derror("A multidisplay command should be at least 5 and be a multiple "
               "of 5, not %d",
               params.size());
        return ret;
    }
    int i = 0;
    for (i = 0; i < params.size(); i += 5) {
        if (params[i] == 0 || params[i] > 3) {
            derror("Multidisplay index should be 1, 2, or 3 not %d", params[i]);
            ret.clear();
            return ret;
        }
        if (!multiDisplayParamValidate(params[i], params[i + 1], params[i + 2],
                                      params[i + 3], params[i + 4])) {
            derror("Invalid index: %d, width: %d, height: %d, or dpi: %d "
                   "for multidisplay command,",
                   params[i], params[i + 1], params[i + 2], params[i + 3]);
            ret.clear();
            return ret;
        }
        ret.emplace(params[i],
                    MultiDisplayInfo(-1, -1, params[i + 1], params[i + 2],
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
        LOG(DEBUG) << "config multidisplay with command-line";
        for (const auto& i : info) {
            setMultiDisplay(i.first, -1, -1, i.second.width, i.second.height,
                            i.second.dpi, i.second.flag, true);
            mWindowAgent->updateUIMultiDisplayPage(i.first);
        }
    } else {
        LOG(DEBUG) << "config multidisplay with config.ini "
                     << getConsoleAgents()->settings->hw()->hw_display1_width << "x"
                     << getConsoleAgents()->settings->hw()->hw_display1_height << " "
                     << getConsoleAgents()->settings->hw()->hw_display2_width << "x"
                     << getConsoleAgents()->settings->hw()->hw_display2_height << " "
                     << getConsoleAgents()->settings->hw()->hw_display3_width << "x"
                     << getConsoleAgents()->settings->hw()->hw_display3_height;
        if (getConsoleAgents()->settings->hw()->hw_display1_width != 0 &&
            getConsoleAgents()->settings->hw()->hw_display1_height != 0) {
            LOG(DEBUG) << " add display 1";
            setMultiDisplay(1, getConsoleAgents()->settings->hw()->hw_display1_xOffset,
                            getConsoleAgents()->settings->hw()->hw_display1_yOffset,
                            getConsoleAgents()->settings->hw()->hw_display1_width,
                            getConsoleAgents()->settings->hw()->hw_display1_height,
                            getConsoleAgents()->settings->hw()->hw_display1_density,
                            getConsoleAgents()->settings->hw()->hw_display1_flag, true);
            mWindowAgent->updateUIMultiDisplayPage(1);
        }
        if (getConsoleAgents()->settings->hw()->hw_display2_width != 0 &&
            getConsoleAgents()->settings->hw()->hw_display2_height != 0) {
            LOG(DEBUG) << " add display 2";
            setMultiDisplay(2, getConsoleAgents()->settings->hw()->hw_display2_xOffset,
                            getConsoleAgents()->settings->hw()->hw_display2_yOffset,
                            getConsoleAgents()->settings->hw()->hw_display2_width,
                            getConsoleAgents()->settings->hw()->hw_display2_height,
                            getConsoleAgents()->settings->hw()->hw_display2_density,
                            getConsoleAgents()->settings->hw()->hw_display2_flag, true);
            mWindowAgent->updateUIMultiDisplayPage(2);
        }
        if (getConsoleAgents()->settings->hw()->hw_display3_width != 0 &&
            getConsoleAgents()->settings->hw()->hw_display3_height != 0) {
            LOG(DEBUG) << " add display 3";
            setMultiDisplay(3, getConsoleAgents()->settings->hw()->hw_display3_xOffset,
                            getConsoleAgents()->settings->hw()->hw_display3_yOffset,
                            getConsoleAgents()->settings->hw()->hw_display3_width,
                            getConsoleAgents()->settings->hw()->hw_display3_height,
                            getConsoleAgents()->settings->hw()->hw_display3_density,
                            getConsoleAgents()->settings->hw()->hw_display3_flag, true);
            mWindowAgent->updateUIMultiDisplayPage(3);
        }
    }
}

bool MultiDisplay::hotPlugDisplayEnabled() {
    if (featurecontrol::isEnabled(android::featurecontrol::Minigbm) &&
        ((getConsoleAgents()->settings->android_cmdLineOptions()->hotplug_multi_display ||
         getConsoleAgents()->settings->hw()->hw_hotplug_multi_display))) {
        LOG(DEBUG) << "use hotplug multiDisplay";
        return true;
    }
    return false;
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
    base::loadCollection(
            stream, &displaysOnLoad,
            [this](base::Stream* stream)
                    -> std::map<uint32_t, MultiDisplayInfo>::value_type {
                const uint32_t idx = stream->getBe32();
                const int32_t pos_x = stream->getBe32();
                const int32_t pos_y = stream->getBe32();
                const uint32_t width = stream->getBe32();
                const uint32_t height = stream->getBe32();
                const uint32_t dpi = stream->getBe32();
                const uint32_t flag = stream->getBe32();
                const uint32_t cb = stream->getBe32();
                const bool enabled = stream->getByte();
                return {idx,
                        {pos_x, pos_y, width, height, dpi, flag, enabled, cb}};
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
        for (const auto& iter : displaysOnLoad) {
            ids.insert(iter.first);
        }
        activeBeforeLoad = getNumberActiveMultiDisplaysLocked() > 1;
        if (activeBeforeLoad && isMultiDisplayWindow()) {
            for (auto const& c : mMultiDisplay) {
                if (c.second.cb > 0) {
                    mWindowAgent->addMultiDisplayWindow(c.first, false, 0, 0);
                }
            }
        }
        mMultiDisplay.clear();
        mMultiDisplay = displaysOnLoad;
        activeAfterLoad = getNumberActiveMultiDisplaysLocked() > 1;
        if (activeAfterLoad && isMultiDisplayWindow()) {
            for (auto const& c : mMultiDisplay) {
                if (c.second.cb > 0) {
                    mWindowAgent->addMultiDisplayWindow(
                            c.first, true, c.second.width, c.second.height);
                }
            }
        }
        getCombinedDisplaySizeLocked(&combinedDisplayWidth,
                                     &combinedDisplayHeight);
    }
    if (!isMultiDisplayWindow()) {
        if (activeAfterLoad) {
            if (!activeBeforeLoad) {
                mWindowAgent->setNoSkin();
            }
            mWindowAgent->setUIDisplayRegion(0, 0, combinedDisplayWidth,
                                             combinedDisplayHeight, true);
        } else if (activeBeforeLoad) {
            mWindowAgent->setUIDisplayRegion(0, 0, combinedDisplayWidth,
                                             combinedDisplayHeight, true);
            mWindowAgent->restoreSkin();
        }
    }
    for (const auto& iter : ids) {
        mWindowAgent->updateUIMultiDisplayPage(iter);
    }
}

}  // namespace android

void android_init_multi_display(
        const QAndroidEmulatorWindowAgent* const windowAgent,
        const QAndroidRecordScreenAgent* const recordAgent,
        const QAndroidVmOperations* const vmAgent,
        bool isGuestMode) {
    android::sMultiDisplay = new android::MultiDisplay(windowAgent, recordAgent,
                                                       vmAgent, isGuestMode);
}

extern "C" {
void android_load_multi_display_config() {
    if (!android::sMultiDisplay) {
        derror("Multidisplay has not yet been initialized, not loading configuration");
        return;
    }
    android::sMultiDisplay->loadConfig();
}
}
