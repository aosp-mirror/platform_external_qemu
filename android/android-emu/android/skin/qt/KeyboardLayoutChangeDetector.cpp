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

#include "android/skin/keyboard.h"

namespace Ui {

KeyboardLayoutChangeDetector::Ptr KeyboardLayoutChangeDetector::create(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs) {
    auto inst =
            Ptr(new KeyboardLayoutChangeDetector(adb, looper, checkIntervalMs));
    return inst;
}

KeyboardLayoutChangeDetector::KeyboardLayoutChangeDetector(
        android::emulation::AdbInterface* adb,
        android::base::Looper* looper,
        android::base::Looper::Duration checkIntervalMs)
    : mAdb(adb),
      mLooper(looper),
      mCheckIntervalMs(checkIntervalMs),
      mRecurrentTask(
              looper,
              [this]() { return runKeyboardLayoutCheck(); },
              checkIntervalMs),
      mKeyboardLayout(skin_keyboard_current_active_layout()) {}

bool KeyboardLayoutChangeDetector::runKeyboardLayoutCheck() {
    std::string curKeyboardLayout(skin_keyboard_current_active_layout());
    if (curKeyboardLayout.compare(mKeyboardLayout) != 0) {
        fprintf(stderr, "### change keyboard layout to %s \n", curKeyboardLayout.c_str());
        mKeyboardLayout = curKeyboardLayout;
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
