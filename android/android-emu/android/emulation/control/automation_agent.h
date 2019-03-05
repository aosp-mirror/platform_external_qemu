// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "android/automation/AutomationController.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef struct QAndroidAutomationAgent {
    // Start a recording to a file.
    android::automation::StartResult (*startRecording)(
            android::base::StringView filename);

    // Stops a recording to a file.
    android::automation::StopResult (*stopRecording)();

    // Start a playback from a file.
    android::automation::StartResult (*startPlayback)(
            android::base::StringView filename);

    // Stop playback from a file.
    android::automation::StopResult (*stopPlayback)();
} QAndroidAutomationAgent;

ANDROID_END_HEADER