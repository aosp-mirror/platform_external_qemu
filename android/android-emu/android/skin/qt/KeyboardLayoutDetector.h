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

#pragma once

#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/async/RecurrentTask.h"
#include "android/emulation/control/AdbInterface.h"

#include <memory>
#include <string>

namespace Ui {

// Once device boots, run checks periodically and use "adb shell input physicalkeyboard <layout>" to
// sync with guest. The input command could be unavailable, thus we will change
// the keyboard layout on the basis of best effort.
class KeyboardLayoutDetector
    : public std::enable_shared_from_this<KeyboardLayoutDetector> {
public:
    using Ptr = std::shared_ptr<KeyboardLayoutDetector>;
    static KeyboardLayoutDetector::Ptr create(
            android::emulation::AdbInterface* adb,
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs);
    void start();
    void stop();

protected:
    KeyboardLayoutDetector(
            android::emulation::AdbInterface* adb,
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs);

private:
    bool runKeyboardLayoutChangeCheck();
    android::emulation::AdbInterface* mAdb;
    android::emulation::AdbCommandPtr mChangeLayoutCmd;
    android::base::Looper* const mLooper;
    android::base::RecurrentTask mRecurrentTask;
    const android::base::Looper::Duration mCheckIntervalMs;
    std::string mKeyboardLayout;
    bool mShouldContinue;
    DISALLOW_COPY_AND_ASSIGN(KeyboardLayoutDetector);
};

}  // namespace Ui
