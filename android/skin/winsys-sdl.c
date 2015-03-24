/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "android/skin/winsys.h"

#include "android/skin/resource.h"
#include "android/utils/setenv.h"

#include <SDL.h>
#include <SDL_syswm.h>

void skin_winsys_set_relative_mouse_mode(bool enabled) {
    SDL_ShowCursor(enabled ? 1 : 0);
    SDL_WM_GrabInput(enabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// Return window handle of main UI.
void* skin_winsys_get_window_handle(void) {
    SDL_SysWMinfo  wminfo;
    void*          winhandle;

    memset(&wminfo, 0, sizeof(wminfo));
    SDL_GetWMInfo(&wminfo);
#ifdef _WIN32
    winhandle = (void*)wminfo.window;
#elif defined(__APPLE__)
    winhandle = (void*)wminfo.nsWindowPtr;
#else
    winhandle = (void*)wminfo.info.x11.window;
#endif
    return winhandle;
}

void skin_winsys_get_monitor_rect(SkinRect* rect) {
    SDL_Rect monitor;
    SDL_WM_GetMonitorRect(&monitor);
    rect->pos.x = monitor.x;
    rect->pos.y = monitor.y;
    rect->size.w = monitor.w;
    rect->size.h = monitor.h;
}

int skin_winsys_get_monitor_dpi(int* x_dpi, int* y_dpi) {
    return SDL_WM_GetMonitorDPI(x_dpi, y_dpi);
}

void skin_winsys_set_window_pos(int window_x, int window_y) {
    SDL_WM_SetPos(window_x, window_y);
}

void skin_winsys_get_window_pos(int* window_x, int* window_y) {
    SDL_WM_GetPos(window_x, window_y);
}

// Set window title.
void skin_winsys_set_window_title(const char* title) {
    SDL_WM_SetCaption( title, title );
}

bool skin_winsys_is_window_fully_visible(void) {
    return !!SDL_WM_IsFullyVisible(0);
}

void *readpng(const unsigned char*  base, size_t  size, unsigned *_width, unsigned *_height);

void skin_winsys_set_window_icon(const unsigned char* icon_data,
                                 size_t icon_size) {
    static int window_icon_set;

    if (!window_icon_set) {
#ifdef _WIN32
        HANDLE handle = GetModuleHandle(NULL);
        HICON icon   = LoadIcon(handle, MAKEINTRESOURCE(1));
        SDL_SysWMinfo  wminfo;

        SDL_GetWMInfo(&wminfo);
        SetClassLongPtr(wminfo.window, GCLP_HICON, (LONG)icon);
#else  /* !_WIN32 */
        unsigned icon_w, icon_h;
        void* icon_pixels;

        window_icon_set = 1;

        icon_pixels = readpng(icon_data, icon_size, &icon_w, &icon_h);
        if (!icon_pixels) {
            return;
        }
       /* the data is loaded into memory as RGBA bytes by libpng. we want to manage
        * the values as 32-bit ARGB pixels, so swap the bytes accordingly depending
        * on our CPU endianess
        */
        {
            unsigned*  d     = icon_pixels;
            unsigned*  d_end = d + icon_w * icon_h;

            for ( ; d < d_end; d++ ) {
                unsigned  pix = d[0];
#if HOST_WORDS_BIGENDIAN
                /* R,G,B,A read as RGBA => ARGB */
                pix = ((pix >> 8) & 0xffffff) | (pix << 24);
#else
                /* R,G,B,A read as ABGR => ARGB */
                pix = (pix & 0xff00ff00) | ((pix >> 16) & 0xff) | ((pix & 0xff) << 16);
#endif
                d[0] = pix;
            }
        }

        SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
                icon_pixels,
                icon_w,
                icon_h,
                32,
                icon_w * 4,
#if HOST_WORDS_BIGENDIAN
                0x00ff0000,
                0x0000ff00,
                0x000000ff,
                0xff000000
#else
                0x00ff0000,
                0x0000ff00,
                0x000000ff,
                0xff000000
#endif
                );
        if (icon != NULL) {
            SDL_WM_SetIcon(icon, NULL);
            SDL_FreeSurface(icon);
            free(icon_pixels);
        }
#endif  /* !_WIN32 */
    }
}

void skin_winsys_start(bool no_window, bool raw_keys) {
    /* we're not a game, so allow the screensaver to run */
    setenv("SDL_VIDEO_ALLOW_SCREENSAVER","1",1);

    int flags = SDL_INIT_NOPARACHUTE;
    if (!no_window) {
        flags |= SDL_INIT_VIDEO;
    }

    if(SDL_Init(flags)) {
        fprintf(stderr, "SDL init failure, reason is: %s\n", SDL_GetError() );
        exit(1);
    }

    if (!no_window) {
        SDL_EnableUNICODE(!raw_keys);
        SDL_EnableKeyRepeat(0,0);
    }
}

void skin_winsys_quit(void) {
    SDL_Quit();
}
