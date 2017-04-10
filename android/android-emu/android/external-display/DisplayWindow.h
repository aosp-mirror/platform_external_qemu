// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details
//
// window.h

#pragma once

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Thread.h"
//#include "android/skin/qt/video-player/VideoPlayerWidget.h"

//#include <QWidget>
#include <memory>

struct AVFrame;

//class VideoPlayerWidget;

namespace android {
namespace display {

class Mirroring;

struct ExtDisplayWaitInfo {
    android::base::Lock lock;  // protects other parts of this struct
    bool done = false;
    android::base::ConditionVariable cvDone;
};

class DisplayWindow {
public:
    DisplayWindow() = default;
    virtual ~DisplayWindow() = default;

    virtual void remove() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void update(unsigned char* rgb, int size, int width, int height) = 0;
    virtual void update_yuv(AVFrame* frame,
                    int y_data_size,
                    unsigned char* y_data,
                    int u_data_size,
                    unsigned char* u_data,
                    int v_data_size,
                    unsigned char* v_data,
                    int width,
                    int height) = 0;
    virtual void notifyGuestOnClosing() = 0;

    void setMirroring(Mirroring* mirroring) { this->mMirroring = mirroring; }

protected:
    Mirroring* mMirroring = nullptr;
};

class ExternalDisplayWindowProvider
{
public:
    virtual DisplayWindow* createWindow(int width, int height) = 0;
    virtual void deleteWindow(DisplayWindow* win) = 0;
};

}  // namespace display
}  // namespace android
