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
#include "android/qt/qt_path.h"
#include "android/skin/rect.h"
#include "android/skin/resource.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/snapshot-page.h"
#include "android/skin/qt/init-qt.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/QtLogger.h"
#include "android/utils/setenv.h"
#include "android/main-common-ui.h"

#include <QtCore>
#include <QApplication>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QRect>
#include <QSemaphore>
#include <QThread>
#include <QWidget>

#include <string>

#ifdef Q_OS_LINUX
// This include needs to be after all the Qt includes
// because it defines macros/types that conflict with
// qt's own macros/types.
#include <X11/Xlib.h>
#endif

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

struct GlobalState {
    int argc;
    char** argv;
    QCoreApplication* app;
    bool window_pos_saved;
    int window_pos_x;
    int window_pos_y;
};

static GlobalState* globalState() {
    static GlobalState sGlobalState = {
        .argc = 0,
        .argv = NULL,
        .app = NULL,
        .window_pos_saved = false,
        .window_pos_x = 0,
        .window_pos_y = 0,
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
    return QThread::currentThread() == qApp->thread();
}

std::shared_ptr<void> skin_winsys_get_shared_ptr() {
    return std::static_pointer_cast<void>(EmulatorQtWindow::getInstancePtr());
}

extern void skin_winsys_enter_main_loop(bool no_window) {

    if (no_window) {
        D("Starting QEMU main loop\n");
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
    } else {
        // We're using Qt
        D("Starting QT main loop\n");
        // In order for QProcess to correctly handle processes that exit we need
        // to enable SIGCHLD. That's how Qt knows to wait for the child process. If
        // it doesn't wait the process will be left as a zombie and the finished
        // signal will not be emitted from QProcess.
        enableSigChild();
        GlobalState* g = globalState();
        g->app->exec();
        D("Finished QT main loop\n");
    }
}

extern void skin_winsys_get_monitor_rect(SkinRect *rect)
{
    D("skin_winsys_get_monitor_rect: begin\n");
    bool haveScreenRect = false;
    D("skin_winsys_get_monitor_rect: get Qt window\n");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    D("skin_winsys_get_monitor_rect: get Qt window (done)\n");
    // Qt method of getting screen dimensions is unreliable.
    // Use platform-specific queries.
    if (!rect) return;
    rect->pos.x = 0;
    rect->pos.y = 0;
    D("skin_winsys_get_monitor_rect: Begin calling platform specific display queries.\n");
#if defined(_WIN32)
    D("skin_winsys_get_monitor_rect: Windows: GetSystemMetrics(SM_CXSCREEN)\n");
    rect->size.w = (int)GetSystemMetrics(SM_CXSCREEN);
    D("skin_winsys_get_monitor_rect: Windows: GetSystemMetrics(SM_CYSCREEN)\n");
    rect->size.h = (int)GetSystemMetrics(SM_CYSCREEN);
#elif defined(__APPLE__)
    D("skin_winsys_get_monitor_rect: macOS: CGMainDisplayID()\n");
    int displayId = CGMainDisplayID();
    D("skin_winsys_get_monitor_rect: macOS: CGDisplayPixelsWide()\n");
    rect->size.w = CGDisplayPixelsWide(displayId);
    D("skin_winsys_get_monitor_rect: macOS: CGDisplayPixelsHigh()\n");
    rect->size.h = CGDisplayPixelsHigh(displayId);
#else // Linux
    D("skin_winsys_get_monitor_rect: Linux: XOpenDisplay(NULL)\n");
    Display* defaultDisplay = XOpenDisplay(NULL);
    D("skin_winsys_get_monitor_rect: Linux: XOpenDisplay(NULL) (done)\n");
    if (defaultDisplay) {
        D("skin_winsys_get_monitor_rect: Linux: DefaultScreenOfDisplay(defaultDisplay)\n");
        Screen* defaultScreen = DefaultScreenOfDisplay(defaultDisplay);
        D("skin_winsys_get_monitor_rect: Linux: DefaultScreenOfDisplay(defaultDisplay) (done)\n");
        if (defaultScreen) {
            rect->size.w = defaultScreen->width;
            rect->size.h = defaultScreen->height;
        }
    }
#endif
    D("%s: (%d,%d) %dx%d", __FUNCTION__, rect->pos.x, rect->pos.y,
      rect->size.w, rect->size.h);
}

extern int skin_winsys_get_device_pixel_ratio(double *dpr)
{
    D("skin_winsys_get_device_pixel_ratio");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return -1;
    }
    if (onMainQtThread()) {
        // Main GUI thread - the call is blocking already.
        window->getDevicePixelRatio(dpr, nullptr);
    } else {
        QSemaphore semaphore;
        window->getDevicePixelRatio(dpr, &semaphore);
        semaphore.acquire();
    }
    D("%s: result=%f", __FUNCTION__, *dpr);
    return 0;
}

extern void *skin_winsys_get_window_handle()
{
    D("skin_winsys_get_window_handle");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return NULL;
    }
    const WId handle = window->getWindowId();
    D("%s: result = 0x%p", __FUNCTION__, (void*)handle);
    return (void*)handle;
}

extern void skin_winsys_get_window_pos(int *x, int *y)
{
    D("skin_winsys_get_window_pos");
    GlobalState* g = globalState();
    if (g->window_pos_saved) {
        *x = g->window_pos_x;
        *y = g->window_pos_y;
    } else {
        EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
        if (window == NULL) {
            D("%s: Could not get window handle", __FUNCTION__);
            return;
        }

        if (onMainQtThread()) {
            window->getWindowPos(x, y, nullptr);
        } else {
            QSemaphore semaphore;
            window->getWindowPos(x, y, &semaphore);
            semaphore.acquire();
        }
    }
    D("%s: x=%d y=%d", __FUNCTION__, *x, *y);
}

extern void skin_winsys_get_window_size(int *w, int *h)
{
    D("skin_winsys_get_frame_size");

    QSemaphore semaphore;
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->getWindowSize(w, h, &semaphore);
    semaphore.acquire();

    D("%s: size: %d x %d", __FUNCTION__, *w, *h);
}

extern void skin_winsys_get_frame_pos(int *x, int *y)
{
    QSemaphore semaphore;
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->getFramePos(x, y, &semaphore);
    semaphore.acquire();

    D("%s: x=%d y=%d", __FUNCTION__, *x, *y);
}

extern bool skin_winsys_window_has_frame()
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return false;
    }
    bool hasFrame;
    QSemaphore semaphore;
    window->windowHasFrame(&hasFrame, &semaphore);
    semaphore.acquire();

    D("%s: outValue=%d", __FUNCTION__, hasFrame);
    return hasFrame;
}

extern void skin_winsys_set_device_geometry(const SkinRect* rect) {
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    QRect qrect(rect->pos.x, rect->pos.y, rect->size.w, rect->size.h);
    window->setDeviceGeometry(qrect, nullptr);
}

extern void skin_winsys_save_window_pos() {
    int x = 0, y = 0;
    skin_winsys_get_window_pos(&x, &y);
    GlobalState* g = globalState();
    g->window_pos_saved = true;
    g->window_pos_x = x;
    g->window_pos_y = y;
}

extern bool skin_winsys_is_window_fully_visible()
{
    D("skin_winsys_is_window_fully_visible");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return true;
    }
    bool value;
    if (onMainQtThread()) {
        window->isWindowFullyVisible(&value);
    } else {
        QSemaphore semaphore;
        window->isWindowFullyVisible(&value, &semaphore);
        semaphore.acquire();
    }
    D("%s: result = %s", __FUNCTION__, value ? "true" : "false");
    return value;
}

extern bool skin_winsys_is_window_off_screen()
{
    D("skin_winsys_is_window_off_screen");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return true;
    }
    bool value;
    if (onMainQtThread()) {
        window->isWindowOffScreen(&value);
    } else {
        QSemaphore semaphore;
        window->isWindowOffScreen(&value, &semaphore);
        semaphore.acquire();
    }
    D("%s: result = %s", __FUNCTION__, value ? "true" : "false");
    return value;
}

WinsysPreferredGlesBackend skin_winsys_override_glesbackend_if_auto(WinsysPreferredGlesBackend backend) {
    WinsysPreferredGlesBackend currentPreferred = skin_winsys_get_preferred_gles_backend();
    if (currentPreferred == WINSYS_GLESBACKEND_PREFERENCE_AUTO) {
        return backend;
    }
    return currentPreferred;
}

extern WinsysPreferredGlesBackend skin_winsys_get_preferred_gles_backend()
{
    D("skin_winsys_get_preferred_gles_backend");
    QSettings settings;
    return (WinsysPreferredGlesBackend)settings.value(Ui::Settings::GLESBACKEND_PREFERENCE, 0).toInt();
}

void skin_winsys_set_preferred_gles_backend(WinsysPreferredGlesBackend backend)
{
    D("skin_winsys_set_preferred_gles_backend");
    QSettings settings;
    settings.setValue(Ui::Settings::GLESBACKEND_PREFERENCE, backend);
}

extern WinsysPreferredGlesApiLevel skin_winsys_get_preferred_gles_apilevel()
{
    D("skin_winsys_get_preferred_gles_apilevel");
    QSettings settings;
    return (WinsysPreferredGlesApiLevel)settings.value(Ui::Settings::GLESAPILEVEL_PREFERENCE, 0).toInt();
}

extern void skin_winsys_quit_request()
{
    D(__FUNCTION__);
    if (auto window = EmulatorQtWindow::getInstance()) {
        window->requestClose();
    } else if (auto nowindow = EmulatorNoQtNoWindow::getInstance()){
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
    } else {
        D("%s: Could not get window handle", __FUNCTION__);
    }
}

void skin_winsys_destroy() {
    D(__FUNCTION__);

    QtLogger::stop();

    // Mac is still causing us troubles - it somehow manages to not call the
    // main window destructor (in qemu1 only!) and crashes if QApplication
    // is destroyed right here. So let's delay the deletion for now
#ifdef __APPLE__
    atexit([] {
        delete globalState()->app;
        globalState()->app = nullptr;
    });
#else
    delete globalState()->app;
    globalState()->app = nullptr;
#endif
}

extern void skin_winsys_set_window_icon(const unsigned char *data, size_t size)
{
    D("skin_winsys_set_window_icon");
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowIcon(data, size);
}

extern void skin_winsys_set_window_pos(int x, int y)
{
    D("skin_winsys_set_window_pos %d, %d", x, y);
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowPos(x, y, nullptr);
}

extern void skin_winsys_set_window_size(int w, int h)
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowSize(w, h, nullptr);
}

extern void skin_winsys_set_window_cursor_resize(int which_corner)
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowCursorResize(which_corner, nullptr);
}

extern void skin_winsys_paint_overlay_for_resize(int mouse_x, int mouse_y)
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->paintWindowOverlayForResize(mouse_x, mouse_y, nullptr);
}

extern void skin_winsys_set_window_overlay_for_resize(int which_corner)
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowOverlayForResize(which_corner, nullptr);
}

extern void skin_winsys_clear_window_overlay()
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->clearWindowOverlay(nullptr);
}

extern void skin_winsys_set_window_cursor_normal()
{
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setWindowCursorNormal(nullptr);
}

extern void skin_winsys_set_window_title(const char *title)
{
    D("skin_winsys_set_window_title [%s]", title);
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->setTitle(QString(title), nullptr);
}

extern void skin_winsys_update_rotation(SkinRotation rotation)
{
    // When running "rotate" command via command line, UI
    // does not know that it has rotate, so notify it.
    D("%s", __FUNCTION__);
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->updateRotation(rotation);
}

extern void skin_winsys_show_virtual_scene_controls(bool show) {
    D("skin_winsys_show_virtual_scene_controls [%d]", show ? 1 : 0);
    EmulatorQtWindow* window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    window->showVirtualSceneControls(show);
}

extern void skin_winsys_spawn_thread(bool no_window,
                                     StartFunction f,
                                     int argc,
                                     char** argv) {
    D("skin_spawn_thread");
    if (no_window) {
        EmulatorNoQtNoWindow* guiless_window = EmulatorNoQtNoWindow::getInstance();
        if (guiless_window == NULL) {
            D("%s: Could not get window handle", __FUNCTION__);
            return;
        }
        guiless_window->startThread([f, argc, argv] { f(argc, argv); });
    } else {
        EmulatorQtWindow* window = EmulatorQtWindow::getInstance();
        if (window == NULL) {
            D("%s: Could not get window handle", __FUNCTION__);
            return;
        }
        window->startThread(f, argc, argv);
    }
}

extern void skin_winsys_touch_qt_extended_virtual_sensors(void) {
    EmulatorQtWindow* qtWindow = EmulatorQtWindow::getInstance();
    if (qtWindow == nullptr) return;

    qtWindow->runOnUiThread([qtWindow]() {
        ToolWindow* toolWindow = qtWindow->toolWindow();
        if (toolWindow == nullptr) {
            D("%s: toolWindow null", __FUNCTION__);
            return;
        }
        toolWindow->touchExtendedWindow();
    });
}

void skin_winsys_setup_library_paths() {
    // Make Qt look at the libraries within this installation
    // Despite the fact that we added the plugins directory to the environment
    // we have to add it here as well to support extended unicode characters
    // in the library path. Without adding the plugin path here that won't work.
    // What's even more interesting is that adding the plugin path here is not
    // enough in itself. It also has to be set through the environment variable
    // or extended unicode characters won't work
    std::string qtPluginsPath = androidQtGetPluginsDir();
    QStringList pathList;
    pathList.append(QString::fromUtf8(qtPluginsPath.c_str()));
    QApplication::setLibraryPaths(pathList);
    D("Qt lib path: %s\n", androidQtGetLibraryDir().c_str());
    D("Qt plugin path: %s\n", qtPluginsPath.c_str());
}

extern void skin_winsys_init_args(int argc, char** argv) {
    GlobalState* g = globalState();
    g->argc = argc;
    g->argv = argv;
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QtLogger* q = QtLogger::get();
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        q->write("Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        q->write("Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        q->write("Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        q->write("Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        q->write("Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    }
}

extern int skin_winsys_snapshot_control_start() {
    GlobalState* g = globalState();

    g->app = new QApplication(g->argc, g->argv);
    androidQtDefaultInit();
    // Pop up a stand-alone Snapshot pane
    SnapshotPage* pSP = new SnapshotPage(nullptr, true);
    if (pSP == nullptr) {
        return 1;
    }
    pSP->show();
    return g->app->exec();
}

extern void skin_winsys_start(bool no_window) {
    GlobalState* g = globalState();
#ifdef Q_OS_LINUX
    // This call is required to make doing OpenGL stuff on the UI
    // thread safe. The AA_X11InitThreads flag in Qt does not actually
    // work (confirmed by grepping through Qt code).
    XInitThreads();
#endif
    skin_winsys_setup_library_paths();

    qInstallMessageHandler(myMessageOutput);
    if (no_window) {
        g->app = nullptr;
        EmulatorNoQtNoWindow::create();
    } else {
        g->app = new QApplication(g->argc, g->argv);
        g->app->setAttribute(Qt::AA_UseHighDpiPixmaps);
        androidQtDefaultInit();

        EmulatorQtWindow::create();
#ifdef __APPLE__
        // On OS X, Qt automatically generates an application menu with a "Quit"
        // item. For whatever reason, the auto-generated "quit" does not work,
        // or works intermittently.
        // For that reason, we explicitly create a "Quit" action for Qt to use
        // instead of the auto-generated one, and set it up to correctly quit
        // the emulator.
        // Note: the objects pointed to by quitMenu, quitAction and mainBar will remain
        // for the entire lifetime of the application so we don't bother cleaning
        // them up.
        QMenu* quitMenu = new QMenu(nullptr);
        QAction* quitAction = new QAction(g->app->tr("Quit Emulator"), quitMenu);
        QMenuBar* mainBar = new QMenuBar(nullptr);

        // make sure we never try to call into a dangling pointer, and also
        // that we don't hold it alive just because of a signal connection
        const auto winPtr = EmulatorQtWindow::getInstancePtr();
        const auto winWeakPtr = std::weak_ptr<EmulatorQtWindow>(winPtr);
        QObject::connect(quitAction, &QAction::triggered,
            [winWeakPtr] {
                if (const auto win = winWeakPtr.lock()) {
                    win->requestClose();
                }
            }
        );
        quitMenu->addAction(quitAction);
        mainBar->addMenu(quitMenu);
        qt_mac_set_dock_menu(quitMenu);
#endif
    }
}

void skin_winsys_run_ui_update(SkinGenericFunction f, void* data,
                               bool wait) {
    D(__FUNCTION__);
    EmulatorQtWindow* const window = EmulatorQtWindow::getInstance();
    if (window == NULL) {
        D("%s: Could not get window handle", __FUNCTION__);
        return;
    }
    if (wait) {
        QSemaphore semaphore;
        window->runOnUiThread([f, data]() { f(data); }, &semaphore);
        semaphore.acquire();
    } else {
        window->runOnUiThread([f, data]() { f(data); }, nullptr);
    }
}
extern void skin_winsys_error_dialog(const char* message, const char* title) {
    // Lambdas can only be converted to function pointers if they don't have a
    // capture. So instead we use the void* parameter to pass our data in a
    // struct.
    struct Params {
        const char* Message;
        const char* Title;
    } params = {message, title};

    // Then create a non-capturing lambda that takes those parameters, unpacks
    // them, and shows the error dialog
    auto showDialog = [](void* data) {
        auto params = static_cast<Params*>(data);
        showErrorDialog(params->Message, params->Title);
    };

    // Make sure we show the dialog on the UI thread or it will crash. This is
    // a blocking call so referencing params and its contents from another
    // thread is safe.
    skin_winsys_run_ui_update(showDialog, &params, true);
}

void skin_winsys_set_ui_agent(const UiEmuAgent* agent) {
    ToolWindow::earlyInitialization(agent);

    // Set more early init stuff here:
    // 1. Clipboard sharing
    // 2. Mouse wheel disable
    if (const auto window = EmulatorQtWindow::getInstance()) {
        window->runOnUiThread([agent, window] {
            QSettings settings;
            window->toolWindow()->setClipboardCallbacks(agent);

            const auto disableMouseWheel =
                settings.value(Ui::Settings::DISABLE_MOUSE_WHEEL, false).toBool();

            window->setIgnoreWheelEvent(disableMouseWheel);

            if (!window->toolWindow()->clipboardSharingSupported()) return;

            const auto enableClipboard =
                settings.value(Ui::Settings::CLIPBOARD_SHARING, true).toBool();

            window->toolWindow()->switchClipboardSharing(enableClipboard);
        });
    }
}

void skin_winsys_report_entering_main_loop(void) {
    ToolWindow::onMainLoopStart();
}

extern bool skin_winsys_is_folded() {
    return ToolWindow::isFolded();
}

#ifdef _WIN32
extern "C" int qt_main(int, char**);

#ifdef _MSC_VER
// For msvc build, qt calls main() instead of qMain (see
// qt-src/.../qtmain_win.cpp)
int main(int argc, char** argv) {
#else
int qMain(int argc, char** argv) {
#endif
    // The arguments coming in here are encoded in whatever local code page
    // Windows is configured with but we need them to be UTF-8 encoded. So we
    // use GetCommandLineW and CommandLineToArgvW to get a UTF-16 encoded argv
    // which we then convert to UTF-8.
    //
    // According to the Qt documentation Qt itself doesn't really care about
    // these as it also uses GetCommandLineW on Windows so this shouldn't cause
    // problems for Qt. But the emulator uses argv[0] to determine the path of
    // the emulator executable so we need that to be encoded correctly.
    int numArgs = 0;
    const auto wideArgv = android::base::makeCustomScopedPtr(
                        CommandLineToArgvW(GetCommandLineW(), &numArgs),
                        [](wchar_t** ptr) { LocalFree(ptr); });
    if (!wideArgv) {
        // If this fails we can at least give it a try with the local code page
        // As long as there are only ANSI characters in the arguments this works
        return qt_main(argc, argv);
    }

    // Store converted strings and pointers to those strings, the pointers are
    // what will become the argv for qt_main.
    // Also reserve a slot for an array null-terminator as QEMU command line
    // parsing code relies on it (and it's a part of C Standard, actually).
    std::vector<std::string> arguments(numArgs);
    std::vector<char*> argumentPointers(numArgs + 1);

    for (int i = 0; i < numArgs; ++i) {
        arguments[i] = Win32UnicodeString::convertToUtf8(wideArgv.get()[i]);
        argumentPointers[i] = &arguments[i].front();
    }
    argumentPointers.back() = nullptr;

    return qt_main(numArgs, argumentPointers.data());
}
#endif  // _WIN32
