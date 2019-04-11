/* Copyright (C) 2014-2015 The Android Open Source Project
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
#include <stdio.h>
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif

#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/globals.h"
#include "android/skin/rect.h"
#include "android/skin/resource.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/utils/setenv.h"
#include "android/main-common-ui.h"

#include <string>

#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <signal.h>
#endif

using android::base::System;
#ifdef _WIN32
using android::base::Win32UnicodeString;
#endif

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

struct QCoreApplication;

struct GlobalState {
    int argc;
    char** argv;
    QCoreApplication* app;
    bool geo_saved;
    int window_geo_x;
    int window_geo_y;
    int window_geo_w;
    int window_geo_h;
    int frame_geo_x;
    int frame_geo_y;
    int frame_geo_w;
    int frame_geo_h;
};

static GlobalState* globalState() {
    static GlobalState sGlobalState = {
        .argc = 0,
        .argv = NULL,
        .app = NULL,
        .geo_saved = false,
        .window_geo_x = 0,
        .window_geo_y = 0,
        .window_geo_w = 0,
        .window_geo_h = 0,
        .frame_geo_x = 0,
        .frame_geo_y = 0,
        .frame_geo_w = 0,
        .frame_geo_h = 0,
    };
    return &sGlobalState;
}

static bool sMainLoopShouldExit = false;

#ifdef _WIN32
static HANDLE sWakeEvent;
#endif

static void enableSigChild() {
    // The issue only occurs on Darwin so to be safe just do this on Darwin
    // to prevent potential issues. The function exists on all platforms to
    // make the calling code look cleaner. In addition the issue only occurs
    // when the extended window has been created. We do not currently know
    // why this only happens on Darwin and why it only happens once the
    // extended window is created. The sigmask is not changed after the
    // extended window has been created.
#ifdef __APPLE__
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    // We only need to enable SIGCHLD for the Qt main thread since that's where
    // all the Qt stuff runs. The main loop should eventually make syscalls that
    // trigger signals.
    int result = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    if (result != 0) {
        D("Could not set thread sigmask: %d", result);
    }
#endif
}

static bool onMainQtThread() {
    return true;
}

std::shared_ptr<void> skin_winsys_get_shared_ptr() {
    return std::static_pointer_cast<void>(EmulatorNoQtNoWindow::getInstancePtr());
}

extern void skin_winsys_enter_main_loop(bool no_window) {
    (void)no_window;

#ifdef _WIN32
    sWakeEvent = CreateEvent(NULL,                             // Default security attributes
                             TRUE,                             // Manual reset
                             FALSE,                            // Initially nonsignaled
                             TEXT("winsys-qt::sWakeEvent"));   // Object name

    while (1) {
        WaitForSingleObject(sWakeEvent, INFINITE);
        if (sMainLoopShouldExit) break;
        // Loop and wait again
    }
#else
    while (1) {
        sigset_t mask;
        sigset_t origMask;

        sigemptyset (&mask);
        if (sigprocmask(SIG_BLOCK, &mask, &origMask) < 0) {
            fprintf(stderr, "%s %s: sigprocmask() failed!\n", __FILE__, __FUNCTION__);
            break;
        }
        sigsuspend(&mask);
        if (sMainLoopShouldExit) break;
        // Loop and wait again
    }
#endif
    // We're in windowless mode and ready to exit
    auto noQtNoWindow = EmulatorNoQtNoWindow::getInstance();
    if (noQtNoWindow) {
        noQtNoWindow->requestClose();
        noQtNoWindow->waitThread();
    }
    D("Finished QEMU main loop\n");
}

extern void skin_winsys_get_monitor_rect(SkinRect *rect)
{
    D("skin_winsys_get_monitor_rect: begin\n");
    bool haveScreenRect = false;

    rect->pos.x = 0;
    rect->pos.y = 0;
    rect->size.w = android_hw->hw_lcd_width;
    rect->size.h = android_hw->hw_lcd_height;
    D("%s: (%d,%d) %dx%d", __FUNCTION__, rect->pos.x, rect->pos.y,
      rect->size.w, rect->size.h);
}

extern int skin_winsys_get_device_pixel_ratio(double *dpr)
{
    D("skin_winsys_get_device_pixel_ratio");
    *dpr = 1.0;
    return 0;
}

extern void *skin_winsys_get_window_handle()
{
    return nullptr;
}

extern void skin_winsys_get_window_pos(int *x, int *y)
{
    D("skin_winsys_get_window_pos (noqt)");
    *x = 0;
    *y = 0;
}

extern void skin_winsys_get_window_size(int *w, int *h)
{
    D("skin_winsys_get_window_size (noqt)");
    *w = android_hw->hw_lcd_width;
    *h = android_hw->hw_lcd_height;
    D("%s: size: %d x %d", __FUNCTION__, *w, *h);
}

extern void skin_winsys_get_frame_pos(int *x, int *y)
{
    D("skin_winsys_get_frame_pos (noqt)");
    *x = 0;
    *y = 0;
    D("%s: x=%d y=%d", __FUNCTION__, *x, *y);
}

extern void skin_winsys_get_frame_size(int *w, int *h)
{
    D("skin_winsys_get_frame_size (noqt)");
    *w = android_hw->hw_lcd_width;
    *h = android_hw->hw_lcd_height;
    D("%s: size: %d x %d", __FUNCTION__, *w, *h);
}

extern bool skin_winsys_window_has_frame()
{
    return false;
}

extern void skin_winsys_set_device_geometry(const SkinRect* rect) {
    (void)rect;
}

extern void skin_winsys_save_window_geo() {
    int x = 0, y = 0, w = 0, h = 0;
    int frameX = 0, frameY = 0, frameW = 0, frameH = 0;
    skin_winsys_get_window_pos(&x, &y);
    skin_winsys_get_window_size(&w, &h);
    skin_winsys_get_frame_pos(&frameX, &frameY);
    skin_winsys_get_frame_size(&frameW, &frameH);
    GlobalState* g = globalState();
    g->geo_saved = true;
    g->window_geo_x = x;
    g->window_geo_y = y;
    g->window_geo_w = w;
    g->window_geo_h = h;
    g->frame_geo_x = frameX;
    g->frame_geo_y = frameY;
    g->frame_geo_w = frameW;
    g->frame_geo_h = frameH;
}

extern bool skin_winsys_is_window_fully_visible() {
    return true;
}

extern bool skin_winsys_is_window_off_screen() {
    return false;
}

WinsysPreferredGlesBackend skin_winsys_override_glesbackend_if_auto(WinsysPreferredGlesBackend backend) {
    WinsysPreferredGlesBackend currentPreferred = skin_winsys_get_preferred_gles_backend();
    if (currentPreferred == WINSYS_GLESBACKEND_PREFERENCE_AUTO) {
        return backend;
    }
    return currentPreferred;
}

extern WinsysPreferredGlesBackend skin_winsys_get_preferred_gles_backend() {
    D("skin_winsys_get_preferred_gles_backend");
    return WINSYS_GLESBACKEND_PREFERENCE_AUTO;
}

void skin_winsys_set_preferred_gles_backend(WinsysPreferredGlesBackend backend) {
    D("skin_winsys_set_preferred_gles_backend");
    (void)backend;
}

extern WinsysPreferredGlesApiLevel skin_winsys_get_preferred_gles_apilevel() {
    D("skin_winsys_get_preferred_gles_apilevel");
    return WINSYS_GLESAPILEVEL_PREFERENCE_AUTO;
}

extern void skin_winsys_quit_request() {
    D(__FUNCTION__);
    sMainLoopShouldExit = true;
#ifdef _WIN32
    if ( !SetEvent(sWakeEvent) ) {
        fprintf(stderr, "%s %s: SetEvent() failed!\n", __FILE__, __FUNCTION__);
    }
#else
    if ( kill(getpid(), SIGUSR1) ) {
       fprintf(stderr, "%s %s: kill() failed!\n", __FILE__, __FUNCTION__);
    }
#endif
}

void skin_winsys_destroy() {
    D(__FUNCTION__);
}

extern void skin_winsys_set_window_icon(const unsigned char *data, size_t size) {
    D("skin_winsys_set_window_icon");
    (void)data;
    (void)size;
}

extern void skin_winsys_set_window_pos(int x, int y) {
    D("skin_winsys_set_window_pos %d, %d", x, y);
    (void)x;
    (void)y;
}

extern void skin_winsys_set_window_size(int w, int h) {
    (void)w;
    (void)h;
}

extern void skin_winsys_set_window_cursor_resize(int which_corner) {
    (void)which_corner;
}

extern void skin_winsys_paint_overlay_for_resize(int mouse_x, int mouse_y) {
    (void)mouse_x;
    (void)mouse_y;
}

extern void skin_winsys_set_window_overlay_for_resize(int which_corner) {
    (void)which_corner;
}

extern void skin_winsys_clear_window_overlay() { }

extern void skin_winsys_set_window_cursor_normal() { }

extern void skin_winsys_set_window_title(const char *title) { }

extern void skin_winsys_update_rotation(SkinRotation rotation) {
    (void)rotation;
}

extern void skin_winsys_show_virtual_scene_controls(bool show) {
    (void)show;
}

extern void skin_winsys_spawn_thread(bool no_window,
                                     StartFunction f,
                                     int argc,
                                     char** argv) {
    D("skin_spawn_thread");
    (void)no_window;
    EmulatorNoQtNoWindow* guiless_window = EmulatorNoQtNoWindow::getInstance();
    if (guiless_window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    guiless_window->startThread([f, argc, argv] { f(argc, argv); });
}

void skin_winsys_setup_library_paths() { }

extern void skin_winsys_init_args(int argc, char** argv) {
    GlobalState* g = globalState();
    g->argc = argc;
    g->argv = argv;
}

extern int skin_winsys_snapshot_control_start() {
    fprintf(stderr, "%s: Snapshot UI not available yet in headless mode\n", __func__);
    return 0;
}

extern void skin_winsys_start(bool no_window) {
    (void)no_window;
    GlobalState* g = globalState();
    skin_winsys_setup_library_paths();

    g->app = nullptr;
    EmulatorNoQtNoWindow::create();
}

void skin_winsys_run_ui_update(SkinGenericFunction f, void* data,
                               bool wait) {
    D(__FUNCTION__);
    (void)f;
    (void)data;
    (void)wait;
}

extern void skin_winsys_error_dialog(const char* message, const char* title) {
    fprintf(stderr, "%s: error dialog: title: [%s] msg: [%s]\n", __func__, title, message);
}

void skin_winsys_set_ui_agent(const UiEmuAgent* agent) {
    EmulatorNoQtNoWindow::earlyInitialization(agent);
}

void skin_winsys_report_entering_main_loop(void) { }

extern bool skin_winsys_is_folded() {
    return false;
}

extern void skin_winsys_touch_qt_extended_virtual_sensors(void) { }

// Other skin functions (Just fix link errors for now)

extern "C" int sim_is_present() {
    return 1;
}
