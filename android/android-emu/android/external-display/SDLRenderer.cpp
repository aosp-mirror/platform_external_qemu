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

//
// SDLRenderer.cpp
//
// use SDL to render the mirroring video
//

#include "android/external-display/SDLRenderer.h"
#include "android/external-display/SDLDisplayWindow.h"
#include "android/utils/debug.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#if defined(_WIN32) || defined(__APPLE__)
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <list>

namespace android {
namespace display {

namespace {

const int USER_EVENT_CREATE_WINDOW = 1;
const int USER_EVENT_CLOSE_WINDOW = 2;
const int USER_EVENT_DRAW_FRAME = 3;

static ExtDisplayWaitInfo g_delete_waitInfo;

static bool g_has_event_loop = false;

// maintain a list of all created windows
std::list<SDLDisplayWindow*> g_windows;

};  // namespace

void SDLRenderer::draw_frame(DisplayWindow* pWin, AVFrame* frame) {
    SDLDisplayWindow* pSDLWin = (SDLDisplayWindow*)pWin;
    SDL_Event me;
    me.type = SDL_USEREVENT;
    me.user.code = USER_EVENT_DRAW_FRAME;
    me.user.data1 = pSDLWin;
    me.user.data2 = frame;
    SDL_PushEvent(&me);

    struct ExtDisplayWaitInfo* pwi = &pSDLWin->mFrameWaitInfo;
    android::base::AutoLock lock(pwi->lock);
    while (!pwi->done) {
        pwi->cvDone.wait(&pwi->lock);
    }
}

void SDLRenderer::draw_frame_internal(SDLDisplayWindow* pWin, AVFrame* frame) {
    SDL_Rect video_rect;
    video_rect.x = 0;
    video_rect.y = 0;
    video_rect.w = frame->width;
    video_rect.h = frame->height;
    SDL_UpdateYUVTexture(pWin->mTexture, &video_rect, frame->data[0],
                         frame->linesize[0], frame->data[1], frame->linesize[1],
                         frame->data[2], frame->linesize[2]);

    struct ExtDisplayWaitInfo* pwi = &pWin->mFrameWaitInfo;
    android::base::AutoLock lock(pwi->lock);
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);

    SDL_RenderClear(pWin->mRenderer);
    SDL_RenderCopy(pWin->mRenderer, pWin->mTexture, NULL, NULL);
    SDL_RenderPresent(pWin->mRenderer);
}

#ifdef _WIN32

// http://www.codeproject.com/Tips/76427/How-to-bring-window-to-top-with-SetForegroundWindo
static void SetForegroundWindowInternal(HWND hWnd) {
    if (!::IsWindow(hWnd))
        return;

    BYTE keyState[256] = {0};
    // to unlock SetForegroundWindow we need to imitate Alt pressing
    if (::GetKeyboardState((LPBYTE)&keyState)) {
        if (!(keyState[VK_MENU] & 0x80)) {
            ::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
        }
    }

    ::SetForegroundWindow(hWnd);

    if (::GetKeyboardState((LPBYTE)&keyState)) {
        if (!(keyState[VK_MENU] & 0x80)) {
            ::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                          0);
        }
    }
}

#endif  // _WIN32

static void close_all_windows() {
    // close all windows, send notifications to the android side too
    for (std::list<SDLDisplayWindow*>::const_iterator it = g_windows.begin();
         it != g_windows.end(); ++it) {
        SDLDisplayWindow* pWin = *it;
        if (pWin->mTexture != NULL) {
            SDL_DestroyTexture(pWin->mTexture);
            pWin->mTexture = NULL;
        }
        if (pWin->mRenderer != NULL) {
            SDL_DestroyRenderer(pWin->mRenderer);
            pWin->mRenderer = NULL;
        }
        if (pWin->mWindow != NULL) {
            SDL_DestroyWindow(pWin->mWindow);
            pWin->mWindow = NULL;
        }
        pWin->notifyGuestOnClosing();
    }

    g_windows.clear();
}

static void* event_loop(int w, int h) {
    SDL_Event e;
    bool quit = false;

    g_has_event_loop = true;

    while (!quit) {
        SDL_WaitEvent(&e);
        switch (e.type) {
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    VERBOSE_PRINT(extdisplay, "Window close event received.\n");
                    close_all_windows();
                    quit = true;
                }
                break;

            case SDL_QUIT:
                // close all windows, send notifications to the android side too
                close_all_windows();
                quit = true;
                break;

            case SDL_USEREVENT:
                switch (e.user.code) {
                    case USER_EVENT_DRAW_FRAME: {
                        SDLDisplayWindow* pWin = (SDLDisplayWindow*)e.user.data1;
                        AVFrame* frame = (AVFrame*)e.user.data2;
                        SDLRenderer::draw_frame_internal(pWin, frame);
                        break;
                    }

                    case USER_EVENT_CLOSE_WINDOW:
                        android::base::AutoLock lock(g_delete_waitInfo.lock);
                        SDLDisplayWindow* pWin = (SDLDisplayWindow*)e.user.data1;
                        if (pWin->mTexture != NULL) {
                            SDL_DestroyTexture(pWin->mTexture);
                            pWin->mTexture = NULL;
                        }
                        if (pWin->mRenderer != NULL) {
                            SDL_DestroyRenderer(pWin->mRenderer);
                            pWin->mRenderer = NULL;
                        }
                        if (pWin->mWindow != NULL) {
                            SDL_DestroyWindow(pWin->mWindow);
                            pWin->mWindow = NULL;
                        }
                        pWin->mClosed = true;
                        g_windows.remove(pWin);

                        if (g_windows.size() <= 0) {
                            // if there is only one window, quit the loop. We
                            // need to keep this loop if there is a window still
                            // exists
                            quit = true;
                        }

                        g_delete_waitInfo.done = true;
                        g_delete_waitInfo.cvDone.signalAndUnlock(
                                &g_delete_waitInfo.lock);
                        break;
                }
                break;
        }
    }

    VERBOSE_PRINT(extdisplay, "event_loop() exited.\n");
    g_has_event_loop = false;

    return NULL;
}

SDLDisplayWindow* SDLRenderer::recreateSDLWindow(int width, int height) {
    SDLDisplayWindow* pWin = new SDLDisplayWindow();
    if (pWin == NULL)
        return NULL;

    g_windows.push_back(pWin);

    pWin->mWidth = width;
    pWin->mHeight = height;
    pWin->mWindow = NULL;
    pWin->mRenderer = NULL;
    pWin->mTexture = NULL;

    if (pWin->mTexture != NULL) {
        SDL_DestroyTexture(pWin->mTexture);
        pWin->mTexture = NULL;
    }
    if (pWin->mRenderer != NULL) {
        SDL_DestroyRenderer(pWin->mRenderer);
        pWin->mRenderer = NULL;
    }
    if (pWin->mWindow != NULL) {
        SDL_DestroyWindow(pWin->mWindow);
        pWin->mWindow = NULL;
    }

    int flags = SDL_WINDOW_SHOWN;
    pWin->mWindow =
            SDL_CreateWindow("ExternalDisplay", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, width, height, flags);
    if (NULL == pWin->mWindow) {
        derror("cannot create window.\n");
        delete pWin;
        return NULL;
    }
    pWin->mRenderer = SDL_CreateRenderer(
            pWin->mWindow, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (NULL == pWin->mRenderer) {
        derror("cannot create renderer.\n");
        delete pWin;
        return NULL;
    }
    pWin->mTexture =
            SDL_CreateTexture(pWin->mRenderer, SDL_PIXELFORMAT_YV12,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
    if (NULL == pWin->mTexture) {
        derror("cannot create texture.\n");
        delete pWin;
        return NULL;
    }

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    SDL_RenderClear(pWin->mRenderer);
    SDL_SetRenderDrawColor(pWin->mRenderer, 50, 50, 50, 255);
    SDL_RenderFillRect(pWin->mRenderer, &rect);
    SDL_RenderPresent(pWin->mRenderer);

    return pWin;
}

#ifdef _WIN32
static void* get_window_id(SDL_Window* sdlWin) {
    SDL_SysWMinfo wminfo;
    void* winhandle;

    memset(&wminfo, 0, sizeof(wminfo));

    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(sdlWin, &wminfo);

#ifdef WIN32
    winhandle = (void*)wminfo.info.win.window;
#elif TARGET_OS_IOS
    winhandle = (void*)wminfo.info.uikit.window;
#elif defined(__APPLE__)
    winhandle = (void*)wminfo.info.cocoa.window;
#else
    winhandle = (void*)wminfo.info.x11.window;
#endif
    return (winhandle);
}
#endif

void sdl_thread_func(PrivateData* pData) {
    int width = pData->width;
    int height = pData->height;
    static bool sdl_inited = false;

    // do this in the same thread as create_window() in window.cpp
    if (!sdl_inited) {
        // init sdl2, same as in ffplay.c
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            derror("Error init SDL: %s\n", SDL_GetError());
            return;
        }
        sdl_inited = true;
    }

    SDLDisplayWindow* win = SDLRenderer::recreateSDLWindow(width, height);
    pData->window = win;

#ifdef _WIN32
    SDLDisplayWindow* win = pData->mWindowdow;
    if (win) {
        HWND hwnd = (HWND)get_window_id(win->mWindow);
        SetForegroundWindowInternal(hwnd);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
    }
#endif

    struct ExtDisplayWaitInfo* pwi = &pData->waitInfo;
    android::base::AutoLock lock(pwi->lock);
    pwi->done = true;
    pwi->cvDone.signalAndUnlock(&pwi->lock);
    if (!g_has_event_loop)
        event_loop(width, height);
}

void SDLRenderer::resize_sdl_window(SDLDisplayWindow* pWin,
                                    int width,
                                    int height) {
    if (pWin && pWin->mWindow) {
        SDL_SetWindowSize(pWin->mWindow, width, height);
        SDL_SetWindowPosition(pWin->mWindow, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);

        if (pWin->mTexture != NULL) {
            SDL_DestroyTexture(pWin->mTexture);
            pWin->mTexture = NULL;
        }
        if (pWin->mRenderer != NULL) {
            SDL_DestroyRenderer(pWin->mRenderer);
            pWin->mRenderer = NULL;
        }

        pWin->mRenderer = SDL_CreateRenderer(
                pWin->mWindow, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (NULL == pWin->mRenderer) {
            derror("cannot create renderer.\n");
        }
        pWin->mTexture =
                SDL_CreateTexture(pWin->mRenderer, SDL_PIXELFORMAT_YV12,
                                  SDL_TEXTUREACCESS_STREAMING, width, height);
        if (NULL == pWin->mTexture) {
            derror("cannot create texture.\n");
        }
    }
}

void SDLRenderer::delete_sdl_window(SDLDisplayWindow* pWin) {
    if (pWin->mClosed)
        return;
    android::base::AutoLock lock(g_delete_waitInfo.lock);
    SDL_Event me;
    me.type = SDL_USEREVENT;
    me.user.code = USER_EVENT_CLOSE_WINDOW;
    me.user.data1 = pWin;
    SDL_PushEvent(&me);
    while (!g_delete_waitInfo.done) {
        g_delete_waitInfo.cvDone.wait(&g_delete_waitInfo.lock);
    }
}

}  // namespace display
}  // namespace android
