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
#include "android/external-display/DisplayWindow.h"
#include "android/external-display/SDLRenderer.h"
//#include "android/skin/qt/video-player/VideoPlayerWidget.h"

//#include <QWidget>
#include <memory>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct AVFrame;

//class VideoPlayerWidget;

namespace android {
namespace display {

class SDLRender;
class SDLDisplayWindow;

struct PrivateData {
    int width = 0;
    int height = 0;
    SDLDisplayWindow* window;
    ExtDisplayWaitInfo waitInfo;
};

class SdlThread : public android::base::Thread {
public:
    SdlThread(PrivateData* pData) : mData(pData) {}

    // Main thread function
    virtual intptr_t main() override {
        sdl_thread_func(mData);
        return 0;
    }

private:
    PrivateData* mData = nullptr;
};

class SDLDisplayWindow : public DisplayWindow {
public:
    SDLDisplayWindow() = default;
    virtual ~SDLDisplayWindow() { delete mThread; }

    static SDLDisplayWindow* create(int width, int height);
    virtual void remove();
    virtual void resize(int width, int height);
    virtual void update(unsigned char* rgb, int size, int width, int height);
    virtual void update_yuv(AVFrame* frame,
                    int y_data_size,
                    unsigned char* y_data,
                    int u_data_size,
                    unsigned char* u_data,
                    int v_data_size,
                    unsigned char* v_data,
                    int width,
                    int height);
    virtual void notifyGuestOnClosing();

public:
    int mWidth = 1280;
    int mHeight = 720;
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Texture* mTexture = nullptr;
    bool mClosed = false;

    ExtDisplayWaitInfo mFrameWaitInfo;

    //static std::unique_ptr<android::videoplayer::VideoPlayerWidget> mVideoWidget;

private:    
    SdlThread* mThread = nullptr;
};

}  // namespace display
}  // namespace android
