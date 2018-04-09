/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include "android/recording/screen-recorder.h"
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidRecordScreenAgent {
    // Start recording. Returns false if already recording.
    // |recordingInfo| is the recording information the encoder should use. At
    // the minimum, the filename cannot be null. For the other parameters, if
    // the value is invalid, default values will be used in place of them.
    bool (*startRecording)(const RecordingInfo* recordingInfo);
    // Async version of startRecording(). Use |recordingInfo->cb| to get the
    // recording state.
    bool (*startRecordingAsync)(const RecordingInfo* recordingInfo);

    // Stop recording.
    bool (*stopRecording)(void);
    // Async version of stopRecording(). Use |recordingInfo->cb| to get the
    // recording state.
    bool (*stopRecordingAsync)(void);
    // Get the state of the recorder.
    RecorderState (*getRecorderState)(void);
    // Take a screenshot.
    void (*doSnap)(const char* dirname);

    bool (*startWebRtcModule)(const char* module, int fps);

    bool (*stopWebRtcModule)();
} QAndroidRecordScreenAgent;

ANDROID_END_HEADER
