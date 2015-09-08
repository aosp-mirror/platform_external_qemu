/* Copyright (C) 2015 The Android Open Source Project
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
#ifndef SKIN_QT_EMULATOR_QT_WINDOW_H
#define SKIN_QT_EMULATOR_QT_WINDOW_H

#include <QtCore>
#include <QApplication>
#include <QFrame>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QObject>
#include <QPainter>
#include <QWidget>

#include "android/skin/event.h"
#include "android/skin/surface.h"
#include "android/skin/winsys.h"
#include "android/skin/qt/tool-window.h"

namespace Ui {
    class EmulatorWindow;
}

typedef struct SkinSurface SkinSurface;

class MainLoopThread : public QThread
{
    Q_OBJECT
public:
    MainLoopThread(StartFunction f, int argc, char **argv) : start_function(f), argc(argc), argv(argv) {}
    void run() Q_DECL_OVERRIDE { if (start_function) start_function(argc, argv); }
private:
    StartFunction start_function;
    int argc;
    char **argv;
};

class EmulatorQtWindow : public QFrame
{
    Q_OBJECT
public:
    explicit EmulatorQtWindow(QWidget *parent = 0);
    virtual ~EmulatorQtWindow();

    static EmulatorQtWindow *getInstance();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    void moveEvent(QMoveEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void show();
    void startThread(StartFunction f, int argc, char **argv);
    void wheelEvent(QWheelEvent *event);

    /*
     In Qt, signals are normally events of interest that a class can emit, which can be hooked up to arbitrary slots. Here
     we use this mechanism for a different purpose: it's to allow the QEMU thread to request an operation be performed on
     the Qt thread; Qt allows signals to be emitted from any thread. When used in this fashion, the signal is queued and
     handled asyncrhonously. Since we sometimes will call these signals from Qt's thread as well, we can't use
     BlockingQueuedConnections for these signals, since this connection type will deadlock if called from the same thread.
     For that reason, we use a normal non-blocking connection type, and allow all of the signals to pass an optional semaphore
     that will be released by the slot when it is done processing. If you want to block on the completion of the signal, simply
     pass in the semaphore to the signal and acquire it after the call returns. If you're passing in pointers to data structures
     that could change or go away, you will need to make sure you block to maintain the integrity of the data while the signal runs.

     TODO: allow nonblocking calls to these signals by having the signal take ownership of object pointers. This would allow QEMU
     to do things like update the screen without blocking, which would make it run faster.
     */
signals:
    void blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore = NULL);
    void createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore = NULL);
    void fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore = NULL);
    void getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore = NULL);
    void getMonitorDpi(int *out_dpi, QSemaphore *semaphore = NULL);
    void getScreenDimensions(QRect *out_rect, QSemaphore *semaphore = NULL);
    void getWindowId(WId *out_id, QSemaphore *semaphore = NULL);
    void getWindowPos(int *x, int *y, QSemaphore *semaphore = NULL);
    void isWindowFullyVisible(bool *out_value, QSemaphore *semaphore = NULL);
    void pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore = NULL);
    void queueEvent(SkinEvent *event, QSemaphore *semaphore = NULL);
    void releaseBitmap(SkinSurface *s, QSemaphore *sempahore = NULL);
    void requestClose(QSemaphore *semaphore = NULL);
    void requestUpdate(const QRect *rect, QSemaphore *semaphore = NULL);
    void setWindowIcon(const unsigned char *data, int size, QSemaphore *semaphore = NULL);
    void setWindowPos(int x, int y, QSemaphore *semaphore = NULL);
    void setTitle(const QString *title, QSemaphore *semaphore = NULL);
    void showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore = NULL);

public:
    void simulateKeyPress(int keyCode, int modifiers);
    void simulateQuit();
    void simulateSetScale(double scale);

private slots:
    void slot_blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore = NULL);
    void slot_clearInstance();
    void slot_createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore = NULL);
    void slot_fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore = NULL);
    void slot_getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore = NULL);
    void slot_getMonitorDpi(int *out_dpi, QSemaphore *semaphore = NULL);
    void slot_getScreenDimensions(QRect *out_rect, QSemaphore *semaphore = NULL);
    void slot_getWindowId(WId *out_id, QSemaphore *semaphore = NULL);
    void slot_getWindowPos(int *x, int *y, QSemaphore *semaphore = NULL);
    void slot_isWindowFullyVisible(bool *out_value, QSemaphore *semaphore = NULL);
    void slot_pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore = NULL);
    void slot_queueEvent(SkinEvent *event, QSemaphore *semaphore = NULL);
    void slot_releaseBitmap(SkinSurface *s, QSemaphore *sempahore = NULL);
    void slot_requestClose(QSemaphore *semaphore = NULL);
    void slot_requestUpdate(const QRect *rect, QSemaphore *semaphore = NULL);
    void slot_setWindowIcon(const unsigned char *data, int size, QSemaphore *semaphore = NULL);
    void slot_setWindowPos(int x, int y, QSemaphore *semaphore = NULL);
    void slot_setWindowTitle(const QString *title, QSemaphore *semaphore = NULL);
    void slot_showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore = NULL);

    /*
     Here are conventional slots that perform interesting high-level functions in the emulator. These can be hooked up to signals
     from UI elements or called independently.
     */
public slots:
    void slot_back();
    void slot_down();
    void slot_home();
    void slot_left();
    void slot_menu();
    void slot_recents();
    void slot_right();
    void slot_screenrecord();
    void slot_screenshot();
    void slot_up();
    void slot_voice();
    void slot_zoom();

private:
    void handleEvent(SkinEventType type, QMouseEvent *event);
    SkinEvent *createSkinEvent(SkinEventType type);
    void handleKeyEvent(SkinEventType type, QKeyEvent *pEvent);

    SkinSurface *backing_surface;
    QQueue<SkinEvent*> event_queue;
    ToolWindow *tool_window;
};

struct SkinSurface {
    int refcount;
    int id;
    QImage *bitmap;
    int w, h, original_w, original_h;
    EmulatorQtWindow *window;
};

#endif // SKIN_QT_EMULATOR_QT_WINDOW_H
