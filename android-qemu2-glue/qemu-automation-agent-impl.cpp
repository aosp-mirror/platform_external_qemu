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

namespace android {
namespace automation {

static void reset() {
    return AutomationController::get().reset();
}

static StartResult start_recording(android::base::StringView filename) {
    return AutomationController::get().startRecording(filename);
}

static StopResult stop_recording() {
    return AutomationController::get().stopRecording();
}

static StartResult start_playback(android::base::StringView filename) {
    return AutomationController::get().startPlayback(filename);
}

static StopResult stop_playback() {
    return AutomationController::get().stopPlayback();
}

static StartResult start_playback_with_callback(
        android::base::StringView filename,
        void (*onStopCallback)()) {
    return AutomationController::get().startPlaybackWithCallback(
            filename, onStopCallback);
}

static void set_macro_name(android::base::StringView macroName,
                           android::base::StringView filename) {
    AutomationController::get().setMacroName(macroName, filename);
}

static android::base::StringView get_macro_name(
        android::base::StringView filename) {
    return AutomationController::get().getMacroName(filename);
}

static uint64_t get_duration_ns(android::base::StringView filename) {
    return AutomationController::get().getDurationNs(filename);
}

static const QAndroidAutomationAgent sQAndroidAutomationAgent = {
        .reset = reset,
        .startRecording = start_recording,
        .stopRecording = stop_recording,
        .startPlayback = start_playback,
        .stopPlayback = stop_playback,
        .startPlaybackWithCallback = start_playback_with_callback,
        .setMacroName = set_macro_name,
        .getMacroName = get_macro_name,
        .getDurationNs = get_duration_ns};

extern "C" const QAndroidAutomationAgent* const gQAndroidAutomationAgent =
        &sQAndroidAutomationAgent;

}  // namespace automation
}  // namespace android
