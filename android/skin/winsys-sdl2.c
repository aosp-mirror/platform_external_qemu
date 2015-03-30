/* Copyright (C) 2014 The Android Open Source Project
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

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

// Set to 1 to enable debugging.
#define DEBUG 0

#if DEBUG
#  define DBG(...)  fprintf(stderr, "%s: ", __FUNCTION__), fprintf(stderr, __VA_ARGS__)
#else
#  define DBG(...)  ((void)0)
#endif

typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} Borders;

static SDL_Window* s_window = NULL;
static SDL_Renderer* s_renderer = NULL;
static SDL_Surface* s_window_icon = NULL;
static Borders s_window_borders = { 0, 0, 0, 0 };

static void set_window_icon(void) {
    if (s_window && s_window_icon) {
        void* pixels = s_window_icon->pixels;
        SDL_SetWindowIcon(s_window, s_window_icon);
        SDL_FreeSurface(s_window_icon);
        free(pixels);
        s_window_icon = NULL;
    }
}

SDL_Window* skin_winsys_get_window(void) {
    return s_window;
}

void skin_winsys_set_window(SDL_Window* window) {
    s_window = window;
#ifdef _WIN32
    s_window_borders.left = GetSystemMetrics(SM_CXFRAME);
    s_window_borders.right = GetSystemMetrics(SM_CXFRAME);
    s_window_borders.top =
            GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
    s_window_borders.bottom = GetSystemMetrics(SM_CYFRAME);

    DBG("window borders left=%d right=%d top=%d bottom=%d\n",
        s_window_borders.left,
        s_window_borders.right,
        s_window_borders.top,
        s_window_borders.bottom);
#endif
    set_window_icon();
}

SDL_Renderer* skin_winsys_get_renderer(void) {
    return s_renderer;
}

void skin_winsys_set_renderer(SDL_Renderer* renderer) {
    if (s_renderer) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    s_renderer = renderer;
}

void skin_winsys_set_relative_mouse_mode(bool enabled) {
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
}

// Return window handle of main UI.
void* skin_winsys_get_window_handle(void) {
    if (!s_window) {
        return NULL;
    }

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(s_window, &info)) {
        return NULL;
    }

    switch (info.subsystem) {
#ifdef _WIN32
    case SDL_SYSWM_WINDOWS:
        return (void*)info.info.win.window;
#elif defined(__APPLE__)
    case SDL_SYSWM_COCOA:
        return (void*)info.info.cocoa.window;
#else
    case SDL_SYSWM_X11:
        return (void*)info.info.x11.window;
#endif
    default: return NULL;
    }
}

void skin_winsys_get_monitor_rect(SkinRect* rect) {
    // Return largest monitor rectangle.
    SDL_Rect monitor = { 0, 0, 640, 480 };
    int count = SDL_GetNumVideoDisplays();
    int nn;
    bool first = true;
    for (nn = 0; nn < count; ++nn) {
        SDL_Rect r;
        if (SDL_GetDisplayBounds(nn, &r) != 0) {
            continue;
        }
        if (first) {
            monitor = r;
            first = false;
        } else {
            int monitor_size = monitor.w * monitor.h;
            int r_size = r.w * r.h;
            if (r_size > monitor_size) {
                monitor = r;
            }
        }
    }
    Borders* borders = &s_window_borders;
    rect->pos.x = monitor.x + borders->left;
    rect->pos.y = monitor.y + borders->top;
    rect->size.w = monitor.w - borders->right;
    rect->size.h = monitor.h - borders->bottom;
}

int skin_winsys_get_monitor_dpi(int* x_dpi, int* y_dpi) {
    // SDL2 doesn't provide this information.
    return 96;
}

void skin_winsys_set_window_pos(int window_x, int window_y) {
    if (s_window) {
        SDL_SetWindowPosition(s_window, window_x, window_y);
    }
}

void skin_winsys_get_window_pos(int* window_x, int* window_y) {
    if (s_window) {
        SDL_GetWindowPosition(s_window, window_x, window_y);
    } else {
        *window_x = *window_y = 0;
    }
}

// Set window title.
void skin_winsys_set_window_title(const char* title) {
    if (s_window) {
        SDL_SetWindowTitle(s_window, title);
    }
}

bool skin_winsys_is_window_fully_visible(void) {
    if (!s_window) {
        DBG("no window\n");
        return false;
    }
    SDL_Rect w;
    SDL_GetWindowPosition(s_window, &w.x, &w.y);
    SDL_GetWindowSize(s_window, &w.w, &w.h);

    Borders* borders = &s_window_borders;
    w.x -= borders->left;
    w.w += borders->left + borders->right;
    w.y -= borders->top;
    w.h += borders->top + borders->bottom;

    DBG("Window pos=(%d,%d) dim=(%d,%d)\n", w.x, w.y, w.w, w.h);
    int count = SDL_GetNumVideoDisplays();
    int nn;
    DBG("There are %d screens\n", count);
    for (nn = 0; nn < count; ++nn) {
        SDL_Rect r;
        if (SDL_GetDisplayBounds(nn, &r) != 0) {
            DBG("Can't get bounds of screen #%d\n", nn + 1);
            continue;
        }
        DBG("Screen #%d (%d,%d) (%d,%d)\n", nn + 1, r.x, r.y, r.w, r.h);
        if (w.x >= r.x &&
            w.y >= r.y &&
            w.x + w.w <= r.x + r.w &&
            w.y + w.h <= r.y + r.h) {
            // The window is contained in this display.
            DBG("Window fully inside screen %d\n", nn + 1);
            return true;
        }
    }
    DBG("Window is not fully visible\n");
    return false;
}

void *readpng(const unsigned char*  base, size_t  size, unsigned *_width, unsigned *_height);

void skin_winsys_set_window_icon(const unsigned char* icon_data,
                                 size_t icon_size) {
    static int window_icon_set;

    if (!window_icon_set) {
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
                0x00ff0000,
                0x0000ff00,
                0x000000ff,
                0xff000000
                );
        if (icon != NULL) {
            s_window_icon = icon;
            set_window_icon();
        }
    }
}

void skin_winsys_start(bool no_window, bool raw_keys) {
    /* we're not a game, so allow the screensaver to run */
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");

    /* disallow texture-based window creation since it currently makes it
     * impossible to render both the GL subwindow and the rest of the skin */
    //SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    int flags = 0;
    if (!no_window) {
        flags |= SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    }

    (void)raw_keys;

    if (SDL_Init(flags)) {
        fprintf(stderr, "SDL init failure, reason is: %s\n", SDL_GetError() );
        exit(1);
    }
}

void skin_winsys_quit(void) {
    SDL_Quit();
}
