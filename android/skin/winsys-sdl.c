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

#include <SDL.h>
#include <SDL_syswm.h>

void skin_winsys_show_cursor(bool enabled) {
    SDL_ShowCursor(enabled ? 1 : 0);
}

// Enable/Disable input grab.
void skin_winsys_grab_input(bool enabled) {
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