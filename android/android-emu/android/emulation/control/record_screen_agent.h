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

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stdint.h>

ANDROID_BEGIN_HEADER

typedef enum {
    RECORD_START_INITIATED,
    RECORD_STARTED,
    RECORD_START_FAILED,
    RECORD_STOP_INITIATED,
    RECORD_STOPPED,
    RECORD_STOP_FAILED,
} RecordingStatus;

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
typedef void (*RecordingCallback)(void* opaque, RecordingStatus status);
typedef enum {
    RECORDER_STARTING,
    RECORDER_RECORDING,
    RECORDER_STOPPING,
    RECORDER_STOPPED,
} RecorderState;


typedef struct {
    RecorderState state;
    uint32_t displayId;
} RecorderStates;


typedef struct RecordingInfo {
    const char* fileName;
    uint32_t width;
    uint32_t height;
    uint32_t videoBitrate;
    uint32_t timeLimit;
    uint32_t fps;
    uint32_t displayId;
    RecordingCallback cb;
    void* opaque;
} RecordingInfo;


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
    RecorderStates (*getRecorderState)(void);
    // Take a screenshot.
    bool (*doSnap)(const char* dirname, uint32_t displayId);

    // Setup a shared memory region. The framerate should ideally be fps
    // Returns the name of the memory handle, or null if initialization failed.
    const char* (*startSharedMemoryModule)(int desiredFps);

    bool (*stopSharedMemoryModule)();
} QAndroidRecordScreenAgent;

ANDROID_END_HEADER
