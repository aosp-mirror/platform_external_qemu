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
#include "android/external-display/SDLRenderer.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct AVFrame;

namespace android {
namespace display {

class SDLRender;
class DisplayWindow;
class Mirroring;

struct ExtDisplayWaitInfo {
    android::base::Lock lock;  // protects other parts of this struct
    bool done = false;
    android::base::ConditionVariable cvDone;
};

struct PrivateData {
    int width = 0;
    int height = 0;
    DisplayWindow* window;
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

class DisplayWindow {
public:
    DisplayWindow() = default;
    virtual ~DisplayWindow() { delete mThread; }

    static DisplayWindow* create(int width, int height);
    void setMirroring(Mirroring* mirroring) { this->mMirroring = mirroring; }
    void remove();
    void resize(int width, int height);
    void update(unsigned char* rgb, int size, int width, int height);
    void update_yuv(AVFrame* frame,
                    int y_data_size,
                    unsigned char* y_data,
                    int u_data_size,
                    unsigned char* u_data,
                    int v_data_size,
                    unsigned char* v_data,
                    int width,
                    int height);
    void notifyGuestOnClosing();

public:
    int mWidth = 1280;
    int mHeight = 720;
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Texture* mTexture = nullptr;
    bool mClosed = false;

    ExtDisplayWaitInfo mFrameWaitInfo;

private:
    Mirroring* mMirroring = nullptr;
    SdlThread* mThread = nullptr;
};

}  // namespace display
}  // namespace android
