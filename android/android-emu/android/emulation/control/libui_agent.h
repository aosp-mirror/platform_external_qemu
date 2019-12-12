// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

struct EGLDispatch;
struct GLESv2Dispatch;

#ifdef __cplusplus
namespace android {
namespace videoplayer {
class VideoPlayer;
class VideoPlayerNotifier;
}  // namespace videoplayer
}  // namespace android
using VideoPlayer = android::videoplayer::VideoPlayer;
using VideoPlayerNotifier = android::videoplayer::VideoPlayerNotifier;
class VideoPlayerRenderTarget;
#else
struct VideoPlayer;
struct VideoPlayerNotifier;
struct VideoPlayerRenderTarget;
typedef struct VideoPlayer  VideoPlayer;
typedef struct VideoPlayerNotifier VideoPlayerNotifier;
typedef struct VideoPlayerRenderTarget VideoPlayerRenderTarget;
#endif

typedef void (*LibuiKeyCodeSendFunc)(int* codes, int count, void* context);

typedef struct QAndroidLibuiAgent {
    // Converts the passed text into a set of UI input events and sends them
    // using |sendFunc|.
    bool (*convertUtf8ToKeyCodeEvents)(const unsigned char* text,
                                       int len,
                                       LibuiKeyCodeSendFunc sendFunc);

    // Requests UI code to gracefully shut down and exit. Doesn't wait for it to
    // complete.
    void (*requestExit)(int exitCode, const char* message);

    // Requests UI code to restart the emulator. Also doesn't wait.
    void (*requestRestart)(int exitCode, const char* message);

    void (*show_virtual_scene_controls)(bool show);
    const struct EGLDispatch* (*eglDispatcher)();
    const struct GLESv2Dispatch* (*glesV2Dispatcher)();
    // Note that the video player will take ownership of the notifier.
    VideoPlayer* (*createVideoPlayer)(const char* path,
                                      VideoPlayerRenderTarget* renderTarget,
                                      VideoPlayerNotifier* notifier);

} QAndroidLibuiAgent;

ANDROID_END_HEADER
