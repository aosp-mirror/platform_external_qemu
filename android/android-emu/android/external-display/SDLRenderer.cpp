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
#include "android/external-display/DisplayWindow.h"

#include "android/utils/debug.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include <pthread.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#if defined(_WIN32) || defined(__APPLE__)
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#define USER_EVENT_CREATE_WINDOW 1
#define USER_EVENT_CLOSE_WINDOW 2
#define USER_EVENT_DRAW_FRAME 3

namespace android {
namespace display {

// for sdl 2.0
static pthread_mutex_t recreate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t recreate_cond = PTHREAD_COND_INITIALIZER;
int lastwidth = 0;
int lastheight = 0;

static int g_num_windows = 0;
static bool g_has_event_loop = false;

void SDLRenderer::draw_frame(DisplayWindow* pWin, AVFrame* frame) {
    SDL_Event me;
    me.type = SDL_USEREVENT;
    me.user.code = USER_EVENT_DRAW_FRAME;
    me.user.data1 = pWin;
    me.user.data2 = frame;
    SDL_PushEvent(&me);

    pthread_mutex_lock(&pWin->frame_mutex);
    pthread_cond_wait(&pWin->frame_cond, &pWin->frame_mutex);
    pthread_mutex_unlock(&pWin->frame_mutex);
}

void SDLRenderer::draw_frame_internal(DisplayWindow* pWin, AVFrame* frame) {
    SDL_Rect video_rect;
    video_rect.x = 0;
    video_rect.y = 0;
    video_rect.w = frame->width;   // lastwidth; // frame size might change,
                                   // e.g., chrome tab mirroring
    video_rect.h = frame->height;  // lastheight;
    SDL_UpdateYUVTexture(pWin->tex, &video_rect, frame->data[0],
                         frame->linesize[0], frame->data[1], frame->linesize[1],
                         frame->data[2], frame->linesize[2]);

    pthread_mutex_lock(&pWin->frame_mutex);
    pthread_cond_signal(&pWin->frame_cond);
    pthread_mutex_unlock(&pWin->frame_mutex);

    SDL_RenderClear(pWin->ren);
    SDL_RenderCopy(pWin->ren, pWin->tex, NULL, NULL);
    SDL_RenderPresent(pWin->ren);
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

//#define DUMP
pthread_mutex_t dump_lock = PTHREAD_MUTEX_INITIALIZER;

static void* event_loop(int w, int h) {
    SDL_Event e;
    bool quit = false;

    g_has_event_loop = true;

    while (!quit) {
        SDL_WaitEvent(&e);
        switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_USEREVENT:
                switch (e.user.code) {
                    case USER_EVENT_DRAW_FRAME: {
                        DisplayWindow* pWin = (DisplayWindow*)e.user.data1;
                        AVFrame* frame = (AVFrame*)e.user.data2;
                        SDLRenderer::draw_frame_internal(pWin, frame);
                        break;
                    }

                    case USER_EVENT_CLOSE_WINDOW:
                        pthread_mutex_lock(&recreate_lock);
                        DisplayWindow* pWin = (DisplayWindow*)e.user.data1;
                        if (pWin->tex != NULL) {
                            SDL_DestroyTexture(pWin->tex);
                            pWin->tex = NULL;
                        }
                        if (pWin->ren != NULL) {
                            SDL_DestroyRenderer(pWin->ren);
                            pWin->ren = NULL;
                        }
                        if (pWin->win != NULL) {
                            SDL_DestroyWindow(pWin->win);
                            pWin->win = NULL;
                        }
                        delete pWin;

                        if (g_num_windows <= 0) {
                            // if there is only one window, quit the loop. We
                            // need to keep this loop if there is a window still
                            // exists
                            quit = true;
                        }

                        pthread_cond_signal(&recreate_cond);
                        pthread_mutex_unlock(&recreate_lock);
                        break;
                }
                break;
        }
    }

    VERBOSE_PRINT(extdisplay, "event_loop() exited.\n");
    g_has_event_loop = false;

    return NULL;
}

DisplayWindow* SDLRenderer::recreateSDLWindow(int width, int height) {
    DisplayWindow* pWin = new DisplayWindow();
    if (pWin == NULL)
        return NULL;

    pWin->width = width;
    pWin->height = height;
    pWin->win = NULL;
    pWin->ren = NULL;
    pWin->tex = NULL;

    pthread_mutex_init(&pWin->frame_mutex, NULL);
    pthread_cond_init(&pWin->frame_cond, NULL);

    if (pWin->tex != NULL) {
        SDL_DestroyTexture(pWin->tex);
        pWin->tex = NULL;
    }
    if (pWin->ren != NULL) {
        SDL_DestroyRenderer(pWin->ren);
        pWin->ren = NULL;
    }
    if (pWin->win != NULL) {
        SDL_DestroyWindow(pWin->win);
        pWin->win = NULL;
    }

    int flags =
            SDL_WINDOW_SHOWN;  // SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;
    pWin->win = SDL_CreateWindow("ExternalDisplay", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, width, height, flags);
    if (NULL == pWin->win) {
        derror("cannot create window.\n");
        delete pWin;
        return NULL;
    }
    pWin->ren = SDL_CreateRenderer(
            pWin->win, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (NULL == pWin->ren) {
        derror("cannot create renderer.\n");
        delete pWin;
        return NULL;
    }
    pWin->tex = SDL_CreateTexture(pWin->ren, SDL_PIXELFORMAT_YV12,
                                  SDL_TEXTUREACCESS_STREAMING, width, height);
    if (NULL == pWin->tex) {
        derror("cannot create texture.\n");
        delete pWin;
        return NULL;
    }

    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    SDL_RenderClear(pWin->ren);
    SDL_SetRenderDrawColor(pWin->ren, 50, 50, 50, 255);
    SDL_RenderFillRect(pWin->ren, &rect);
    SDL_RenderPresent(pWin->ren);

    lastwidth = width;
    lastheight = height;

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

static bool sdl_inited = false;

void* sdl_thread_func(void* arg) {
    PRIVATE_DATA* pData = (PRIVATE_DATA*)arg;
    int width = pData->width;
    int height = pData->height;

    // do this in the same thread as create_window() in window.cpp
    if (!sdl_inited) {
        // init sdl2, same as in ffplay.c
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            derror("Error init SDL: %s\n", SDL_GetError());
            return NULL;
        }
        sdl_inited = true;
    }

    DisplayWindow* win = SDLRenderer::recreateSDLWindow(width, height);
    pData->wfi = win;
    //

    g_num_windows++;

#ifdef _WIN32
    DisplayWindow* win = pData->wfi;
    if (win) {
        HWND hwnd = (HWND)get_window_id(win->win);
        SetForegroundWindowInternal(hwnd);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
    }
#endif

    pthread_mutex_lock(&pData->mutex);
    pthread_cond_signal(&pData->cond);
    pthread_mutex_unlock(&pData->mutex);

    if (!g_has_event_loop)
        event_loop(width, height);

    return 0;
}

void SDLRenderer::resize_sdl_window(DisplayWindow* pWin,
                                    int width,
                                    int height) {
    if (pWin && pWin->win) {
        // scale_to_fit_screen(width, height, &width, &height);

        SDL_SetWindowSize(pWin->win, width, height);
        SDL_SetWindowPosition(pWin->win, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);

        if (pWin->tex != NULL) {
            SDL_DestroyTexture(pWin->tex);
            pWin->tex = NULL;
        }
        if (pWin->ren != NULL) {
            SDL_DestroyRenderer(pWin->ren);
            pWin->ren = NULL;
        }

        pWin->ren = SDL_CreateRenderer(
                pWin->win, -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (NULL == pWin->ren) {
            derror("cannot create renderer.\n");
        }
        pWin->tex =
                SDL_CreateTexture(pWin->ren, SDL_PIXELFORMAT_YV12,
                                  SDL_TEXTUREACCESS_STREAMING, width, height);
        if (NULL == pWin->tex) {
            derror("cannot create texture.\n");
        }

        lastwidth = width;
        lastheight = height;
    }
}

void SDLRenderer::delete_sdl_window(DisplayWindow* pWin) {
    if (g_num_windows > 0)
        g_num_windows--;

    pthread_mutex_lock(&recreate_lock);
    SDL_Event me;
    me.type = SDL_USEREVENT;
    me.user.code = USER_EVENT_CLOSE_WINDOW;
    me.user.data1 = pWin;
    SDL_PushEvent(&me);
    pthread_cond_wait(&recreate_cond, &recreate_lock);
    pthread_mutex_unlock(&recreate_lock);
}

}  // namespace display
}  // namespace android
