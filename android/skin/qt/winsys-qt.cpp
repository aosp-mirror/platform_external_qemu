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
#include <stdio.h>

#include "android/skin/rect.h"
#include "android/skin/resource.h"
#include "android/skin/winsys.h"
#include "android/utils/setenv.h"
#include "qt-context.h"

#include <QtCore>
#include <QApplication>
#include <QDesktopWidget>
#include <QRect>
#include <QWidget>
#include <QThread>
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#include "../winsys.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

void skin_winsys_set_relative_mouse_mode(bool) {
    D("skin_winsys_set_relative_mouse_mode");
}

void* skin_winsys_get_window_handle(void) {
    D("skin_winsys_get_window_handle");
    WId handle;
    QemuContext::instance->getWindowId(&handle);
    return (void*)handle;
}

//static QWidget* get_window_handle(void) {
//    return get_context()->emulator_window;
//}

void skin_winsys_get_monitor_rect(SkinRect* rect) {
//    D("skin_winsys_get_monitor_rect");
    QRect qrect;
    QemuContext::instance->getScreenDimensions(&qrect);
    rect->pos.x = qrect.left();
    rect->pos.y = qrect.top();
    rect->size.w = qrect.width();
    rect->size.h = qrect.height();
}

int skin_winsys_get_monitor_dpi(int*, int*) {
    D("skin_winsys_get_monitor_dpi");
    return 96;
}

void skin_winsys_set_window_pos(int window_x, int window_y) {
    D("skin_winsys_set_window_pos %d, %d", window_x, window_y);
    QemuContext::instance->setWindowPos(window_x, window_y);
}

void skin_winsys_get_window_pos(int* window_x, int* window_y) {
    D("skin_winsys_get_window_pos");
    QemuContext::instance->getWindowPos(window_x, window_y);
}

void skin_winsys_set_window_title(const char* title) {
//    D("skin_winsys_set_window_title %s", title);
    QemuContext::instance->setWindowTitle(QString(title));
}

bool skin_winsys_is_window_fully_visible(void) {
    D("skin_winsys_is_window_fully_visible");
    return true;
}

void skin_winsys_start(bool, bool) {
//    D("skin_winsys_start");
    init_qt();
}

void skin_enter_main_loop() {
//    D("skin_enter_main_loop");
    qt_main_loop();
}

void skin_winsys_set_window_icon(const unsigned char*,
                                 size_t) {
    D("skin_winsys_set_window_icon");
}

StartFunction static_f;
void* doSomeThing(void *) {
    static_f();
    return NULL;
}

void skin_spawn_thread(StartFunction f) {
    D("skin_spawn_thread");
    QemuContext::instance->startThread(f);

//    pthread_t blah;
//    static_f = f;
//    int err = pthread_create(&blah, NULL, doSomeThing, NULL);
//    D("Spawned thread with err %d", err);

//    f();
}

void skin_winsys_quit(void) {
    D("skin_winsys_quit");
}
