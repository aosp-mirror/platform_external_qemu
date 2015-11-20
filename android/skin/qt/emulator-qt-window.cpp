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
#include <QCursor>
#include <QAbstractSlider>
#include <QCursor>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QIcon>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSemaphore>
#include <QWindow>

#include "android/base/files/PathUtils.h"
#include "android/emulation/control/user_event_agent.h"
#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/winsys-qt.h"
#include "android/ui-emu-agent.h"

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

using namespace android::base;

static EmulatorQtWindow *instance;

EmulatorQtWindow::EmulatorQtWindow(QWidget *parent) :
        QFrame(parent),
        mContainer(this),
        mOverlay(this, &mContainer),
        mZoomFactor(1.0),
        mInZoomMode(false),
        mNextIsZoom(false),
        mGrabKeyboardInput(false),
        mMouseInside(false),
        mMainLoopThread(nullptr)
{
    instance = this;
    backing_surface = NULL;
    batteryState    = NULL;

    tool_window = new ToolWindow(this, &mContainer);

    this->setAcceptDrops(true);

    QObject::connect(this, &EmulatorQtWindow::blit, this, &EmulatorQtWindow::slot_blit);
    QObject::connect(this, &EmulatorQtWindow::createBitmap, this, &EmulatorQtWindow::slot_createBitmap);
    QObject::connect(this, &EmulatorQtWindow::fill, this, &EmulatorQtWindow::slot_fill);
    QObject::connect(this, &EmulatorQtWindow::getBitmapInfo, this, &EmulatorQtWindow::slot_getBitmapInfo);
    QObject::connect(this, &EmulatorQtWindow::getDevicePixelRatio, this, &EmulatorQtWindow::slot_getDevicePixelRatio);
    QObject::connect(this, &EmulatorQtWindow::getMonitorDpi, this, &EmulatorQtWindow::slot_getMonitorDpi);
    QObject::connect(this, &EmulatorQtWindow::getScreenDimensions, this, &EmulatorQtWindow::slot_getScreenDimensions);
    QObject::connect(this, &EmulatorQtWindow::getWindowId, this, &EmulatorQtWindow::slot_getWindowId);
    QObject::connect(this, &EmulatorQtWindow::getWindowPos, this, &EmulatorQtWindow::slot_getWindowPos);
    QObject::connect(this, &EmulatorQtWindow::isWindowFullyVisible, this, &EmulatorQtWindow::slot_isWindowFullyVisible);
    QObject::connect(this, &EmulatorQtWindow::pollEvent, this, &EmulatorQtWindow::slot_pollEvent);
    QObject::connect(this, &EmulatorQtWindow::queueEvent, this, &EmulatorQtWindow::slot_queueEvent);
    QObject::connect(this, &EmulatorQtWindow::releaseBitmap, this, &EmulatorQtWindow::slot_releaseBitmap);
    QObject::connect(this, &EmulatorQtWindow::requestClose, this, &EmulatorQtWindow::slot_requestClose);
    QObject::connect(this, &EmulatorQtWindow::requestUpdate, this, &EmulatorQtWindow::slot_requestUpdate);
    QObject::connect(this, &EmulatorQtWindow::setWindowIcon, this, &EmulatorQtWindow::slot_setWindowIcon);
    QObject::connect(this, &EmulatorQtWindow::setWindowPos, this, &EmulatorQtWindow::slot_setWindowPos);
    QObject::connect(this, &EmulatorQtWindow::setTitle, this, &EmulatorQtWindow::slot_setWindowTitle);
    QObject::connect(this, &EmulatorQtWindow::showWindow, this, &EmulatorQtWindow::slot_showWindow);
    QObject::connect(this, &EmulatorQtWindow::runOnUiThread, this, &EmulatorQtWindow::slot_runOnUiThread);
    QObject::connect(QApplication::instance(), &QCoreApplication::aboutToQuit, this, &EmulatorQtWindow::slot_clearInstance);

    QObject::connect(&mScreencapProcess, SIGNAL(finished(int)), this, SLOT(slot_screencapFinished(int)));
    QObject::connect(&mScreencapPullProcess, SIGNAL(finished(int)), this, SLOT(slot_screencapPullFinished(int)));

    QObject::connect(mContainer.horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slot_horizontalScrollChanged(int)));
    QObject::connect(mContainer.verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slot_verticalScrollChanged(int)));
    QObject::connect(mContainer.horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(slot_scrollRangeChanged(int,int)));
    QObject::connect(mContainer.verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(slot_scrollRangeChanged(int,int)));
    QObject::connect(tool_window, SIGNAL(skinUIEvent(SkinEvent*)), this, SLOT(slot_queueEvent(SkinEvent*)));
}

EmulatorQtWindow::~EmulatorQtWindow()
{
    delete tool_window;
    delete mMainLoopThread;
}

EmulatorQtWindow *EmulatorQtWindow::getInstance()
{
    return instance;
}

void EmulatorQtWindow::closeEvent(QCloseEvent *event)
{
    if (mMainLoopThread && mMainLoopThread->isRunning()) {
        queueEvent(createSkinEvent(kEventQuit));
        event->ignore();
    } else {
        event->accept();
    }
}

void EmulatorQtWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept all drag enter events with any URL, then filter more in drop events
    // TODO: check this with hasFormats() using MIME type for .apk?
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void EmulatorQtWindow::dropEvent(QDropEvent *event)
{
    // Get the first url - if it's an APK and the only file, attempt to install it
    QList<QUrl> urls = event->mimeData()->urls();
    QString url = urls[0].toLocalFile();

    if (url.endsWith(".apk") && urls.length() == 1) {
        tool_window->runAdbInstall(url);
        return;
    } else {

        // If any of the files is an APK, intent was ambiguous
        for (int i = 0; i < urls.length(); i++) {
            if (urls[i].path().endsWith(".apk")) {
                tool_window->showErrorDialog(tr("Drag-and-drop can either install a single APK"
                                                " file or copy one or more non-APK files to the"
                                                " Emulator SD card."),
                                             tr("Drag and Drop"));
                return;
            }
        }
        tool_window->runAdbPush(urls);
    }
}

void EmulatorQtWindow::keyPressEvent(QKeyEvent *event)
{
    handleKeyEvent(kEventKeyDown, event);
}

void EmulatorQtWindow::keyReleaseEvent(QKeyEvent *event)
{
    handleKeyEvent(kEventKeyUp, event);
    if (mGrabKeyboardInput && mouseInside()) {
        if (event->text().length() > 0) {
            SkinEvent *skin_event = createSkinEvent(kEventTextInput);
            skin_event->u.text.down = false;
            strncpy((char*)skin_event->u.text.text, (const char*)event->text().constData(), 32);
            queueEvent(skin_event);
        }
    }
}

void EmulatorQtWindow::mouseMoveEvent(QMouseEvent *event)
{
    handleMouseEvent(kEventMouseMotion, event);
}

void EmulatorQtWindow::mousePressEvent(QMouseEvent *event)
{
    handleMouseEvent(kEventMouseButtonDown, event);
}

void EmulatorQtWindow::mouseReleaseEvent(QMouseEvent *event)
{
    handleMouseEvent(kEventMouseButtonUp, event);
}

void EmulatorQtWindow::paintEvent(QPaintEvent *)
{
    if (backing_surface) {
        QPainter painter(this);

        QRect bg(QPoint(0, 0), this->size());
        painter.fillRect(bg, Qt::black);

        QRect r(0, 0, backing_surface->w, backing_surface->h);
        // Rescale with smooth transformation to avoid aliasing
        QImage scaled_bitmap =
                backing_surface->bitmap->scaled(r.size() * devicePixelRatio(),
                                                Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);
        scaled_bitmap.setDevicePixelRatio(devicePixelRatio());
        painter.drawImage(r, scaled_bitmap);
    } else {
        D("Painting emulator window, but no backing bitmap");
    }
}

void EmulatorQtWindow::activateWindow()
{
    mContainer.activateWindow();
}

void EmulatorQtWindow::raise()
{
    mContainer.raise();
    tool_window->raise();
}

void EmulatorQtWindow::show()
{
    mContainer.show();
    QFrame::show();
    tool_window->show();
    tool_window->dockMainWindow();

    QObject::connect(window()->windowHandle(), SIGNAL(screenChanged(QScreen *)), this, SLOT(slot_screenChanged(QScreen *)));
}

void EmulatorQtWindow::showMinimized()
{
    mContainer.showMinimized();
}

void EmulatorQtWindow::startThread(StartFunction f, int argc, char **argv)
{
    if (!mMainLoopThread) {
        mMainLoopThread = new MainLoopThread(f, argc, argv);
        QObject::connect(mMainLoopThread, &QThread::finished, &mContainer, &EmulatorWindowContainer::close);
        mMainLoopThread->start();
    } else {
        D("mMainLoopThread already started");
    }
}

void EmulatorQtWindow::slot_blit(QImage *src, QRect *srcRect, QImage *dst, QPoint *dstPos, QPainter::CompositionMode *op, QSemaphore *semaphore)
{
    QPainter painter(dst);
    painter.setCompositionMode(*op);
    painter.drawImage(*dstPos, *src, *srcRect);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_clearInstance()
{
    skin_winsys_save_window_pos();
    delete instance;
    instance = NULL;
}

void EmulatorQtWindow::slot_createBitmap(SkinSurface *s, int w, int h, QSemaphore *semaphore) {
    s->bitmap = new QImage(w, h, QImage::Format_ARGB32);
    s->bitmap->fill(0);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_fill(SkinSurface *s, const QRect *rect, const QColor *color, QSemaphore *semaphore)
{
    QPainter painter(s->bitmap);
    painter.fillRect(*rect, *color);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_getBitmapInfo(SkinSurface *s, SkinSurfacePixels *pix, QSemaphore *semaphore)
{
    pix->pixels = (uint32_t*)s->bitmap->bits();
    pix->w = s->original_w;
    pix->h = s->original_h;
    pix->pitch = s->bitmap->bytesPerLine();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_getDevicePixelRatio(double *out_dpr, QSemaphore *semaphore)
{
    *out_dpr = devicePixelRatio();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_getMonitorDpi(int *out_dpi, QSemaphore *semaphore)
{
    *out_dpi = QApplication::screens().at(0)->logicalDotsPerInch();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_getScreenDimensions(QRect *out_rect, QSemaphore *semaphore)
{
    QRect rect = ((QApplication*)QApplication::instance())->desktop()->screenGeometry();
    out_rect->setX(rect.x());
    out_rect->setY(rect.y());
    out_rect->setWidth(rect.width());
    out_rect->setHeight(rect.height());
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_getWindowId(WId *out_id, QSemaphore *semaphore)
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

void EmulatorQtWindow::slot_getWindowPos(int *xx, int *yy, QSemaphore *semaphore)
{
    *xx = mContainer.x();
    *yy = mContainer.y();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_isWindowFullyVisible(bool *out_value, QSemaphore *semaphore)
{
    *out_value = ((QApplication*)QApplication::instance())->desktop()->screenGeometry().contains(mContainer.geometry());
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_pollEvent(SkinEvent *event, bool *hasEvent, QSemaphore *semaphore)
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

void EmulatorQtWindow::slot_queueEvent(SkinEvent *event, QSemaphore *semaphore)
{
    const bool firstEvent = event_queue.isEmpty();

    // For the following two events, only the "last" example of said event matters, so ensure
    // that there is only one of them in the queue at a time.
    bool replaced = false;
    if (event->type == kEventScrollBarChanged || event->type == kEventZoomedWindowResized) {
        for (int i = 0; i < event_queue.size(); i++) {
            if (event_queue.at(i)->type == event->type) {
                SkinEvent *toDelete = event_queue.at(i);
                event_queue.replace(i, event);
                delete toDelete;
                replaced = true;
                break;
            }
        }
    }

    if (!replaced) event_queue.enqueue(event);

    const auto uiAgent = tool_window->getUiEmuAgent();
    if (firstEvent && uiAgent && uiAgent->userEvents
            && uiAgent->userEvents->onNewUserEvent) {
        // we know that as soon as emulator starts processing user events
        // it processes them until there are none. So we can notify it only
        // if this event is the first one
        uiAgent->userEvents->onNewUserEvent();
    }

    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_releaseBitmap(SkinSurface *s, QSemaphore *semaphore)
{
    if (backing_surface == s) {
        backing_surface = NULL;
    }
    delete s->bitmap;
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_requestClose(QSemaphore *semaphore)
{
    mContainer.close();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_requestUpdate(const QRect *rect, QSemaphore *semaphore)
{
    QRect r(rect->x()  *backing_surface->w / backing_surface->original_w,
            rect->y()  *backing_surface->h / backing_surface->original_h,
            rect->width()  *backing_surface->w / backing_surface->original_w,
            rect->height()  *backing_surface->h / backing_surface->original_h);
    update(r);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_setWindowPos(int x, int y, QSemaphore *semaphore)
{
    mContainer.move(x, y);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_setWindowIcon(const unsigned char *data, int size, QSemaphore *semaphore)
{
    QPixmap image;
    image.loadFromData(data, size);
    QIcon icon(image);
    QApplication::setWindowIcon(icon);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_setWindowTitle(const QString *title, QSemaphore *semaphore)
{
    mContainer.setWindowTitle(*title);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore)
{
    // Zooming forces the scroll bar to be visible for sizing purpose, so reset them
    // back to the default policy.
    mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    backing_surface = surface;
    if (is_fullscreen) {
        showFullScreen();
    } else {
        showNormal();
        setFixedSize(rect->size());

        // If this was the result of a zoom, don't change the overall window size, and adjust the
        // scroll bars to reflect the desired focus point.
        if (mNextIsZoom) {
            mContainer.move(rect->topLeft());
            recenterFocusPoint();
        } else {
            mContainer.setGeometry(*rect);
        }
        mNextIsZoom = false;
    }
    show();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_screenChanged(QScreen*)
{
    queueEvent(createSkinEvent(kEventScreenChanged));
}

void EmulatorQtWindow::slot_horizontalScrollChanged(int value)
{
    simulateScrollBarChanged(value, mContainer.verticalScrollBar()->value());
}

void EmulatorQtWindow::slot_verticalScrollChanged(int value)
{
    simulateScrollBarChanged(mContainer.horizontalScrollBar()->value(), value);
}

void EmulatorQtWindow::slot_scrollRangeChanged(int min, int max)
{
    simulateScrollBarChanged(mContainer.horizontalScrollBar()->value(),
                             mContainer.verticalScrollBar()->value());
}

void EmulatorQtWindow::screenshot()
{
    if (mScreencapProcess.state() != QProcess::NotRunning) {
        // Modal dialogs should prevent this
        return;
    }

    QStringList args;
    QString command = tool_window->getAdbFullPath(&args);
    if (command.isNull()) {
        return;
    }

    // Add the arguments
    args << "shell";                // Running a shell command
    args << "screencap";            // Take a screen capture
    args << "-p";                   // Print it to a file
    args << REMOTE_SCREENSHOT_FILE; // The temporary screenshot file

    // Keep track of this process
    mScreencapProcess.start(command, args);
    mScreencapProcess.waitForStarted();
}


void EmulatorQtWindow::slot_screencapFinished(int exitStatus)
{
    if (exitStatus) {
        QByteArray er = mScreencapProcess.readAllStandardError();
        er = er.replace('\n', "<br/>");
        QString msg = tr("The screenshot could not be captured. Output:<br/><br/>") + QString(er);
        tool_window->showErrorDialog(msg, tr("Screenshot"));
    } else {

        // Pull the image from its remote location to the desired location
        QStringList args;
        QString command = tool_window->getAdbFullPath(&args);
        if (command.isNull()) {
            return;
        }

        // Add the arguments
        args << "pull";                 // Pulling a file
        args << REMOTE_SCREENSHOT_FILE; // Which file to pull

        QString fileName = tool_window->getScreenshotSaveFile();
        if (fileName.isEmpty()) {
            tool_window->showErrorDialog(tr("The screenshot save location is invalid.<br/>"
                                            "Check the settings page and ensure the directory "
                                            "exists and is writeable."),
                                         tr("Screenshot"));
            return;
        }

        args << fileName;

        // Use a different process to avoid infinite looping when pulling the file
        mScreencapPullProcess.start(command, args);
        mScreencapPullProcess.waitForStarted();
    }
}

void EmulatorQtWindow::slot_screencapPullFinished(int exitStatus)
{
    if (exitStatus) {
        QByteArray er = mScreencapPullProcess.readAllStandardError();
        er = er.replace('\n', "<br/>");
        QString msg = tr("The screenshot could not be loaded from the device. Output:<br/><br/>")
                        + QString(er);
        tool_window->showErrorDialog(msg, tr("Screenshot"));
    }
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

SkinEvent *EmulatorQtWindow::createSkinEvent(SkinEventType type)
{
    SkinEvent *skin_event = new SkinEvent();
    skin_event->type = type;
    return skin_event;
}

void EmulatorQtWindow::doResize(const QSize &size)
{
    if (backing_surface) {
        QSize newSize(backing_surface->original_w, backing_surface->original_h);
        newSize.scale(size, Qt::KeepAspectRatio);

        double newScale = (double) newSize.width() / (double) backing_surface->original_w;
        simulateSetScale(newScale);
    }
}


void EmulatorQtWindow::handleMouseEvent(SkinEventType type, QMouseEvent *event)
{
    SkinEvent *skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = event->button() == Qt::RightButton ? kMouseButtonRight : kMouseButtonLeft;
    skin_event->u.mouse.x = event->x();
    skin_event->u.mouse.y = event->y();
    skin_event->u.mouse.xrel = 0;
    skin_event->u.mouse.yrel = 0;

    queueEvent(skin_event);
}

void EmulatorQtWindow::handleKeyEvent(SkinEventType type, QKeyEvent *event)
{
    bool must_ungrab = event->key() == Qt::Key_Alt &&
                       event->modifiers() == (Qt::ControlModifier + Qt::AltModifier);
    if (mGrabKeyboardInput && must_ungrab) {
        mGrabKeyboardInput = false;
    }

    if (mGrabKeyboardInput && mouseInside()) {
        SkinEvent *skin_event = createSkinEvent(type);
        SkinEventKeyData *keyData = &skin_event->u.key;
        keyData->keycode = convertKeyCode(event->key());

        Qt::KeyboardModifiers modifiers = event->modifiers();
        if (modifiers & Qt::ShiftModifier) keyData->mod |= kKeyModLShift;
        if (modifiers & Qt::ControlModifier) keyData->mod |= kKeyModLCtrl;
        if (modifiers & Qt::AltModifier) keyData->mod |= kKeyModLAlt;

        queueEvent(skin_event);
    } else {

        // When in zoom mode, the overlay should be toggled
        if (mInZoomMode) {
            if (event->key() == Qt::Key_Control) {
                if (type == kEventKeyDown) {
                    mOverlay.show();
                } else if (type == kEventKeyUp) {
                    mOverlay.hide();
                }
            }
        }

        tool_window->handleQtKeyEvent(event);
    }
}

void EmulatorQtWindow::simulateKeyPress(int keyCode, int modifiers)
{
    SkinEvent *event = createSkinEvent(kEventKeyDown);
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    slot_queueEvent(event);

    event = createSkinEvent(kEventKeyUp);
    event->u.key.keycode = keyCode;
    event->u.key.mod = modifiers;
    slot_queueEvent(event);
}

void EmulatorQtWindow::simulateScrollBarChanged(int x, int y)
{
    SkinEvent *event = createSkinEvent(kEventScrollBarChanged);
    event->u.scroll.x = x;
    event->u.scroll.xmax = mContainer.horizontalScrollBar()->maximum();
    event->u.scroll.y = y;
    event->u.scroll.ymax = mContainer.verticalScrollBar()->maximum();
    slot_queueEvent(event);
}

void EmulatorQtWindow::simulateSetScale(double scale)
{
    // Avoid zoom and scale events clobbering each other if the user rapidly changes zoom levels
    if (mNextIsZoom) {
        return;
    }

    // Reset our local copy of zoom factor
    mZoomFactor = 1.0;

    SkinEvent *event = createSkinEvent(kEventSetScale);
    event->u.window.scale = scale;
    slot_queueEvent(event);
}

void EmulatorQtWindow::simulateSetZoom(double zoom)
{
    // Avoid zoom and scale events clobbering each other if the user rapidly changes zoom levels
    if (mNextIsZoom || mZoomFactor == zoom) {
        return;
    }

    // Qt Widgets do not get properly sized unless they appear at least once. The scroll bars
    // *must* be properly sized in order for zoom to create the correct GLES subwindow, so this
    // ensures they will be. This is reset as soon as the window is shown.
    mContainer.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mContainer.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    mNextIsZoom = true;
    mZoomFactor = zoom;

    simulateWindowMoved(mContainer.pos());

    QSize viewport = mContainer.viewportSize();

    SkinEvent *event = createSkinEvent(kEventSetZoom);
    event->u.window.x = viewport.width();
    event->u.window.y = viewport.height();

    QScrollBar *horizontal = mContainer.horizontalScrollBar();
    event->u.window.scroll_h = horizontal->isVisible() ? horizontal->height() : 0;
    event->u.window.scale = zoom;
    slot_queueEvent(event);
}

void EmulatorQtWindow::simulateWindowMoved(const QPoint &pos)
{
    SkinEvent *event = createSkinEvent(kEventWindowMoved);
    event->u.window.x = pos.x();
    event->u.window.y = pos.y();
    slot_queueEvent(event);

    mOverlay.move(mContainer.mapToGlobal(QPoint()));
}

void EmulatorQtWindow::simulateZoomedWindowResized(const QSize &size)
{
    SkinEvent *event = createSkinEvent(kEventZoomedWindowResized);
    QScrollBar *horizontal = mContainer.horizontalScrollBar();
    event->u.scroll.x = horizontal->value();
    event->u.scroll.y = mContainer.verticalScrollBar()->value();
    event->u.scroll.xmax = size.width();
    event->u.scroll.ymax = size.height();
    event->u.scroll.scroll_h = horizontal->isVisible() ? horizontal->height() : 0;
    slot_queueEvent(event);

    mOverlay.resize(size);
}


void EmulatorQtWindow::slot_runOnUiThread(SkinGenericFunction* f, void* data, QSemaphore* semaphore) {
    (*f)(data);
    if (semaphore) semaphore->release();
}

bool EmulatorQtWindow::isInZoomMode()
{
    return mInZoomMode;
}

void EmulatorQtWindow::toggleZoomMode()
{
    mInZoomMode = !mInZoomMode;

    // Exiting zoom mode snaps back to aspect ratio
    if (!mInZoomMode) {
        mContainer.setCursor(QCursor(Qt::ArrowCursor));
        doResize(mContainer.size());
        mOverlay.hide();
    }
}

void EmulatorQtWindow::recenterFocusPoint()
{
    mContainer.horizontalScrollBar()->setValue(mFocus.x() * width() - mViewportFocus.x());
    mContainer.verticalScrollBar()->setValue(mFocus.y() * height() - mViewportFocus.y());

    mFocus = QPointF();
    mViewportFocus = QPoint();
}

void EmulatorQtWindow::saveZoomPoints(const QPoint &focus, const QPoint &viewportFocus)
{
    // The underlying frame will change sizes, so get what "percentage" of the frame was
    // clicked, where (0,0) is the top-left corner and (1,1) is the bottom right corner.
    mFocus = QPointF((float) focus.x() / this->width(),
                     (float) focus.y() / this->height());

    // Save to re-align the container with the underlying frame.
    mViewportFocus = viewportFocus;
}

void EmulatorQtWindow::zoomIn(const QPoint &focus, const QPoint &viewportFocus)
{
    saveZoomPoints(focus, viewportFocus);

    // The below scale = x creates a skin equivalent to calling "window scale x" through the
    // emulator console. At scale = 1, the device should be at a 1:1 pixel mapping with the
    // monitor. We allow going to twice this size.
    double scale = ((double) size().width() / (double) backing_surface->original_w);
    double maxZoom = mZoomFactor * 2.0 / scale;

    if (scale < 2) {
        simulateSetZoom(MIN(mZoomFactor + .25, maxZoom));
    }
}

void EmulatorQtWindow::zoomOut(const QPoint &focus, const QPoint &viewportFocus)
{
    saveZoomPoints(focus, viewportFocus);
    if (mZoomFactor > 1) {
        simulateSetZoom(MAX(mZoomFactor - .25, 1));
    }
}

void EmulatorQtWindow::zoomReset()
{
    simulateSetZoom(1);
}

void EmulatorQtWindow::zoomTo(const QPoint &focus, const QSize &rectSize)
{
    saveZoomPoints(focus, QPoint(mContainer.width() / 2, mContainer.height() / 2));

    // The below scale = x creates a skin equivalent to calling "window scale x" through the
    // emulator console. At scale = 1, the device should be at a 1:1 pixel mapping with the
    // monitor. We allow going to twice this size.
    double scale = ((double) size().width() / (double) backing_surface->original_w);

    // Calculate the "ideal" zoom factor, which would perfectly frame this rectangle, and the
    // "maximum" zoom factor, which makes scale = 1, and pick the smaller one.
    // Adding 20 accounts for the scroll bars potentially cutting off parts of the selection
    double maxZoom = mZoomFactor * 2.0 / scale;
    double idealWidthZoom = mZoomFactor * (double) mContainer.width() / (double) (rectSize.width() + 20);
    double idealHeightZoom = mZoomFactor * (double) mContainer.height() / (double) (rectSize.height() + 20);

    simulateSetZoom(MIN(MIN(idealWidthZoom, idealHeightZoom), maxZoom));
}

bool EmulatorQtWindow::mouseInside() {
    QPoint widget_cursor_coords = mapFromGlobal(QCursor::pos());
    return widget_cursor_coords.x() >= 0 &&
           widget_cursor_coords.x() < width() &&
           widget_cursor_coords.y() >= 0 &&
           widget_cursor_coords.y() < height();
}

