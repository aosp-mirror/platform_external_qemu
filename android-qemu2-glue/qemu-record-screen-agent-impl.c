// Copyright 2015 The Android Open Source Project
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

#include "host-common/record_screen_agent.h"
#include "android/emulator-window.h"
#include "host-common/opengles.h"
#include "host-common/screen-recorder.h"

#include <stdbool.h>

static const QAndroidRecordScreenAgent sQAndroidRecordScreenAgent = {
        .startRecording = emulator_window_start_recording,
        .stopRecording = emulator_window_stop_recording,
        .startRecordingAsync = emulator_window_start_recording_async,
        .stopRecordingAsync = emulator_window_stop_recording_async,
        .getRecorderState = emulator_window_recorder_state_get,
        .doSnap = android_screenShot,
        .startSharedMemoryModule = start_shared_memory_module,
        .stopSharedMemoryModule = stop_shared_memory_module
};

const QAndroidRecordScreenAgent* const gQAndroidRecordScreenAgent =
        &sQAndroidRecordScreenAgent;
