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

#include "KeyboardLayoutChangeDetector.h"

#include "android/globals.h"
#include "android/skin/keyboard.h"

#include <vector>
using android::base::Looper;
using android::emulation::AdbInterface;
using android::emulation::OptionalAdbCommandResult;
using android::base::System;
namespace Ui {

KeyboardLayoutChangeDetector::Ptr KeyboardLayoutChangeDetector::create(
        AdbInterface* adb,
        Looper* looper,
        Looper::Duration checkIntervalMs) {
    auto inst =
            Ptr(new KeyboardLayoutChangeDetector(adb, looper, checkIntervalMs));
    return inst;
}

KeyboardLayoutChangeDetector::KeyboardLayoutChangeDetector(
        AdbInterface* adb,
        Looper* looper,
        Looper::Duration checkIntervalMs)
    : mAdb(adb),
      mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      mRecurrentTask(
              looper,
              [this]() { return runKeyboardLayoutChangeCheck(); },
              checkIntervalMs) {}

bool KeyboardLayoutChangeDetector::runKeyboardLayoutChangeCheck() {
    if(!guest_boot_completed){
        return true;
    }

    std::string curKeyboardLayout(skin_keyboard_current_active_layout());
    if (curKeyboardLayout.compare(mKeyboardLayout) != 0) {
        if (mChangeLayoutCmd != nullptr) {
            mChangeLayoutCmd->cancel();
            mChangeLayoutCmd.reset();
        }
        mKeyboardLayout = curKeyboardLayout;
        const char* ptr = strrchr(curKeyboardLayout.c_str(), '.');
        if (ptr) {
            std::string lastItem(ptr+1);
            std::vector<std::string> inputCmds{"shell", "input", "physicalkeyboardlayout"};
            const char* android_kb_layout = nullptr;
            if ((android_kb_layout = skin_translate_host_kb_layout_to_android(mKeyboardLayout.c_str()))) {
                inputCmds.emplace_back(android_kb_layout);
            } else {
                inputCmds.emplace_back(std::move(curKeyboardLayout));
            }
            // The input command could be unavailable, thus we will change the keyboard layout
            // on the basis of best effort.
            auto shared = shared_from_this();
            mChangeLayoutCmd = mAdb->runAdbCommand(inputCmds, [shared](const OptionalAdbCommandResult& result) {
                shared->mChangeLayoutCmd.reset();
            }, System::kInfinite, false);
        }
    }
    return true;
}

void KeyboardLayoutChangeDetector::start() {
    mRecurrentTask.start(false);
}

void KeyboardLayoutChangeDetector::stop() {
    mRecurrentTask.stopAsync();
}

}  // namespace Ui
