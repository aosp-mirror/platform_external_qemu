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

#include <string_view>

namespace android {
namespace automation {

static void reset() {
    return AutomationController::get().reset();
}

static StartResult start_recording(std::string_view filename) {
    return AutomationController::get().startRecording(filename.data());
}

static StopResult stop_recording() {
    return AutomationController::get().stopRecording();
}

static StartResult start_playback(std::string_view filename) {
    return AutomationController::get().startPlayback(filename.data());
}

static StopResult stop_playback() {
    return AutomationController::get().stopPlayback();
}

static StartResult start_playback_with_callback(std::string_view filename,
                                                void (*onStopCallback)()) {
    return AutomationController::get().startPlaybackWithCallback(
            filename.data(), onStopCallback);
}

static void set_macro_name(std::string_view macroName,
                           std::string_view filename) {
    AutomationController::get().setMacroName(macroName, filename);
}

static std::string get_macro_name(std::string_view filename) {
    return AutomationController::get().getMacroName(filename);
}

static std::pair<uint64_t, uint64_t> get_metadata(std::string_view filename) {
    return AutomationController::get().getMetadata(filename);
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
        .getMetadata = get_metadata};

extern "C" const QAndroidAutomationAgent* const gQAndroidAutomationAgent =
        &sQAndroidAutomationAgent;

}  // namespace automation
}  // namespace android
