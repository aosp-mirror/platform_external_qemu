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

#include <stddef.h>
#include <algorithm>
#include <cstdint>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "aemu/base/LayoutResolver.h"
#include "aemu/base/files/Stream.h"
#include "aemu/base/files/StreamSerializing.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/logging/LogSeverity.h"
#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/AutoDisplays.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulation/resizable_display_config.h"
#include "android/hw-sensors.h"
#include "android/skin/file.h"
#include "host-common/FeatureControl.h"
#include "host-common/Features.h"
#include "host-common/MultiDisplayPipe.h"
#include "host-common/misc.h"
#include "host-common/screen-recorder.h"

using android::base::AutoLock;
using android::automotive::DisplayManager;

#define MULTI_DISPLAY_DEBUG 1

/* set  >1 for very verbose debugging */
#if MULTI_DISPLAY_DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

#if MULTI_DISPLAY_DEBUG >= 3
#define D(fmt, ...) fprintf(stderr, "%s %d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define D(...) (void)0
#endif

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

    LOG(DEBUG) << "setMultiDisplay id " << id << " " << x << " " << y << " "
               << w << " " << h << " " << dpi << " " << flag << " "
               << (add ? "add" : "del");
    if (!featurecontrol::isEnabled(android::featurecontrol::MultiDisplay)) {
        return -ENODEV;
    }

    if (android_foldable_any_folded_area_configured()) {
        return -ENOSYS;
    }

    if (resizableEnabled()) {
        return -ENOSYS;
    }

    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
                AVD_TV ||
        avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
                AVD_WEAR) {
        LOG(ERROR) << "Multidisplay does not support TV or WEAR";
        return -ENOSYS;
    }

    if (mGuestMode) {
        return -ENOSYS;
    }

    if (add && !multiDisplayParamValidate(id, w, h, dpi, flag)) {
        return -EINVAL;
    }

    if (flag == 0) {
        auto avd = getConsoleAgents()->settings->avdInfo();
        if (avd) {
            if (avdInfo_getAvdFlavor(avd) == AVD_ANDROID_AUTO) {
                flag = automotive::getDefaultFlagsForDisplay(id);
                LOG(DEBUG) << "Setting flags " << flag << " for display id " << id;
            } else if (avdInfo_getApiLevel(avd) >= 31) {
                // bug: 227218392
                // starting from S (android 11, api 31), this flag is
                // required to support Presentation API
                const int DEFAULT_FLAGS_SINCE_S = DisplayManager::VIRTUAL_DISPLAY_FLAG_PUBLIC |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_ROTATES_WITH_CONTENT |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_TRUSTED |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_SUPPORTS_TOUCH |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_SHOULD_SHOW_SYSTEM_DECORATIONS |
                    DisplayManager::VIRTUAL_DISPLAY_FLAG_PRESENTATION;
                flag = DEFAULT_FLAGS_SINCE_S;
            }
        }
    }

    SkinLayout* layout = (SkinLayout*)getConsoleAgents()->emu->getLayout();
    if (layout) {
        rotation = layout->orientation;
    }

    if (rotation != SKIN_ROTATION_0) {
        mWindowAgent->showMessage(
                "Please apply multiple displays without rotation",
                WINDOW_MESSAGE_ERROR, 1000);
        return -EINVAL;
    }

    if (hotPlugDisplayEnabled()) {
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
            return -EPIPE;
        }

        {
            auto avd = getConsoleAgents()->settings->avdInfo();
            if (avd && avdInfo_getAvdFlavor(avd) == AVD_ANDROID_AUTO && avdInfo_getApiLevel(avd) <= 33) {
                adbInterface->enqueueCommand(
                    {"shell", "am", "broadcast", "-a",
                     "com.android.emulator.multidisplay.START", "-n",
                     "com.android.emulator.multidisplay/"
                     ".MultiDisplayServiceReceiver"});
            } else {
                adbInterface->enqueueCommand(
                    {"shell", "am", "broadcast", "-a",
                     "com.android.emulator.multidisplay.START", "-n",
                     "com.android.emulator.multidisplay/"
                     ".MultiDisplayServiceReceiver",
                     "--user 0"});
            }
        }

        MultiDisplayPipe* pipe = MultiDisplayPipe::getInstance();
        if (pipe) {
            std::vector<uint8_t> data;
            pipe->fillData(data, id, w, h, dpi, flag, add);
            LOG(DEBUG) << "MultiDisplayPipe send " << (add ? "add" : "del")
                       << " id " << id << " width " << w << " height " << h
                       << " dpi " << dpi << " flag " << flag;
            pipe->send(std::move(data));
        } else {
            LOG(ERROR) << "MultiDisplayPipe is not up yet. Please try again.";
            return -EPIPE;
        }
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
        *w = mMultiDisplay[id].originalWidth;
    }
    if (h) {
        *h = mMultiDisplay[id].originalHeight;
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
            *w = i->second.originalWidth;
        }
        if (h) {
            *h = i->second.originalHeight;
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

    if (android_foldable_is_pixel_fold()) {
        if (android_foldable_is_folded()) {
            *displayId = android_foldable_pixel_fold_second_display_id();
        } else {
            constexpr int primary_display_id = 0;
            *displayId = primary_display_id;
        }
        return true;
    }

    bool isPortrait = true;
    bool isNormalOrder = true;
    SkinLayout* layout = (SkinLayout*)getConsoleAgents()->emu->getLayout();
    if (layout) {
        auto rotation = layout->orientation;
        if (rotation == SKIN_ROTATION_90 || rotation == SKIN_ROTATION_270) {
            isPortrait = false;
        }
        if (rotation == SKIN_ROTATION_90 || rotation == SKIN_ROTATION_180) {
            isNormalOrder = false;
        }
    }

    D("input x %d y %d\n", *x, *y);
    if (isPortrait) {
        if (isNormalOrder) {
            // this is rotation 0
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
        } else {
            // this is rotation 180
            // origin of incoming x and y from QT is at right-bottom
            // positive x is pointing to west;
            // positive y is ponting to north
            // display 0 is at right most
            AutoLock lock(mLock);
            uint32_t totalH, totalW, pos_x, pos_y, w, h;
            getCombinedDisplaySizeLocked(&totalW, &totalH);
            uint32_t currXoffset = 0;
            for (const auto iter : mMultiDisplay) {
                if (iter.first != 0 && iter.second.cb == 0) {
                    continue;
                }
                const auto normal_x = totalW - *x;  // this is points to east
                pos_x = iter.second.pos_x;
                pos_y = totalH - iter.second.height - iter.second.pos_y;
                w = iter.second.width;
                h = iter.second.height;
                const auto delta = totalH - iter.second.height;
                if ((normal_x - pos_x) < w && (*y - delta) <= h && (*y -delta) >=0) {
                    *x = *x - currXoffset;
                    *displayId = iter.first;
                    *y = *y - delta;
                    D("display %d x %d y %d\n\n", *displayId, *x, *y);
                    return true;
                }
                currXoffset += w;
            }
        }
    } else {  // landscape mode, rotated
        if (isNormalOrder) {
            // this is rotation 270
            AutoLock lock(mLock);
            uint32_t totalH, pos_x, pos_y, w, h;
            getCombinedDisplaySizeLocked(nullptr, &totalH);
            uint32_t currYoffset = 0;
            for (const auto iter : mMultiDisplay) {
                if (iter.first != 0 && iter.second.cb == 0) {
                    continue;
                }
                w = iter.second.width;
                h = iter.second.height;
                pos_x = iter.second.pos_x;
                pos_y = iter.second.pos_y;
                uint32_t dispId = iter.first;
                if ((*x - pos_y) < h && (*y - pos_x) < w && (*y - pos_x) >= 0) {
                    auto myy = *x - pos_y;
                    auto myx = *y - pos_x;
                    *y = myx;
                    *x = myy;
                    *displayId = iter.first;
                    return true;
                }
                currYoffset += h;
            }
        } else {
            // this is rotation 90
            // origin is at upper right corner
            // x points south, y points west
            // display 0 at the top
            AutoLock lock(mLock);
            uint32_t totalH, totalW, pos_x, pos_y, w, h;
            getCombinedDisplaySizeLocked(&totalW, &totalH);
            uint32_t currYoffset = 0;
            for (const auto iter : mMultiDisplay) {
                if (iter.first != 0 && iter.second.cb == 0) {
                    continue;
                }
                w = iter.second.width;
                h = iter.second.height;
                const auto delta_w = totalW - w;
                pos_x = iter.second.pos_x;
                pos_y = iter.second.pos_y;
                const auto normal_x = totalH - *x;  // this is points to north
                const auto normal_y = totalW - *y;  // this is points to north
                uint32_t dispId = iter.first;
                if ((normal_x - pos_y) < h && (normal_y - pos_x) < w &&
                    (normal_y - pos_x) >= 0) {
                    *x = *x - currYoffset;
                    *y -= delta_w;
                    *displayId = iter.first;
                    return true;
                }
                currYoffset += h;
            }
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
        return -ENOSYS;
    }

    if (displayId == nullptr) {
        derror("Cannot create a display, the displayId is a nullptr");
        return -EINVAL;
    }

    {
        AutoLock lock(mLock);

        if (mMultiDisplay.size() > s_maxNumMultiDisplay) {
            derror("Failed to create display. The limit of %d displays has "
                   "been "
                   "reached.",
                   s_maxNumMultiDisplay);
            return -EINVAL;
        }
        if (mMultiDisplay.find(*displayId) != mMultiDisplay.end()) {
            return 0;
        }

        // displays created by internal rcCommands
        if (*displayId == s_invalidIdMultiDisplay) {
            for (int i = s_displayIdInternalBegin; i < s_maxNumMultiDisplay;
                 i++) {
                if (mMultiDisplay.find(i) == mMultiDisplay.end()) {
                    *displayId = i;
                    break;
                }
            }
        }
        if (*displayId == s_invalidIdMultiDisplay) {
            derror("Failed to create display. The limit of %d displays has "
                   "been "
                   "reached.",
                   s_maxNumMultiDisplay);
            return -EINVAL;
        }

        mMultiDisplay.emplace(*displayId, MultiDisplayInfo());
        LOG(DEBUG) << "create display " << *displayId;
    }

    fireEvent(DisplayChangeEvent{DisplayChange::DisplayAdded, *displayId});
    notifyDisplayChanges();
    return 0;
}

int MultiDisplay::destroyDisplay(uint32_t displayId) {
    uint32_t width, height;
    bool needUIUpdate = false;
    bool restoreSkin = false;

    if (mGuestMode) {
        return -ENOSYS;
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
    bool rotationChanged = false;
    bool checkRecording = false;
    uint32_t width, height;
    int32_t current_rotation = 0;

    SkinLayout* layout = (SkinLayout*)getConsoleAgents()->emu->getLayout();
    if (layout) {
        current_rotation = layout->orientation;
        if (current_rotation != mRotation) {
            rotationChanged = true;
        }
    }
    if (mGuestMode) {
        return -ENOSYS;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            derror("Display with id: %d does not exist", displayId);
            return -EINVAL;
        }
        bool displaySizeChanged = false;
        if (mMultiDisplay[displayId].cb != 0 &&
            (mMultiDisplay[displayId].width != w ||
             mMultiDisplay[displayId].height != h)) {
            checkRecording = true;
        }
        displaySizeChanged = (mMultiDisplay[displayId].width != w || mMultiDisplay[displayId].height != h);
        mMultiDisplay[displayId].width = w;
        mMultiDisplay[displayId].height = h;
        mMultiDisplay[displayId].originalWidth = w;
        mMultiDisplay[displayId].originalHeight = h;
        mMultiDisplay[displayId].dpi = dpi;
        mMultiDisplay[displayId].flag = flag;
        mMultiDisplay[displayId].pos_x = x;
        mMultiDisplay[displayId].pos_y = y;
        mMultiDisplay[displayId].rotation = current_rotation;
        if (mMultiDisplay[displayId].cb != 0) {
            if (!isMultiDisplayWindow()) {
                if (x == -1 && y == -1) {
                    recomputeLayoutLocked();
                }
                getCombinedDisplaySizeLocked(&width, &height);
            }
            if (!android_foldable_is_pixel_fold() && displayId > 0 && displaySizeChanged) {
                UIUpdate = true;
            }
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

void MultiDisplay::performRotation(int rot) {
    AutoLock lock(mLock);
    performRotationLocked(rot);
}

void MultiDisplay::performRotationLocked(int mOrientation) {

    if (android_foldable_is_pixel_fold()) {
        constexpr int primary_display_id = 0;
        const int secondary_display_id = android_foldable_pixel_fold_second_display_id();
        const bool second_display_exists = (mMultiDisplay.find(secondary_display_id) != mMultiDisplay.end());
        mMultiDisplay[primary_display_id].pos_x = 0;
        mMultiDisplay[primary_display_id].pos_y = 0;
        mMultiDisplay[primary_display_id].rotation = mOrientation;

        if (second_display_exists) {
            mMultiDisplay[secondary_display_id].pos_x = 0;
            mMultiDisplay[secondary_display_id].pos_y = 0;
            mMultiDisplay[secondary_display_id].rotation = mOrientation;
        }
        return;
    }

    const bool pileUp = (mOrientation == SKIN_ROTATION_90 ||
                         mOrientation == SKIN_ROTATION_270);
    const bool normalOrder = (mOrientation == SKIN_ROTATION_0 ||
                              mOrientation == SKIN_ROTATION_270);
    // set up the multidisplay windows x,y, w and h
    {
        if (normalOrder) {
            D("normal order orientation is %d\n", mOrientation);
            int pos_x, pos_y;
            uint32_t width, height, id, dpi, flag;
            uint32_t total_w = 0;
            uint32_t total_h = 0;
            for (auto& it : mMultiDisplay) {
                width = it.second.originalWidth;
                height = it.second.originalHeight;
                if (pileUp) {
                    std::swap(width, height);
                    total_w = std::max(total_w, width);
                }
            }
            for (auto& it : mMultiDisplay) {
                id = it.first;
                pos_x = it.second.pos_x;
                pos_y = it.second.pos_y;
                width = it.second.originalWidth;
                height = it.second.originalHeight;
                // stack upward
                if (pileUp) {
                    D("pile up %d\n", mOrientation);
                    pos_y = total_h;
                    std::swap(width, height);
                    total_h += height;
                    pos_x = total_w - width;
                } else {
                    D("not pile up %d\n", mOrientation);
                    pos_x = total_w;
                    pos_y = 0;
                    total_w += width;
                    // std::swap(pos_x, pos_y);
                }
                it.second.width = width;
                it.second.height = height;
                it.second.pos_x = pos_x;
                it.second.pos_y = pos_y;
                it.second.rotation = mOrientation;
                D("display id %d x %d y %d w %d h %d\n", id, pos_x, pos_y, width, height);
            }
        } else {
            D("not normal order orientation is %d\n", mOrientation);
            int pos_x, pos_y;
            uint32_t width, height, id, dpi, flag;
            uint32_t total_w = 0;
            uint32_t total_h = 0;
            for (auto& it : mMultiDisplay) {
                width = it.second.originalWidth;
                height = it.second.originalHeight;
                if (pileUp) {
                    std::swap(width, height);
                    total_h += height;
                } else {
                    total_w += width;
                    total_h = std::max(height, total_h);
                }
            }
            for (auto& it : mMultiDisplay) {
                id = it.first;
                pos_x = it.second.pos_x;
                pos_y = it.second.pos_y;
                width = it.second.originalWidth;
                height = it.second.originalHeight;
                // stack upward
                if (pileUp) {
                    D("pile up %d\n", mOrientation);
                    std::swap(width, height);
                    total_h -= height;
                    pos_x = 0;
                    pos_y = total_h;
                } else {
                    D("not pile up %d\n", mOrientation);
                    total_w -= width;
                    pos_x = total_w;
                    pos_y = total_h - height;
                    // std::swap(pos_x, pos_y);
                }
                it.second.width = width;
                it.second.height = height;
                it.second.pos_x = pos_x;
                it.second.pos_y = pos_y;
                it.second.rotation = mOrientation;
                D("display id %d x %d y %d w %d h %d\n", id, pos_x, pos_y, width, height);
            }
        }
    }
}

int MultiDisplay::getDisplayPose(uint32_t displayId,
                                 int32_t* x,
                                 int32_t* y,
                                 uint32_t* w,
                                 uint32_t* h) {
    if (mGuestMode) {
        return -ENOSYS;
    }
    AutoLock lock(mLock);
    if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
        derror("Display with id: %d does not exist", displayId);
        return -EINVAL;
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
        return -ENOSYS;
    }
    {
        AutoLock lock(mLock);
        if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
            derror("Display with id: %d does not exist", displayId);
            return -EINVAL;
        }
        if (mMultiDisplay[displayId].cb == colorBuffer) {
            return 0;
        }
        bool firstTimeColorbufferUpdate = false;
        if (mMultiDisplay[displayId].cb == 0) {
            firstTimeColorbufferUpdate = true;
            mMultiDisplay[displayId].cb = colorBuffer;
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
        // no need to update pixel_fold, as it has just one
        // active display; no need to do this for primary display
        if (!android_foldable_is_pixel_fold() && displayId > 0 && firstTimeColorbufferUpdate) {
            needUpdate = true;
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
    LOG(VERBOSE) << "setDisplayColorBuffer " << displayId << " cb "
               << colorBuffer;
    return 0;
}

int MultiDisplay::getDisplayColorBuffer(uint32_t displayId,
                                        uint32_t* colorBuffer) {
    if (mGuestMode) {
        return -ENOSYS;
    }
    AutoLock lock(mLock);
    if (mMultiDisplay.find(displayId) == mMultiDisplay.end()) {
        return -EINVAL;
    }
    *colorBuffer = mMultiDisplay[displayId].cb;
    return 0;
}

int MultiDisplay::getColorBufferDisplay(uint32_t colorBuffer,
                                        uint32_t* displayId) {
    if (mGuestMode) {
        return -ENOSYS;
    }
    AutoLock lock(mLock);
    for (const auto& iter : mMultiDisplay) {
        if (iter.second.cb == colorBuffer) {
            *displayId = iter.first;
            return 0;
        }
    }
    return -EINVAL;
}

void MultiDisplay::getCombinedDisplaySize(uint32_t* w, uint32_t* h) {
    AutoLock lock(mLock);
    getCombinedDisplaySizeLocked(w, h);
    D("combined w %d h %d\n", *w, *h);
}

bool MultiDisplay::notifyDisplayChanges() {
    fireEvent(
            DisplayChangeEvent{DisplayChange::DisplayTransactionCompleted, 0});
    return true;
}

void MultiDisplay::getCombinedDisplaySizeLocked(uint32_t* w, uint32_t* h) {
    uint32_t total_h = 0;
    uint32_t total_w = 0;
    if (android_foldable_is_pixel_fold()) {
        constexpr int primary_display_id = 0;
        const int secondary_display_id = android_foldable_pixel_fold_second_display_id();
        const bool second_display_exists = (mMultiDisplay.find(secondary_display_id) != mMultiDisplay.end());
        if (android_foldable_is_folded() && second_display_exists) {
            total_w = mMultiDisplay[secondary_display_id].originalWidth;
            total_h = mMultiDisplay[secondary_display_id].originalHeight;
        } else {
            total_w = mMultiDisplay[primary_display_id].originalWidth;
            total_h = mMultiDisplay[primary_display_id].originalHeight;
        }
        SkinRotation rotation = SKIN_ROTATION_0;
        SkinLayout* layout = (SkinLayout*)getConsoleAgents()->emu->getLayout();
        if (layout) {
            rotation = layout->orientation;
        }
        if (rotation == SKIN_ROTATION_90 || rotation == SKIN_ROTATION_270) {
            std::swap(total_w, total_h);
        }

    } else {
        for (const auto& iter : mMultiDisplay) {
            if (iter.first == 0 || iter.second.cb != 0) {
                total_h = std::max(total_h, iter.second.height + iter.second.pos_y);
                total_w = std::max(total_w, iter.second.width + iter.second.pos_x);
            }
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

bool MultiDisplay::isPixelFold() {
    return android_foldable_is_pixel_fold();
}

bool MultiDisplay::isOrientationSupported() {
    return getConsoleAgents()->settings->hw()->hw_sensors_orientation;
}

/*
 * Just use simple way to make it work in multidisplay+rotation
 */
void MultiDisplay::recomputeLayoutLocked() {
    SkinRotation rotation = SKIN_ROTATION_0;
    SkinLayout* layout = (SkinLayout*)getConsoleAgents()->emu->getLayout();
    if (layout) {
        rotation = layout->orientation;
    }
    performRotationLocked(rotation);
    if (android_is_automotive()) {
        // Apply stacked layout for automotive devices
        recomputeStackedLayoutLocked();
    }
}

/*
 * Use stacked layout
 */
void MultiDisplay::recomputeStackedLayoutLocked() {
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>
            rectangles, newRectangles;
    for (const auto& iter : mMultiDisplay) {
        if (iter.first == 0 || iter.second.cb != 0) {
            rectangles[iter.first] =
                    std::make_pair(iter.second.width, iter.second.height);
        }
    }
    const bool isDistantDisplay =
        android::automotive::isDistantDisplaySupported(getConsoleAgents()->settings->avdInfo());
    newRectangles = android::base::resolveStackedLayout(rectangles, isDistantDisplay);

    for (const auto& iter : newRectangles) {
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
        derror("%s", msg);
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
                   << getConsoleAgents()->settings->hw()->hw_display1_width
                   << "x"
                   << getConsoleAgents()->settings->hw()->hw_display1_height
                   << " "
                   << getConsoleAgents()->settings->hw()->hw_display2_width
                   << "x"
                   << getConsoleAgents()->settings->hw()->hw_display2_height
                   << " "
                   << getConsoleAgents()->settings->hw()->hw_display3_width
                   << "x"
                   << getConsoleAgents()->settings->hw()->hw_display3_height;
        if (getConsoleAgents()->settings->hw()->hw_display1_width != 0 &&
            getConsoleAgents()->settings->hw()->hw_display1_height != 0) {
            LOG(DEBUG) << " add display 1";
            setMultiDisplay(
                    1, getConsoleAgents()->settings->hw()->hw_display1_xOffset,
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
            setMultiDisplay(
                    2, getConsoleAgents()->settings->hw()->hw_display2_xOffset,
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
            setMultiDisplay(
                    3, getConsoleAgents()->settings->hw()->hw_display3_xOffset,
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
        ((getConsoleAgents()
                  ->settings->android_cmdLineOptions()
                  ->hotplug_multi_display ||
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
    if (!isMultiDisplayWindow() && !android_foldable_is_pixel_fold()) {
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
        fireEvent(DisplayChangeEvent{DisplayChange::DisplayChanged, iter});
        LOG(DEBUG) << "loaded display " << iter;
    }
    if (!ids.empty()) {
        notifyDisplayChanges();
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
        derror("Multidisplay has not yet been initialized, not loading "
               "configuration");
        return;
    }
    android::sMultiDisplay->loadConfig();
}
}
