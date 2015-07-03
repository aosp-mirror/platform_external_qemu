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

#include <QtCore>
#include <QDesktopWidget>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSemaphore>

#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-window.h"
#include "android/skin/qt/winsys-qt.h"

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#endif

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

#define MIN(a,b) (a < b ? a : b)

static EmulatorWindow *instance;

EmulatorWindow::EmulatorWindow(QWidget *parent) :
        QFrame(parent)
{
    instance = this;
    backing_surface = NULL;
    tool_window = new ToolWindow(this);

    QObject::connect(this, &EmulatorWindow::blit, this, &EmulatorWindow::slot_blit);
    QObject::connect(this, &EmulatorWindow::createBitmap, this, &EmulatorWindow::slot_createBitmap);
    QObject::connect(this, &EmulatorWindow::fill, this, &EmulatorWindow::slot_fill);
    QObject::connect(this, &EmulatorWindow::getBitmapInfo, this, &EmulatorWindow::slot_getBitmapInfo);
    QObject::connect(this, &EmulatorWindow::getMonitorDpi, this, &EmulatorWindow::slot_getMonitorDpi);
    QObject::connect(this, &EmulatorWindow::getScreenDimensions, this, &EmulatorWindow::slot_getScreenDimensions);
    QObject::connect(this, &EmulatorWindow::getWindowId, this, &EmulatorWindow::slot_getWindowId);
    QObject::connect(this, &EmulatorWindow::getWindowPos, this, &EmulatorWindow::slot_getWindowPos);
    QObject::connect(this, &EmulatorWindow::isWindowFullyVisible, this, &EmulatorWindow::slot_isWindowFullyVisible);
    QObject::connect(this, &EmulatorWindow::pollEvent, this, &EmulatorWindow::slot_pollEvent);
    QObject::connect(this, &EmulatorWindow::queueEvent, this, &EmulatorWindow::slot_queueEvent);
    QObject::connect(this, &EmulatorWindow::releaseBitmap, this, &EmulatorWindow::slot_releaseBitmap);
    QObject::connect(this, &EmulatorWindow::requestClose, this, &EmulatorWindow::slot_requestClose);
    QObject::connect(this, &EmulatorWindow::requestUpdate, this, &EmulatorWindow::slot_requestUpdate);
    QObject::connect(this, &EmulatorWindow::setWindowIcon, this, &EmulatorWindow::slot_setWindowIcon);
    QObject::connect(this, &EmulatorWindow::setWindowPos, this, &EmulatorWindow::slot_setWindowPos);
    QObject::connect(this, &EmulatorWindow::setTitle, this, &EmulatorWindow::slot_setWindowTitle);
    QObject::connect(this, &EmulatorWindow::showWindow, this, &EmulatorWindow::slot_showWindow);
    QObject::connect(QApplication::instance(), &QCoreApplication::aboutToQuit, this, &EmulatorWindow::slot_clearInstance);
}

EmulatorWindow::~EmulatorWindow()
{
    delete tool_window;
}

EmulatorWindow *EmulatorWindow::getInstance()
{
    return instance;
}

void EmulatorWindow::keyPressEvent(QKeyEvent *event)
{
    handleKeyEvent(kEventKeyDown, event);
}

void EmulatorWindow::keyReleaseEvent(QKeyEvent *event)
{
    handleKeyEvent(kEventKeyUp, event);
    if (event->text().length() > 0) {
        SkinEvent *skin_event = createSkinEvent(kEventTextInput);
        skin_event->u.text.down = false;
        strncpy((char*)skin_event->u.text.text, (const char*)event->text().constData(), 32);
        queueEvent(skin_event);
    }
}

void EmulatorWindow::mouseMoveEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseMotion, event);
}

void EmulatorWindow::mousePressEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseButtonDown, event);
}

void EmulatorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseButtonUp, event);
}

void EmulatorWindow::paintEvent(QPaintEvent *)
{
    if (backing_surface) {
        QPainter painter(this);
        QRect r(0, 0, backing_surface->w, backing_surface->h);
        painter.drawImage(r, *backing_surface->bitmap);
    } else {
        D("Painting emulator window, but no backing bitmap");
    }
}

void EmulatorWindow::show()
{
    QFrame::show();
    tool_window->show();
}

void EmulatorWindow::startThread(StartFunction f, int argc, char **argv)
{
    MainLoopThread *t = new MainLoopThread(f, argc, argv);
    t->start();
}

void EmulatorWindow::wheelEvent(QWheelEvent *event)
{
    int delta = event->delta();
    SkinEvent *skin_event = createSkinEvent(kEventMouseButtonDown);
    skin_event->u.mouse.button = delta >= 0 ? kMouseButtonScrollUp : kMouseButtonScrollDown;
    skin_event->u.mouse.x = event->globalX();
    skin_event->u.mouse.y = event->globalY();
    skin_event->u.mouse.xrel = event->x();
    skin_event->u.mouse.yrel = event->y();
    queueEvent(skin_event);
}

void EmulatorWindow::slot_blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore)
{
    QPainter painter(dst);
    painter.setCompositionMode(*op);
    painter.drawImage(*dstPos, *src, *srcRect);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_clearInstance()
{
    skin_winsys_save_window_pos();
    instance = NULL;
}

void EmulatorWindow::slot_createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore) {
    s->bitmap = new QImage(w, h, QImage::Format_ARGB32);
    s->bitmap->fill(0);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore)
{
    QPainter painter(s->bitmap);
    painter.fillRect(*rect, *color);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore)
{
    pix->pixels = (uint32_t*)s->bitmap->bits();
    pix->w = s->original_w;
    pix->h = s->original_h;
    pix->pitch = s->bitmap->bytesPerLine();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_getMonitorDpi(int *out_dpi, QSemaphore *semaphore)
{
    *out_dpi = QApplication::screens().at(0)->logicalDotsPerInch();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_getScreenDimensions(QRect *out_rect, QSemaphore *semaphore)
{
    QRect rect = ((QApplication*)QApplication::instance())->desktop()->screenGeometry();
    out_rect->setX(rect.x());
    out_rect->setY(rect.y());
    out_rect->setWidth(rect.width());
    out_rect->setHeight(rect.height());
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_getWindowId(WId *out_id, QSemaphore *semaphore)
{
    WId wid = effectiveWinId();
    D("Effective win ID is %lx", wid);
#if defined(__APPLE__)
    wid = (WId)getNSWindow((void*)wid);
    D("After finding parent, win ID is %lx", wid);
#endif
    *out_id = wid;
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_getWindowPos(int *xx, int *yy, QSemaphore *semaphore)
{
    *xx = x();
    *yy = y();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_isWindowFullyVisible(bool *out_value, QSemaphore *semaphore)
{
    *out_value = ((QApplication*)QApplication::instance())->desktop()->screenGeometry().contains(geometry());
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore)
{
    if (event_queue.isEmpty()) {
        *hasEvent = false;
    } else {
        *hasEvent = true;
        SkinEvent *newEvent = event_queue.dequeue();
        memcpy(event, newEvent, sizeof(SkinEvent));
        delete newEvent;
    }
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_queueEvent(SkinEvent *event, QSemaphore *semaphore)
{
    event_queue.enqueue(event);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_releaseBitmap(SkinSurface *s, QSemaphore *semaphore)
{
    if (backing_surface == s) {
        backing_surface = NULL;
    }
    delete s->bitmap;
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_requestClose(QSemaphore *semaphore)
{
    close();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_requestUpdate(const QRect *rect, QSemaphore *semaphore)
{
    QRect r(rect->x()  *backing_surface->w / backing_surface->original_w,
            rect->y()  *backing_surface->h / backing_surface->original_h,
            rect->width()  *backing_surface->w / backing_surface->original_w,
            rect->height()  *backing_surface->h / backing_surface->original_h);
    update(r);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_setWindowPos(int x, int y, QSemaphore *semaphore)
{
    move(x, y);
    tool_window->move(this->x() + width() + 10, this->y() + 10);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_setWindowIcon(const unsigned char *, int, QSemaphore *semaphore)
{
    //    QPixmap image;
    //    image.loadFromData(data, size);
    //    QIcon icon(image);
    //    setWindowIcon(icon);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_setWindowTitle(const QString *title, QSemaphore *semaphore)
{
    setWindowTitle(*title);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_showWindow(int x, int y, int w, int h, int, QSemaphore *semaphore)
{
    move(x, y);
    resize(w, h);
    show();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorWindow::slot_fullscreen()
{
    simulateKeyPress(KEY_ENTER, kKeyModLAlt);
}

void EmulatorWindow::slot_power()
{
    simulateKeyPress(KEY_F7, 0);
}

void EmulatorWindow::slot_rotate()
{
    simulateKeyPress(KEY_F12, kKeyModLCtrl);
}

void EmulatorWindow::slot_volumeDown()
{
    simulateKeyPress(KEY_F6, kKeyModLCtrl);
}

void EmulatorWindow::slot_volumeUp()
{
    simulateKeyPress(KEY_F5, kKeyModLCtrl);
}

void EmulatorWindow::slot_zoom()
{
}

// Convert a Qt::Key_XXX code into the corresponding Linux keycode value.
// On failure, return -1.
static int convertKeyCode(int sym)
{
#define KK(x,y)  { Qt::Key_ ## x, KEY_ ## y }
#define K1(x)    KK(x,x)
    static const struct {
        int qt_sym;
        int keycode;
    } kConvert[] = {
        KK(Left, LEFT),
        KK(Right, RIGHT),
        KK(Up, UP),
        KK(Down, DOWN),
        K1(0),
        K1(1),
        K1(2),
        K1(3),
        K1(4),
        K1(5),
        K1(6),
        K1(7),
        K1(8),
        K1(9),
        K1(F1),
        K1(F2),
        K1(F3),
        K1(F4),
        K1(F5),
        K1(F6),
        K1(F7),
        K1(F8),
        K1(F9),
        K1(F10),
        K1(F11),
        K1(F12),
        K1(A),
        K1(B),
        K1(C),
        K1(D),
        K1(E),
        K1(F),
        K1(G),
        K1(H),
        K1(I),
        K1(J),
        K1(K),
        K1(L),
        K1(M),
        K1(N),
        K1(O),
        K1(P),
        K1(Q),
        K1(R),
        K1(S),
        K1(T),
        K1(U),
        K1(V),
        K1(W),
        K1(X),
        K1(Y),
        K1(Z),
        KK(Minus, MINUS),
        KK(Equal, EQUAL),
        KK(Backspace, BACKSPACE),
        KK(Home, HOME),
        KK(Escape, ESC),
        KK(Comma, COMMA),
        KK(Period,DOT),
        KK(Space, SPACE),
        KK(Slash, SLASH),
        KK(Return,ENTER),
        KK(Tab, TAB),
        KK(BracketLeft, LEFTBRACE),
        KK(BracketRight, RIGHTBRACE),
        KK(Backslash, BACKSLASH),
        KK(Semicolon, SEMICOLON),
        KK(Apostrophe, APOSTROPHE),
    };
    const size_t kConvertSize = sizeof(kConvert) / sizeof(kConvert[0]);
    size_t nn;
    for (nn = 0; nn < kConvertSize; ++nn) {
        if (sym == kConvert[nn].qt_sym) {
            return kConvert[nn].keycode;
        }
    }
    return -1;
}

SkinEvent *EmulatorWindow::createSkinEvent(SkinEventType type)
{
    SkinEvent *skin_event = new SkinEvent();
    skin_event->type = type;
    return skin_event;
}

void EmulatorWindow::handleEvent(SkinEventType type, QMouseEvent *event)
{
    SkinEvent *skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = event->button() == Qt::RightButton ? kMouseButtonRight : kMouseButtonLeft;
    skin_event->u.mouse.x = event->x();
    skin_event->u.mouse.y = event->y();
    skin_event->u.mouse.xrel = 0;
    skin_event->u.mouse.yrel = 0;
    queueEvent(skin_event);
}

void EmulatorWindow::handleKeyEvent(SkinEventType type, QKeyEvent *pEvent)
{
    SkinEvent *skin_event = createSkinEvent(type);
    SkinEventKeyData *keyData = &skin_event->u.key;
    keyData->keycode = convertKeyCode(pEvent->key());
    Qt::KeyboardModifiers modifiers = pEvent->modifiers();
    if (modifiers & Qt::ShiftModifier) keyData->mod |= kKeyModLShift;
    if (modifiers & Qt::ControlModifier) keyData->mod |= kKeyModLCtrl;
    if (modifiers & Qt::AltModifier) keyData->mod |= kKeyModLAlt;
    queueEvent(skin_event);
}

void EmulatorWindow::simulateKeyPress(int keyCode, int modifiers)
{
    SkinEvent *event = new SkinEvent();
    event->type = kEventKeyDown;
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    slot_queueEvent(event);
    event = new SkinEvent();
    event->type = kEventKeyUp;
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    slot_queueEvent(event);
}
