// Copyright (C) 2019 The Android Open Source Project
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

#include "KeyboardLayoutDetector.h"

#include "android/globals.h"
#include "android/skin/keyboard.h"

#include <string>
#include <vector>

using android::base::Looper;
using android::emulation::AdbInterface;
using android::emulation::OptionalAdbCommandResult;
using android::base::System;

namespace Ui {

KeyboardLayoutDetector::Ptr KeyboardLayoutDetector::create(
        AdbInterface* adb,
        Looper* looper,
        Looper::Duration checkIntervalMs) {
    auto inst =
            Ptr(new KeyboardLayoutDetector(adb, looper, checkIntervalMs));
    return inst;
}

KeyboardLayoutDetector::KeyboardLayoutDetector(
        AdbInterface* adb,
        Looper* looper,
        Looper::Duration checkIntervalMs)
    : mAdb(adb),
      mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      mShouldContinue(true),
      mRecurrentTask(
              looper,
              [this]() { return runKeyboardLayoutChangeCheck(); },
              checkIntervalMs) {}

bool KeyboardLayoutDetector::runKeyboardLayoutChangeCheck() {
    if(!guest_boot_completed){
        return mShouldContinue;
    }
    const char* curKeyboardLayout = skin_keyboard_current_active_layout();
    if (mKeyboardLayout.compare(curKeyboardLayout) != 0) {
        if (mChangeLayoutCmd != nullptr) {
            mChangeLayoutCmd->cancel();
            mChangeLayoutCmd.reset();
        }

        mKeyboardLayout = curKeyboardLayout;
        std::vector<std::string> inputCmds{"shell", "input",
                                      "physicalkeyboardlayout"};
        const char*  androidKeyboardLayout =
                skin_translate_host_kb_layout_to_android(mKeyboardLayout.c_str());
        if (androidKeyboardLayout) {
            inputCmds.emplace_back(androidKeyboardLayout);
            auto shared = shared_from_this();
            mChangeLayoutCmd = mAdb->runAdbCommand(
                    inputCmds,
                    [shared](const OptionalAdbCommandResult& result) {
                        // For now, run the adb command only once after device boots to prevent performance degradation
                        shared->mShouldContinue = false;
                        shared->mChangeLayoutCmd.reset();
                    },
                    System::kInfinite, true);
        }
    }
    return mShouldContinue;
}

void KeyboardLayoutDetector::start() {
    mRecurrentTask.start(false);
}

void KeyboardLayoutDetector::stop() {
    mRecurrentTask.stopAsync();
}

}  // namespace Ui
