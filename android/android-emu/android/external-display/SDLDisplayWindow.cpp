// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/external-display/SDLDisplayWindow.h"
#include "android/external-display/Mirroring.h"
#include "android/external-display/SDLRenderer.h"
#include "android/utils/debug.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace android {
namespace display {


//std::unique_ptr<android::videoplayer::VideoPlayerWidget> mVideoWidget;

SDLDisplayWindow* SDLDisplayWindow::create(int width, int height) {
    PrivateData data;

    VERBOSE_PRINT(extdisplay, "SDLDisplayWindow::create(): width=%d, height=%d\n",
                  width, height);

    data.width = width;
    data.height = height;


    //mVideoWidget.reset(new android::videoplayer::VideoPlayerWidget());
    //mVideoWidget->resize(width, height);

    SdlThread* thread = new SdlThread(&data);
    thread->start();

    struct ExtDisplayWaitInfo* pwi = &data.waitInfo;
    android::base::AutoLock lock(pwi->lock);
    while (!pwi->done) {
        pwi->cvDone.wait(&pwi->lock);
    }

    data.window->mThread = thread;
    return data.window;
}

void SDLDisplayWindow::resize(int width, int height) {
    SDLRenderer::resize_sdl_window(this, width, height);
}

void SDLDisplayWindow::update(unsigned char* rgb,
                           int size,
                           int width,
                           int height) {}

void SDLDisplayWindow::update_yuv(AVFrame* frame,
                               int y_data_size,
                               unsigned char* y_data,
                               int u_data_size,
                               unsigned char* u_data,
                               int v_data_size,
                               unsigned char* v_data,
                               int width,
                               int height) {}

void SDLDisplayWindow::remove() {
    SDLRenderer::delete_sdl_window(this);
}

// tell the android side this display is closed
void SDLDisplayWindow::notifyGuestOnClosing() {
    printf("SDLDisplayWindow::notifyGuestOnClosing(): mirroring=%p\n",
           this->mMirroring);
    if (this->mMirroring != nullptr) {
        this->mMirroring->sendSinkStatus(false);
    }
}


// SdlExternalDisplayWindowProvider implementations
class SdlExternalDisplayWindowProvider : public ExternalDisplayWindowProvider
{
public:
    SdlExternalDisplayWindowProvider() = default;

    virtual DisplayWindow* createWindow(int width, int height) {
        return SDLDisplayWindow::create(width, height);
    }


    virtual void deleteWindow(DisplayWindow* win) {
        if (win != nullptr) {
            delete win;
        }
    }
};

void registerSDLExternalDisplayWindowProvider()
{
    Mirroring::registerExternalDisplayWindowProvider(new SdlExternalDisplayWindowProvider());
}

}  // namespace display
}  // namespace android
