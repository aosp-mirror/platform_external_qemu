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

#include "android/emulation/control/automation_agent.h"

using android::automation::AutomationController;

static android::automation::StartResult start_recording(
        android::base::StringView filename) {
    return AutomationController::get().startRecording(filename);
}

static android::automation::StopResult stop_recording() {
    return AutomationController::get().stopRecording();
}

static android::automation::StartResult start_playback(
        android::base::StringView filename) {
    return AutomationController::get().startPlayback(filename);
}

static android::automation::StopResult stop_playback() {
    return AutomationController::get().stopPlayback();
}

static const QAndroidAutomationAgent sQAndroidAutomationAgent = {
        .startRecording = start_recording,
        .stopRecording = stop_recording,
        .startPlayback = start_playback,
        .stopPlayback = stop_playback};

extern "C" const QAndroidAutomationAgent* const gQAndroidAutomationAgent =
        &sQAndroidAutomationAgent;
