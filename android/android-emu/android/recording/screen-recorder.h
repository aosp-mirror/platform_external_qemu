// Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include "android/emulation/control/display_agent.h"
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER
// This callback will be called in the following scenarios:
//
//    1) The encoding has been stopped
//    2) The encoding is finished
//    3) An error has occurred while encoding was trying to finish.
//
// When screen_recorder_stop_async() is called, this callback will get called,
// with success set to 0. There is some time elapsed when we want to stop
// recording and when the encoding is actually finished, so we'll get a second
// call to the callback once the encoding is finished, with success set to 1. If
// any errors occur while stopping the recording, success will be set to -1.
typedef enum {
    RECORD_START_INITIATED,
    RECORD_STARTED,
    RECORD_START_FAILED,
    RECORD_STOP_INITIATED,
    RECORD_STOPPED,
    RECORD_STOP_FAILED,
} RecordingStatus;

typedef enum {
    RECORDER_STARTING,
    RECORDER_RECORDING,
    RECORDER_STOPPING,
    RECORDER_STOPPED,
} RecorderState;

typedef void (*RecordingCallback)(void* opaque, RecordingStatus status);

typedef struct RecordingInfo {
    const char* fileName;
    uint32_t width;
    uint32_t height;
    uint32_t videoBitrate;
    uint32_t timeLimit;
    RecordingCallback cb;
    void* opaque;
} RecordingInfo;

// Initializes internal global structure. Call this before doing any recording
// operations. |w| and |h| are the FrameBuffer width and height. |dpy_agent| is
// the display agent for recording in guest mode. If |dpy_agent| is NULL, then
// the recorder will assume it is in host mode.
extern void screen_recorder_init(uint32_t w,
                                 uint32_t h,
                                 const QAndroidDisplayAgent* dpy_agent);
// Starts recording the screen. When stopped, the file will be saved as
// |info->filename|. Set |async| true do not block as recording initialization
// takes time. Returns true if recorder started recording, false if it failed.
extern bool screen_recorder_start(const RecordingInfo* info, bool async);
// Stop recording. After calling this function, the encoder will stop processing
// frames. The encoder still needs to process any remaining frames it has, so
// calling this does not mean that the encoder has finished and |filename| is
// ready. Attach a RecordingStoppedCallback to get an update when the encoder
// has finished. Set |async| to false if you want to block until recording is
// finished.
extern bool screen_recorder_stop(bool async);
// Get the recorder's current state.
extern RecorderState screen_recorder_state_get(void);
// Starts the webrtc module
extern bool start_webrtc_module(const char* handle, int fps);
// Stops the webrtc module
extern bool stop_webrtc_module();
ANDROID_END_HEADER
