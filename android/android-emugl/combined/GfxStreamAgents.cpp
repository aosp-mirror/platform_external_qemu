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
#include "GfxStreamAgents.h"

#include <stdint.h>  // for uint32_t
#include <stdio.h>   // for fprintf

#include <map>      // for map, __ma...
#include <utility>  // for pair

#include "android/emulation/HostmemIdMapping.h"             // for android_e...
#include "android/emulation/MultiDisplay.h"                 // for MultiDisp...
#include "android/emulation/control/callbacks.h"            // for LineConsu...
#include "android/emulation/control/multi_display_agent.h"  // for QAndroidM...
#include "android/emulation/control/vm_operations.h"        // for SnapshotC...
#include "android/emulation/control/window_agent.h"         // for WindowMes...
#include "android/skin/rect.h"                              // for SKIN_ROTA...

std::map<uint32_t, android::MultiDisplayInfo> mMultiDisplay;
static int32_t sDisplayX = 0;
static int32_t sDisplayY = 0;
static int32_t sDisplayW = 0;
static int32_t sDisplayH = 0;
static int32_t sDisplayDpi = 0;

static const QAndroidMultiDisplayAgent sMultiDisplayAgent = {
        .setMultiDisplay = [](uint32_t id,
                              int32_t x,
                              int32_t y,
                              uint32_t w,
                              uint32_t h,
                              uint32_t dpi,
                              uint32_t flag,
                              bool add) -> int {
            fprintf(stderr,
                    "setMultiDisplay (mock): %d, x: %d, y: %d, w: %d, h: %d, "
                    "dpi: %d, flag: %d\n",
                    id, x, y, w, h, dpi, flag);
            mMultiDisplay[id] =
                    android::MultiDisplayInfo(x, y, w, h, dpi, flag, add);
        },
        .getMultiDisplay = [](uint32_t id,
                              int32_t* x,
                              int32_t* y,
                              uint32_t* w,
                              uint32_t* h,
                              uint32_t* dpi,
                              uint32_t* flag,
                              bool* enable) -> bool {
            fprintf(stderr, "getMultiDisplay (mock) id %u\n", id);
            if (x)
                *x = mMultiDisplay[id].pos_x;
            if (y)
                *y = mMultiDisplay[id].pos_y;
            if (w)
                *w = mMultiDisplay[id].width;
            if (h)
                *h = mMultiDisplay[id].height;
            if (dpi)
                *dpi = mMultiDisplay[id].dpi;

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
            fprintf(stderr, "getNextMultiDisplay (mock) start_id %u\n",
                    start_id);
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
        .getCombinedDisplaySize = [](uint32_t* width, uint32_t* height) {},
        .multiDisplayParamValidate = [](uint32_t id,
                                        uint32_t w,
                                        uint32_t h,
                                        uint32_t dpi,
                                        uint32_t flag) -> bool { return true; },
        .translateCoordination =
                [](uint32_t* x, uint32_t* y, uint32_t* displayId) -> bool {
            return true;
        },
        .setGpuMode = [](bool isGuestMode, uint32_t w, uint32_t h) {},
        .createDisplay = [](uint32_t* displayId) -> int {
            mMultiDisplay.emplace(*displayId, android::MultiDisplayInfo());
            return 0;
        },
        .destroyDisplay = [](uint32_t displayId) -> int {
            fprintf(stderr, "destroyDisplay (mock): %d\n", displayId);
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
            if (x)
                *x = mMultiDisplay[displayId].pos_x;
            if (y)
                *y = mMultiDisplay[displayId].pos_y;
            if (w)
                *w = mMultiDisplay[displayId].width;
            if (h)
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
        }};

static bool sIsFolded = false;

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".getEmulatorWindow\n");
                    return (EmulatorWindow*)nullptr;
                },
        .rotate90Clockwise =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".rotate90Clockwise\n");
                    return true;
                },
        .rotate =
                [](SkinRotation rotation) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".rotate90Clockwise\n");
                    return true;
                },
        .getRotation =
                [](void) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .getRotation\n");
                    return SKIN_ROTATION_0;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .showMessage %s\n",
                            message);
                },
        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: "
                            ".showMessageWithDismissCallback %s\n",
                            message);
                },
        .fold =
                [](bool is_fold) -> bool {
                    fprintf(stderr, "window-agent-GfxStream-impl: .fold %d\n",
                            is_fold);
                    sIsFolded = is_fold;
                    return true;
                },
        .isFolded = [](void) -> bool { return sIsFolded; },
        .getFoldedArea = [](int* x, int* y, int* w, int* h) -> bool {
            fprintf(stderr, "window-agent-GfxStream-impl: .getFoldedArea\n");
            return true;
        },
        .updateFoldablePostureIndicator = [](bool) {
            fprintf(stderr, "window-agent-GfxStream-impl: updateFoldablePostureIndicator\n");
        },
        .setUIDisplayRegion =
                [](int x_offset, int y_offset, int w, int h) {
                    fprintf(stderr,
                            "window-agent-GfxStream-impl: .setUIDisplayRegion "
                            "%d %d %dx%d\n",
                            x_offset, y_offset, w, h);
                },
        .getMultiDisplay = 0,
        .setNoSkin = [](void) {},
        .restoreSkin = [](void) {},
        .updateUIMultiDisplayPage =
                [](uint32_t id) {
                    fprintf(stderr, "updateMultiDisplayPage\n");
                },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) -> bool {
                    if (w)
                        *w = 2500;
                    if (h)
                        *h = 1600;
                    return true;
                },
};

static const QAndroidVmOperations sQAndroidVmOperations = {
        .vmStop = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm stop\n");
            return true;
        },
        .vmStart = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm start\n");
            return true;
        },
        .vmReset =
                []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
        .vmShutdown =
                []() { fprintf(stderr, "goldfish-opengl vm ops: vm reset\n"); },
        .vmPause = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm pause\n");
            return true;
        },
        .vmResume = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm resume\n");
            return true;
        },
        .vmIsRunning = []() -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: vm is running\n");
            return true;
        },
        .snapshotList =
                [](void*, LineConsumerCallback, LineConsumerCallback) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot list\n");
            return true;
        },
        .snapshotSave = [](const char* name,
                           void* opaque,
                           LineConsumerCallback) -> bool {
            fprintf(stderr, "gfxstream vm ops: snapshot save\n");
            return true;
        },
        .snapshotLoad = [](const char* name,
                           void* opaque,
                           LineConsumerCallback) -> bool {
            fprintf(stderr, "gfxstream vm ops: snapshot load\n");
            return true;
        },
        .snapshotDelete = [](const char* name,
                             void* opaque,
                             LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot delete\n");
            return true;
        },
        .snapshotRemap = [](bool shared,
                            void* opaque,
                            LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot remap\n");
            return true;
        },
        .snapshotExport = [](const char* snapshot,
                             const char* dest,
                             void* opaque,
                             LineConsumerCallback errConsumer) -> bool {
            fprintf(stderr, "goldfish-opengl vm ops: snapshot export image\n");
            return true;
        },
        .setSnapshotCallbacks =
                [](void* opaque, const SnapshotCallbacks* callbacks) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set snapshot callbacks\n");
                },
        .mapUserBackedRam =
                [](uint64_t gpa, void* hva, uint64_t size) {
                    fprintf(stderr, "%s: map user backed ram\n", __func__);
                },
        .unmapUserBackedRam =
                [](uint64_t gpa, uint64_t size) {
                    fprintf(stderr, "%s: unmap user backed ram\n", __func__);
                },
        .getVmConfiguration =
                [](VmConfiguration* out) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: get vm configuration\n");
                },
        .setFailureReason =
                [](const char* name, int failureReason) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set failure reason\n");
                },
        .setExiting =
                []() {
                    fprintf(stderr, "goldfish-opengl vm ops: set exiting\n");
                },
        .allowRealAudio =
                [](bool allow) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: allow real audio\n");
                },
        .physicalMemoryGetAddr =
                [](uint64_t gpa) {
                    fprintf(stderr, "%s: physmemGetAddr\n", __func__);
                    return (void*)nullptr;
                },
        .isRealAudioAllowed =
                [](void) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: is real audiop allowed\n");
                    return true;
                },
        .setSkipSnapshotSave =
                [](bool used) {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: set skip snapshot save\n");
                },
        .isSnapshotSaveSkipped =
                []() {
                    fprintf(stderr,
                            "goldfish-opengl vm ops: is snapshot save "
                            "skipped\n");
                    return false;
                },
        .hostmemRegister = android_emulation_hostmem_register,
        .hostmemUnregister = android_emulation_hostmem_unregister,
        .hostmemGetInfo = android_emulation_hostmem_get_info,
};

namespace android {
namespace emulation {

const QAndroidVmOperations* const
GfxStreamAndroidConsoleFactory::android_get_QAndroidVmOperations() const {
    return &sQAndroidVmOperations;
}

const QAndroidMultiDisplayAgent* const
GfxStreamAndroidConsoleFactory::android_get_QAndroidMultiDisplayAgent() const {
    return &sMultiDisplayAgent;
}

const QAndroidEmulatorWindowAgent* const
GfxStreamAndroidConsoleFactory::android_get_QAndroidEmulatorWindowAgent()
        const {
    return &sQAndroidEmulatorWindowAgent;
}
}  // namespace emulation
}  // namespace android
