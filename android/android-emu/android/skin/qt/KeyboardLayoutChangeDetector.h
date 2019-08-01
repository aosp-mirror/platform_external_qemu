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

// After boot completion, check if keyboard layout has been changed
// periodically. If so, change the Android system keyboard layout accordingly
// on a basis of best effort.
class KeyboardLayoutChangeDetector
    : public std::enable_shared_from_this<KeyboardLayoutChangeDetector> {
public:
    using Ptr = std::shared_ptr<KeyboardLayoutChangeDetector>;
    static KeyboardLayoutChangeDetector::Ptr create(
            android::emulation::AdbInterface* adb,
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs);
    void start();
    void stop();

protected:
    KeyboardLayoutChangeDetector(
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
    DISALLOW_COPY_AND_ASSIGN(KeyboardLayoutChangeDetector);
};

}  // namespace Ui
