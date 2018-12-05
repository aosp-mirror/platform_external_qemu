/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include "android/base/async/Looper.h"
#include "android/emulation/control/AdbInterface.h"

// The EmulatorNoQtNoWindow class is used to build a QT-Widget-less
// event loop when the parameter -no-window is passed to the android
// emulator. Much like EmulatorQtWindow, startThread(f) will spawn
// a new thread to execute function f (qemu main loop).

class EmulatorNoQtNoWindow final {

public:
    using Ptr = std::shared_ptr<EmulatorNoQtNoWindow>;

    static void create();
    static EmulatorNoQtNoWindow* getInstance();
    static Ptr getInstancePtr();

    void startThread(std::function<void()> looperFunction);
    void requestClose();

private:
    explicit EmulatorNoQtNoWindow();

    android::base::Looper* mLooper;
    std::unique_ptr<android::emulation::AdbInterface> mAdbInterface;
};
