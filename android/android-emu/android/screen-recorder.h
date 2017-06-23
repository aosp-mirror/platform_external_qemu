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

#include "android/utils/compiler.h"
#include "android/utils/looper.h"

ANDROID_BEGIN_HEADER
// Initializes internal global structure. Call this before doing any recording
// operations. |w| and |h| are the FrameBuffer width and height.
extern void screen_recorder_init(bool isGuestMode, int w, int h);
// Starts recording the screen. When stopped, the file will be saved as
// |filename|. Returns true if recorder started recording, false if it failed.
extern bool screen_recorder_start(const char* filename);
// Stop recording. After calling this function, the file will be complete and at
// |filename|.
extern void screen_recorder_stop(void);
ANDROID_END_HEADER


