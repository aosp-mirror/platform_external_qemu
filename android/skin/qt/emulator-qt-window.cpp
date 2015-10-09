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
#include <QFileDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSemaphore>

#include "android/base/files/PathUtils.h"
#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
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

using namespace android::base;

static EmulatorQtWindow *instance;

EmulatorQtWindow::EmulatorQtWindow(QWidget *parent) :
        QFrame(parent)
{
    instance = this;
    backing_surface = NULL;
    batteryState    = NULL;

    tool_window = new ToolWindow(this);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    this->setAcceptDrops(true);

    QObject::connect(this, &EmulatorQtWindow::blit, this, &EmulatorQtWindow::slot_blit);
    QObject::connect(this, &EmulatorQtWindow::createBitmap, this, &EmulatorQtWindow::slot_createBitmap);
    QObject::connect(this, &EmulatorQtWindow::fill, this, &EmulatorQtWindow::slot_fill);
    QObject::connect(this, &EmulatorQtWindow::getBitmapInfo, this, &EmulatorQtWindow::slot_getBitmapInfo);
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
}

EmulatorQtWindow::~EmulatorQtWindow()
{
    delete tool_window;
}

EmulatorQtWindow *EmulatorQtWindow::getInstance()
{
    return instance;
}

bool EmulatorQtWindow::event(QEvent *e)
{
    // Ignore MetaCall events
    if (e->type() == QEvent::MetaCall) {
        return QFrame::event(e);
    }

    // Add to the event buffer, but keep it a reasonable size - a few events, such as repaint,
    // can occur in between resizes but before the release happens
    mEventBuffer.push_back(e->type());
    if (mEventBuffer.size() > 8) {
        mEventBuffer.removeFirst();
    }

    // Scan to see if a resize event happened recently
    bool foundResize = false;
    unsigned i = 0;
    for (; i < mEventBuffer.size(); i++) {
        if (mEventBuffer[i] == QEvent::Resize) {
            foundResize = true;
            i++;
            break;
        }
    }

    // Determining resize-over is OS / window-manager specific
    // Do so by scanning the remainder of the event buffer for specific combinations
    if (foundResize) {

#ifdef _WIN32

        for (; i < mEventBuffer.size() - 1; i++) {
            if (mEventBuffer[i] == QEvent::NonClientAreaMouseButtonRelease) {
                doResize();
                break;
            }
        }

#else // !_WIN32

        for (; i + 2 < mEventBuffer.size(); i++) {
            if (mEventBuffer[i] == QEvent::WindowActivate &&
                mEventBuffer[i+1] == QEvent::ActivationChange &&
                mEventBuffer[i+2] == QEvent::InputMethodQuery) {
                doResize();
                break;
            }
        }

#endif

    }

    return QFrame::event(e);
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
                                                " SD card."),
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
    if (event->text().length() > 0) {
        SkinEvent *skin_event = createSkinEvent(kEventTextInput);
        skin_event->u.text.down = false;
        strncpy((char*)skin_event->u.text.text, (const char*)event->text().constData(), 32);
        queueEvent(skin_event);
    }
}

void EmulatorQtWindow::mouseMoveEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseMotion, event);
}

void EmulatorQtWindow::mousePressEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseButtonDown, event);
}

void EmulatorQtWindow::mouseReleaseEvent(QMouseEvent *event)
{
    handleEvent(kEventMouseButtonUp, event);
}

void EmulatorQtWindow::paintEvent(QPaintEvent *)
{
    if (backing_surface) {
        QPainter painter(this);

        QRect bg(QPoint(0, 0), this->size());
        painter.fillRect(bg, Qt::black);

        QRect r(0, 0, backing_surface->w, backing_surface->h);
        painter.drawImage(r, *backing_surface->bitmap);
    } else {
        D("Painting emulator window, but no backing bitmap");
    }
}

void EmulatorQtWindow::moveEvent(QMoveEvent *event)
{
    QFrame::moveEvent(event);
    simulateWindowMoved(pos());
    tool_window->dockMainWindow();
}

void EmulatorQtWindow::show()
{
    QFrame::show();
    tool_window->show();
}

void EmulatorQtWindow::startThread(StartFunction f, int argc, char **argv)
{
    MainLoopThread *t = new MainLoopThread(f, argc, argv);
    t->start();
}

void EmulatorQtWindow::wheelEvent(QWheelEvent *event)
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
    *xx = x();
    *yy = y();
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_isWindowFullyVisible(bool *out_value, QSemaphore *semaphore)
{
    *out_value = ((QApplication*)QApplication::instance())->desktop()->screenGeometry().contains(geometry());
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
    event_queue.enqueue(event);
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
    close();
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
    move(x, y);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_setWindowIcon(const unsigned char *, int, QSemaphore *semaphore)
{
    //    QPixmap image;
    //    image.loadFromData(data, size);
    //    QIcon icon(image);
    //    setWindowIcon(icon);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_setWindowTitle(const QString *title, QSemaphore *semaphore)
{
    setWindowTitle(*title);
    if (semaphore != NULL) semaphore->release();
}

void EmulatorQtWindow::slot_showWindow(SkinSurface* surface, const QRect* rect, int is_fullscreen, QSemaphore *semaphore)
{
    backing_surface = surface;
    if (is_fullscreen) {
        showFullScreen();
    } else {
        showNormal();
        setGeometry(*rect);
    }
    show();
    if (semaphore != NULL) semaphore->release();
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
        tool_window->showErrorDialog(tr("The screenshot could not be captured."),
                                     tr("Screenshot"));
    } else {

        // Pull the image from its remote location to the designated tmp directory
        QStringList args;
        QString command = tool_window->getAdbFullPath(&args);
        if (command.isNull()) {
            return;
        }

        // Add the arguments
        args << "pull";                 // Pulling a file
        args << REMOTE_SCREENSHOT_FILE; // Which file to pull
        args << getTmpImagePath();      // Where to place the temp file

        // Use a different process to avoid infinite looping when pulling the file
        mScreencapPullProcess.start(command, args);
        mScreencapPullProcess.waitForStarted();
    }
}

void EmulatorQtWindow::slot_screencapPullFinished(int exitStatus)
{
    if (exitStatus) {
        tool_window->showErrorDialog(tr("The image could not be loaded properly"),
                                     tr("Screenshot"));
    } else {

        // Load the data into an image
        QPixmap screencap;
        bool isOk = screencap.load(getTmpImagePath(), "PNG");
        if (!isOk) {
            tool_window->showErrorDialog(tr("The image could not be loaded properly."),
                                         tr("Screenshot"));
            return;
        }

        // Show a preview image - falls out of scope and disappears when the function exits
        QPixmap preview = screencap.scaled(size(), Qt::KeepAspectRatio);
        QGraphicsScene scene;
        scene.addPixmap(preview);

        QGraphicsView view(&scene);
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view.setWindowTitle(tr("Screenshot Preview"));
        view.setFixedSize(preview.size());
        view.show();

        // Pop up the save file dialog, and save the file if a name is given
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Screenshot Save File"),
                                                        ".",
                                                        tr("PNG files (*.png);;All files (*)"));
        if (fileName.isNull()) return;

        // Qt doesn't force-append .png, so ensure it's there or the save may fail
        if (!fileName.endsWith(".png")) {
            fileName.append(".png");
        }
        screencap.save(fileName);
    }
}

QString EmulatorQtWindow::getTmpImagePath()
{
    StringVector imageVector;
    imageVector.push_back(String(QDir::tempPath().toStdString().data()));
    imageVector.push_back(String(LOCAL_SCREENSHOT_FILE));
    return QString(PathUtils::recompose(imageVector).c_str());
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

void EmulatorQtWindow::doResize()
{
    mEventBuffer.clear(); // Ensures one resize release does not produce multiple set-scales
    if (backing_surface) {
        QSize newSize(backing_surface->original_w, backing_surface->original_h);
        newSize.scale(this->size(), Qt::KeepAspectRatio);

        double newScale = (double) newSize.width() / (double) backing_surface->original_w;
        simulateSetScale(newScale);
    }
}

void EmulatorQtWindow::handleEvent(SkinEventType type, QMouseEvent *event)
{
    SkinEvent *skin_event = createSkinEvent(type);
    skin_event->u.mouse.button = event->button() == Qt::RightButton ? kMouseButtonRight : kMouseButtonLeft;
    skin_event->u.mouse.x = event->x();
    skin_event->u.mouse.y = event->y();
    skin_event->u.mouse.xrel = 0;
    skin_event->u.mouse.yrel = 0;
    queueEvent(skin_event);
}

void EmulatorQtWindow::handleKeyEvent(SkinEventType type, QKeyEvent *pEvent)
{
    // See if there is a Qt-specific handler for this event
    if (handleQtKeyEvent(type, pEvent)) return;

    SkinEvent *skin_event = createSkinEvent(type);
    SkinEventKeyData *keyData = &skin_event->u.key;
    keyData->keycode = convertKeyCode(pEvent->key());
    Qt::KeyboardModifiers modifiers = pEvent->modifiers();
    if (modifiers & Qt::ShiftModifier) keyData->mod |= kKeyModLShift;
    if (modifiers & Qt::ControlModifier) keyData->mod |= kKeyModLCtrl;
    if (modifiers & Qt::AltModifier) keyData->mod |= kKeyModLAlt;
    queueEvent(skin_event);
}

bool EmulatorQtWindow::handleQtKeyEvent(SkinEventType type, QKeyEvent *event)
{
    bool usedEvent = false;
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // TODO: add more Qt-specific keyboard shortcuts

    if (key == Qt::Key_F10 && (modifiers & Qt::ControlModifier)) {
        screenshot();
        usedEvent = true;
    }

    return usedEvent;
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

void EmulatorQtWindow::simulateSetScale(double scale)
{
    SkinEvent *event = createSkinEvent(kEventSetScale);
    event->u.window.x = pos().x();
    event->u.window.y = pos().y();
    event->u.window.scale = scale;
    slot_queueEvent(event);
}

void EmulatorQtWindow::simulateWindowMoved(const QPoint &pos)
{
    SkinEvent *event = createSkinEvent(kEventWindowMoved);
    event->u.window.x = pos.x();
    event->u.window.y = pos.y();
    slot_queueEvent(event);
}

void EmulatorQtWindow::slot_runOnUiThread(SkinGenericFunction* f, void* data, QSemaphore* semaphore) {
    (*f)(data);
    if (semaphore) semaphore->release();
}
