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
#ifdef CONFIG_POSIX
#include <pthread.h>
#endif

#include "android/skin/rect.h"
#include "android/skin/resource.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-window.h"
#include "android/utils/setenv.h"

#include <QtCore>
#include <QApplication>
#include <QDesktopWidget>
#include <QRect>
#include <QSemaphore>
#include <QThread>
#include <QWidget>

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

static char **static_argv;
static int static_argc;
static QApplication *app = NULL;

extern void skin_winsys_enter_main_loop(int argc, char **argv)
{
    D("Starting QT main loop\n");
    static_argc = argc;
    static_argv = argv;
    app->exec();
    D("Finished QT main loop\n");
    exit(0);
}

extern void skin_winsys_get_monitor_rect(SkinRect *rect)
{
    QRect qrect;
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->getScreenDimensions(&qrect, &semaphore);
    semaphore.acquire();
    rect->pos.x = qrect.left();
    rect->pos.y = qrect.top();
    rect->size.w = qrect.width();
    rect->size.h = qrect.height();
}

extern int skin_winsys_get_monitor_dpi(int *x, int *y)
{
    D("skin_winsys_get_monitor_dpi");
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return -1;
    int value;
    window->getMonitorDpi(&value, &semaphore);
    semaphore.acquire();
    *x = *y = value;
    return 0;
}

extern void *skin_winsys_get_window_handle(void)
{
    D("skin_winsys_get_window_handle");
    WId handle;
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return NULL;
    window->getWindowId(&handle, &semaphore);
    semaphore.acquire();
    return (void*)handle;
}

extern void skin_winsys_get_window_pos(int *x, int *y)
{
    D("skin_winsys_get_window_pos");
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->getWindowPos(x, y, &semaphore);
    semaphore.acquire();
}

extern bool skin_winsys_is_window_fully_visible()
{
    D("skin_winsys_is_window_fully_visible");
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return true;
    bool value;
    window->isWindowFullyVisible(&value, &semaphore);
    semaphore.acquire();
    return value;
}

extern void skin_winsys_quit()
{
    D("skin_winsys_quit");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->requestClose();
}

extern void skin_winsys_set_relative_mouse_mode(bool)
{
    D("skin_winsys_set_relative_mouse_mode");
}

extern void skin_winsys_set_window_icon(const unsigned char *data, size_t size)
{
    D("skin_winsys_set_window_icon");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->setWindowIcon(data, size);
}

extern void skin_winsys_set_window_pos(int x, int y)
{
    D("skin_winsys_set_window_pos %d, %d", x, y);
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->setWindowPos(x, y);
}

extern void skin_winsys_set_window_title(const char *title)
{
    D("skin_winsys_set_window_title %s", title);
    QSemaphore semaphore;
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    QString qtitle(title);
    window->setTitle(&qtitle, &semaphore);
    semaphore.acquire();
}

extern void skin_winsys_spawn_thread(StartFunction f, int argc, char **argv)
{
    D("skin_spawn_thread");
    EmulatorWindow *window = EmulatorWindow::getInstance();
    if (window == NULL) return;
    window->startThread(f, argc, argv);
}

extern void skin_winsys_start(bool, bool)
{
    app = new QApplication(static_argc, static_argv);
    new EmulatorWindow(NULL);
}

#ifdef _WIN32
extern "C" int qt_main(int, char**);

int qMain(int argc, char** argv) {
    return qt_main(argc, argv);
}
#endif  // _WIN32

